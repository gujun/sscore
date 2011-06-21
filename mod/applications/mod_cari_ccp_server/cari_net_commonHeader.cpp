#pragma warning(disable:4996)
#include "cari_net_commonHeader.h"

#if defined(_WIN32) || defined(WIN32)
	#include <direct.h>

#else //linux系统使用

	#include <sys/types.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	

///************************************************************************/
///*                    代码转换操作类                                    */
///************************************************************************/
//class CodeConverter {
//private:
//	iconv_t cd;
//public:
//	//构造
//	CodeConverter(const char *from_charset,const char *to_charset) {
//		cd = iconv_open(to_charset,from_charset);
//	}
//
//	//析构
//	~CodeConverter() {
//		iconv_close(cd);
//	}
//
//	//转换输出
//	int convert(char *inbuf,int inlen,char *outbuf,int outlen) {
//		char **pin = &inbuf;
//		char **pout = &outbuf;
//
//		memset(outbuf,0,outlen);
//		return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
//	}
//};


/************************************************************************/
/*                    代码转换操作类                                    */
/************************************************************************/

//构造
CodeConverter::CodeConverter(const char *from_charset,const char *to_charset) 
{
	cd = iconv_open(to_charset,from_charset);
}

//析构
CodeConverter::~CodeConverter() 
{
	iconv_close(cd);
}

//转换输出
int CodeConverter::convert(char *inbuf,int inlen,char *outbuf,int outlen) 
{
	char **pin = &inbuf;
	char **pout = &outbuf;

	memset(outbuf,0,outlen);
	return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
}


#endif

#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"
#include "cari_net_stateInfo.h"

//全局变量的定义
bool loopFlag	= true;
int  global_port_num = PORT_SERVER; 
int  global_client_num = 0;
int  shutdownFlag; //平台是否按下shutdown命令操作

//宏定义的map
map<string,string> global_define_vars_map;

pthread_mutex_t mutex_syscmd ;//pthread自带的互斥锁

//--------------------------------------------------------------------------------------------------//
//公共函数

//初始化锁
void initmutex()
{
	pthread_mutex_init(&mutex_syscmd, NULL);
}

//异步命令处理线程的入口函数
void * asynProCmd(void *attr)
{
	int					iResult			= CARICCP_SUCCESS_STATE_CODE;
	inner_CmdReq_Frame	*inner_reqFrame	= NULL;

	//使用循环标识控制
	while (loopFlag) {
		//从队列中取出一个元素(以前存放的"请求帧")
		iResult = CMsgProcess::getInstance()->popCmdFromQueue(inner_reqFrame);

		if (CARICCP_ERROR_STATE_CODE == iResult || NULL == inner_reqFrame) {
			//销毁"请求帧"
			CARI_CCP_VOS_DEL(inner_reqFrame);
			continue;
		}

		//处理请求帧,内部处理必须保证顺序性,既:同步处理
		//新建一个"响应帧",并设置一些必要的参数
		inner_ResultResponse_Frame	*inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

		//初始化内部响应帧,并从通用请求帧拷贝有关数据给响应帧
		initInnerResponseFrame(inner_reqFrame, inner_respFrame);

		//直接进行逻辑处理 :针对队列中的每条命令本质上还是同步处理
		CMsgProcess::getInstance()->syncProCmd(inner_reqFrame, inner_respFrame);

		//将结果帧直接发送给对应的客户端,这个结果帧不用在放置到队列中
		//否则造成客户端可能会等待较长的一段时间获得响应结果
		CMsgProcess::getInstance()->commonSendRespMsg(inner_respFrame);

		//销毁 "请求帧"
		CARI_CCP_VOS_DEL(inner_reqFrame);

		//销毁 "响应帧"
		CARI_CCP_VOS_DEL(inner_respFrame);
	}


//	if (NULL != attr) {
//		pthread_attr_t	attr_Thd;
//		attr_Thd = *((pthread_attr_t *) attr); //shutdown的时候出现问题??
//
//#if defined(_WIN32) || defined(WIN32)
//		pthread_exit(attr_Thd);//线程退出?????
//#else
//#endif
//	} else {
//		pthread_exit(NULL);
//	}

	return NULL;	  //return也会导致本函数所在的线程退出
}

/*初始化请求帧
*/
void initCommonRequestFrame(common_CmdReq_Frame *&common_reqFrame)
{
	memset(common_reqFrame, 0, sizeof(common_CmdReq_Frame));
}

/*初始化内部"请求帧"
*/
void initInnerRequestFrame(inner_CmdReq_Frame *&innner_reqFrame)
{
	memset(innner_reqFrame, 0, sizeof(inner_CmdReq_Frame));
}

void initCommonResponseFrame(common_ResultResponse_Frame *&common_respFrame)
{
	memset(common_respFrame, 0, sizeof(common_ResultResponse_Frame));
}

/*初始化内部"响应帧",并拷贝有关数据
*/
void initInnerResponseFrame(inner_CmdReq_Frame *&inner_reqFrame, //内部请求帧
							inner_ResultResponse_Frame *&inner_respFrame) //输出参数 :初始化的"内部响应帧"
{
	//初始化
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化响应帧的值
	inner_respFrame->header.sClientID = inner_reqFrame->header.sClientID;
	inner_respFrame->header.m_NEID = inner_reqFrame->header.m_NEID;
	inner_respFrame->header.sClientChildModID = inner_reqFrame->header.sClientChildModID;
	inner_respFrame->header.iCmdCode = inner_reqFrame->body.iCmdCode;
	inner_respFrame->header.bSynchro = inner_reqFrame->body.bSynchro;
	inner_respFrame->header.bNotify = inner_reqFrame->body.bNotify;

	inner_respFrame->header.isFirst = true;
	inner_respFrame->header.isLast = true;

	//结果是否需要立即返回给client
	inner_respFrame->isImmediateSend = true; 

	//其他关键信息设置
	inner_respFrame->socketClient = inner_reqFrame->socketClient;

	// 	memcpy(inner_respFrame->header.strLoginUserName,
	// 		inner_reqFrame->header.strLoginUserName,
	// 		sizeof(inner_respFrame->header.strLoginUserName));
	// 	memcpy(inner_respFrame->header.timeStamp,
	// 		inner_reqFrame->header.timeStamp,
	// 		sizeof(inner_respFrame->header.timeStamp));

	//使用myMemcpy替换memcpy
	myMemcpy(inner_respFrame->header.strCmdName, inner_reqFrame->body.strCmdName, sizeof(inner_respFrame->header.strCmdName));
}

/*初始化内部响应帧,从switch_event_t拷贝有关信息,主要是用于"通知"类型的事件处理
*/
void initInnerResponseFrame(inner_ResultResponse_Frame *&inner_respFrame, switch_event_t *event)
{
	////解析"命令码/命令名"等关键信息,以前遗留下的部分字段,无用
	//const char	*socketid	= switch_event_get_header(event, SOCKET_ID);
	const char	*clientId	= switch_event_get_header(event, CLIENT_ID);
	//const char	*neId		= switch_event_get_header(event, CARICCP_NE_ID);
	//const char	*csubmodId	= switch_event_get_header(event, CARICCP_SUBCLIENTMOD_ID);
	const char	*cmdCode	= switch_event_get_header(event, CARICCP_CMDCODE);
	const char	*cmdName	= switch_event_get_header(event, CARICCP_CMDNAME);
	const char	*bnotify	= switch_event_get_header(event, CARICCP_NOTIFY);		//通知标识

	//返回的结果信息.注:结果集不在此出处理
	const char	*returnCode	= switch_event_get_header(event, CARICCP_RETURNCODE);	//代表结果码/通知码
	const char	*resuleDesc	= switch_event_get_header(event, CARICCP_RESULTDESC);

	////对内部响应帧进行赋值

	//下面的标识影响到客户端的显示问题
	inner_respFrame->header.isFirst = true;
	inner_respFrame->header.isLast = true;
	inner_respFrame->isImmediateSend = true; //结果是否需要立即返回给client!!!!!!!!!!!!!!!!!!!!!!


	////将返回结果中关键的信息设置到内部响应帧中
	//inner_respFrame->socketClient = (u_int) stringToInt(socketid);
	//inner_respFrame->header.sClientChildModID = (u_short) stringToInt(csubmodId);
	//inner_respFrame->header.iCmdCode = stringToInt(cmdCode);
	inner_respFrame->header.bNotify = (u_short) stringToInt(bnotify);    //通知标识,true或false
	inner_respFrame->header.iResultCode = stringToInt(returnCode);       //通知码或返回码

	if (NULL != clientId) {
		inner_respFrame->header.sClientID = (u_short) stringToInt(clientId);
	}
	if (NULL != cmdName) {
		memcpy(inner_respFrame->header.strCmdName, cmdName, sizeof(inner_respFrame->header.strCmdName));
	}
	if (NULL != cmdCode){
		inner_respFrame->header.iCmdCode = stringToInt(cmdCode);
	}
	if (NULL != resuleDesc) {
		memcpy(inner_respFrame->header.strResuleDesc, resuleDesc, sizeof(inner_respFrame->header.strResuleDesc));
	}
}

/*参数类型的转换
*@param  strParam :
*@param  vecParam :输出参数,存放参数帧的结构
*@param  splitflag :分割符
*/
void covertStrToParamStruct(char *strParameter, 
							vector<simpleParamStruct> &vecParam,//输出参数:存放参数结构的容器
							const char *splitflag)
{
	string strParam(strParameter); //参数串

	int				iPara;
	string			strTmp, strName;
	string			strS, strBegin, strMid, strValue;
	vector< string>	resultVec;

	size_t			beginIndex, midIndex, endIndex;

	bool	bTmpFlag	= false;//标识:表示参数的值是否含有引号

	//有一种情况必须考虑,如果参数的内容含有逗号","的情况???
	//如 :name ="ab,cd";
	splitString(strParam, splitflag, resultVec);//参数之间的分割符号","
	//体格式如 :name:1 = stu , password:2 =123

	for (vector< string>::iterator iter = resultVec.begin(); iter != resultVec.end(); ++iter) {
		/*------------------------------------------*/
		simpleParamStruct	paramStruct;//是否需要new???

		strS = *iter;
		trim(strS);  //1:name = stu CARICCP_EQUAL_MARK

		//解析参数名
		beginIndex = strS.find_first_of(CARICCP_EQUAL_MARK);		 //"="标识符
		if (string::npos != beginIndex) {
			strBegin = strS.substr(0, beginIndex);			//参数号:参数名
			strValue = strS.substr(beginIndex + 1);			//参数值

			//把参数号和参数名解析出来
			midIndex = strBegin.find_last_of(CARICCP_COLON_MARK);	//":"
			if (string::npos != midIndex) {
				//参数号
				strTmp = strBegin.substr(0, midIndex);		//参数号:参数名
				trim(strTmp);
				iPara = atoi(strTmp.c_str());
				//参数名
				strTmp = strBegin.substr(midIndex + 1);
				trim(strTmp);

				//
				paramStruct.strParaName = strTmp;
				paramStruct.sParaSerialNum = iPara;
			} else {
				trim(strBegin);
				//只有参数名
				paramStruct.strParaName = strBegin;
				paramStruct.sParaSerialNum = CARICCP_MAX_SHORT;		//无效值
			}

			trim(strValue);

			//如果是"文本"或"密码"类型的参数,其值引号封装
			//如: name="tiom"
			beginIndex = strValue.find_first_of(CARICCP_QUO_MARK);	//第一个"\""标识符
			if (string::npos != beginIndex) {
				endIndex = strValue.find_last_of(CARICCP_QUO_MARK);	//最后一个"\""标识符
				if (string::npos != endIndex) {
					//有可能相等,如: "aaade
					if (beginIndex < endIndex) {
						//获得对应的真正的值
						strValue = strValue.substr(beginIndex + 1, endIndex - beginIndex - 1);	 //参数值
					} else {
						//获得对应的真正的值,去掉参数封装的第一个引号
						strValue = strValue.substr(beginIndex + 1);	  //参数值
					}
				}//end 不含有"\""

				//其他情况,直接当成参数的值处理
			}

			paramStruct.strParaValue = strValue;

			vecParam.push_back(paramStruct);				//存放到容器中保持起来
		}
		/*---此分支的处理比较特殊,针对参数的 "值"中含有逗号和引号的特殊情况---------*/
		//end 参数值不含有"="号,此时可以当作是上个参数的值部分
		else {
			//先取出上个参数(的引用)
			vector< simpleParamStruct>::reference	lastParam	= vecParam.back();

			if (bTmpFlag) {
				lastParam.strParaValue.append(CARICCP_QUO_MARK);
				bTmpFlag = false;
			}

			//填充上个参数对应的值
			lastParam.strParaValue.append(splitflag);

			//如果是"文本"或"密码"类型的参数,其值都含有引号,需要去除
			endIndex = strS.find_last_of(CARICCP_QUO_MARK);//"\""
			if (string::npos != endIndex) {
				string	strTmp	= "";
				size_t	iTmp	= endIndex + 1;
				if (strS.length() == iTmp) {
					bTmpFlag = true;
					strTmp = strS.substr(0, endIndex);	 //参数值,从位置0开始,取前endIndex个值
					lastParam.strParaValue.append(strTmp);
				}
			} else {
				lastParam.strParaValue.append(strS);
			}
		}
	}
}

/*初始化元素
 */
void initSocket3element(socket3element &element)
{
	//此处主要初始化客户端号
	element.sClientID = CARICCP_MAX_SHORT;
}

/*初始化事件对象
*/
void initEventObj(inner_CmdReq_Frame *&reqFrame, switch_event_t *&event)
{
	//主要是涉及到SOCKET/客户端号/是否广播等关键信息
	string	strSocketID				= intToString(reqFrame->socketClient);
	string	strClientID				= intToString(reqFrame->header.sClientID);
	string	strClientSubModuleID	= intToString(reqFrame->header.sClientChildModID);
	string	strSynchro				= intToString(reqFrame->body.bSynchro);
	string	strCmdCode				= intToString(reqFrame->body.iCmdCode);
	string	strCmdName				= reqFrame->body.strCmdName;
	string	strNotify				= intToString(reqFrame->body.bNotify);

	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, SOCKET_ID, strSocketID.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CLIENT_ID, strClientID.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_SUBCLIENTMOD_ID, strClientSubModuleID.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_CMDCODE, strCmdCode.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_CMDNAME, strCmdName.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_SYNCHRO, strSynchro.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, strNotify.c_str());
}

///*数据库调用的回调函数,为什么不能执行???
//*/
//static int db_callback(void *pArg, int argc, char **argv, char **columnNames)
//{
//	char **r = (char **) pArg;
//	*r = strdup(argv[0]);
//
//	return 0;
//}

/*从数据库中获得记录
*/
int getRecordTableFromDB(switch_core_db_t *db,		//由调用者负责释放
						 const char *sql, 
						 vector<string>& resultVec,	//输出的结果数据表
						 int *nrow,                 //记录的行数
						 int *ncolumn,              //记录的列数
						 char **errMsg) //目前暂不使用
{
	int iRes = CARICCP_ERROR_STATE_CODE;
	char *errmsg = NULL;
	char **resultp = NULL;
	int iCount = 0;

#ifdef SWITCH_HAVE_ODBC
#else
#endif

	//switch_core_db_exec(db, sql, db_callback, NULL, &errmsg);//采用回调函数的方式,但不成功???
	switch_core_db_get_table(db,sql,&resultp,nrow,ncolumn,&errmsg);
	if (errmsg){
		goto end;
	}

	//获得的记录的数量
	//注意:真正的数据从nrow开始
	iCount = (*nrow) + (*nrow) * (*ncolumn);
	for(int i = *nrow; i < iCount; i++){
		char *chRes = resultp[i];
		resultVec.push_back(chRes);
	}

	//while(resultp && *resultp){//当循环次数超过(*nrow) * (*ncolumn),此时的指针就有问题了
	//	char *r1 = *resultp;
	//	//printf("%s\n",r1);
	//	resultp++;
	//}

	//释放查询获得的记录内存
	switch_core_db_free_table(resultp);
	iRes = CARICCP_SUCCESS_STATE_CODE;

end:
	switch_core_db_free(errmsg);
	return iRes;
}

/*初始化"主动上报的响应帧"
*/
void initDownRespondFrame(inner_ResultResponse_Frame *&inner_respFrame, 
						  int clientModuleID,	 
						  int cmdCode)
{
	//初始化"内部响应帧"结构体
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化赋值,设置了"通知类型的标识"和"通知码"
	//下面的标识影响到客户端的显示问题
	inner_respFrame->header.isFirst = true;
	inner_respFrame->header.isLast = true;
	inner_respFrame->isImmediateSend = true; //结果是否需要立即返回给client!!!!!!!!!!!!!!!!!!!!!!

	////将返回结果中关键的信息设置到内部响应帧中
	//inner_respFrame->socketClient = (u_int) stringToInt(socketid);
	inner_respFrame->header.sClientChildModID = (u_short) clientModuleID;//客户端负责处理的模块
	inner_respFrame->header.iCmdCode = cmdCode;
	inner_respFrame->header.bNotify = 0;    //通知标识,true或false
	inner_respFrame->header.iResultCode = 0;//返回码
}

/*根据参数名获得对应的参数值
*/
string getValue(string name, inner_CmdReq_Frame *&reqFrame)
{
	string										strV	= "";
	simpleParamStruct							simpleParam;
	vector<simpleParamStruct>::const_iterator	iter	= reqFrame->innerParam.m_vecParam.begin();
	for (; iter != reqFrame->innerParam.m_vecParam.end(); ++iter) {
		simpleParam = *iter;
		if (isEqualStr(name, simpleParam.strParaName))//查找到参数名字
		{
			strV = simpleParam.strParaValue;
			break;
		}
	}

	return strV;
}

/*自定义的优化的内存拷贝,主要应用于字符串的拷贝使用.
*如果source的长度过长,会被截掉
*/
void myMemcpy(char *dest, const char *source, int sizeDest)
{
	if(!source){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "myMemcpy error,source is NULL \n");
		memcpy(dest, "", 1);
		return;
	}

	int	sizeSource	= (int) strlen(source);
	int	size		= CARICCP_MIN(sizeDest, sizeSource);
	memcpy(dest, source, size);
}

/*判断map容器中的key是否存在
*/
// template<typename TKey,typename TValue>
// bool isValidKey(const map<TKey,TValue> &mapContainer,TKey key)
// {
// 	map<TKey,TValue>::const_iterator it = mapContainer.find(key);
// 	if (it != mapContainer.end())
// 	{
// 		return true;//key元素存在
// 	}
// 	
// 	return false;
// }

bool isValidKey()
{
	return true;
}

/*判断字串是否为有效数字
*/
bool isNumber(string strvalue)
{
	char ch;
	int iLength = strvalue.length();
	for (int i = 0; i < iLength; i++) {
		ch = strvalue.at(i);
		if ('0' > ch || '9' < ch) {
			return false;
		}
	}

	return true;
}

/*判断是由有效的IP地址
*/
bool isValidIPV4(string strIP)
{
	vector<string> vec;
	string str1,str2,str3,str4;
	int iFiled1,iFiled2,iFiled3,iFiled4;

	trim(strIP);
	if (0 == strIP.length()){
		return false;
	}
	splitString(strIP,CARICCP_DOT_MARK,vec,true);
	if (4 != vec.size()){//ip地址无效
		return false;
	}

	str1 = getValueOfVec(0,vec);
	str2 = getValueOfVec(1,vec);
	str3 = getValueOfVec(2,vec);
	str4 = getValueOfVec(3,vec);
	trim(str1);
	trim(str2);
	trim(str3);
	trim(str4);
	if (0 == str1.length()
		|| 0 == str2.length()
		|| 0 == str3.length()
		|| 0 == str4.length()){
		return false;
	}

	//判断是否有效数字,位数不能超过3,数值不能超过255
	if (!isNumber(str1.c_str()) 
		|| !isNumber(str2.c_str())
		|| !isNumber(str3.c_str())
		|| !isNumber(str4.c_str())){
		return false;
	}
	if ( 3<str1.length() 
		|| 3<str2.length()
		|| 3<str3.length()
		|| 3<str4.length()){
		return false;
	}

	//每个域段的取值范围
	iFiled1 = stringToInt(str1.c_str());
	iFiled2 = stringToInt(str2.c_str());
	iFiled3 = stringToInt(str3.c_str());
	iFiled4 = stringToInt(str4.c_str());
	if (255 < iFiled1
		|| 255 < iFiled2
		|| 255 < iFiled3
		|| 255 < iFiled4){
		return false;
	}

	//第一个字段不能为0
	if(0 == iFiled1){
		return false;
	}
	
	return true;
}

/*按照分割字符将字符串分割成多个子串
*@param strSource :源字符串
*@param strSplit  :分割字符
*@param resultVec :输出参数:存放分割后的字串
*/
//template<typename T>
void splitString(const string &strSource, 
				 const string &strSplit, 
				 vector<string> &resultVec, 
				 bool bOutNullFlag)
{
	string	strTmp;
	size_t	p1 = 0, p2	= 0, len = 0;
	len = strlen(strSplit.c_str());
	if(0 == len){
		return;
	}

	//需要考虑空串"",将其当成正常的值存放到容器中,对于查询类型的结果可能需要这类情况
	if (bOutNullFlag) {
		while (string::npos != (p2 = strSource.find(strSplit, p1))) {
			strTmp = strSource.substr(p1, p2 - p1);

			//////////////////////////////////////////////////////////////////////////
			//trimRight(strTmp);
			//if (0 != strTmp.size()) {
			resultVec.push_back(strTmp);//存放到容器中
			//}

			p1 = p2 + len;
		}
	}
	else{//不需要考虑空串"",无效数据,摒弃
		while (string::npos != (p2 = strSource.find(strSplit, p1))) {
			strTmp = strSource.substr(p1, p2 - p1);
			trim(strTmp);//先trim处理一下
			if (0 != strTmp.size()) {
				resultVec.push_back(strTmp);//存放到容器中
			}

			p1 = p2 + len;
		}
	}

	//处理最后一个字串
    strTmp = strSource.substr(p1);
	if (bOutNullFlag){
		resultVec.push_back(strTmp);
	}
	else{
		trim(strTmp);
		if(0 != strTmp.size()){
			resultVec.push_back(strTmp);
		}
	}
}

/* 过滤相同的字串,如:A,A,B,应返回A,B
*/
string filterSameStr(string strSource,
					 string strSplit)
{
	string str1,str2,strResult="";
	bool bSame = false;
	vector<string> resultVec,newVec;
	int isize =0,i=0;
	splitString(strSource,strSplit,resultVec);

	for (vector<string>::iterator it = resultVec.begin(); resultVec.end() != it; it++){
		bSame = false;
		str1 = *it;
		for (vector<string>::iterator it2 = newVec.begin(); newVec.end() != it2; it2++){
			str2 = *it2;
			if (isEqualStr(str1,str2)){
				bSame = true;
				break;
			}
		}
		if (!bSame){
			newVec.push_back(str1);
		}
	}

    isize = newVec.size();
	for (vector<string>::iterator it2 = newVec.begin(); newVec.end() != it2; it2++){
		i++;
		str2 = *it2;
		strResult.append(str2);
		if (isize != i){
			strResult.append(strSplit);
		}
	}

	return strResult;
}

/*自定义取出字符首尾空格的方法
*/
string & trim(string &str)
{
	if (str.empty()) {
		return str;
	}

	//先替换特殊符号 tab
	replace_all(str, CARICCP_TAB_MARK, CARICCP_SPACEBAR);

	//\r\n一般出现在末尾处
	replace_all(str, CARICCP_ENTER_KEY, CARICCP_SPACEBAR);

	//继续优化一下
	replace_all(str, "\n", CARICCP_SPACEBAR);
	replace_all(str, "\r", CARICCP_SPACEBAR);

	//左右的空格键
	str.erase(0, str.find_first_not_of(CARICCP_SPACEBAR));
	str.erase(str.find_last_not_of(CARICCP_SPACEBAR) + 1);

	return str;
}

/*只去除右边的符号
*/
string & trimRight(string &str)
{
	if (str.empty()) {
		return str;
	}


	//右的空格键
	str.erase(str.find_last_not_of(CARICCP_SPACEBAR) + 1);
	return str;
}

/*重载函数
*/
char * trimChar(char *ibuf) //最好使用char[]类型的参数
{
	int	nlen, i;

	/*判断字符指针是否为空及字符长度是否为0*/
	if (ibuf == NULL) {
		return CARICCP_NULL_MARK;
	}

	nlen = strlen(ibuf);
	if (nlen == 0)
		return CARICCP_NULL_MARK;

	/*去前空格*/
	i = 0;
	while (i < nlen) {
		/*if (ibuf[i]!=' ')*/
		if ((ibuf[i] != 0x20) && (ibuf[i] != '\t') && (ibuf[i] != '\r') && (ibuf[i] != '\n')) {
			break;
		}

		i ++;
	}

	if (i == nlen) {
		ibuf[0] = 0;
		return CARICCP_NULL_MARK;
	}

	if (i != 0) {
		nlen = nlen - i + 1;
		memcpy(ibuf, &ibuf[i], nlen);
	}

	/*去后空格*/
	nlen = strlen(ibuf) - 1;
	while (nlen >= 0) {
		/*if (ibuf[nlen] != ' ')*/
		if ((ibuf[nlen] != 0x20) && (ibuf[nlen] != '\t') && (ibuf[nlen] != '\r') && (ibuf[nlen] != '\n')) {
			ibuf[nlen + 1] = 0;

			break;
		}
		nlen --;
	}

	return ibuf;
}

/*字符是否为空
*/
bool isNullStr(const char* chBuf)
{
	if (NULL == chBuf){
		return true;
	}

	string str = chBuf;
	if (0 == str.length()){
		return true;
	}
	return false;
}

/*替换字符串中指定的字符
*@param str 	 :源字串
*@param old_mark :待替换的旧字串
*@paramnew_mark  :新的字串
*/
string & replace_all(string &str, const string &old_mark, const string &new_mark)
{
	int	iOldLen	= old_mark.length();
	for (; ;) {
		string::size_type	pos	(0);   
		if (string::npos != (pos = str.find(old_mark))) {
			str.replace(pos, iOldLen, new_mark);
		} else {
			break;
		}
	}  

	return   str;
}   

/*判断两个字符串是否相等,忽略大小写
*/
bool isEqualStr(const string str1, const string str2)
{
	int		iResult	= 0;
	size_t	max	= CARICCP_MAX(str1.length(), str2.length());
	iResult = strncasecmp(str1.c_str(), str2.c_str(), max);
	if (0 == iResult) {
		return true;
	}

	return false;
}

/*
 */
string stradd(char *str, char c) //str为原字符串,c为要追加的字符
{
	//strcat(str, (const char *)c); 
	//	int n = strlen(str);
	// 	str[n] = c; //追加字符
	// 	str[n+1] = 0; //添加结束符
	string strTest(str);
	char	szText[sizeof(str) + 1]	= {
		0
	};
	memset(szText, 0, sizeof(szText));
	strcpy(szText, strTest.c_str());
	szText[sizeof(str)] = c;

	string	s	= &(szText[0]); 
	return trim(s);
} 


#if defined(_WIN32) || defined(WIN32)
/*
* A simple printf function. Only support the following format:
* Code Format
* %c character
* %d signed integers
* %i signed integers
* %s a string of characters
* %o octal
* %x unsigned hexadecimal
*/
int my_printf(const char *format, ...)
{
	va_list	arg;
	int		done	= 0;

	va_start(arg, format);
	//done = vfprintf (stdout, format, arg);

	while (*format != '\0') {
		if (*format == '%') {
			if (*(format + 1) == 'c') {
				char	c	= (char) va_arg(arg, int);
				putc(c, stdout);
			} else if (*(format + 1) == 'd' || *(format + 1) == 'i') {
				char	store[20];
				int		i	= va_arg(arg, int);
				char	*str	= store;
				itoa(i, store, 10);
				while (*str != '\0')
					putc(*str++, stdout);
			} else if (*(format + 1) == 'o') {
				char	store[20];
				int		i	= va_arg(arg, int);
				char	*str	= store;
				itoa(i, store, 8);
				while (*str != '\0')
					putc(*str++, stdout);
			} else if (*(format + 1) == 'x') {
				char	store[20];
				int		i	= va_arg(arg, int);
				char	*str	= store;
				itoa(i, store, 16);
				while (*str != '\0')
					putc(*str++, stdout);
			} else if (*(format + 1) == 's') {
				char *str	= va_arg(arg, char *);
				while (*str != '\0')
					putc(*str++, stdout);
			}

			// Skip this two characters.

			format += 2;
		} else {
			putc(*format++, stdout);
		}
	}

	va_end(arg);

	return done;
} 
#endif

/* 一个简单的类似于printf的实现,通过变参实现
* Code Format
* %c character
* %d signed integers
* %i signed integers
* %s a string of characters
* %o octal
* %x unsigned hexadecimal
*
* 注 :字符串类型参数传入时必选使用c_str()形式转换
*/
//string formatStr(const char *fmt, ...)  	  //一个简单的类似于printf的实现
//											 //注:参数为字符串不能使用变量,必须转换为const char*形式
//{
//	vector< char>	vecCh;
//	vecCh.clear();
//
//	//char* pArg=NULL;  			 //等价于原来的va_list
//	va_list	pArg;
//
//	char	c;
//	//pArg = (char*) &fmt;  		//注意不要写成p = fmt !!因为这里要对参数取址,而不是取值
//	//pArg += sizeof(fmt);  		//等价于原来的va_start  	   
//
//	va_start(pArg, fmt);
//
//	do {
//		c = *fmt;
//		if (c != '%') {
//			vecCh.push_back(c);
//
//			//照原样输出字符
//			//putchar(c);   		 
//		} else {
//			//按格式字符输出"参数"
//			switch (*(++fmt)) {
//			case 'd':
//				//十进制数
//				 {
//					//printf("%d",*((int*)pArg));  
//
//					int		sc	= (int) *((int *) pArg);
//					char	sTmp[16];//16个单位长度足够长
//					sprintf(sTmp, "%d", sc);
//
//
//					char *sT	= sTmp;
//					while ('\0' != *sT) {
//						vecCh.push_back(*sT);
//						sT++;
//					}
//
//					//pArg += sizeof(int); 
//					va_arg(pArg, int);
//					break;
//				}
//
//			case 'x':
//				//printf("%#x",*((int*)pArg));
//				//pArg += sizeof(int);
//				va_arg(pArg, int);
//				break;
//
//			case 'f':
//				//printf("%f",*((float*)pArg));
//				//pArg += sizeof(int);
//				va_arg(pArg, int);
//				break;
//			case 's':
//				//字符串
//				 {
//					char *ch	= *((char **) pArg);
//					//printf("%s",ch);
//
//					while ('\0' != *ch) {
//						vecCh.push_back(*ch++);
//					}
//
//					//pArg += sizeof(char*);
//					va_arg(pArg, char*);
//					break;
//				}
//			default:
//				break;
//			}
//
//			//pArg += sizeof(int);  			 //等价于原来的va_arg
//			//va_arg(pArg,int);
//		}
//
//		++fmt;
//	} while ('\0' != *fmt);
//
//
//	//pArg = NULL;  							 //等价于va_end
//	va_end(pArg);
//
//	//将vecCh传唤成字符串
//
//	string	strResult	= "";
//	char	chTmp;
//	for (vector< char>::iterator iter = vecCh.begin(); iter != vecCh.end(); ++iter) {
//		chTmp = *iter;
//		if ('\0' != chTmp) {
//			strResult += chTmp;
//		}
//	}
//
//	return  strResult;
//}

string formatStr(const char* fmt,...) //注:参数为字符串不能使用变量,必须转换为const char*形式.string则使用c_str()函数处理
{
	vector<char> vecCh;
	vecCh.clear();

	char* pArg=NULL;               //等价于原来的va_list
	//va_list pArg;

	char c;
	pArg = (char*) &fmt;          //注意不要写成p = fmt !!因为这里要对参数取址,而不是取值
	pArg += sizeof(fmt);          //等价于原来的va_start         

	//va_start(pArg,fmt);

	do
	{
		c =*fmt;
		if (c != '%')
		{
			vecCh.push_back(c);

			//照原样输出字符
			//putchar(c);            
		}
		else
		{
			//按格式字符输出"参数"
			switch(*(++fmt))
			{
			case 'd'://十进制数
				{
					int sc = (int)*((int*)pArg);
					char sTmp[16];			//16个单位长度足够长
					sprintf(sTmp,"%d",sc);  //组装成新的字串 sTmp


					char *sT =sTmp;
					while('\0' != *sT)
					{
						vecCh.push_back(*sT);
						sT++;
					}

					pArg += sizeof(int); 
					break;
				}

			case 'x':
				pArg += sizeof(int); 
				break;

			case 'f':
				printf("%f",*((float*)pArg));
				pArg += sizeof(int); 
				break;
			case 's'://字符串
				{
					char* ch = *((char**)pArg);

					//优化处理
					//if (NULL == ch){
					//	break;
					//}

					while('\0' != *ch)
					{
						vecCh.push_back(*ch++);
					}

					pArg += sizeof(char*); 
					break;
				}
			default:

				break;
			}

			//pArg += sizeof(int);               //等价于原来的va_arg
			//va_arg(pArg,int);
		}

		++fmt;

	}while ('\0' != *fmt);


	pArg = NULL;                                //等价于va_end
	//va_end(pArg);

	//将vecCh传唤成字符串

	string strResult = "";
	char chTmp;
	for(vector<char>::iterator iter=vecCh.begin();iter!=vecCh.end();++iter)
	{
		chTmp = *iter;
		if ('\0' != chTmp)
		{
			strResult += chTmp;
		}
	}

	return  strResult;
}

/*把int类型的值转换为bool类型
*/
bool intToBool(int ivalue)
{
	if (0 == ivalue) {
		return false;
	}

	return true;
}

/*把int类型的值转换为string类型
*/
string intToString(int ivalue)
{
	char	buffer[10];//长度定义必须<=10
	memset(buffer, 0, sizeof(buffer));

#if defined(_WIN32) || defined(WIN32)
	itoa(ivalue, buffer, 10);
#else
	sprintf(buffer,"%d",ivalue);
#endif
	
	string str(buffer);
	return str;
}

/*把string类型转换为int类型
*/
int stringToInt(const char *strValue)
{
	if (NULL == strValue) {
		return 0;
	}
	//return atoi(strValue);
	return strtoul(strValue,NULL,10);
}

/*转换成小写字符
*/
void stringToLower(string &strSource)
{
#if defined(_WIN32) || defined(WIN32)
	strlwr((char*)strSource.c_str()); //转换成小写 
#else
	string newstr = "";
	char buf[8];
	int ilen = strSource.length();
	for (int i = 0;i < ilen; i++){
		char ch = strSource.at(i);
		ch = tolower(ch);
		sprintf(buf,"%c",ch);
		newstr.append(buf);
	}
	strSource = newstr;
#endif
}

/*字符转换成小写
*/
char tolower(char t)
{   
	if(t>='A'&&t<='Z')   
		t+='a'-'A';   
	return t;   
}   

char toupper(char t)
{   
	if(t>='a'&&t<='z')   
		t-='a'-'A';   
	return   t;   
}

/*转换成百分数
*/
string convertToPercent(string strMolecule,   //分子
						string strDenominator,//分母
						bool bContainedDot)   //是否包含小数点
{
	int i_molecule,i_denominator,iRes=0;
	double dRtio;
	string strRes="";
	char buf[16];
	if (!isNumber(strMolecule.c_str()) 
		|| !isNumber(strDenominator.c_str())){
		goto end;
	}

	i_molecule = stringToInt(strMolecule.c_str());
	i_denominator = stringToInt(strDenominator.c_str());
	if (0 == i_denominator){
		goto end;
	}

	dRtio = i_molecule * 1.0/i_denominator;

	//是否要包含小数点
	if (bContainedDot){
		iRes = (int)(dRtio*100);
	}
	else{
		iRes = (int)(dRtio*100);
	}

	sprintf(buf,"%d%s",iRes,CARICCP_PERCENT_MARK);
	strRes = buf;

end:
	return strRes;
}

/*获得时间戳
*/
char * getLocalTime(char *strTime)
{
	struct tm	*ptr;
	time_t		now;

	time(&now);
	ptr = localtime(&now);

	strftime(strTime, CARICCP_MAX_TIME_LENGTH, "%Y-%m-%d %H:%M:%S", ptr);//年-月-日 时:分:秒

	return strTime;
}

/*格式化获得的结果
*/
void formatResult(const string &msg, map< string,string> &mapRes)//输出参数
{
	if (0 == msg.length()) {
		return;
	}

	vector< string>	vecRes;
	splitString(msg, CARICCP_SPECIAL_SPLIT_FLAG, vecRes);
	string	strTmp, strKey, strValue;
	size_t	beginIndex;

	for (vector< string>::const_iterator iter = vecRes.begin(); iter != vecRes.end(); iter++) {
		strTmp = *iter;

		beginIndex = strTmp.find_first_of(CARICCP_EQUAL_MARK);
		if (string::npos != beginIndex) {
			strKey = strTmp.substr(0, beginIndex);
			strValue = strTmp.substr(beginIndex + 1);
		} else {
			strKey = strTmp;
			strValue = CARICCP_NULL_MARK;
		}

		trim(strKey);
		trim(strValue);

		mapRes.insert(map< string,string>::value_type(strKey, strValue));
	}
}

/*长整数或起始以"0"开始的整数,进行增加操作
*/
void addLongInteger(char operand1[],
					char operand2[],
					char result[])   
{   
	int i,e,d;   
	int n,m;   
	char temp; 

	//防止更改参数,此处使用局部变量操作
	char tmpOperand[MAX_INTLENGTH];
	memset(tmpOperand,0,MAX_INTLENGTH);
	myMemcpy(tmpOperand,operand1,MAX_INTLENGTH);
	
	n=strlen(operand1);   
	m=strlen(operand2);   
	for(i=0;i<n/2;i++){
		temp=tmpOperand[i];
		tmpOperand[i]=tmpOperand[n-1-i];
		tmpOperand[n-1-i]=temp;
	}   
	for(i=0;i<m/2;i++){
		temp=operand2[i];
		operand2[i]=operand2[m-1-i];
		operand2[m-1-i]=temp;
	}   

	e=0;   
	for(i=0;i<n&&i<m;i++){   
		d=tmpOperand[i]-'0'+operand2[i]-'0'+e;   
		e=d/10;   
		result[i]=d%10+'0';   
	}   
	if(i<m){   
		for(;i<m;i++){   
			d=operand2[i]-'0'+e;   
			e=d/10;   
			result[i]=d%10+'0';   
		}   
	}   
	else{   
		for(;i<n;i++){   
			d=tmpOperand[i]-'0'+e;   
			e=d/10;   
			result[i]=d%10+'0';   
		}   
	}   
	if(e){
		result[i++]=e+'0'; 
	}
	result[i]=0;   
	n=i;   

	for(i=0;i<n/2;i++){
		temp=result[i];
		result[i]=result[n-1-i];
		result[n-1-i]=temp;
	}   
} 

/*封装0开头的字符串
*/
string encapSpecialStr(string sourceStr,
					   int oriLen)
{
	int ilen = sourceStr.length();
	string str="";
	//开始封装0开头
	for (int i=0; i<(oriLen-ilen);i++){
		str.append("0");
	}
	str.append(sourceStr);

	return str;
}

/*封装结果,根据vec中存放的信息进行封装
*/
string encapResultFromVec(string splitkey,    //分割符
						  vector<string>& vec)//容器
{
	string strRes = "",strTmp;
	int icout = 0,isize;
	isize = vec.size();
	if (0 == isize){
		return strRes;
	}

	for (vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		icout++;//计数器
		strTmp = *it;

		//不要去除空格优化
		strRes.append(strTmp);
		if(isize !=icout){
			strRes.append(splitkey);//最后结尾不需要再增加此splitkey
		}
	}

	return strRes;
}

/*根据map和key,获得对应的value
*/
string cari_net_map_getvalue(map< string,string> &mapRes, string key, string &value)
{
	map< string,string>::const_iterator	it	= mapRes.find(key);
	if (it != mapRes.end()) {
		value = it->second;
	}

	//value = mapContainer[key]  //编译出现问题!!!!!
	return value;
}

/*从vec容器中删除某个元素,如果元素不存在则返回-1
*/
int eraseVecElement(vector< string> &vec, string element)
{
	int		ipos	= 0;
	string	strT	= "";
	for (vector< string>::iterator it = vec.begin(); it != vec.end(); it++) {
		strT = *it;

		//位置从0开始计算
		ipos++;

		if (!strncasecmp(strT.c_str(), element.c_str(), CARICCP_MAX_STR_LEN(strT.c_str(), element.c_str()))) {
			//删除此元素
			vec.erase(it); //备注 :此时it成了野指针了

			return ipos; //返回所在的位置序列
		}
	}

	return -1;
}

/*封装标准的每一行的xml的内容
*/
string encapSingleXmlString(string strsource, int ispacnum)
{
	string	strT	= "";
	for (int i = 0; i < ispacnum; i++) {
		strT.append("  ");//封装
	}

	strT.append(strsource);

	return strT;
}

/*封装标准的xml内容,主要用于写入到xml文件中.可以针对绝大多数情况进行封装处理
*/
string encapStandXMLContext(string sourceContext)
{
	int				ipos		= 0;
	string			strNew		= "",strTmp	= "";
	string			strLine, strBefore,strSave,strChidname;
	vector< string>	vec, vecSavedChildName, vecNewStr;
	int				beginIndex	= 0,midIndex = 0,endIndex = 0;

	/*splitString(sourceContext, "\n", vec);*/
	splitString(sourceContext, "<", vec);//此方法保证可靠
	for (vector< string>::iterator it = vec.begin(); it != vec.end(); it++) {
		strTmp = *it;
		trim(strTmp);
		if (0 == strTmp.length()) {
			continue;
		}

		//根据下面的逻辑查看是否需要保存此child属性名
		strBefore="";
		beginIndex = strTmp.find(" ");
		if (0 > beginIndex){
			beginIndex = strTmp.find(">");
			if (0 < beginIndex){
				strBefore =  strTmp.substr(0, beginIndex);
			}
			else{
				//非标准的格式
				continue;
			}
		}
		else{
			strBefore =  strTmp.substr(0, beginIndex);
		}
		trim(strBefore);
		if (0 == strBefore.length()) {
			continue;
		}

		//将删除掉的"<"补充回来
		strLine = "";
		strLine.append("<");
		strLine.append(strTmp);

		beginIndex = strLine.find("<!--");//注释行
		if (0 == beginIndex) {
			//也考虑保持原样
			ipos = vecSavedChildName.size();
			ipos++;//显示的位置偏移一下
			strTmp = encapSingleXmlString(strLine, ipos);
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		beginIndex = strLine.find("-->");//注释行
		if (0 == beginIndex) {
			ipos = vecSavedChildName.size();
			ipos++;//显示的位置偏移一下
			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		beginIndex = strLine.find("</");//child标识的结束行
		if (0 <= beginIndex) {
			//查找标识符,"</"占2个字符的位置
			midIndex = strLine.find(">");
			strChidname = strLine.substr(beginIndex + 2, midIndex - (beginIndex + 2));//</variables>,注意数量

			//是否在vecSavedChildName中包含????,一定包含,否则肯定有问题

			//查看级别,在容器中存放的位置,使用空格封装字串
			//vecSavedChildName容器中去除,newstrvec容器保存封装后的新字串
			ipos = eraseVecElement(vecSavedChildName, strChidname);
			if (0 > ipos){//防止出现问题,加强处理一下
				ipos = vecSavedChildName.size();
			}

			ipos--;//位置减少一个

			//合并优化一下,格式上简洁
			if (0 != strSave.length()
				&& (!strcmp(strChidname.c_str(),strSave.c_str()))){
				int idex_0 = 0;
				
				//上次已经封装好的保存下来的行
				strTmp = vecNewStr.back();
				idex_0 = strTmp.find(">");
				strTmp = strTmp.substr(0, idex_0);
				strTmp.append("/>");

				//先删除,再增加
				vecNewStr.pop_back();
			}
			else{
				strTmp = encapSingleXmlString(strLine, ipos);
			}

			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		//如果是一个完整的xml行,如:<param name=\"password\" value=\"%s\"/>
		//<A>或<A/>或<A name= >或<A name = />
		endIndex = strLine.find("/>");
		if (0 < endIndex){ 
			//查看vecSavedChildName中含有几个元素,则就增加几个空格,封装一下
			ipos = vecSavedChildName.size();
			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		//child的属性名
		strChidname = strBefore;

		//是否在vecSavedChildName中已经包含strChidname标识
		//如果包含,vecSavedChildName容器中去除
		ipos = eraseVecElement(vecSavedChildName, strChidname);
		if (-1 != ipos) {
			//封装处理
			strTmp = encapSingleXmlString(strLine, ipos);//主要是获得存放的位置,进行格式封装
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}
		
		//先获得当前的元素个数,确定位置
		ipos = vecSavedChildName.size();
		strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
		vecNewStr.push_back(strTmp);

		//vecSavedChildName保存此child的name标识符号
		vecSavedChildName.push_back(strChidname);

		//-----------------保存起来---------------------
		strSave = strBefore;
		//----------------------------------------------
	}

	//根据保存好的封装好的新的字串,输出,此处再进行优化处理一下
	for (vector< string>::iterator it2 = vecNewStr.begin(); it2 != vecNewStr.end(); it2++) {
		strLine = *it2;
		strNew.append(strLine);
		strNew.append("\n");
	}

	return strNew;
}

///*封装标准的xml内容,主要用于写入到xml文件中.可以针对绝大多数情况进行封装处理
//*/
//string encapStandXMLContext(string sourceContext)
//{
//	int				ipos		= 0;
//	string			strNew		= "",strTmp	= "";
//	string			strLine, strChidname;
//	vector< string>	vec, vecSavedChildName, vecNewStr;
//	int				beginIndex	= 0,midIndex = 0,endIndex = 0;
//
//	/*splitString(sourceContext, "\n", vec);*/
//	splitString(sourceContext, "<", vec);//此方法保证可靠
//	for (vector< string>::iterator it = vec.begin(); it != vec.end(); it++) {
//		strTmp = *it;
//		trim(strTmp);
//		if (0 == strTmp.length()) {
//			continue;
//		}
//
//		//将去除掉的"<"补充回来
//		strLine = "";
//		strLine.append("<");
//		strLine.append(strTmp);
//
//		beginIndex = strLine.find("<!--");//注释行
//		if (0 == beginIndex) {
//			//也考虑保持原样
//			ipos = vecSavedChildName.size();
//			ipos++;//显示的位置偏移一下
//			strTmp = encapSingleXmlString(strLine, ipos);
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//		beginIndex = strLine.find("-->");//注释行
//		if (0 == beginIndex) {
//			ipos = vecSavedChildName.size();
//			ipos++;//显示的位置偏移一下
//			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//			vecNewStr.push_back(strTmp);
//			continue;
//		}
//
//		beginIndex = strLine.find("</");//child标识的结束行
//		if (0 == beginIndex) {
//			//查找标识符
//			midIndex = strLine.find(">");
//			strChidname = strLine.substr(beginIndex + 2, midIndex - beginIndex - 2);//</variables>
//
//			//是否在vecSavedChildName中包含????,一定包含,否则肯定有问题
//
//			//查看级别,在容器中存放的位置,使用空格封装字串
//			//vecSavedChildName容器中去除,newstrvec容器保存封装后的新字串
//			ipos = eraseVecElement(vecSavedChildName, strChidname);
//			if (0 > ipos)//防止出现问题,加强处理一下
//			{
//				ipos = vecSavedChildName.size();
//			}
//
//			ipos--;//位置从0开始计算的
//			strTmp = encapSingleXmlString(strLine, ipos);
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//
//		beginIndex = strLine.find("<");
//		if (0 != beginIndex)//不以<开头,标识此为注释行内容
//		{
//			ipos = vecSavedChildName.size();
//			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//			vecNewStr.push_back(strTmp);
//			continue;
//		}
//
//		//<A>或<A/>或<A name= >或<A name = />
//		endIndex = strLine.find("/>");
//		if (0 < endIndex) //一个完整的xml行,如:<param name=\"password\" value=\"%s\"/>
//		{
//			//查看vecSavedChildName中含有几个元素,则就增加几个空格,封装一下
//			ipos = vecSavedChildName.size();
//			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//
//		//要考虑特殊情况: <X-PRE-PROCESS cmd="include" data="default/*.xml"></X-PRE-PROCESS>
//		//				  <X-PRE-PROCESS></X-PRE-PROCESS>
//		midIndex = strLine.find(" ");
//		if (0 > midIndex) {
//			endIndex = strLine.find("</");//上面描述的一种特殊情况
//			if (0 > endIndex) {
//				endIndex = strLine.find(">");
//
//				//格式:<action>
//				if (0 > endIndex) {
//					//无效的行,为了保持完整,存放起来-----------------------
//					ipos = vecSavedChildName.size();
//					strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//					vecNewStr.push_back(strTmp);
//
//					continue;
//				}
//
//				strChidname = strLine.substr(beginIndex + 1, endIndex - beginIndex - 1);
//			} else//此情况如上面列出
//			{
//				ipos = vecSavedChildName.size();
//				strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//				vecNewStr.push_back(strTmp);
//
//				continue;
//			}
//		}
//		//考虑是否完整的行
//		else {
//			endIndex = strLine.find("</");
//			if (-1 != endIndex)//完整的xml行
//			{
//				ipos = vecSavedChildName.size();
//				strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//				vecNewStr.push_back(strTmp);
//
//				continue;
//			}
//
//			strChidname = strLine.substr(beginIndex + 1, midIndex - beginIndex - 1);//格式:<action 
//		}
//
//		//是否在vecSavedChildName中已经包含strChidname标识
//		//如果包含,vecSavedChildName容器中去除
//		ipos = eraseVecElement(vecSavedChildName, strChidname);
//		if (-1 != ipos) {
//			//封装处理
//			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//			vecNewStr.push_back(strTmp);
//			continue;
//		}
//
//		//如果不包含,则是第一次出现
//		beginIndex = strLine.find("/>");
//		if (0 < beginIndex)//此行为一完整的行
//		{
//			//查看vecSavedChildName中含有几个元素,则就增加几个空格,封装一下
//			ipos = vecSavedChildName.size();
//			ipos--;//位置从0开始计算
//			strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//
//		//先获得当前的元素个数,确定位置
//		ipos = vecSavedChildName.size();
//		strTmp = encapSingleXmlString(strLine, ipos);//根据位置进行封装
//		vecNewStr.push_back(strTmp);
//
//		//vecSavedChildName保存起来此child的name标识符号
//		vecSavedChildName.push_back(strChidname);
//	}
//
//
//	//根据保存好的封装好的新的字串,输出
//	for (vector< string>::iterator it2 = vecNewStr.begin(); it2 != vecNewStr.end(); it2++) {
//		strLine = *it2;
//		strNew.append(strLine);
//		strNew.append("\n");
//	}
//
//	return strNew;
//}


/*获得宏定义变量的值,具体参见配置文件cari_ccp_macro.ini
*/
string getValueOfDefinedVar(string strVar)
{
	char *res="";
	string strVal = "",strKey = "";
	strKey = strVar;

	//mod by xxl 2010-9-25 :将关键字转换成小写
	stringToLower(strKey);

	map<string,string>::const_iterator it = global_define_vars_map.find(strKey);
	if (it != global_define_vars_map.end()){
		 strVal = it->second;
	}

	//优化处理一下:如果val不存在,则返回var的值
	if (0 == strVal.length()){
		strVal = strVar;
	}
	return strVal;
}

/*根据vector,获得某个索引值的字段名
*/
string getValueOfVec(int index,           //索引值从0开始
					 vector<string>& vec) //容器
{
    int i=0;
	string strVal = "";

	vector<string>::const_iterator it = vec.begin();
	for (;it != vec.end(); it++)
	{
	  if (i == index)
	  {
	    strVal = *it;
		break;
	  }

	  i++;
	}

	return strVal;
}

/*根据string,获得某个索引值的字段名
*/
string getValueOfStr(string splitkey,  //分割符
					 int index,		   //索引值从0开始
					 string strSource) //源字串      
{  
	string strTmp,strValue;
	vector<string> vec;
	splitString(strSource,splitkey,vec);
	strValue = getValueOfVec(index,vec);
	return strValue;
}

/*根据索引号插入元素到vector容器中
*/
void insertElmentToVec(int index,
					   string elment,
					   vector<string> &vec)
{
	int iSize = vec.size();
	if (index>=iSize){
		vec.push_back(elment);
		return;
	}

	int icount = -1;
	for (vector<string>::iterator it = vec.begin(); it != vec.end();it++){
		icount++;
		if (index == icount){
			vec.insert(it,1,elment);
			break;
		}
	}
}


/************************************************************************/
/*  																	*/
/************************************************************************/

/*自定义取出字符首尾空格的方法
*/
string & trimSpace(string &str)
{
	if (str.empty()) {
		return str;
	}

	//左右的空格键
	str.erase(0, str.find_first_not_of(CARICCP_SPACEBAR));
	str.erase(str.find_last_not_of(CARICCP_SPACEBAR) + 1);

	str.erase(0, str.find_first_not_of(" "));
	str.erase(str.find_last_not_of(" ") + 1);

	return str;
}

/*呼叫用户的状态发生变化,通知cari net server同步更改
*/
//void cari_switch_state_machine_notify(const char *mobUserID,    //用户号
//													  const char *mobUserState, //用户状态
//													  const char *mobUserInfo)  //用户的信息
//{
//	char *strNotifyCode						= NULL;
//	switch_stream_handle_t stream			= { 0 };
//	int istatus;
//	char * pSendInfo						=NULL;
//
//	if (NULL == mobUserID 
//		|| NULL == mobUserState)
//	{
//		return;
//	}
//
//	//通知码和结果集
//	strNotifyCode  = switch_mprintf("%d", CARICCP_NOTIFY_MOBUSER_STATE_CHANGED); //通知类型
//
//	//返回错误的信息提示操作用户:终端用户不存在
//	pSendInfo = switch_mprintf("%s%s%s%s",				CARICCP_NOTIFY,			CARICCP_EQUAL_MARK, "1",			CARICCP_SPECIAL_SPLIT_FLAG); //通知标识
//	pSendInfo = switch_mprintf("%s%s%s%s%s", pSendInfo, CARICCP_RETURNCODE,		CARICCP_EQUAL_MARK, strNotifyCode,	CARICCP_SPECIAL_SPLIT_FLAG); //返回码=通知码
//	pSendInfo = switch_mprintf("%s%s%s%s%s", pSendInfo, CARICCP_MOBUSERNAME,	CARICCP_EQUAL_MARK, mobUserID,		CARICCP_SPECIAL_SPLIT_FLAG); //用户的号码
//	pSendInfo = switch_mprintf("%s%s%s%s%s", pSendInfo, CARICCP_MOBUSERSTATE,	CARICCP_EQUAL_MARK, mobUserState,	CARICCP_SPECIAL_SPLIT_FLAG); //用户的状态
//	pSendInfo = switch_mprintf("%s%s%s%s",   pSendInfo, CARICCP_MOBUSERINFO,	CARICCP_EQUAL_MARK, mobUserInfo);   					 //用户的信息
//
//
//	//发送通知消息,用户的状态改变.调用系统其他模块的对外接口函数
//	SWITCH_STANDARD_STREAM(stream);
//	istatus = switch_api_execute(CARI_NET_PRO_NOTIFY, pSendInfo, NULL, &stream);
//	if (SWITCH_STATUS_SUCCESS != istatus)//函数本身执行有问题
//	{
//		//打印日志
//	}
//}

#if defined(_WIN32) || defined(WIN32)

void UTF_8ToUnicode(wchar_t* pOut,char *pText)
{
	char* uchar = (char *)pOut;

	uchar[1] = ((pText[0] & 0x0F) << 4) + ((pText[1] >> 2) & 0x0F);
	uchar[0] = ((pText[1] & 0x03) << 6) + (pText[2] & 0x3F);

	return;
}

void UnicodeToUTF_8(char* pOut,wchar_t* pText)
{
	// 注意 WCHAR高低字的顺序,低字节在前，高字节在后
	char* pchar = (char *)pText;

	pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
	pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
	pOut[2] = (0x80 | (pchar[0] & 0x3F));

	return;
}

void UnicodeToGB2312(char* pOut,wchar_t uData)
{
	WideCharToMultiByte(CP_ACP,NULL,&uData,1,pOut,sizeof(wchar_t),NULL,NULL);
	return;
}     

void Gb2312ToUnicode(wchar_t* pOut,char *gbBuffer)
{
	::MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,gbBuffer,2,pOut,1);
	return ;
}

void GB2312ToUTF_8(string& pOut,char *pText, int pLen)
{
	if (!pText
		||'\0' == (*pText)){
			return;
	}

	char buf[4];
	int nLength = pLen* 3;
	char* rst = new char[nLength];

	memset(buf,0,4);
	memset(rst,0,nLength);

	int i = 0;
	int j = 0;      
	while(i < pLen)
	{
		//如果是英文直接复制
		if( *(pText + i) >= 0)
		{
			rst[j++] = pText[i++];
		}
		else
		{
			wchar_t pbuffer;
			Gb2312ToUnicode(&pbuffer,pText+i);

			UnicodeToUTF_8(buf,&pbuffer);

			unsigned short int tmp = 0;
			tmp = rst[j] = buf[0];
			tmp = rst[j+1] = buf[1];
			tmp = rst[j+2] = buf[2];    

			j += 3;
			i += 2;
		}
	}
	//rst[j] = ' ';

	//返回结果
	pOut = rst;             
	delete []rst;   

	return;
}

void UTF_8ToGB2312(string &pOut, char *pText, int pLen)
{
	if (!pText
		||'\0' == (*pText)){
			return;
	}

	char * newBuf = new char[pLen];
	char Ctemp[4];

	memset(Ctemp,0,4);
	memset(newBuf,0,pLen);

	int i =0;
	int j = 0;

	while(i < pLen)
	{
		//if(pText > 0)
		if( *(pText + i) >= 0)
		{
			newBuf[j++] = pText[i++];                       
		}
		else                 
		{
			WCHAR Wtemp;
			UTF_8ToUnicode(&Wtemp,pText + i);

			UnicodeToGB2312(Ctemp,Wtemp);

			newBuf[j] = Ctemp[0];
			newBuf[j + 1] = Ctemp[1];

			i += 3;    
			j += 2;   
		}
	}

	/*newBuf[j] = ' ';*/ 
	//newBuf[j] = '\0';

	pOut = newBuf;

	delete []newBuf;

	return; 
}

//linux系统下的操作方式
#else

//代码转换:从一种编码转为另一种编码   
int code_convert(char   *from_charset,char   *to_charset,char   *inbuf,int   inlen,char   *outbuf,int   outlen)   
{   
	iconv_t   cd;   
	int   rc;   
	char   **pin   =   &inbuf;   
	char   **pout   =   &outbuf;   

	cd   =   iconv_open(to_charset,from_charset);   
	if   (cd==0)   return   -1;   
	memset(outbuf,0,outlen);   
	//printf("inlen=%d\n",inlen);   
	if (iconv(cd,pin,(size_t*)&inlen,pout,(size_t*)&outlen)==-1)   
		return   -1;   
	iconv_close(cd); 

	return   0;   
}   
//UNICODE码转为GB2312码   
int u2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("UNICODE","gb2312",inbuf,inlen,outbuf,outlen);   
	//return   code_convert("UTF-8","GB2312",inbuf,inlen,outbuf,outlen);   
}   
//GB2312码转为UNICODE码   
int g2u(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	//return   code_convert("GB2312","UTF-8",inbuf,inlen,outbuf,outlen);   
	return   code_convert("GB2312","UNICODE",inbuf,inlen,outbuf,outlen);   
} 

//GB2312码转为UNICODE码   
int utf2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("UTF-8","GB2312",inbuf,inlen,outbuf,outlen);   
} 

//GB2312码转为UTF-8码   
int g2utf(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("GB2312","UTF-8",inbuf,inlen,outbuf,outlen);     
} 

//ascii码转为UTF-8码   
int asc2utf(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("ASCII","UTF-8",inbuf,inlen,outbuf,outlen);     
} 

void convertFrameCode(common_ResultResponse_Frame *&common_respFrame,
					  char *sourceCode,
					  char *dstSource)
{

	//test 测试中文转换
	//CodeConverter cc = CodeConverter(sourceCode,dstSource);
	int ilen = 0;
	string str;


	//printf("back before: %s\n",common_respFrame->strTableContext);

	//if (!switch_strlen_zero(common_respFrame->header.strResuleDesc)){
	//	ilen = sizeof(common_respFrame->header.strResuleDesc);
	//	char outDesc[sizeof(common_respFrame->header.strResuleDesc)];
	//	memset(outDesc,0,sizeof(outDesc));
	//	//cc.convert(common_respFrame->header.strResuleDesc, ilen,outDesc,ilen);
	//	g2utf(common_respFrame->header.strResuleDesc,ilen,outDesc,sizeof(outDesc));
	//	memset(common_respFrame->header.strResuleDesc,0,sizeof(common_respFrame->header.strResuleDesc));
	//	myMemcpy(common_respFrame->header.strResuleDesc,outDesc,sizeof(outDesc));
	//}

	//if (!switch_strlen_zero(common_respFrame->strTableName)){
	//	//printf("back before: %s\n",common_respFrame->strTableContext);
	//	ilen = sizeof(common_respFrame->strTableName);
	//	char outTableName[sizeof(common_respFrame->strTableName)];
	//	memset(outTableName,0,sizeof(outTableName));
	//	cc.convert(common_respFrame->strTableName, ilen,outTableName,ilen);

	//	//printf("back convert: %s\n",out);
	//	memset(common_respFrame->strTableName,0,sizeof(common_respFrame->strTableName));
	//	myMemcpy(common_respFrame->strTableName,outTableName,sizeof(outTableName));
	//	//printf("back after: %s\n",common_respFrame->strTableContext);
	//}

	//if (!switch_strlen_zero(common_respFrame->strTableContext)){
	//	ilen = sizeof(common_respFrame->strTableContext);
	//	char outTableContext[sizeof(common_respFrame->strTableContext)];
	//	memset(outTableContext,0,sizeof(outTableContext));
	//	cc.convert(common_respFrame->strTableContext, ilen,outTableContext,ilen);

	//	//printf("back convert: %s\n",out);
	//	memset(common_respFrame->strTableContext,0,sizeof(common_respFrame->strTableContext));
	//	myMemcpy(common_respFrame->strTableContext,outTableContext,sizeof(outTableContext));
	//}
}
#endif


/************************************************************************/
/*  																	*/
/************************************************************************/

//初始化环境信息,新增加一些文件和目录
void cari_net_init()
{
	string strPort;

	//1 新建一个\conf\directory\groups目录
	const char	*err[1]	= {
		0
	};
	char		*groupspath=NULL, *diagroup_xmlfile=NULL, *context=NULL;
	groupspath = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIR_DEFINE_GROUPS); //groups

	//判断目录是否已经存在
	if (!cari_common_isExistedFile(groupspath)) {
		cari_common_creatDir(groupspath, err);
	}
	switch_safe_free(groupspath);

	////2 新建一个\conf\dialplan\02_group_dial.xml文件
	//diagroup_xmlfile = switch_mprintf("%s%s%s%s%s%s%s%s", 
	//	SWITCH_GLOBAL_dirs.conf_dir, 
	//	SWITCH_PATH_SEPARATOR, 
	//	DIAPLAN_DIRECTORY, 
	//	SWITCH_PATH_SEPARATOR,
	//	DEFAULT_DIRECTORY, 
	//	SWITCH_PATH_SEPARATOR, 
	//	DIAPLAN_GROUP_XMLFILE,  //固定的文件名 02_group_dial
	//	CARICCP_XML_FILE_TYPE);


	//context = "<include>\n" \
	//"</include>";

	////判断文件是否已经存在
	//if (!cari_common_isExistedFile(diagroup_xmlfile)) {
	//	cari_common_creatXMLFile(diagroup_xmlfile, context, err);
	//}
	//switch_safe_free(diagroup_xmlfile);

	//3 加载配置的变量信息
	loadVarsConfigCmd();

	//4 设置端口等信息
	strPort = getValueOfDefinedVar("port");
	if (isNumber(strPort)){
		global_port_num = stringToInt(strPort.c_str());
	}

	//5 建立数据库和表信息
}


/*初始加载变量定义的文件cari_ccp_macro.ini
*/
bool loadVarsConfigCmd()
{
	//解析vars配置文件,对每条命令都建立相应的数据结构,并存放起来
	string filePath = SWITCH_GLOBAL_dirs.conf_dir;
	filePath.append(SWITCH_PATH_SEPARATOR);
	filePath.append(CARI_CCP_MACRO_INI_FILE);
	//string filePath = "\\conf\\cari_ccp_macro.ini";

	vector<string> vecFile;//临时存放文件内容
	string strLine,strKey,strVal;
	size_t index;
	vecFile.clear();

    int read_fd = -1;
	switch_size_t cur = 0, ml = 0;
	char buf[2048];
	int line = 0;
	int ilen = 0;
	
	if ((read_fd = open(filePath.c_str(), O_RDONLY, 0)) < 0) {
		const char *reason = strerror(errno);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldnt open %s (%s)\n", filePath.c_str(), reason);
		
		return false;
	}

	//循环读取源文件的每一行信息
	while ((cur = switch_fd_read_line(read_fd, buf, sizeof(buf))) > 0) {
		strLine = buf;
		trim(strLine);

		vecFile.push_back(strLine);//将配置文件的每一行数据存放起来
		continue;
	}

	close(read_fd);

	//ifstream fin;
	//fin.open(filePath.c_str());

	////文件的路径中不要含有"中文"字符
	//if (fin){//打开文件成功
	//	while(getline(fin,strLine)){    
	//		trim(strLine);
	//		vecFile.push_back(strLine);//将配置文件的每一行数据存放起来
	//	}

	//	fin.clear();
	//	fin.close();
	//}
	//else{	
	//	//打开文件失败
	//	return false;
	//}


	if (0 == vecFile.size()){
		//文件中没有配置变量
	}

	//详细处理每一行
	for(vector<string>::iterator iterLine=vecFile.begin(); iterLine!=vecFile.end(); ++iterLine){
		strLine = *iterLine;
		index = strLine.find_first_of(CARICCP_SEMICOLON_MARK);      //";"注释行不需要考虑
		if (0 == index
			|| 0 ==strLine.size()){ //空行不需要考虑
			continue;
		}
		index = strLine.find_first_of(CARICCP_LEFT_SQUARE_BRACKETS);//"["
		if (0 == index){
			continue;
		}

		index = strLine.find_first_of(CARICCP_EQUAL_MARK);          //"="
		if (0 >= index){
			continue;
		}

		//取得变量名和对应的变量值
		strKey = strLine.substr(0,index);
		strVal = strLine.substr(index + 1);
		trim(strKey);
		trim(strVal);

		if (0 == strKey.length()){
			continue;
		}

		//使用下面的操作方式会出现异常,因此,建议将配置文件cari_ccp_macro.ini保存为utf-8格式类型
//#ifdef CODE_TRANSFER_FLAG
//	#if defined(_WIN32) || defined(WIN32)
//		string strConvert="";
//		GB2312ToUTF_8(strConvert,(char*)strVal.c_str(),strlen(strVal.c_str()));
//		strVal = strConvert;
//	#else
//		char chOut[CONVERT_OUTLEN];
//		//要进行编码转换,linux操作系统默认为utf-8
//		memset(chOut,0,sizeof(chOut));
//		ilen = /*strlen(strVal.c_str());*/strVal.length();
//		g2utf((char*)strVal.c_str(),(size_t)ilen,chOut,CONVERT_OUTLEN);
//		strVal = chOut;
//	#endif
//#endif

		//mod by xxl 2010-9-25 :将key转换成小写方式
		stringToLower(strKey);

		//将宏定义变量存放到容器中
		global_define_vars_map.insert(map<string,string>::value_type(strKey,strVal));

	}//end  整个文件内容全部解析完毕

	vecFile.clear();

	return true;
}

/*循环标识
*/
void setLoopFlag(switch_bool_t bf)
{
	if (bf){
		loopFlag = true;
	}
	else{
		loopFlag = false;
	}
}

/*获得有效的字符串,linux系统的工具获得的结果有可能含有不识别的字符
*/
void cari_common_getValidStr(string &strSouce)
{
	string strNew = "";
	char ch;
	char	buffer[6];
	int iLength = strSouce.length();
	for (int i = 0; i < iLength; i++) {
		ch = strSouce.at(i);
		if ((32 <= ch && 126 >= ch)//ascii码
			//||(8<=ch && 13 >= ch)//特殊字符:退格符、制表符、换行符和回车符
			||10==ch   //CR,
			||13 == ch //LF
			||(0> ch)){//中文字符
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer,"%c",ch);
			strNew.append(buffer);
		}
		//else{
		//	//printf("%c",ch);
		//}
	}

	//printf("\n");
	strSouce = strNew;
}

/*执行shell 命令返回结果信息,此函数涉及到多线程争用
*/
void cari_common_getSysCmdResult(const char *syscmd,
								 string &strRes)//输出参数:命令执行的结果
{
	//先加锁
	pthread_mutex_lock(&mutex_syscmd);

#if defined(_WIN32) || defined(WIN32)
#else
	FILE   *fp; 
	char   buf[OUTLEN];
	//char   out[OUTLEN];
	memset(buf, '\0', sizeof(buf));//初始化buf,以免后面写如乱码到文件中

	//将 syscmd 命令的输出通过管道读取(“r”参数)到FILE* fp
	//多行会被截断,是由fgets()函数引起,参见下面的说明
	fp = popen(syscmd, "r" ); 
	//fgets()用来从参数stream所指的文件内读入字符并存到参数s所指的内存空间,
	//直到出现换行字符、读到文件尾或是已读了size-1个字符为止,最后会加上NULL作为字符串结束。
	//fgets(buf,sizeof(buf),fp);

	//使用fread替换
	fread(buf,sizeof(char), sizeof(buf),fp); //将刚刚FILE* stream的数据流读取到buf中

	////管道输出的结果显示字符出现乱码问题,utf-8-->gb2312(此处转换无效???client就是gb2312显示)
	//CodeConverter cc = CodeConverter("utf-8","gb2312");
	//cc.convert(buf,sizeof(buf),out,OUTLEN);
	//strRes = out;

	strRes = buf;

	//封装优化一下
	cari_common_getValidStr(strRes);
	pclose(fp); 

	////通过读写文件的方式进行处理
	//FILE   *fp;  
	//FILE    *wp;
	//char   buf[OUTLEN]; 
	//memset( buf, '\0', sizeof(buf) );//初始化buf,以免后面写如乱码到文件中
	//fp = popen(syscmd, "r" ); //将syscmd 命令的输出 通过管道读取（“r”参数）到FILE* fp
	//wp = fopen("syscmd_popen.txt", "w+"); //新建一个可写的文件

	//fread(buf, sizeof(char), sizeof(buf),fp);  //将刚刚FILE* fp的数据流读取到buf中
	//fwrite(buf, 1, sizeof(buf), wp);//将buf中的数据写到FILE  *wp对应的流中,也是写到文件中

	//pclose(stream);  
	//fclose(wstream);

	//strRes = buf;

#endif
	
	//释放锁
	pthread_mutex_unlock(&mutex_syscmd);
}

/*执行系统命令,不需要返回结果
*/
int cari_common_excuteSysCmd(const char *syscmd)
{
	if (NULL == syscmd){
		return -1;
	}
	int	iRes = 0;
	iRes = system(syscmd);
	if (0 != iRes) {
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
//其他通用的函数
void cari_common_convertStrToEvent(const char *context, const char *split, switch_event_t *event)
{
	string	strSource	= context;
	string	strSplit	= split;
	string	strTmp, strKey, strVal, strMid;
	size_t	p1			= 0, p2	= 0, p3	= 0;

	if (NULL == context || NULL == event) {
		return;
	}

	while (string::npos != (p2 = strSource.find(strSplit, p1))) {
		strTmp = strSource.substr(p1, p2 - p1);
		trimSpace(strTmp);
		if (0 != strTmp.size()) {
			//resultVec.push_back(strTmp);//存放到容器中
			//需要再继续区分,key和value
			if (string::npos != (p3 = strTmp.find(CARICCP_EQUAL_MARK, 0))) {
				strKey = strTmp.substr(0, p3);
				strVal = strTmp.substr(p3 + 1);
				trimSpace(strKey);
				trimSpace(strVal);

				//存放起来
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, strKey.c_str(), strVal.c_str());
			}
		}

		p1 = p2 + strlen(strSplit.c_str()); //
	}

	//处理最后一个字串
	strTmp = strSource.substr(p1);
	trimSpace(strTmp);
	if (0 != strTmp.size()) {
		//resultVec.push_back(strTmp);
		if (string::npos != (p3 = strTmp.find(CARICCP_EQUAL_MARK, 0))) {
			strKey = strTmp.substr(0, p3);
			strVal = strTmp.substr(p3 + 1);
			trimSpace(strKey);
			trimSpace(strVal);

			//存放起来
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, strKey.c_str(), strVal.c_str());
		}
	}
}

//
////其他通用的函数
//SWITCH_DECLARE(void) cari_common_convertStrToEvent(const char *context,	
//												   char split,
//												   switch_event_t *event)
//{
//	int i= 0;
//	char *array[1024] = { 0 };		//最多1024个元素
//	char *key_value[2] = { 0 };		//最多2个,key和value
//	int size = (sizeof(key_value) / sizeof(key_value[0]));
//
//	int splitNum_1 = 0;
//	int splitNum_2 = 0;
//
//	splitNum_1 = switch_separate_string((char *)context, split, array, sizeof(array) / sizeof(array[0]));
//	//依次处理组的信息
//	for(i=0; i<splitNum_1;i++)
//	{
//		splitNum_2 = switch_separate_string(array[i], CARICCP_CHAR_EQUAL_MARK, key_value, size);
//		if (2 > splitNum_2)
//		{
//			continue;
//		}
//
//		//存放起来
//		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, key_value[0],  key_value[1]); 
//	}
//}

/*创建新的目录 :建议调用系统的命令进行操作.如果当前的目录已经存在,如何强制创建???
*/
bool cari_common_creatDir(char *dir, const char **err)
{
	//判断目录是否已经存在
	if (cari_common_isExistedFile(dir)) {
		return true;
	}

#if defined(_WIN32) || defined(WIN32)

	//此处需要屏蔽处理一下,如果路径的某个目录含有空格,则在windows下此命令的操作有问题
	//直接调用系统方法
	mkdir(dir);
	return true;

#else

	bool bRes = false;

	//含有中文字符创建的目录可能有问题,直接使用系统提供的函数进行处理
	//int	iRes		= 0;
	//char *pDir = NULL;
	//char *comaddDir	= "mkdir %s";
	//pDir = switch_mprintf(comaddDir, dir);
	//iRes = system(pDir);
	//if (0 != iRes) {
	//	bRes = false;
	//}

	//switch_safe_free(pDir);

	//考虑到"中文"目录创建问题
	int ires = mkdir(dir,0755);//权限 :rwxr-xr-x 
	if (0 == ires){
		bRes = true;
	}
	return bRes;

#endif
}

/*删除目录 :建议调用系统的命令进行操作
*/
bool cari_common_delDir(char *dir, const char **err)
{
	if (NULL ==dir){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "rm dir failed, dir is null!");
		return false;
	}

	bool bRes = true;
	int	iRes  = 0;
	char *syscmd = NULL,*comdelDir=NULL;

	*err = "rm dir error!";

#if defined(_WIN32) || defined(WIN32)
	//iRes = rmdir(dir); 
	//if (0 != iRes){
	//	bRes = false;
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "rm dir %s error!",dir);
	//}
	comdelDir	= "rmdir /s /q %s";// /s /q 标识后继参数表示目录中的文件也会一起删除,也包括子目录等 
#else
	comdelDir	= "rm -rf %s";     // -rf标识后继参数表示目录中的文件也会一起删除,也包括子目录等
#endif
	syscmd = switch_mprintf(comdelDir, dir);
	iRes = system(syscmd);
	if (0 != iRes) {
		bRes = false;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "rm dir %s error!",dir);
	}

	switch_safe_free(syscmd);
	return bRes;
}

/*修改目录
*/
bool cari_common_modDir(char *sourceDir, 
						char *destDir, 
						const char **err)
{
	if (NULL ==sourceDir 
		|| NULL == destDir){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "mv dir failed, dir is null!");
		return false;
	}

	bool	bRes		= false;
	char			*commodDir	= "";//后继参数表示目录中的文件也会一起删除,也包括子目录等
	char			*oldDirAll	= NULL,*syscmd = NULL;
	int	iRes		= 0;

	//下面的方法ren执行不成功
	//#if defined(_WIN32) || defined(WIN32)
	//	commodDir="ren %s %s";
	//#else
	//	commodDir="mv %s %s";
	//#endif
	//
	//	commodDir = switch_mprintf(commodDir,sourceDir,destDir);


	//先创建新的目录
	bRes = cari_common_creatDir(destDir, err);
	if (!bRes) {
		goto end;
	}

	//把旧目录中的所有用户文件拷贝到新的目录中
	oldDirAll = switch_mprintf("%s%s%s", sourceDir, SWITCH_PATH_SEPARATOR, "*.*");

#if defined(_WIN32) || defined(WIN32)
	commodDir = "move /y %s %s";//强行覆盖所有的重名文件的话,给move加上个 /Y 的开关
#else
	commodDir = "mv %s %s";
#endif

	syscmd = switch_mprintf(commodDir, oldDirAll, destDir);

	//此处的特殊情况:destDir不存在,oldDirAll中无用户xml文件,直接处理不返回
	iRes = system(syscmd);
	if (0 != iRes) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s error!",syscmd);
	}

	//删除旧的目录
	bRes = cari_common_delDir(sourceDir, err);
	if (!bRes) {
		goto end;
	}

end:
	switch_safe_free(oldDirAll);
	switch_safe_free(syscmd);

	bRes = true;
	return bRes;
}

/*创建新的文件,主要是针对xml文件的操作
*/
bool cari_common_creatXMLFile(char *file, //完整的文件名
							  char *filecontext, //文件的内容   
							  const char **err) //输出的描述信息
{
	int				write_fd	= -1; //文件的读写句柄,何时进行关闭
	switch_size_t	cur			= 0;
	//char filebuf[2048],ebuf[8192]; //已经限制了长度,不推荐
	char			*var		= NULL,*errMsg = NULL;
	vector< string>	vec;
	string			newStr;
	if (!filecontext ||(!file)) {
		*err = "file name is null.";
		return true;
	}

	//重新封装xml文件的内容,按照xml的节点顺序,此处统一进行处理
	newStr = encapStandXMLContext(filecontext);
	var = (char *)(newStr.c_str());

	//开始创建xml文件
	write_fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (0 > write_fd) {
		*err = "Cannot open file";
		return false;
	}

	//转换成char[]结构
	//memset(filebuf,0,sizeof(filebuf));
	//switch_copy_string(filebuf,filecontext,sizeof(filebuf));
	//注意 :此方法针对特殊字符"$${"的处理!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//var = switch_xml_get_expand_vars(filebuf, ebuf, sizeof(ebuf), &cur);

	//var = filecontext;
	cur = strlen(var);

	if (write(write_fd, var, (unsigned) cur) != (int) cur) {
		*err = "Write file failed:";

		//关闭句柄
		close(write_fd);
		write_fd = -1;
		return false;
	}

	*err = "Success";

	//关闭句柄
	close(write_fd);
	write_fd = -1;

	return true;
}

/*删除xml文件,此操作主要针对删除文件,删除目录另外处理
*/
bool cari_common_delFile(char *file, //必须是绝对路径
						 const char **err)
{
	//先使用remove(),如果失败了再使用系统的命令 del XXX
	bool bres = false;
	int	iRes	= 0;
	if (!file) {
		goto end;
	}
	*err = "del file failed.";
	iRes = remove(file);//先调用api函数删除操作
	if (0 != iRes) {
		char *ch = NULL;
		char* comdel;

#if defined(_WIN32) || defined(WIN32)
		ch = "del  ";
#else
		ch	= "rm -rf ";     // -rf标识强制删除
#endif
		comdel = switch_mprintf("%s%s", ch, file);
		if (!comdel) {
			goto end;
		}

		//根本不知道执行的结果是否成功,此处只是返回函数的处理结果
		iRes = system(comdel);
		if (0 != iRes) {
			*err = "del file failed.";
		}
		switch_safe_free(comdel);
		goto end;
	}
	bres = true;

end:
	
	return bres;
}

/*复制文件
*/
bool cari_common_cpFile(char* sourceDir,
						char* destDir,
						const char** err)
{
	bool bres = false;
	int	iRes	= 0;
	char *ch = NULL,*comdel = NULL;
	if (!sourceDir 
		|| !destDir) {
		goto end;
	}

	*err = "copy file failed.";

#if defined(_WIN32) || defined(WIN32)
		ch = "copy /Y  %s %s";
#else
		ch	= "cp  %s %s";
#endif
	comdel = switch_mprintf(ch, sourceDir, destDir);
	if (!comdel) {
		goto end;
	}
	iRes = system(comdel);
	if (0 != iRes) {
		*err = "copy file failed.";
	}
	else{			
		bres = true;
	}
	switch_safe_free(comdel);

end:

	return bres;
}

/*判断文件或目录是否存在
*/
bool cari_common_isExistedFile(char *filename)
{
	/*0-检查文件是否存在		 
	1-检查文件是否可运行		
	2-检查文件是否可写访问    
	4-检查文件是否可读访问     
	6-检查文件是否可读/写访问 
	*/
	if (0 == access(filename, 0)) {
		return true;
	}

	return false;
}

/*获得用户组
*/
void cari_common_getMobUserGroupInfo(char *mobusername, const char **mobGroupInfo)
{
	if (switch_strlen_zero(mobusername)) {
		return;
	}

	//方法1:通过default目录下的xml的<variable name="callgroup" value="techsupport" /> 
	//来获得用户组的信息

	//方法2:遍历root xml结构来获得
}

/*根据文件的内容,获得对应的xml结构,封装switch_xml_parse_fd()函数
 *此返回的xml结构必须要释放
*/
switch_xml_t cari_common_parseXmlFromFile(const char *file)
{
	int				read_fd	= -1;
	switch_xml_t	x_xml	= NULL;

	//先获得此用户的xml文件的内容信息
	read_fd = open(file, O_RDONLY, 0);
	if (0 > read_fd) {
		goto end;
	}

	//获得对应的xml结构
	x_xml = switch_xml_parse_fd(read_fd);

	//关闭流的句柄
	close(read_fd);
	read_fd = -1;

	end : return x_xml;
}

/*是否包含中文字符
*/
bool cari_common_isExsitedChinaChar(const char *ibuf)
{
	char	ch;
	if (NULL == ibuf) {
		return false;
	}

	ch = *ibuf;
	while ('\0' != ch) {
		//由于汉字是2个字节,所以判断以负数为准
		//if (ch   >=   0x4e00   &&   ch   <=   0x9fa5) 
		if (0 > ch) {
			return  true;
		}

		ibuf++;
		ch = *ibuf;
	}

	return false;
}

/*自定义取出字符首尾空格/tab/\r\n的方法
*/
char * cari_common_trimStr(char *ibuf) //参数最好定义为char[]类型
{
	size_t	nlen, i;

	/*判断字符指针是否为空及字符长度是否为0*/
	if (switch_strlen_zero(ibuf)) {
		return CARICCP_NULL_MARK;
	}

	nlen = strlen(ibuf);
	if (nlen == 0)
		return CARICCP_NULL_MARK;

	/*去前空格*/
	i = 0;
	while (i < nlen) {
		/*if (ibuf[i]!=' ')*/
		if ((ibuf[i] != 0x20) && (ibuf[i] != '\t') && (ibuf[i] != '\r') && (ibuf[i] != '\n')) {
			break;
		}

		i ++;
	}

	if (i == nlen) {
		ibuf[0] = 0;
		return CARICCP_NULL_MARK;
	}

	if (i != 0) {
		nlen = nlen - i + 1;
		memcpy(ibuf, &ibuf[i], nlen);
	}

	/*去后空格*/
	nlen = strlen(ibuf) - 1;
	while (nlen >= 0) {
		/*if (ibuf[nlen] != ' ')*/
		if ((ibuf[nlen] != 0x20) && (ibuf[nlen] != '\t') && (ibuf[nlen] != '\r') && (ibuf[nlen] != '\n')) {
			ibuf[nlen + 1] = 0;

			break;
		}
		nlen --;
	}

	return ibuf;
}

////解析xml文件,类似switch_xml_parse_file()函数
//switch_xml_t cari_common_copy_parse_file(const char *sourcefile, //源文件	
//										 const char *detestfile)  //目的文件
//{
//	int				fd			= -1, write_fd = -1;
//	switch_xml_t	xml			= NULL;
//	char			*new_file	= NULL;
//
//	if (switch_strlen_zero(sourcefile) || switch_strlen_zero(detestfile)) {
//		return NULL;
//	}
//
//	//不能直接赋值,否则内存中显示有误
//	//new_file = (char *)detestfile;
//	if (!(new_file = switch_mprintf("%s", detestfile))) {
//		return NULL;
//	}
//
//	//开始创建(open)新的(O_CREAT)文件
//	if ((write_fd = open(detestfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
//		goto done;
//	}
//
//	//比较关键的处理,将源文件的内容拷贝到新的目的文件中,并返回对应的xml结构
//	if (switch_xml_preprocess(SWITCH_GLOBAL_dirs.conf_dir, sourcefile, write_fd, 0) > -1) {
//		close(write_fd);
//		write_fd = -1;
//		if ((fd = open(new_file, O_RDONLY, 0)) > -1) {
//			//创建xml结构
//			if ((xml = switch_xml_parse_fd(fd))) {
//				xml->free_path = new_file;
//				new_file = NULL;
//			}
//			close(fd);
//			fd = -1;
//		}
//	}
//
//	done:
//	if (write_fd > -1) {
//		close(write_fd);
//	}
//	if (fd > -1) {
//		close(fd);
//	}
//
//	switch_safe_free(new_file);
//	return xml;
//}

static char not_so_threadsafe_error_buffer[256] = "";

///*自定义的重加载root xml文件,去掉线程锁,主要是"写锁",线程锁容易引起阻塞
//*/
//static switch_xml_t  cari_switch_xml_open_root(uint8_t reload, 
//											   const char **err)
//{
//	char path_buf[1024];
//	uint8_t hasmain = 0, errcnt = 0;
//	switch_xml_t new_main, r = NULL;
//
//	switch_mutex_t *xml_lock = switch_xml_get_xml_lock();
//
//	switch_mutex_lock(xml_lock);
//
//	//最原始的配置文件freeswitch.xml
//	switch_snprintf(path_buf, sizeof(path_buf), "%s%s%s", SWITCH_GLOBAL_dirs.conf_dir, SWITCH_PATH_SEPARATOR, "freeswitch.xml");
//
//	if ((new_main = switch_xml_parse_file(path_buf))) {
//		*err = switch_xml_error(new_main);
//		switch_copy_string(not_so_threadsafe_error_buffer, *err, sizeof(not_so_threadsafe_error_buffer));
//		*err = not_so_threadsafe_error_buffer;
//		if (!switch_strlen_zero(*err)) {
//			switch_xml_free(new_main);
//			new_main = NULL;
//			errcnt++;
//		} else {
//			//switch_xml_t old_root;
//			*err = "Success";
//
//			//重新设置内存
//			switch_xml_reset_rootxml(new_main);
//
//			//以前的逻辑
//			//old_root = /*MAIN_XML_ROOT*/switch_xml_root();
//			//MAIN_XML_ROOT = new_main; //!!!!!!!!!!!!!!!!!!!!!将新建的xml结构保存到 MAIN_XML_ROOT中
//			//switch_set_flag(MAIN_XML_ROOT, SWITCH_XML_ROOT);
//			//switch_xml_free(old_root);
//			/* switch_xml_free_in_thread(old_root); */
//		}
//	} else {
//		*err = "Cannot Open log directory or XML Root!";
//		errcnt++;
//	}
//
//	if (errcnt == 0) {
//		switch_event_t *event;
//		if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
//			if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
//				switch_event_destroy(&event);
//			}
//		}
//		r = switch_xml_root();
//	}
//
//	switch_mutex_unlock(xml_lock);
//
//	return r;
//}

/*
*重新设置mainroot的xml文件的结构
*/
int cari_common_reset_mainroot(const char **err)
{
	////保留一个备份的功能
	//char			source_rootxml_pathbuf[1024], dest_bak_rootxml_pathbuf[1024];

	////主要是在log目录下,备份一份原始的文件
	//switch_snprintf(source_rootxml_pathbuf, sizeof(source_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.log_dir, 
	//	SWITCH_PATH_SEPARATOR, 
	//	"freeswitch.xml.fsxml");
	//switch_snprintf(dest_bak_rootxml_pathbuf, sizeof(dest_bak_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.log_dir, 
	//	SWITCH_PATH_SEPARATOR, 
	//	"freeswitch.xml.fsxml.bak");

	////拷贝文件
	//cari_common_cpFile(source_rootxml_pathbuf,dest_bak_rootxml_pathbuf,err);


	////方案0 :调用自身的方法----------------------------------------------------------------------------------------
	//int	 ires	= SWITCH_STATUS_FALSE;
	//switch_xml_t xml_root;

	////此种操作是否会造成其他的指令重加载root不能正常???主要是线程"锁"写入的时候阻塞
	////此操作要注意释放线程锁
	//if ((xml_root = cari_switch_xml_open_root(1, err))) {		
	//	switch_xml_free(xml_root);
	//	*err = "Success";
	//	ires =  SWITCH_STATUS_SUCCESS;
	//}

	//方案2 :最初的逻辑--------------------------------------------------------------------------------------------
//	int				ires	= SWITCH_STATUS_SUCCESS;
//	char			source_rootxml_pathbuf[1024], dest_bak_rootxml_pathbuf[1024], error_buffer[256];
//	switch_xml_t	new_xml_root;
//	switch_mutex_t	*xmllock	= NULL;
//
//	xmllock = switch_xml_get_xml_lock();
//	if (xmllock) {
//		switch_mutex_lock(xmllock);
//	}
//
//	//考虑重新加载root xml文件.最原始的配置文件freeswitch.xml,先bak(备份)
//	switch_snprintf(source_rootxml_pathbuf, sizeof(source_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.conf_dir, 
//		SWITCH_PATH_SEPARATOR, 
//		"freeswitch.xml");
//
//	switch_snprintf(dest_bak_rootxml_pathbuf, sizeof(dest_bak_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.log_dir, 
//		SWITCH_PATH_SEPARATOR, 
//		"freeswitch.new.xml.fsxml");
//
//	//根据备份文件freeswitch.bak.fsxml,重新创建root xml文件的结构
//	new_xml_root = cari_common_copy_parse_file(source_rootxml_pathbuf, dest_bak_rootxml_pathbuf);
//	if (!new_xml_root) {
//		*err = "Copy  freeswitch.xml failed!";
//		ires = SWITCH_STATUS_FALSE;
//		goto end;
//	}
//
//	*err = (char*) switch_xml_error(new_xml_root);
//	switch_copy_string(error_buffer, *err, sizeof(error_buffer));
//	*err = error_buffer;
//	if (!switch_strlen_zero(*err)) {
//		switch_xml_free(new_xml_root);
//		new_xml_root = NULL;
//		ires = SWITCH_STATUS_FALSE;
//
//		goto end;
//	} 
//
//	//产生此事件
//	switch_event_t *event;
//	if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
//		if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
//			switch_event_destroy(&event);
//		}
//	}
//
//	//重新设置root xml的结构,内存的更改,但是原来的内存不能释放???!!!!!!!!!!!!!!!!!!!!!!!
//	switch_xml_reset_rootxml(new_xml_root);
//	*err = "Success";
//
//
//end:
//	if (xmllock) {
//		switch_mutex_unlock(xmllock);
//	}

	//----------------------------------------------------------------------------------------------------------------
	//方案2 :使用目前提供的方式,但是有个锁的问题阻塞:switch_thread_rwlock_wrlock(RWLOCK);此函数调用时死锁.
	int	 ires	= SWITCH_STATUS_FALSE;
	switch_xml_t xml_root;
	
	//优化锁的问题
	//switch_thread_rwlock_t *rwlock= switch_xml_get_thread_lock();
	//if (rwlock){
	//	for (int i=0;i<6;i++){//多释放几次,但是此处容易引起阻塞
	//		switch_thread_rwlock_unlock(rwlock);
	//		switch_thread_rwlock_unlock(rwlock);
	//		switch_thread_rwlock_unlock(rwlock);
	//	}
	//}
	if ((xml_root = switch_xml_open_root(1, err))) {	
		switch_xml_free(xml_root);
		*err = "Success";
		ires =  SWITCH_STATUS_SUCCESS;
	}

	////方案3 :调用其他模块提供的api方法------------------------------------------------------------------------------
	//switch_status_t istatus;
	//switch_stream_handle_t stream	= { 0 };
	//SWITCH_STANDARD_STREAM(stream);
	//istatus = switch_api_execute("reloadxml", "reloadxml", NULL, &stream);
	//if (SWITCH_STATUS_SUCCESS != istatus){//函数本身执行有问题
	//	//打印日志
	//	*err = "switch_api_execute :reload_acl_function failed!";
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "switch_api_execute :reload_acl_function failed! \n");
	//}
	//else{
	//	ires = SWITCH_STATUS_SUCCESS;
	//	*err = "Success";
	//}

	return ires;
}
