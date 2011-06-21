#include "CMsgProcess.h"

#define     BUFFER_SIZE                     sizeof(common_ResultResponse_Frame)

int			inum							= 0;
static int  err_broadsendnum                = 0;
static int  err_unicastsendnum              = 0;

//#define INFINITE  		  0xFFFFFFFF  // Infinite timeout

/*静态成员的初始化*/
CMsgProcess	*CMsgProcess::m_reqProInstance	= CARI_CCP_VOS_NEW(CMsgProcess);//需要在什么地方注销掉呢????????


/*构造函数,初始化变量
 */
CMsgProcess::CMsgProcess()
{
	m_listRequestFrame.clear();
	i_full_waiters = 0;
	i_empty_waiters = 0;
	i_terminated = 0;

#if defined(_WIN32) || defined(WIN32)
	//m_semaphore = CreateSemaphore(NULL, 0, 1, NULL);
#endif

	/* 用默认属性初始化一个互斥锁对象*/
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	//创建异步命令处理线程
	creatAsynCmdThread();
}

/*析构函数
 */
CMsgProcess::~CMsgProcess()
{
	//m_listRequestFrame.clear();//shutdown的时候出现了问题???
}

/*获得单实例对象
 */
CMsgProcess *& CMsgProcess::getInstance()
{
	if (NULL == m_reqProInstance) {
		m_reqProInstance = CARI_CCP_VOS_NEW(CMsgProcess);
	}
	return m_reqProInstance;
}

/*
*/
void CMsgProcess::clear()
{
	//m_reqProInstance = NULL;
}

/*创建异步命令处理线程
*/
void CMsgProcess::creatAsynCmdThread()
{
	pthread_attr_t	attr_asynCmd;

	/*初始化属性*/
	pthread_attr_init(&attr_asynCmd); 

	//启动异步处理命令的线程,可考虑多创建几次
	int	iRet	= 0;
	for (int i = 1; i <= CARICCP_MAX_CREATTHREAD_NUM; i++) {
		//int iRet= pthread_create(&thread_asynPro,NULL,asynProCmd,NULL);
		iRet = pthread_create(&thread_asynPro, &attr_asynCmd, asynProCmd, (void*) &attr_asynCmd);
		if (0 != iRet)//可考虑多创建几次
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "CMsgProcess::creatThread() %d count error!\n", i);
			continue;
		}

		break;
	}
}

/*请求命名的入口函数,涉及到多线程的处理,此处是否要要考虑加锁???
*/
void CMsgProcess::proCmd(u_int socketID, //连接的客户端的socket号
						 common_CmdReq_Frame *&common_reqFrame)  //通用的客户端请求帧
{
	//将客户端的请求帧转化为内部帧
	inner_CmdReq_Frame	*inner_reqFrame	= CARI_CCP_VOS_NEW(inner_CmdReq_Frame);
	initInnerRequestFrame(inner_reqFrame);

	//将"通用"请求帧转换成"内部"请求帧
	//主要是涉及参数的解析,将字符串的格式转换为key-value方式
	covertReqFrame(common_reqFrame, inner_reqFrame);

	//将以前的产生的通用帧进行删除,释放内存
	CARI_CCP_VOS_DEL(common_reqFrame);

	inner_reqFrame->socketClient = socketID;	 //设置连接的客户端的socket号

	inner_reqFrame->sSourceModuleID = BASE_MODULE_CLASSID;//源模块号设置

	//是否为同步命令
	bool	bSynchro	= intToBool(inner_reqFrame->body.bSynchro);

	//异步测试
	/*bSynchro = false;*/
	if (bSynchro) {
		//新建一个"响应帧",并设置一些必要的参数
		inner_ResultResponse_Frame	*inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

		//初始化内部响应帧,并从通用请求帧拷贝有关数据给响应帧
		initInnerResponseFrame(inner_reqFrame, inner_respFrame);

		//直接进行逻辑处理
		syncProCmd(inner_reqFrame, inner_respFrame);

		//将结果帧发送给对应的客户端
		//这个结果帧不能放置到队列中,否则客户端可能会等待一段时间获得值
		commonSendRespMsg(inner_respFrame);

		//销毁"请求帧"
		CARI_CCP_VOS_DEL(inner_reqFrame);

		//销毁"响应帧"
		CARI_CCP_VOS_DEL(inner_respFrame);
	} else//异步命令
	{
		//请求帧先存放到队列中还是考虑另外启动线程处理??????
		//也保证命令的顺序性:必须防止到队列中
		asynProCmd_2(inner_reqFrame);//注意此新建的帧在什么地方要销毁
	}
}

/*同步命令处理:主要查找负责处理的模块
 */
int CMsgProcess::syncProCmd(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_respFrame)//输出参数:响应帧
{
	//先查看命令码
	u_int	iCmdCode	= inner_reqFrame->body.iCmdCode;

	//目的模块号
	//根据配置查看此命令码属于哪个模块负责处理
	u_int	iModuleID	= CModuleManager::getInstance()->getModuleID(iCmdCode);

	//对内部的请求帧内部做下更改
	//inner_reqFrame->header.sSourceModuleID = 0;		 //代表当前此命令是由ModuleManager发送的
	//应该在函数的调用者进行设置
	inner_reqFrame->sDestModuleID = iModuleID;			 //代表此命令将要下发给的子模块

	//socket进行设置
	inner_respFrame->socketClient = inner_reqFrame->socketClient;

	//查找内部注册的子模块,调用相应的回调函数进行处理
	CBaseModule	*subModule	= CModuleManager::getInstance()->getSubModule(iModuleID); //取出存放的子模块
	if (NULL != subModule) {
		//printf("模块号%d \n",iModuleID);
		subModule->receiveReqMsg(inner_reqFrame, inner_respFrame);		//动态回调
	} 

	CARI_CCP_VOS_DEL(inner_reqFrame);  //删除内部请求帧
	return CARICCP_SUCCESS_STATE_CODE;
}

/*异步命令处理
 */
void CMsgProcess::asynProCmd_2(inner_CmdReq_Frame *&inner_reqFrame)
{
	pushCmdToQueue(inner_reqFrame);//注意此新建的帧在什么地方要销毁
}

/*
 */
void CMsgProcess::covertRespParam(inner_ResultResponse_Frame *&inner_respFrame, common_ResultResponse_Frame *&common_respFrame)//输出参数
{
	//common_respFrame->iResultNum = inner_respFrame->iResultNum;

	//数据区的转换

	return;
}

/*将通用的请求帧转换为内部的请求帧
 */
void CMsgProcess::covertReqFrame(common_CmdReq_Frame *&common_reqMsg, inner_CmdReq_Frame *&inner_reqFrame)//输出参数:内部的请求帧结构
{
	//先拷贝"帧头"和"命令"部分
	memcpy(inner_reqFrame, common_reqMsg, sizeof(common_CmdReqFrame_Header) + sizeof(common_CmdReqFrame_Body));  

	//解析"参数区"域
	covertStrToParamStruct(common_reqMsg->param.strParamRegion, inner_reqFrame->innerParam.m_vecParam);//存放新构建的参数结构
}

/*将"内部响应帧"转换成"通用响应帧"
 */
void CMsgProcess::covertRespFrame(inner_ResultResponse_Frame *&inner_respFrame, common_ResultResponse_Frame *&common_respMsg)//输出参数 :通用的响应帧
{
	//先拷贝"帧头"
	memcpy(common_respMsg, inner_respFrame, sizeof(common_ResultRespFrame_Header));

	//数据表头的转换
	//参数区的转换
	covertRespParam(inner_respFrame, common_respMsg);
}

/*请求帧的细化解析,进一步解析参数
 */
void CMsgProcess::analyzeReqFrame(common_CmdReq_Frame *&common_reqMsg)
{
	//先把参数拷贝成内部的结构
	inner_CmdReq_Frame	*aInnerReqFrame	= CARI_CCP_VOS_NEW(inner_CmdReq_Frame);
	initInnerRequestFrame(aInnerReqFrame);
	//先拷贝"帧头"和"命令"部分
	memcpy(aInnerReqFrame, common_reqMsg, sizeof(common_CmdReqFrame_Header) + sizeof(common_CmdReqFrame_Body));  

	//解析"参数区"域
	covertStrToParamStruct(common_reqMsg->param.strParamRegion, aInnerReqFrame->innerParam.m_vecParam);//存放新构建的参数结构

	//将以前的产生的通用帧进行删除,释放内存
	CARI_CCP_VOS_DEL(common_reqMsg);
	CARI_CCP_VOS_DEL(aInnerReqFrame);
}

/*发送结果给响应的客户端:针对同步和异步命令的统一接口
*针对两种情况:命令的返回结果和通知类型的消息处理
*/
void CMsgProcess::commonSendRespMsg(inner_ResultResponse_Frame *&inner_respFrame,
									int nSendType)
{
	//不需要将结果立即返回给client端
	if (!inner_respFrame->isImmediateSend) {
		
		//先屏蔽此限制
		//return;
	}
	bool            bres             = true;
    int             iEndstrlen       = strlen(CARICCP_ENTER_KEY);
	int				iResult		     = CARICCP_SUCCESS_STATE_CODE;
	int				iFrameHeaderSize = sizeof(common_ResultRespFrame_Header); //发送数据的实际长度,先计算帧头
	socket3element	element;
	string			strData				= "",strTmp	= ""; //单次需要发送的记录
	int				iPos				= -1;
	bool         	isFirst				= true;
	int				iSingelDataLength	= 0; //单条记录的长度
	int				iSendedDataSumLen	= 0; //每次发送的总的数据的总长度
	int				iResultNum			= 0; //需要发送的记录数量
	int				iMaxBuffer			= CARICCP_MAX_TABLECONTEXT - 1;//除去header剩余的数据区的长度

	//如果不是通知类型的结果消息,那么此结果一定是当前客户端发送到命令处理完毕后返回的结果
	if (COMMON_RES_TYPE == nSendType) {
		iResult = CModuleManager::getInstance()->getSocket(inner_respFrame->socketClient, element);
		if (CARICCP_ERROR_STATE_CODE == iResult) {
			//打印错误信息
			return;
		}
	}

	//设置时间戳
	getLocalTime(inner_respFrame->header.timeStamp);

	//需要将内部响应帧转换成通用响应帧,发送给对应的客户端
	common_ResultResponse_Frame	*common_respFrame = CARI_CCP_VOS_NEW(common_ResultResponse_Frame);
	initCommonResponseFrame(common_respFrame);
	//covertRespFrame(inner_respFrame,common_respFrame);

	//先拷贝"帧头"
	memcpy(common_respFrame, inner_respFrame, sizeof(common_ResultRespFrame_Header));

	//必须先判断是否为"通知类型".
	//如果是通知类型的消息,跳转到"通知类型"处理,直接返回通知数据
	if (NOTIFY_RES_TYPE == nSendType) {
		goto norifyMark;
	}

	//处理结果不成功,只返回给"发送此命令的客户端".另外,通知类型返回的结果码"非0"
	if (CARICCP_SUCCESS_STATE_CODE != inner_respFrame->header.iResultCode) {
		
		//编码转换
		//convertFrameCode(common_respFrame,"gb2312","utf-8");

		//只需要拷贝消息头部分即可,消息体部分的内容忽略
		//char	sendBuf[sizeof(common_ResultRespFrame_Header)];
		//memset(sendBuf, 0, sizeof(sendBuf));
		//memcpy(sendBuf, common_respFrame, iFrameHeaderSize);

		bres = unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize,inner_respFrame->header.strCmdName);
		if (!bres){
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
				common_respFrame->header.strCmdName);
		}

		goto endMark;
	}

//通知类型的开始处理标识
norifyMark:

	//不属于"查询类"记录,另外,通知类型不一定要返回查询类记录,比如:reboot命令
	if (0 == inner_respFrame->iResultNum 
		&& 0 == inner_respFrame->m_vectTableValue.size()) {
		common_respFrame->iResultNum = inner_respFrame->iResultNum;

		//编码转换
		//convertFrameCode(common_respFrame,"gb2312","utf-8");

		//只需要拷贝消息头部分即可
		//char	sendBuf[sizeof(common_ResultRespFrame_Header)];
		//memset(sendBuf, 0, sizeof(sendBuf));
		//memcpy(sendBuf, common_respFrame, iFrameHeaderSize);

		//是否为"通知"类型
		if (common_respFrame->header.bNotify
			|| DOWN_RES_TYPE == nSendType) {
				char	sendBuf[sizeof(common_ResultRespFrame_Header)];
				memset(sendBuf, 0, sizeof(sendBuf));
				memcpy(sendBuf, common_respFrame, iFrameHeaderSize);
				bres = broadcastRespMsg(sendBuf, iFrameHeaderSize,inner_respFrame->header.strCmdName);//广播
				if (!bres){
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Broadcast cmd \"%s\" resp failed!\n",
						common_respFrame->header.strCmdName);
				}
		} else {
			bres = unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize,inner_respFrame->header.strCmdName);    //单播
			if (!bres){
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
					common_respFrame->header.strCmdName);
			}
		}

		goto endMark;
	}

	//"查询"记录类:目前暂时没有群发是场景,不过在此处统一进行考虑

///*通知类型的消息*/
//norifyMark:

	//表头的拷贝
	memcpy(common_respFrame->strTableName, inner_respFrame->strTableName, sizeof(common_respFrame->strTableName));
	iFrameHeaderSize += sizeof(common_respFrame->iResultNum);   //记录数量的长度
	iFrameHeaderSize += sizeof(common_respFrame->strTableName); //记录列表名的长度

	//属于查询记录 :查询的记录量很大,需要分块发送
	//属于通知类型的记录,此时只有少量的条数
	strData		= "";
	strTmp		= ""; //单次需要发送的记录
	iPos		= -1;
	isFirst		= true;
	iSingelDataLength	= 0; //单条记录的长度
	iSendedDataSumLen	= 0; //每次发送的总的数据的总长度
	iResultNum	= 0; //需要发送的记录数量
	iMaxBuffer	= CARICCP_MAX_TABLECONTEXT - 3;//使用3个字符扩展

	//1 开始对需要返回的记录进行长度计算,不能超出规定的字节数量
	for (vector< string>::iterator iter = inner_respFrame->m_vectTableValue.begin(); 
		iter != inner_respFrame->m_vectTableValue.end(); ++iter) {
		strTmp = *iter;
		iSingelDataLength = strTmp.length();//单条记录的长度
		iSendedDataSumLen += iSingelDataLength;    //计算总的长度
		if (iSendedDataSumLen < iMaxBuffer) {
			//里面的数据已经是封装完整的,格式如下:
			//多列记录之间使用特殊分割符"〒"
			//多行之间必须使用回车换行\r\n分开,否则发送给client端不方便处理

			//2 优化处理一下,查看是否已经封装了\r\n标识符,由此处统一封装优化一下,造成最后一条记录多余\r\n字符!!!!!!!!!!!!!!!!!
			iPos = strTmp.find_last_of(CARICCP_ENTER_KEY);
			if (-1 == iPos) {
				iSendedDataSumLen += iEndstrlen;//尾部准备增加"\r\n"

				//超出发送的数据长度,转到next处理
				if (iSendedDataSumLen >= iMaxBuffer) {
					iSendedDataSumLen -= iEndstrlen; //把多计算的长度要减掉
					goto next;
				}

				strTmp.append(CARICCP_ENTER_KEY);//末尾增加\r\n结束符
			}

			strData.append(strTmp);   
			iResultNum += 1;

			//发送的字符数量没有超出上限,则继续处理
			continue;//----------
		}
/**/
next:
		iSendedDataSumLen -= iSingelDataLength;   //把多计算的长度要减掉,否则再销毁帧的时候,heap会出现问题
		common_respFrame->iResultNum = iResultNum;//需要发送的记录数量

		//设置结束标识
		if (isFirst) {
			isFirst = false;
			common_respFrame->header.isFirst = true;  //代表第一条记录
		} else {
			common_respFrame->header.isFirst = false;
		}
		common_respFrame->header.isLast = false; 	//不是最后一条记录

		//数据区的拷贝---------------------------------------------------------------
		memcpy(common_respFrame->strTableContext, strData.c_str(), iSendedDataSumLen);
		//char sendBuf[sizeof(common_ResultResponse_Frame)];
		//memset(sendBuf, 0, sizeof(sendBuf));

		//编码转换
		//convertFrameCode(common_respFrame,"gb2312","utf-8");
		//memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));

		//3 开始对大批量的数据进行分块发送,是否需要通知所有注册/登录的客户端
		if (NOTIFY_RES_TYPE == nSendType/*common_respFrame->header.bNotify*/
			||DOWN_RES_TYPE == nSendType) {
				char sendBuf[sizeof(common_ResultResponse_Frame)];
				memset(sendBuf, 0, sizeof(sendBuf));
				memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));
				bres = broadcastRespMsg(sendBuf, iFrameHeaderSize + iSendedDataSumLen,inner_respFrame->header.strCmdName);//多注册客户端的响应
				if (!bres){
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Broadcast cmd \"%s\" resp failed!\n",
						common_respFrame->header.strCmdName);
				}
		} else {
		  	bres =unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize + iSendedDataSumLen,inner_respFrame->header.strCmdName);//对单个客户端响应
			if (!bres){
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
					common_respFrame->header.strCmdName);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//4 重新计算,并准备进行下一轮的数据发送
		strData = "";
		iSendedDataSumLen = iSingelDataLength;

		//优化处理一下,查看是否已经封装了\r\n标识符
		iPos = strTmp.find_last_of(CARICCP_ENTER_KEY);
		if (-1 == iPos) {
			iSendedDataSumLen += iEndstrlen;//尾部准备增加"\r\n"

			//超出发送的数据长度
			if (iSendedDataSumLen >= iMaxBuffer) {
				iSendedDataSumLen -= iEndstrlen;//把多计算的长度要减掉
				
				continue;//不再发送此数据,继续处理,这个场景很少!!!
			}

			strTmp.append(CARICCP_ENTER_KEY);//末尾增加\r\n结束符
		}

		strData.append(strTmp);//要保证此记录被发送出去
		iResultNum = 1; 	   //记录赋值为1

	}//end for所有的查询记录

	//5 还要计算一次,将记录剩余的最后部分发送出去
	if (0 != iSendedDataSumLen || 0 != iResultNum) {
		common_respFrame->iResultNum = iResultNum;	  //需要发送的记录数量

		//设置结束标识
		if (isFirst) {
			common_respFrame->header.isFirst = true;  //代表第一条记录
			isFirst = false;
		} else {
		  	common_respFrame->header.isFirst = false; //非第一条记录
		}
		common_respFrame->header.isLast = true;		 //代表最后一条记录

		//数据区的拷贝---------------------------------------------------------------
		memcpy(common_respFrame->strTableContext, strData.c_str(), iSendedDataSumLen);

		//char sendBuf[sizeof(common_ResultResponse_Frame)];
		//memset(sendBuf, 0, sizeof(sendBuf));
		////编码转换
		////convertFrameCode(common_respFrame,"gb2312","utf-8");
		//memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));

		//是否需要广播群发
		if (NOTIFY_RES_TYPE == nSendType/*common_respFrame->header.bNotify*/
			||DOWN_RES_TYPE == nSendType) {
				char sendBuf[sizeof(common_ResultResponse_Frame)];
				memset(sendBuf, 0, sizeof(sendBuf));

				//编码转换
				//convertFrameCode(common_respFrame,"gb2312","utf-8");
				memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));
				bres = broadcastRespMsg(sendBuf, iFrameHeaderSize + iSendedDataSumLen,inner_respFrame->header.strCmdName);//广播通知
				if (!bres){
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Broadcast cmd \"%s\" resp failed!\n",
						common_respFrame->header.strCmdName);
				}
		} else {
			bres = unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize + iSendedDataSumLen, inner_respFrame->header.strCmdName); //单播通知
			if (!bres){
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
					common_respFrame->header.strCmdName);
			}
		}
	
	}//end (0 !=iSendedDataSumLen ...


endMark:
	/*CARI_CCP_VOS_DEL(inner_respFrame);//销毁内部响应帧,由调用者负责销毁*/
	CARI_CCP_VOS_DEL(common_respFrame);//销毁通用响应帧
}

/*"单播"发送响应信息
 */
bool CMsgProcess::unicastRespMsg(socket3element &element,
								 const char *respMsg, 	  
								 int  bufferSize,
								 char* cmdname)
{
	bool                        berrsend= false;
	
	//先发送数据,根据实际长度进行发送数据,发送的性能和cpu有关,需要适当的调用一下
	int	sendbytes	= send(element.socketClient, respMsg, bufferSize, 0);
	if (CARICCP_ERROR_STATE_CODE == sendbytes){//SOCKET_ERROR
		//客户端号,客户端地址
		inum++;
		err_unicastsendnum++;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Return response frame failed, cmdname=%s,clientID :%d ,socketID :%d!\n", 
			cmdname,
			element.sClientID,
			element.socketClient);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Cause :%s\n",strerror(errno));
		//fprintf(stderr,"send error:%s \n",strerror(errno)); 
		
		if (10<err_unicastsendnum){
			//关闭所有client的socket
			berrsend = true;
			goto end;
		}

		return false;
	}

	//printf("发送帧数据 :%d\n",sendbytes);

	//为了防止发生阻塞等现象,此处设置一下发送的时间间隔,0.5秒
	CARI_SLEEP(500);

end:
	//关闭并清除当前的client socket
	if (berrsend){
		err_unicastsendnum = 0;
		CModuleManager::getInstance()->eraseSocket(element.socketClient);
		
		//关闭当前客户端的socket
		CARI_CLOSE_SOCKET(element.socketClient);
		return false;
	}

	return true;
}

/*函数重构:单播返回结果信息
*/
bool CMsgProcess::unicastRespMsg(socket3element& element, 
								 common_ResultResponse_Frame* respFrame,
								 int  bufferSize, 
								 char* cmdname)
{
	int isize = bufferSize;
	bool bres = false;
	char sendBuf[BUFFER_SIZE];
	memset(sendBuf, 0, sizeof(sendBuf));

	//此处统一区分web和lmt client
	if (0 == element.sClientID){
		//格式: [cmdID=201,resultcode=0]操作成功.
		string strres = "",strT;
		strres.append("[");
		strres.append("cmdID=");
		strT = intToString(respFrame->header.iCmdCode);strres.append(strT);strres.append(",");
		strres.append("resultcode=");
		strT = intToString(respFrame->header.iResultCode);strres.append(strT);

		//查询类(一般和分页)
		if (0 !=respFrame->iResultNum){
			//还应该封装是否为最后一条记录的标识,方便web页面分页显示使用
			strres.append(",");
			strres.append("resultNum=");
			strT = intToString(respFrame->iResultNum);strres.append(strT);

			strres.append(",");
			strres.append("isFirst=");
			strT = intToString(respFrame->header.isFirst);strres.append(strT);

			strres.append(",");
			strres.append("isLast=");
			strT = intToString(respFrame->header.isLast);strres.append(strT);

			strres.append(",");
			strres.append("tableName=");
			strres.append(respFrame->strTableName);

			strres.append("]");
			strres.append(respFrame->strTableContext);
			//[blast=1]
		}
		else{//非查询类,重新封装结果
			strres.append("]");
			strres.append(respFrame->header.strResuleDesc);
		}

		//非常关键: java端采用readLine(),接收需要"结束符"
		//strres.append(CARICCP_ENTER_KEY);
		//strres.append("测试TT");
		//strres.append(CARICCP_ENTER_KEY);

		isize = strres.length();
		memcpy(sendBuf, strres.c_str(), isize);

		isize++;//结尾多放置一个结束符'\0'
		isize = CARICCP_MIN(BUFFER_SIZE - 1,isize);

		//java端采用read(),关键的结束符
		sendBuf[isize] = '\0';
		
	}
	else{//一般的lmt端
		memcpy(sendBuf, respFrame, bufferSize);
	}

	
	bres = unicastRespMsg(element,sendBuf,isize,cmdname);//isize设置不当会造成web端接收不到消息帧
	return bres;
}

/*广播发送响应信息
 */
bool CMsgProcess::broadcastRespMsg(const char *respMsg, 
								   int bufferSize,
								   char* cmdname)
{
	bool						bResult	= true;
	bool                        berrsend= false;
	int	sendbytes                       = 0;

	//查找注册的客户端,针对每个客户端依次发送消息
	socket3element						socketClientElement;
	vector< socket3element>				vecTmp	= CModuleManager::getInstance()->getSocketVec();
	vector< socket3element>::iterator	iter	= vecTmp.begin();
	for (; iter != vecTmp.end(); ++iter) {
		//获得注册的客户端的socket号都需要广播通知
		socketClientElement = *iter;

		//web不考虑,无效的clientID不考虑
		if (0 == socketClientElement.sClientID
			|| CARICCP_MAX_SHORT == socketClientElement.sClientID){
			continue;
		}
		
		//printf("广播socketid :%d\n",socketClientElement.socketClient);
		//bResult = unicastRespMsg(socketClient, respMsg, bufferSize,cmdname);//是否成功,每个客户端都要发送一次
	    sendbytes	= send(socketClientElement.socketClient, respMsg, bufferSize, 0);
		if (CARICCP_ERROR_STATE_CODE == sendbytes){//SOCKET_ERROR
			//客户端号,客户端地址
			inum++;
			err_broadsendnum++;
			bResult = false;
			
			//打印失败日志
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Return resp failed,cmdname=%s, clientID :%d ,socketID :%d!\n",
				cmdname,
				socketClientElement.sClientID, 
				socketClientElement.socketClient);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s\n",strerror(errno));
			fprintf(stderr,"send error:%s\n",strerror(errno));
			
			if (10<err_broadsendnum){
				berrsend = true;
				break;
			}

			continue;
		}
		//printf("发送帧数据字节数量 :%d\n",sendbytes);
		CARI_SLEEP(100);//优化
	}

	//为了防止发生阻塞等现象,客户度接收不同信息.此处设置一下发送的时间间隔,0.5秒
	CARI_SLEEP(500);

	//关闭所有的client socket
	if (berrsend){
		err_broadsendnum = 0;
		CModuleManager::getInstance()->closeAllClientSocket();
	}


	//以前的处理逻辑
	// 	//先发送当前的客户端
	// 	bResult = unicastRespMsg(element,respMsg,bufferSize,inner_respFrame->header.strCmdName);
	// 	if (!bResult)// 发送失败,其他注册的客户端不需要在通知
	// 	{
	// 		return false;
	// 	}
	// 
	// 	//查找注册的其他客户端,针对每个客户端依次发送消息
	// 	u_int socketClient = element.socketClient;
	// 	socket3element socketTmp;
	// 	vector<socket3element> vecTmp = CModuleManager::getInstance()->getSocketVec();
	// 	vector<socket3element>::iterator iter=vecTmp.begin();
	// 	for( ; iter != vecTmp.end(); ++iter)
	// 	{
	// 		socketTmp = *iter;
	// 		if (socketClient != socketTmp.socketClient)//其他注册的客户端发送
	// 		{
	// 			bResult = unicastRespMsg(socketTmp,respMsg,bufferSize,inner_respFrame->header.strCmdName); 	//是否成功,每个客户端都要发送一次
	// 		}
	// 	}

	return bResult;
}

/*通知类型的处理,某些类型的命令(增/删)操作,其结果需要通知其他客户端同步更改结果
*/
void CMsgProcess::notifyResult(inner_CmdReq_Frame *&inner_reqFrame, //原请求帧
							   Notify_Type_Code notifyCode,			//通知码
							   string notifyValue)					//返回的记录,多个记录之间使用特殊符号〒隔开
{
	//命令属于通知类型,需要广播通知其他客户端
	//新建一个"响应帧",并设置一些必要的参数
	inner_ResultResponse_Frame	*notify_inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//初始化内部响应帧
	/*	memset(notify_inner_respFrame,0,sizeof(inner_ResultResponse_Frame));*/
	initInnerResponseFrame(inner_reqFrame, notify_inner_respFrame);

	notify_inner_respFrame->header.bNotify = true;			  //设置为通知类型
	notify_inner_respFrame->header.iResultCode = notifyCode;  //通知码

	//查看有几条记录存在,此处的处理将导致各记录之间的CARICCP_ENTER_KEY被删掉
	//在发送结果的时候在进行判断,添加
	splitString(notifyValue, CARICCP_ENTER_KEY, notify_inner_respFrame->m_vectTableValue);

	// 	notify_inner_respFrame->m_vectTableValue.push_back("1\r\n");
	// 	notify_inner_respFrame->m_vectTableValue.push_back("8001");


	//通知类型发送给所有注册的客户端
	commonSendRespMsg(notify_inner_respFrame,NOTIFY_RES_TYPE);

	//销毁内部响应帧
	CARI_CCP_VOS_DEL(notify_inner_respFrame);
}

/*通知类型的处理,需要通知所有的注册的客户端
*/
void CMsgProcess::notifyResult(inner_ResultResponse_Frame *&inner_respFrame)  //"内部响应帧"
{
	//命令属于通知类型,需要广播通知其他客户端
	inner_respFrame->header.bNotify = true;  //设置为通知类型

	//通知类型发送给所有注册的客户端
	commonSendRespMsg(inner_respFrame,NOTIFY_RES_TYPE);

	//销毁内部响应帧
	CARI_CCP_VOS_DEL(inner_respFrame);
}

/*主动上报类型的处理,主要针对某个模块的进行实时上报
*/
void CMsgProcess::initiativeReport(inner_ResultResponse_Frame*& inner_respFrame)
{

}

/*命令分发
 */
void CMsgProcess::dispatchCmd()
{
}

/*加锁处理
 */
void CMsgProcess::lock()
{
	// 	InitializeCriticalSection(m_lock); //Initialize the critical section.   
	// 	EnterCriticalSection(m_lock);      //Enter Critical section

	pthread_mutex_lock(&mutex);
}

/*解锁
 */
void CMsgProcess::unlock()
{
	// 	LeaveCriticalSection(m_lock);     //Leave Critical section
	// 	DeleteCriticalSection(m_lock);    //Delete Critical section

	pthread_mutex_unlock(&mutex);
}

/*命令队列是否已满
 */
bool CMsgProcess::isFullOfCmdQueue()
{
	if (CARICCP_MAX_CMD_NUM == m_listRequestFrame.size()) {
		return true;
	}

	return false;
}

/*命令队列是否已空
 */
bool CMsgProcess::isEmptyOfCmdQueue()
{
	if (0 == m_listRequestFrame.size()) {
		return true;
	}
	return false;
}

/*将待发送的命令帧放置到队列中,队列数据满,则产生阻塞.
 *一旦命令放置完以后,则通知等待的线程取出数据发送出去
 */
int CMsgProcess::pushCmdToQueue(inner_CmdReq_Frame *&reqFrame)
{
	lock();//加锁
	if (isFullOfCmdQueue())//判断队列是否满
	{
		i_full_waiters ++;//标识符号增加1
		//result = WaitForSingleObject(m_semaphore, INFINITE);//当前线程等待,减少信号量

		pthread_cond_wait(&cond, &mutex);

		i_full_waiters--;//标识符减少1

		//已经唤醒但是还是满的,中断退出
		if (isFullOfCmdQueue()) {
			unlock();//解锁
			return CARICCP_ERROR_STATE_CODE;
		}
	}

	m_listRequestFrame.push_back(reqFrame);//加入到容器的end

	//放置完毕,释放信号量
	if (i_empty_waiters) {
		//ReleaseSemaphore(m_semaphore, 1, NULL);//释放信号量

		pthread_cond_signal(&cond);//使用跨平台线程库
	}

	//解锁
	unlock();

	return CARICCP_SUCCESS_STATE_CODE;
}

/*将待发送的命令帧放置到队列中,队列数据满,则产生阻塞.
 *一旦命令放置完以后,则通知等待的线程取出数据发送出去
 */
int CMsgProcess::popCmdFromQueue(inner_CmdReq_Frame *&reqFrame)
{
	lock();//加锁
	if (isEmptyOfCmdQueue())//队列数据为空
	{
		i_empty_waiters++;  //空标识

		//result = WaitForSingleObject(m_semaphore, INFINITE);//当前线程等待,减少信号量
		pthread_cond_wait(&cond, &mutex);//使用跨平台线程库

		i_empty_waiters--;  //标识符减少1

		if (isEmptyOfCmdQueue()) {
			unlock();//线程解锁unlock
			return CARICCP_ERROR_STATE_CODE;
		}
	} 


	reqFrame = m_listRequestFrame.front();//取出容器顶部的命令元素.备注 :此元素在什么时候销毁???
	m_listRequestFrame.pop_front();		  //取出命令后要及时删除

	//此标识有效表示:队列以前"为"满,因为目前刚刚取出了一个数据.此时push()对应的线程一定在阻塞中,需要唤醒
	if (i_full_waiters) {
		//ReleaseSemaphore(m_semaphore, 1, NULL);//释放信号量,唤醒其他正在等待的线程(push())

		pthread_cond_signal(&cond);//使用跨平台线程库
	}

	unlock();//unlock:解锁

	return CARICCP_SUCCESS_STATE_CODE;
}

/*清除队列中的元素
*/
void CMsgProcess::clearAllCmdFromQueue()
{
	inner_CmdReq_Frame* inner = NULL;
	list<inner_CmdReq_Frame*>::iterator it =  m_listRequestFrame.begin();
	while (it != m_listRequestFrame.end()){
		inner = *it;
		CARI_CCP_VOS_DEL(inner);
		it = m_listRequestFrame.erase(it); 
	}
}
