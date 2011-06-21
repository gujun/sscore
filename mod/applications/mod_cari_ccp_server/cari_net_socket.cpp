// ServerSocket.cpp : Defines the entry point for the console application.
//
//#pragma comment(lib,"ws2_32.lib")//lnk2019�Ĵ����Ǳ���ʱûָ��ws2_32.lib����⡣
//����Ŀ¼C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib 
#pragma comment(lib,"pthreadVC2.lib")//�� �������ԡ�--����������--�����롱ҳ�ġ����������,��lib�ļ������Ƽӽ�ȥ��


#pragma warning(disable:4786)
#pragma warning(disable:4996)
#pragma warning(disable:4267)

#include "cari_net_socket.h"

//////////////////////////////////////////////////////////////////////////

//����socket��id��
#if defined(_WIN32) || defined(WIN32)
SOCKET	socket_server_local	= -1;
#else
int		socket_server_local	= -1;    
#endif

//���߳�
pthread_t	cari_ccp_server_thrd;

//////////////////////////////////////////////////////////////////////////
/*ɨ�����е�AC/switch :��������ܵĸ���
*/


/*
*ÿ��socket��Ӧ���̵߳���ں���,�ͻ��˵�socket��Ϣ����.��Ҫ������web�˺�һ���lmt��
*���߳���Ե����Ŀͻ��˽��д���
*/
void * proClientSocket(void *socketHandle)
{
	bool                bWebType = false;       //�Ƿ�Ϊweb����
	int					iErrorCount		= 0;	//������
	int					i				= 0;
	u_short             iClientID       = CARICCP_MAX_SHORT;

	CSocketHandle		*socket_handle	= ((CSocketHandle *) socketHandle);

	//SOCKET	  sock_client = socket_handle->sock_client;//��socket��Ҫ��������
	//sockaddr_in addr_client = socket_handle->addr_client;

	//int nNetTimeout=3000;//3��
	//����ʱ��,�˴����û�Ӱ������״̬������ж�
	//setsockopt(sock_client,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int));

	//int nZero=0;
	//setsockopt(sock_client,SOL_SOCKET,SO_RCVBUF,(char *)&nZero,sizeof(int));


	char				*pClientIP;
	int					sock_client		= socket_handle->sock_client; //��socket��Ҫ��������

#if defined(_WIN32) || defined(WIN32)
	struct sockaddr_in	addr_client		= socket_handle->addr_client;
	pClientIP = inet_ntoa(addr_client.sin_addr);  

#else
	struct sockaddr	addr_client	= socket_handle->addr_client;
	//pClientIP  =  /*inet_ntoa(addr_client)*/addr_client.sa_data;
	//Ҫ���client�˵�ip��ַ,�����sockaddr�ṹǿ��ת����sockaddr_in�ṹ,��Ϊ�ֽڴ�С����ͬ��
	pClientIP = inet_ntoa(((struct sockaddr_in *) (&addr_client))->sin_addr);

	//��ӡ16���Ƶ�mac��ַ��Ϣ
	//printf("%02x:%02x:%02x:%02x:%02x:%02x\n", 
	//	(unsigned char)addr_client.sa_data[0], 
	//	(unsigned char)addr_client.sa_data[1], 
	//	(unsigned char)addr_client.sa_data[2], 
	//	(unsigned char)addr_client.sa_data[3], 
	//	(unsigned char)addr_client.sa_data[4], 
	//	(unsigned char)addr_client.sa_data[5]); 


#endif

	//��ʱ�ͷ����ٵ�ǰsocket�������
	CARI_CCP_VOS_DEL(socket_handle);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ begin :Succeed linked with client(IP :%s, socketID :%d).\n", pClientIP, sock_client);

	//����socket����Ԫ��
	socket3element	clientSocketElement;//�Ƿ���Ҫ�´���,ÿ���̶߳������ʱ����
	initSocket3element(clientSocketElement);
	clientSocketElement.socketClient = sock_client;//�����socket��
	clientSocketElement.addr = addr_client;

	//��ʽ1: ����ͻ��˺�,�˴�Ԥ�ȷ����,�ȴ�client�·������ѯ��ʱ��ֱ�ӷ��ؼ���
	//iClientID = CModuleManager::getInstance()->assignClientID();
	//clientSocketElement.sClientID = iClientID;

	//��ʽ2: client�·������ʱ���ٽ��з���,���ǵ���ǰʹ�õ�id��������
	//Ŀǰ�������ַ�ʽ

	//�Ѵ˿ͻ��˵�socket��������
	CModuleManager::getInstance()->saveSocket(clientSocketElement);

	//switch_status_t ret;
	int	recvbytes	= 0;//�������մ�����

#ifdef WIN32
#else
	//test �ַ�ת��
	CodeConverter cc = CodeConverter("gb2312","UTF-8");
#endif

	//����ڹ̶��Ŀͻ���,��Ӧ���߳�һֱѭ������,����ʱ��/��������Ƿ�����
	while (loopFlag){//ʹ��ѭ����ʶ����
		bool bWeb = false;
		char	recvBuf[sizeof(common_CmdReq_Frame)]; 
		memset(recvBuf, 0, sizeof(recvBuf));
		recvbytes = recv(sock_client, recvBuf, sizeof(recvBuf), 0);  //Ҫ��֤������ģʽ��,����ͻ���(�쳣)�˳�,�ᵼ����ѭ��
		
		//��������쳣,�����ж�
		if (-1 == recvbytes) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "--- end  :Disconnect with client (IP :%s, socketID :%d)! \n", 
				pClientIP, 
				sock_client);

			break;
		}
		
		//�յ�������֡�����ͻ����˳�,���߳�ʼ��������,��ʱ�յ�������֡�Ĵ�С����0
		if (0 == recvbytes) {
			iErrorCount ++;

			//�����յ�����ȷ������N��,��Ҫ�˳�ѭ��,���µȴ��ͻ��˵�����
			if (CARICCP_MAX_ERRRORFRAME_COUNT < iErrorCount) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "--- end  :Client %s already exited.\n",
					pClientIP);

				iErrorCount = 0;
				break;
			}
			continue;
		}
		//shutdownָ��
		if (shutdownFlag){
			break;
		}

		//////////////////////////////////////////////////////////////////////////
		//֡����������:˭����˭��������!!!!!!!!!!!!!!!!!!!
		//�ؼ��������߼�����,��������Կͻ��˵�����֡
		//�½��ͻ��˵�����֡,�ɵ����߸������
		common_CmdReq_Frame	*clientReqFrame	= CARI_CCP_VOS_NEW(common_CmdReq_Frame);
		initCommonRequestFrame(clientReqFrame);

		//add by xxl 2010-5-5: ��Ҫ�ж�һ�´�����������web����һ���client
		if(!bWebType){
			char chHeader = recvBuf[0];
			if ('[' ==chHeader){//�ж��Ƿ�Ϊweb���͵������ʽ
				bWeb = true;
			}
		}
		//��Ҫ���½�����������֡,�����ַ�������ת��
		if (bWebType || bWeb){
			string strres = recvBuf,strHeader,strCmd,strParam,strVal,strName;
			int ival =0;
			int index = strres.find_first_of("]");
			strHeader = strres.substr(1,index-1);
			strCmd= strres.substr(index+1);
			
			//1 �ȹ�����Ϣͷ
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

					//�����ؼ��Ĳ���
					if(isEqualStr("clientID",strName)){
						clientReqFrame->header.sClientID = 0;//web�ıȽϹ̶�
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

			//2 �ٽ����������Ͳ�����
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

			//3 ��������ĳ�ʼ��
			clientReqFrame->body.bSynchro = 1;
			clientReqFrame->header.sClientID = 0;//web�ıȽϹ̶�

			//4 ��������socket,ֻ��Ҫ����һ�μ���
			if(!bWebType){
				bWebType = true;

				//��������clientID��
				CModuleManager::getInstance()->eraseSocket(clientSocketElement.socketClient);
				clientSocketElement.sClientID = 0;//Ĭ�����ΪCARICCP_MAX_SHORT
				CModuleManager::getInstance()->saveSocket(clientSocketElement);
			}
		}
		else{//һ���lmt,������ǰ�ķ�ʽ����
			memcpy(clientReqFrame,recvBuf,sizeof(recvBuf));
		}
		//add by xxl 2010-5-5 end

		//char strParamRegion[CARICCP_MAX_LENGTH];    
#ifdef WIN32
#else
		//printf("before: %s\n",clientReqFrame->param.strParamRegion);
		////test �ַ�ת��
		//char out[CARICCP_MAX_LENGTH];
		//memset(out,0,sizeof(out));
		//cc.convert(clientReqFrame->param.strParamRegion, CARICCP_MAX_LENGTH,out,CARICCP_MAX_LENGTH);
		//printf("convert: %s\n",out);
		//memset(clientReqFrame->param.strParamRegion,0,sizeof(clientReqFrame->param.strParamRegion));
		//myMemcpy(clientReqFrame->param.strParamRegion,out,sizeof(out));
		//printf("after: %s\n",clientReqFrame->param.strParamRegion);
#endif

		//���������ִ��ʱ�����,�˴����������,��ǰ�ͻ���һֱ�ղ�����Ϣ!!!!!!!!!
		//����������̴߳���,�����ܱ�֤����ִ�е�������,��������������ǽ��ٵ�
		//�����ٹ��࿼��!!!!

		//������֡�Ĵ���ģ��(��ں���)����ϸ�ڴ���
		//֡����������:˭����˭��������!!!!!!!!!!!!!!!!!!!!!!
		CMsgProcess::getInstance()->proCmd(sock_client, clientReqFrame);

		//������ϵķ��Ͳ��ٴ˴����,ֱ�ӷŵ���Ӧ��syncProCmd������
		//����,���������ֿ鷢����ʾ,�˴�Ҳ���ʺ�
		//����send��������
		//��α�֤���������ȷ�ķ��͸���Ӧ�Ŀͻ�����???

		iErrorCount = 0;
	}//end while(client��ѭ���������֡)

	//�����ǿ���˳�(shutdown�������),����Ҫ�����ͷ�client id�ŵ�
	if (!shutdownFlag){
		//���ͷ�client id��
		CModuleManager::getInstance()->releaseClientIDBySocket(sock_client);

		//�����������ͻ�������,�ӱ���������������
		CModuleManager::getInstance()->eraseSocket(sock_client);

		//�رյ�ǰ�ͻ��˵�socket
		CARI_CLOSE_SOCKET(sock_client);
	}

	//��¼���û���������
	global_client_num--;

	//�߳�ʵ�������
	//CModuleManager::getInstance()->eraseThrd(sock_client);

	//�˳������ӵ��߳�
	//pthread_exit(NULL);

	return NULL;		 //returnҲ�ᵼ�±��������ڵ��߳��˳�
}

/*�ȴ����еĿͻ��˵�����,���ÿ���ͻ��˶�Ҫ��������һ���µ��߳̽��м�������
*/
void * waitForMutilClients(void *par)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ Begin listen client(s)!\n");
	int		icount		= 0;

	//ϵͳ������ʱ��Ҫ��������߳�
	//1 ������߳�
	//2 ����ַ��߳�

	//////////////////////////////////////////////////////////////////////////
	//ֻ����windows��ʹ��
#if defined(_WIN32) || defined(WIN32)
	//LoadLibrary(_T("pthreadVC2.dll"));//?????????�Ƿ��ҪVC6.0����Ҫ,vs2008???ֻ��Ҫ���õ���ǰ���̵�Ŀ¼�¼���

	for (int j=0; j<MAX_BIND_PORT_NUM_COUNT;j++){
		if (MAX_BIND_PORT_NUM_COUNT == j) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to bind port %d.\n", global_port_num);
			return 0;
		}

		//socket�ĳ�ʼ������
		WSADATA	wsaData;
		WORD	sockVersion	= MAKEWORD(2, 0);
		if (0 != WSAStartup(sockVersion, &wsaData)) {
			return 0;
		}
		//����������socket
		//SOCKET socket_server_local;
		socket_server_local = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//ʵ��Э��socket
		if (INVALID_SOCKET == socket_server_local) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Invalid socket.\n");

			WSACleanup();
			return 0;
		}

		sockaddr_in	addr_sev;  
		addr_sev.sin_addr.s_addr = INADDR_ANY; //�Զ���д�����ĵ�ַ
		addr_sev.sin_family = AF_INET;
		addr_sev.sin_port = htons(global_port_num);//���������ļ�����
		//addr_sev.sin_addr.s_addr = inet_addr("127.0.0.1");

		int nSendBuf = CARICCP_SOCKET_BUFFER_SIZE*2;//8K
		icount =0;
		for (; icount <= MAX_BIND_COUNT; icount++) {
			if (MAX_BIND_COUNT == icount) {//�ﵽһ��������,�˳�
				WSACleanup();
				continue;
			}

			//����socket��״̬,��:���ͻ�����(SO_SNDBUF)�Ĵ�С
			/*����μ�:http://www.cppblog.com/killsound/archive/2009/01/16/72138.html

			��send()��ʱ��,���ص���ʵ�ʷ��ͳ�ȥ���ֽ�(ͬ��)���͵�socket���������ֽ�
			(�첽);ϵͳĬ�ϵ�״̬���ͺͽ���һ��Ϊ8688�ֽ�(ԼΪ8.5K)����ʵ�ʵĹ����з�������
			�ͽ����������Ƚϴ�,��������socket������,��������send(),recv()���ϵ�ѭ���շ���
			// ���ջ�����
			int nRecvBuf=32*1024;//����Ϊ32K
			setsockopt(s,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
			//���ͻ�����
			int nSendBuf=32*1024;//����Ϊ32K
			setsockopt(s,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
			*/
			int set;
			set=setsockopt(socket_server_local,SOL_SOCKET,SO_SNDBUF,(char *)&nSendBuf,sizeof(int));
			if(-1 == set){
				//������
			}
			if (SOCKET_ERROR == bind(socket_server_local, (sockaddr *) &addr_sev, sizeof(addr_sev))){//���ַ���а�
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to bind.\n");

				//�ʵ�������һ��
				CARI_SLEEP(2000);
				continue;
			}

			goto success_flag;
		}

		//�˿ں��Զ�����1
		global_port_num++;
	}

/*---------*/
success_flag:

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ �󶨶˿�(%d)�ɹ�,�ȴ�client��������... ...\n",global_port_num);
		if (SOCKET_ERROR == listen(socket_server_local, NUM_CLIENTS)){//����sock_sev��ʶһ�������ѽ�������δ���ӵ��׽��ֺ�,������Ը����������������.
			//backlog��ʾ�������Ӷ��е���󳤶�,���������Ŷ�����ĸ���,Ŀǰ��������ֵΪ5
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to listen.\n");
			WSACleanup();
			return 0;
		}


	//////////////////////////////////////////////////////////////////////////
	//linuxϵͳ��ʹ��
#else
	//////////////////////////////////////////////////////////////////////////
	//�Ż�����һ��:���һ�ΰ�ʧ��,����,���߿���ʹ��ĳ���˿ںŶ�,ĳ���˿�bindʧ��
	//����ʹ����һ���˿ں�,���εݼ�
	for (int j=0; j<MAX_BIND_PORT_NUM_COUNT;j++){
		if (MAX_BIND_PORT_NUM_COUNT == j) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to bind port %d.\n", global_port_num);
			return 0;
		}
		if(0 != j){
			//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ ��ʼ׼���ذ��¶˿�(%d)\n", global_port_num);
		}
		else{
			//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ ��ʼ׼���󶨶˿�(%d)\n", global_port_num);
		}
		//int socket_server_local;  		//����socket��id��
		struct sockaddr_in	my_addr;	    //������ַ��Ϣ
		//struct sockaddr_in remote_addr;   //�ͻ��˵�ַ��Ϣ
		unsigned int		sin_size;
		int one = 1;
		memset(&my_addr, 0, sizeof(my_addr));

		my_addr.sin_addr.s_addr = INADDR_ANY;  //�Զ���д�����ĵ�ַ inet_addr("127.0.0.1");
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(global_port_num);//���������ļ�����
		bzero(&(my_addr.sin_zero), 8);
		socket_server_local = socket(AF_INET,	//Address families.
			SOCK_STREAM,				//Types 	 
			IPPROTO_TCP);				//ʵ��Э��socket;
		if (0 > socket_server_local) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "0 > socket_local :%d.\n", socket_server_local);
			return 0;
		}
		int nSendBuf = CARICCP_SOCKET_BUFFER_SIZE*2;//8K
		icount =0;
		for (; icount <= MAX_BIND_COUNT; icount++) {
			if (MAX_BIND_COUNT == icount) {//�ﵽһ��������,�˳�
				close(socket_server_local);
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to bind port %d.\n", global_port_num);
				continue;
			}

			//////////////////////////////////////////////////////////////////////////
			//�μ�:\freeswitch-1.0.4\libs\xmlrpc-c\lib\abyss\src\socket_unix.c :bindSocketToPort()����
			//��Ҫ����һ��socket״̬SO_REUSEADDR
			//setsockopt(socket_server_local, SOL_SOCKET, /*SO_REUSEADDR*/SO_BROADCAST, (void *)&one, sizeof(int));
			setsockopt(socket_server_local,SOL_SOCKET,SO_SNDBUF,(char *)&nSendBuf,sizeof(int));
			if (bind(socket_server_local, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) < 0) {
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "--- �󶨶˿�(%d)ʧ��,����%d.��������... ...\n", global_port_num,icount + 1);
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "bind port :%s\n", strerror(errno));

				//�ؼ������ӡ
				//fprintf(stderr,"Bind error:%s\n",strerror(errno)); 

				//�ʵ�������һ��
				CARI_SLEEP(2000);
				continue;
			}
			goto success_flag;
		}

		//�˿ں��Զ�����1
		global_port_num++;
	}

/*---------*/
success_flag:

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "+++ Succeed binding port (%d),waiting for client to link the server... ...\n",global_port_num);
	if (-1 == listen(socket_server_local, NUM_CLIENTS)){//����localSocket��ʶһ�������ѽ�������δ���ӵ��׽��ֺ�,������Ը����������������.
														//NUM_CLIENTS��ʾ�������Ӷ��е���󳤶�,���������Ŷ�����ĸ���,Ŀǰ��������ֵΪ5
		close(socket_server_local);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to listen.\n");
		fprintf(stderr,"Listen error:%s\n",strerror(errno)); 
		return 0;
	}
#endif

	//ѭ�������ж��ٿͻ��˽�����������.���ÿ���ͻ�������ר�Ž���һ���߳�
	while (loopFlag){//ʹ��ѭ����ʶ����
		//////////////////////////////////////////////////////////////////////////
		//ֻ����windows��ʹ��
#if defined(_WIN32) || defined(WIN32)

		SOCKET				sock_client;
		struct sockaddr_in	remote_addr;
		int					nAddrLen	= sizeof(remote_addr);

		//��ÿͻ��˵�����:����sock_sever���л��
		sock_client = accept(socket_server_local, (struct sockaddr *) &remote_addr, &nAddrLen);//û������,��һֱ����?????
		if (INVALID_SOCKET == sock_client){//����������Ͽ�ͻȻ����
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to accept socket.\n");
			if (shutdownFlag){
				break;
			}
			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		//unix������ʹ��
#else
		int				sock_client;	  //�ͻ��˵�socket��
		struct sockaddr	remote_addr;	  //�ͻ��˵�ַ��Ϣ
		socklen_t		sin_size	= sizeof(remote_addr);
		//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n+++++++++++ ccp server ׼�� accept\n");
		sock_client = accept(socket_server_local, &remote_addr, &sin_size);
		//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n+++++++++++ ccp server �Ѿ� accept\n");
		if (-1 == sock_client) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Failed to accept socket.\n");
			if (shutdownFlag){
				break;
			}
			continue;
		}
#endif
		//���δ���һ��"shutdown"����,�Ż�����
		if (shutdownFlag){
			/*CARI_CLOSE_SOCKET(socket_server_local);*/
			break;
		}

		//Ϊ����ͬʱ�Ͷ���ͻ��˽�������,��ͬʱ����,��Ҫ�Ѷ�Ӧ�����Ӵ������
		//���µ��߳��н������д���,ά�ִ�����
		int				iRet			= 0;   

		//���������ͻ��˵�socket
		//ʹ���� proClientSocket �������ٴ˶���
		CSocketHandle	*aSocketHandle	= CARI_CCP_VOS_NEW(CSocketHandle);//�ڶ�Ӧ�����߳��˳���ʱ�����Ĵ˶���
		aSocketHandle->sock_client = (int) sock_client;
		aSocketHandle->addr_client = remote_addr;

		//�Ƿ���Ҫ����fork()��������һ��"�ӽ���"���������ݴ��䲿��???
		//��Դ˿ͻ��˵�������Ҫ�����µ��߳�.����,Ҫ�Ѷ�Ӧ�Ŀͻ��˵�socket��������
		pthread_t	thrd;
		iRet = pthread_create(&thrd, NULL, proClientSocket, (void*) (aSocketHandle));
		if (0 != iRet) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "!!! Create pthread error!\n");

			//Ӧ�÷�֪ͨ���ͻ���,�����������쳣!!!!!

			//�رն�Ӧ�Ŀͻ��˵�socket
			CARI_CLOSE_SOCKET(sock_client);

			//����socket�ľ������
			CARI_CCP_VOS_DEL(aSocketHandle);

			continue;
		} 

		//��¼���û���������
		global_client_num++;

		//�߳�ʵ������
		//CModuleManager::getInstance()->saveThrd(sock_client,&thrd);

		CARI_SLEEP(2*1000);

	}//end ѭ���������пͻ��˵�����

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 4 ccp server exited.\n");
	return 0;
}

/*
*��ģ�����ں���:ģ���ʼ���ص�ʱ��ʼ����
*/
int cari_net_server_start()
{
	//��չ:���ɽ��������ǵ���������ϵͳ

	//�����µ��߳�,�ȴ����е��û���������
	int	iRet = pthread_create(&cari_ccp_server_thrd, NULL, waitForMutilClients, NULL);
	if (0 != iRet) {
		return -1;
	}

	//��ʼ����
	initmutex();

	return 0;
}

/*�ر�socket,�ͷŶ˿ں�
*/
void shutdownSocketAndGarbage()
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n\n--- 1 ccp server shutdown Socket And Garbage()\n");

	setLoopFlag(SWITCH_FALSE);//��ѭ����ʶ����Ϊfalse,��ѭ���߳��˳�

	if (0 > socket_server_local) {
		return;
	}

	//kill client���߳�
	//CModuleManager::getInstance()->killAllClientThrd();

	//kill server�����߳�:���С����˴���,����socket�ȶ����ܼ�ʱ�ر�
	//int kill_rc = pthread_kill(cari_ccp_server_thrd,0);
	//if(kill_rc == ESRCH)
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 2 the ccp server thread did not exits or already quit\n");
	//else if(kill_rc == EINVAL)
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 2 signal is invalid\n");
	//else if(kill_rc == 0)//�̹߳رճɹ�
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "\n--- 2 the ccp server thread is alive and killed success\n");

	//�ر�client��socket,�Ǳ�Ҫ???
	CModuleManager::getInstance()->closeAllClientSocket();

	//�ر�server��socket
	CARI_CLOSE_SOCKET(socket_server_local);

	//�����������������"ʵ������",aCGarbageRecover����������??��ʱ����
	//CGarbageRecover	aCGarbageRecover;
	//ֱ�ӽ������
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n--- 2 CGarbageRecover() begin\n");
	CModuleManager::getInstance()->clear();				//���"ģ��ʵ��"���ڴ���Դ
	CMsgProcess::getInstance()->clear();					//��
	CMsgProcess::getInstance()->clearAllCmdFromQueue();   //�����������е���Դ

	//��ʵ������ϵͳ�˳�ʱ�Զ��ͷ�
	//CARI_CCP_VOS_DEL(CMsgProcess::getInstance());
	//CARI_CCP_VOS_DEL(CModuleManager::getInstance());
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n--- 2 CGarbageRecover() end\n");

	//�ָ���¼���û�����
	global_client_num = 0;

#if defined(_WIN32) || defined(WIN32)
#else
	//�˴���Գ���core�ļ������,�Ż�����,Ŀǰ�޷���λ����������???
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
