// ServerSocket.cpp : Defines the entry point for the console application.
//
//#pragma comment(lib,"ws2_32.lib")//lnk2019的错误是编译时没指定ws2_32.lib这个库。
//所在目录C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib 
#pragma comment(lib,"pthreadVC2.lib")//或 配置属性”--“链接器”--“输入”页的“附加依赖项”,把lib文件的名称加进去。


#pragma warning(disable:4786)
#pragma warning(disable:4996)
#pragma warning(disable:4267)

#include "cari_net_socket.h"

//////////////////////////////////////////////////////////////////////////

//监听socket的id号
#if defined(_WIN32) || defined(WIN32)
SOCKET	socket_server_local	= -1;
#else
int		socket_server_local	= -1;    
#endif

//主线程
pthread_t	cari_ccp_server_thrd;

//////////////////////////////////////////////////////////////////////////
/*扫描所有的AC/switch :考虑其大功能的概念
*/


/*
*每个socket对应的线程的入口函数,客户端的socket信息处理.主要是区分web端和一般的lmt端
*此线程针对单独的客户端进行处理
*/
void * proClientSocket(void *socketHandle)
{
	bool                bWebType = false;       //是否为web类型
	int					iErrorCount		= 0;	//计数器
	int					i				= 0;
	u_short             iClientID       = CARICCP_MAX_SHORT;

	CSocketHandle		*socket_handle	= ((CSocketHandle *) socketHandle);

	//SOCKET	  sock_client = socket_handle->sock_client;//此socket需要保持起来
	//sockaddr_in addr_client = socket_handle->addr_client;

	//int nNetTimeout=3000;//3秒
	//接收时限,此处设置会影响网络状态的情况判断
	//setsockopt(sock_client,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int));

	//int nZero=0;
	//setsockopt(sock_client,SOL_SOCKET,SO_RCVBUF,(char *)&nZero,sizeof(int));


	char				*pClientIP;
	int					sock_client		= socket_handle->sock_client; //此socket需要保持起来

#if defined(_WIN32) || defined(WIN32)
	struct sockaddr_in	addr_client		= socket_handle->addr_client;
	pClientIP = inet_ntoa(addr_client.sin_addr);  

#else
	struct sockaddr	addr_client	= socket_handle->addr_client;
	//pClientIP  =  /*inet_ntoa(addr_client)*/addr_client.sa_data;
	//要获得client端的ip地址,必须把sockaddr结构强制转换成sockaddr_in结构,因为字节大小是相同的
	pClientIP = inet_ntoa(((struct sockaddr_in *) (&addr_client))->sin_addr);

	//打印16进制的mac地址信息
	//printf("%02x:%02x:%02x:%02x:%02x:%02x\n", 
	//	(unsigned char)addr_client.sa_data[0], 
	//	(unsigned char)addr_client.sa_data[1], 
	//	(unsigned char)addr_client.sa_data[2], 
	//	(unsigned char)addr_client.sa_data[3], 
	//	(unsigned char)addr_client.sa_data[4], 
	//	(unsigned char)addr_client.sa_data[5]); 


#endif

	//及时释放销毁当前socket句柄对象
	CARI_CCP_VOS_DEL(socket_handle);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ begin :Succeed linked with client(IP :%s, socketID :%d).\n", pClientIP, sock_client);

	//定义socket的三元组
	socket3element	clientSocketElement;//是否需要新创建,每个线程独享的临时变量
	initSocket3element(clientSocketElement);
	clientSocketElement.socketClient = sock_client;//保存此socket号
	clientSocketElement.addr = addr_client;

	//方式1: 分配客户端号,此处预先分配好,等待client下发命令查询的时候直接返回即可
	//iClientID = CModuleManager::getInstance()->assignClientID();
	//clientSocketElement.sClientID = iClientID;

	//方式2: client下发命令的时候再进行分配,考虑到以前使用的id号能重用
	//目前采用这种方式

	//把此客户端的socket保存起来
	CModuleManager::getInstance()->saveSocket(clientSocketElement);

	//switch_status_t ret;
	int	recvbytes	= 0;//命名接收处理结果

#ifdef WIN32
#else
	//test 字符转换
	CodeConverter cc = CodeConverter("gb2312","UTF-8");
#endif

	//针对于固定的客户端,对应的线程一直循环处理,设置时限/监测连接是否正常
	while (loopFlag){//使用循环标识控制
		bool bWeb = false;
		char	recvBuf[sizeof(common_CmdReq_Frame)]; 
		memset(recvBuf, 0, sizeof(recvBuf));
		recvbytes = recv(sock_client, recvBuf, sizeof(recvBuf), 0);  //要保证是阻塞模式的,如果客户端(异常)退出,会导致死循环
		
		//网络出现异常,网络中断
		if (-1 == recvbytes) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "--- end  :Disconnect with client (IP :%s, socketID :%d)! \n", 
				pClientIP, 
				sock_client);

			break;
		}
		
		//收到的数据帧错误或客户端退出,或者初始建立连接,此时收到的数据帧的大小都是0
		if (0 == recvbytes) {
			iErrorCount ++;

			//连续收到不正确的数据N次,就要退出循环,重新等待客户端的连接
			if (CARICCP_MAX_ERRRORFRAME_COUNT < iErrorCount) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "--- end  :Client %s already exited.\n",
					pClientIP);

				iErrorCount = 0;
				break;
			}
			continue;
		}
		//shutdown指令
		if (shutdownFlag){
			break;
		}

		//////////////////////////////////////////////////////////////////////////
		//帧的销毁遵守:谁调用谁负责销毁!!!!!!!!!!!!!!!!!!!
		//关键的命令逻辑处理,逆解析来自客户端的请求帧
		//新建客户端的请求帧,由调用者负责清除
		common_CmdReq_Frame	*clientReqFrame	= CARI_CCP_VOS_NEW(common_CmdReq_Frame);
		initCommonRequestFrame(clientReqFrame);

		//add by xxl 2010-5-5: 需要判断一下此命令是来自web还是一般的client
		if(!bWebType){
			char chHeader = recvBuf[0];
			if ('[' ==chHeader){//判断是否为web发送的命令格式
				bWeb = true;
			}
		}
		//需要重新解析构造请求帧,根据字符串进行转换
		if (bWebType || bWeb){
			string strres = recvBuf,strHeader,strCmd,strParam,strVal,strName;
			int ival =0;
			int index = strres.find_first_of("]");
			strHeader = strres.substr(1,index-1);
			strCmd= strres.substr(index+1);
			
			//1 先构造消息头
			vector<string> resultVec;
			splitString(strHeader, ",", resultVec);
			vector<string>::const_iterator it = resultVec.begin();
			for (;it != resultVec.end(); it++){
				strVal = *it;
				index = strVal.find_first_of("=");
				if (0 < index){
					strName = strVal.substr(0,index);
					strVal  = strVal.substr(index+1);
                    trim(strName);
					trim(strVal);

					//解析关键的参数
					if(isEqualStr("clientID",strName)){
						clientReqFrame->header.sClientID = 0;//web的比较固定
					}
					else if(isEqualStr("modId",strName)){
						clientReqFrame->header.sClientChildModID = stringToInt(strVal.c_str());
					}
					else if(isEqualStr("cmdID",strName)){
						clientReqFrame->body.iCmdCode = stringToInt(strVal.c_str());
					}
					else if(isEqualStr("logUserName",strName)){
						myMemcpy(clientReqFrame->header.strLoginUserName,strVal.c_str(),strVal.length());
					}
				}
			}//end for

			//2 再解析命令名和参数区
			strName = "";
			trim(strCmd);
			index = strCmd.find_first_of(":");
			if (0 < index){
				strName = strCmd.substr(0,index);
				strParam = strCmd.substr(index+1);
			}
			else{
				strParam = strCmd;
			}

			myMemcpy(clientReqFrame->body.strCmdName,strName.c_str(),strName.length());
			myMemcpy(clientReqFrame->param.strParamRegion,strParam.c_str(),CARICCP_MIN(strParam.length(),CARICCP_MAX_LENGTH));

			//3 其他区域的初始化
			clientReqFrame->body.bSynchro = 1;
			clientReqFrame->header.sClientID = 0;//web的比较固定

			//4 重新设置socket,只需要设置一次即可
			if(!bWebType){
				bWebType = true;

				//重新设置clientID号
				CModuleManager::getInstance()->eraseSocket(clientSocketElement.socketClient);
				clientSocketElement.sClientID = 0;//默认情况为CARICCP_MAX_SHORT
				CModuleManager::getInstance()->saveSocket(clientSocketElement);
			}
		}
		else{//一般的lmt,按照以前的方式处理
			memcpy(clientReqFrame,recvBuf,sizeof(recvBuf));
		}
		//add by xxl 2010-5-5 end

		//char strParamRegion[CARICCP_MAX_LENGTH];    
#ifdef WIN32
#else
		//printf("before: %s\n",clientReqFrame->param.strParamRegion);
		////test 字符转换
		//char out[CARICCP_MAX_LENGTH];
		//memset(out,0,sizeof(out));
		//cc.convert(clientReqFrame->param.strParamRegion, CARICCP_MAX_LENGTH,out,CARICCP_MAX_LENGTH);
		//printf("convert: %s\n",out);
		//memset(clientReqFrame->param.strParamRegion,0,sizeof(clientReqFrame->param.strParamRegion));
		//myMemcpy(clientReqFrame->param.strParamRegion,out,sizeof(out));
		//printf("after: %s\n",clientReqFrame->param.strParamRegion);
#endif

		//如果此命令执行时间过长,此处会产生阻塞,当前客户端一直收不到信息!!!!!!!!!
		//如果再另起线程处理,将不能保证命令执行的有序性,所以上述情况还是较少的
		//不必再过多考虑!!!!

		//由请求帧的处理模块(入口函数)负责细节处理
		//帧的销毁遵守:谁调用谁负责销毁!!!!!!!!!!!!!!!!!!!!!!
		CMsgProcess::getInstance()->proCmd(sock_client, clientReqFrame);

		//结果集合的发送不再此处设计,直接放到对应的syncProCmd函数中
		//另外,如果结果集分块发送显示,此处也不适合
		//发送send单独考虑
		//如何保证结果集能正确的发送给对应的客户端呢???

		iErrorCount = 0;
	}//end while(client端循环获得请求帧)

	//如果是强制退出(shutdown命令操作),不需要考虑释放client id号等
	if (!shutdownFlag){
		//先释放client id号
		CModuleManager::getInstance()->releaseClientIDBySocket(sock_client);

		//下面进行清除客户端连接,从保存的容器中清除掉
		CModuleManager::getInstance()->eraseSocket(sock_client);

		//关闭当前客户端的socket
		CARI_CLOSE_SOCKET(sock_client);
	}

	//登录的用户数量减少
	global_client_num--;

	//线程实例清除掉
	//CModuleManager::getInstance()->eraseThrd(sock_client);

	//退出重连接的线程
	//pthread_exit(NULL);

	return NULL;		 //return也会导致本函数所在的线程退出
}

/*等待所有的客户端的连接,针对每个客户端都要重新启动一个新的线程进行监听处理
*/
void * waitForMutilClients(void *par)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ Begin listen client(s)!\n");
	int		icount		= 0;

	//系统启动的时候要启动多个线程
	//1 命令处理线程
	//2 命令分发线程

	//////////////////////////////////////////////////////////////////////////
	//只能在windows下使用
#if defined(_WIN32) || defined(WIN32)
	//LoadLibrary(_T("pthreadVC2.dll"));//?????????是否必要VC6.0不需要,vs2008???只需要放置到当前工程的目录下即可

	for (int j=0; j<MAX_BIND_PORT_NUM_COUNT;j++){
		if (MAX_BIND_PORT_NUM_COUNT == j) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to bind port %d.\n", global_port_num);
			return 0;
		}

		//socket的初始化设置
		WSADATA	wsaData;
		WORD	sockVersion	= MAKEWORD(2, 0);
		if (0 != WSAStartup(sockVersion, &wsaData)) {
			return 0;
		}
		//本服务器的socket
		//SOCKET socket_server_local;
		socket_server_local = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//实现协议socket
		if (INVALID_SOCKET == socket_server_local) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid socket.\n");

			WSACleanup();
			return 0;
		}

		sockaddr_in	addr_sev;  
		addr_sev.sin_addr.s_addr = INADDR_ANY; //自动填写本机的地址
		addr_sev.sin_family = AF_INET;
		addr_sev.sin_port = htons(global_port_num);//根据配置文件适配
		//addr_sev.sin_addr.s_addr = inet_addr("127.0.0.1");

		int nSendBuf = CARICCP_SOCKET_BUFFER_SIZE*2;//8K
		icount =0;
		for (; icount <= MAX_BIND_COUNT; icount++) {
			if (MAX_BIND_COUNT == icount) {//达到一定的数量,退出
				WSACleanup();
				continue;
			}

			//设置socket的状态,如:发送缓冲区(SO_SNDBUF)的大小
			/*具体参见:http://www.cppblog.com/killsound/archive/2009/01/16/72138.html

			在send()的时候,返回的是实际发送出去的字节(同步)或发送到socket缓冲区的字节
			(异步);系统默认的状态发送和接收一次为8688字节(约为8.5K)；在实际的过程中发送数据
			和接收数据量比较大,可以设置socket缓冲区,而避免了send(),recv()不断的循环收发：
			// 接收缓冲区
			int nRecvBuf=32*1024;//设置为32K
			setsockopt(s,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
			//发送缓冲区
			int nSendBuf=32*1024;//设置为32K
			setsockopt(s,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
			*/
			int set;
			set=setsockopt(socket_server_local,SOL_SOCKET,SO_SNDBUF,(char *)&nSendBuf,sizeof(int));
			if(-1 == set){
				//不处理
			}
			if (SOCKET_ERROR == bind(socket_server_local, (sockaddr *) &addr_sev, sizeof(addr_sev))){//与地址进行绑定
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to bind.\n");

				//适当的休眠一下
				CARI_SLEEP(2000);
				continue;
			}

			goto success_flag;
		}

		//端口号自动递增1
		global_port_num++;
	}

/*---------*/
success_flag:

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ 绑定端口(%d)成功,等待client建立连接... ...\n",global_port_num);
		if (SOCKET_ERROR == listen(socket_server_local, NUM_CLIENTS)){//参数sock_sev标识一个本地已建立、尚未连接的套接字号,服务器愿意从它上面接收请求.
			//backlog表示请求连接队列的最大长度,用于限制排队请求的个数,目前允许的最大值为5
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to listen.\n");
			WSACleanup();
			return 0;
		}


	//////////////////////////////////////////////////////////////////////////
	//linux系统下使用
#else
	//////////////////////////////////////////////////////////////////////////
	//优化处理一下:如果一次绑定失败,则多次,或者考虑使用某个端口号段,某个端口bind失败
	//则考虑使用下一个端口号,依次递加
	for (int j=0; j<MAX_BIND_PORT_NUM_COUNT;j++){
		if (MAX_BIND_PORT_NUM_COUNT == j) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to bind port %d.\n", global_port_num);
			return 0;
		}
		if(0 != j){
			//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ 开始准备重绑定新端口(%d)\n", global_port_num);
		}
		else{
			//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ 开始准备绑定端口(%d)\n", global_port_num);
		}
		//int socket_server_local;  		//监听socket的id号
		struct sockaddr_in	my_addr;	    //本机地址信息
		//struct sockaddr_in remote_addr;   //客户端地址信息
		unsigned int		sin_size;
		int one = 1;
		memset(&my_addr, 0, sizeof(my_addr));

		my_addr.sin_addr.s_addr = INADDR_ANY;  //自动填写本机的地址 inet_addr("127.0.0.1");
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(global_port_num);//根据配置文件适配
		bzero(&(my_addr.sin_zero), 8);
		socket_server_local = socket(AF_INET,	//Address families.
			SOCK_STREAM,				//Types 	 
			IPPROTO_TCP);				//实现协议socket;
		if (0 > socket_server_local) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "0 > socket_local :%d.\n", socket_server_local);
			return 0;
		}
		int nSendBuf = CARICCP_SOCKET_BUFFER_SIZE*2;//8K
		icount =0;
		for (; icount <= MAX_BIND_COUNT; icount++) {
			if (MAX_BIND_COUNT == icount) {//达到一定的数量,退出
				close(socket_server_local);
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to bind port %d.\n", global_port_num);
				continue;
			}

			//////////////////////////////////////////////////////////////////////////
			//参见:\freeswitch-1.0.4\libs\xmlrpc-c\lib\abyss\src\socket_unix.c :bindSocketToPort()函数
			//需要设置一下socket状态SO_REUSEADDR
			//setsockopt(socket_server_local, SOL_SOCKET, /*SO_REUSEADDR*/SO_BROADCAST, (void *)&one, sizeof(int));
			setsockopt(socket_server_local,SOL_SOCKET,SO_SNDBUF,(char *)&nSendBuf,sizeof(int));
			if (bind(socket_server_local, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) < 0) {
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "--- 绑定端口(%d)失败,次数%d.正在重试... ...\n", global_port_num,icount + 1);
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "bind port :%s\n", strerror(errno));

				//关键错误打印
				//fprintf(stderr,"Bind error:%s\n",strerror(errno)); 

				//适当的休眠一下
				CARI_SLEEP(2000);
				continue;
			}
			goto success_flag;
		}

		//端口号自动递增1
		global_port_num++;
	}

/*---------*/
success_flag:

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ Succeed binding port (%d),waiting for client to link the server... ...\n",global_port_num);
	if (-1 == listen(socket_server_local, NUM_CLIENTS)){//参数localSocket标识一个本地已建立、尚未连接的套接字号,服务器愿意从它上面接收请求.
														//NUM_CLIENTS表示请求连接队列的最大长度,用于限制排队请求的个数,目前允许的最大值为5
		close(socket_server_local);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to listen.\n");
		fprintf(stderr,"Listen error:%s\n",strerror(errno)); 
		return 0;
	}
#endif

	//循环监听有多少客户端进行链接请求.针对每个客户的请求专门建立一个线程
	while (loopFlag){//使用循环标识控制
		//////////////////////////////////////////////////////////////////////////
		//只能在windows下使用
#if defined(_WIN32) || defined(WIN32)

		SOCKET				sock_client;
		struct sockaddr_in	remote_addr;
		int					nAddrLen	= sizeof(remote_addr);

		//获得客户端的连接:根据sock_sever进行获得
		sock_client = accept(socket_server_local, (struct sockaddr *) &remote_addr, &nAddrLen);//没有连接,则一直阻塞?????
		if (INVALID_SOCKET == sock_client){//可能是网络断开突然发生
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to accept socket.\n");
			if (shutdownFlag){
				break;
			}
			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		//unix环境下使用
#else
		int				sock_client;	  //客户端的socket号
		struct sockaddr	remote_addr;	  //客户端地址信息
		socklen_t		sin_size	= sizeof(remote_addr);
		//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n+++++++++++ ccp server 准备 accept\n");
		sock_client = accept(socket_server_local, &remote_addr, &sin_size);
		//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n+++++++++++ ccp server 已经 accept\n");
		if (-1 == sock_client) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to accept socket.\n");
			if (shutdownFlag){
				break;
			}
			continue;
		}
#endif
		//屏蔽处理一下"shutdown"命令,优化处理
		if (shutdownFlag){
			/*CARI_CLOSE_SOCKET(socket_server_local);*/
			break;
		}

		//为了能同时和多个客户端建立连接,并同时运行,需要把对应的链接存放起来
		//在新的线程中进行运行处理,维持此链接
		int				iRet			= 0;   

		//保存起来客户端的socket
		//使用者 proClientSocket 负责销毁此对象
		CSocketHandle	*aSocketHandle	= CARI_CCP_VOS_NEW(CSocketHandle);//在对应的子线程退出的时候消耗此对象
		aSocketHandle->sock_client = (int) sock_client;
		aSocketHandle->addr_client = remote_addr;

		//是否需要考虑fork()函数生成一个"子进程"来处理数据传输部分???
		//针对此客户端的链接需要启动新的线程.另外,要把对应的客户端的socket保存起来
		pthread_t	thrd;
		iRet = pthread_create(&thrd, NULL, proClientSocket, (void*) (aSocketHandle));
		if (0 != iRet) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Create pthread error!\n");

			//应该发通知到客户端,服务器出现异常!!!!!

			//关闭对应的客户端的socket
			CARI_CLOSE_SOCKET(sock_client);

			//销毁socket的句柄对象
			CARI_CCP_VOS_DEL(aSocketHandle);

			continue;
		} 

		//登录的用户数量递增
		global_client_num++;

		//线程实例保存
		//CModuleManager::getInstance()->saveThrd(sock_client,&thrd);

		CARI_SLEEP(2*1000);

	}//end 循环监听所有客户端的连接

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 4 ccp server exited.\n");
	return 0;
}

/*
*此模块的入口函数:模块初始加载的时候开始调用
*/
int cari_net_server_start()
{
	//扩展:当成进程来考虑单独的网管系统

	//启动新的线程,等待所有的用户进行连接
	int	iRet = pthread_create(&cari_ccp_server_thrd, NULL, waitForMutilClients, NULL);
	if (0 != iRet) {
		return -1;
	}

	//初始化锁
	initmutex();

	return 0;
}

/*关闭socket,释放端口号
*/
void shutdownSocketAndGarbage()
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n\n--- 1 ccp server shutdown Socket And Garbage()\n");

	setLoopFlag(SWITCH_FALSE);//把循环标识设置为false,让循环线程退出

	if (0 > socket_server_local) {
		return;
	}

	//kill client的线程
	//CModuleManager::getInstance()->killAllClientThrd();

	//kill server的主线程:务必小心如此处理,否则socket等都不能及时关闭
	//int kill_rc = pthread_kill(cari_ccp_server_thrd,0);
	//if(kill_rc == ESRCH)
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 2 the ccp server thread did not exits or already quit\n");
	//else if(kill_rc == EINVAL)
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 2 signal is invalid\n");
	//else if(kill_rc == 0)//线程关闭成功
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 2 the ccp server thread is alive and killed success\n");

	//关闭client的socket,非必要???
	CModuleManager::getInstance()->closeAllClientSocket();

	//关闭server的socket
	CARI_CLOSE_SOCKET(socket_server_local);

	//调用析构函数清除单"实例对象",aCGarbageRecover对象如何清除??临时变量
	//CGarbageRecover	aCGarbageRecover;
	//直接进行清除
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n--- 2 CGarbageRecover() begin\n");
	CModuleManager::getInstance()->clear();				//清除"模块实例"的内存资源
	CMsgProcess::getInstance()->clear();					//无
	CMsgProcess::getInstance()->clearAllCmdFromQueue();   //清除命令队列中的资源

	//单实例对象系统退出时自动释放
	//CARI_CCP_VOS_DEL(CMsgProcess::getInstance());
	//CARI_CCP_VOS_DEL(CModuleManager::getInstance());
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n--- 2 CGarbageRecover() end\n");

	//恢复登录的用户数量
	global_client_num = 0;

#if defined(_WIN32) || defined(WIN32)
#else
	//此处针对出现core文件的情况,优化处理,目前无法定位是如何引起的???
	char *path = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.base_dir, 
		SWITCH_PATH_SEPARATOR, 
		"bin", 
		SWITCH_PATH_SEPARATOR, 
		"core.*");
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	cari_common_delFile(path,err);
	switch_safe_free(path);
#endif
	
	
}
