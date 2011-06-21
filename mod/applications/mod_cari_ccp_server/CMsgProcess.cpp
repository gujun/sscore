#include "CMsgProcess.h"

#define     BUFFER_SIZE                     sizeof(common_ResultResponse_Frame)

int			inum							= 0;
static int  err_broadsendnum                = 0;
static int  err_unicastsendnum              = 0;

//#define INFINITE  		  0xFFFFFFFF  // Infinite timeout

/*��̬��Ա�ĳ�ʼ��*/
CMsgProcess	*CMsgProcess::m_reqProInstance	= CARI_CCP_VOS_NEW(CMsgProcess);//��Ҫ��ʲô�ط�ע������????????


/*���캯��,��ʼ������
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

	/* ��Ĭ�����Գ�ʼ��һ������������*/
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	//�����첽������߳�
	creatAsynCmdThread();
}

/*��������
 */
CMsgProcess::~CMsgProcess()
{
	//m_listRequestFrame.clear();//shutdown��ʱ�����������???
}

/*��õ�ʵ������
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

/*�����첽������߳�
*/
void CMsgProcess::creatAsynCmdThread()
{
	pthread_attr_t	attr_asynCmd;

	/*��ʼ������*/
	pthread_attr_init(&attr_asynCmd); 

	//�����첽����������߳�,�ɿ��Ƕഴ������
	int	iRet	= 0;
	for (int i = 1; i <= CARICCP_MAX_CREATTHREAD_NUM; i++) {
		//int iRet= pthread_create(&thread_asynPro,NULL,asynProCmd,NULL);
		iRet = pthread_create(&thread_asynPro, &attr_asynCmd, asynProCmd, (void*) &attr_asynCmd);
		if (0 != iRet)//�ɿ��Ƕഴ������
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "CMsgProcess::creatThread() %d count error!\n", i);
			continue;
		}

		break;
	}
}

/*������������ں���,�漰�����̵߳Ĵ���,�˴��Ƿ�ҪҪ���Ǽ���???
*/
void CMsgProcess::proCmd(u_int socketID, //���ӵĿͻ��˵�socket��
						 common_CmdReq_Frame *&common_reqFrame)  //ͨ�õĿͻ�������֡
{
	//���ͻ��˵�����֡ת��Ϊ�ڲ�֡
	inner_CmdReq_Frame	*inner_reqFrame	= CARI_CCP_VOS_NEW(inner_CmdReq_Frame);
	initInnerRequestFrame(inner_reqFrame);

	//��"ͨ��"����֡ת����"�ڲ�"����֡
	//��Ҫ���漰�����Ľ���,���ַ����ĸ�ʽת��Ϊkey-value��ʽ
	covertReqFrame(common_reqFrame, inner_reqFrame);

	//����ǰ�Ĳ�����ͨ��֡����ɾ��,�ͷ��ڴ�
	CARI_CCP_VOS_DEL(common_reqFrame);

	inner_reqFrame->socketClient = socketID;	 //�������ӵĿͻ��˵�socket��

	inner_reqFrame->sSourceModuleID = BASE_MODULE_CLASSID;//Դģ�������

	//�Ƿ�Ϊͬ������
	bool	bSynchro	= intToBool(inner_reqFrame->body.bSynchro);

	//�첽����
	/*bSynchro = false;*/
	if (bSynchro) {
		//�½�һ��"��Ӧ֡",������һЩ��Ҫ�Ĳ���
		inner_ResultResponse_Frame	*inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

		//��ʼ���ڲ���Ӧ֡,����ͨ������֡�����й����ݸ���Ӧ֡
		initInnerResponseFrame(inner_reqFrame, inner_respFrame);

		//ֱ�ӽ����߼�����
		syncProCmd(inner_reqFrame, inner_respFrame);

		//�����֡���͸���Ӧ�Ŀͻ���
		//������֡���ܷ��õ�������,����ͻ��˿��ܻ�ȴ�һ��ʱ����ֵ
		commonSendRespMsg(inner_respFrame);

		//����"����֡"
		CARI_CCP_VOS_DEL(inner_reqFrame);

		//����"��Ӧ֡"
		CARI_CCP_VOS_DEL(inner_respFrame);
	} else//�첽����
	{
		//����֡�ȴ�ŵ������л��ǿ������������̴߳���??????
		//Ҳ��֤�����˳����:�����ֹ��������
		asynProCmd_2(inner_reqFrame);//ע����½���֡��ʲô�ط�Ҫ����
	}
}

/*ͬ�������:��Ҫ���Ҹ������ģ��
 */
int CMsgProcess::syncProCmd(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_respFrame)//�������:��Ӧ֡
{
	//�Ȳ鿴������
	u_int	iCmdCode	= inner_reqFrame->body.iCmdCode;

	//Ŀ��ģ���
	//�������ò鿴�������������ĸ�ģ�鸺����
	u_int	iModuleID	= CModuleManager::getInstance()->getModuleID(iCmdCode);

	//���ڲ�������֡�ڲ����¸���
	//inner_reqFrame->header.sSourceModuleID = 0;		 //����ǰ����������ModuleManager���͵�
	//Ӧ���ں����ĵ����߽�������
	inner_reqFrame->sDestModuleID = iModuleID;			 //��������Ҫ�·�������ģ��

	//socket��������
	inner_respFrame->socketClient = inner_reqFrame->socketClient;

	//�����ڲ�ע�����ģ��,������Ӧ�Ļص��������д���
	CBaseModule	*subModule	= CModuleManager::getInstance()->getSubModule(iModuleID); //ȡ����ŵ���ģ��
	if (NULL != subModule) {
		//printf("ģ���%d \n",iModuleID);
		subModule->receiveReqMsg(inner_reqFrame, inner_respFrame);		//��̬�ص�
	} 

	CARI_CCP_VOS_DEL(inner_reqFrame);  //ɾ���ڲ�����֡
	return CARICCP_SUCCESS_STATE_CODE;
}

/*�첽�����
 */
void CMsgProcess::asynProCmd_2(inner_CmdReq_Frame *&inner_reqFrame)
{
	pushCmdToQueue(inner_reqFrame);//ע����½���֡��ʲô�ط�Ҫ����
}

/*
 */
void CMsgProcess::covertRespParam(inner_ResultResponse_Frame *&inner_respFrame, common_ResultResponse_Frame *&common_respFrame)//�������
{
	//common_respFrame->iResultNum = inner_respFrame->iResultNum;

	//��������ת��

	return;
}

/*��ͨ�õ�����֡ת��Ϊ�ڲ�������֡
 */
void CMsgProcess::covertReqFrame(common_CmdReq_Frame *&common_reqMsg, inner_CmdReq_Frame *&inner_reqFrame)//�������:�ڲ�������֡�ṹ
{
	//�ȿ���"֡ͷ"��"����"����
	memcpy(inner_reqFrame, common_reqMsg, sizeof(common_CmdReqFrame_Header) + sizeof(common_CmdReqFrame_Body));  

	//����"������"��
	covertStrToParamStruct(common_reqMsg->param.strParamRegion, inner_reqFrame->innerParam.m_vecParam);//����¹����Ĳ����ṹ
}

/*��"�ڲ���Ӧ֡"ת����"ͨ����Ӧ֡"
 */
void CMsgProcess::covertRespFrame(inner_ResultResponse_Frame *&inner_respFrame, common_ResultResponse_Frame *&common_respMsg)//������� :ͨ�õ���Ӧ֡
{
	//�ȿ���"֡ͷ"
	memcpy(common_respMsg, inner_respFrame, sizeof(common_ResultRespFrame_Header));

	//���ݱ�ͷ��ת��
	//��������ת��
	covertRespParam(inner_respFrame, common_respMsg);
}

/*����֡��ϸ������,��һ����������
 */
void CMsgProcess::analyzeReqFrame(common_CmdReq_Frame *&common_reqMsg)
{
	//�ȰѲ����������ڲ��Ľṹ
	inner_CmdReq_Frame	*aInnerReqFrame	= CARI_CCP_VOS_NEW(inner_CmdReq_Frame);
	initInnerRequestFrame(aInnerReqFrame);
	//�ȿ���"֡ͷ"��"����"����
	memcpy(aInnerReqFrame, common_reqMsg, sizeof(common_CmdReqFrame_Header) + sizeof(common_CmdReqFrame_Body));  

	//����"������"��
	covertStrToParamStruct(common_reqMsg->param.strParamRegion, aInnerReqFrame->innerParam.m_vecParam);//����¹����Ĳ����ṹ

	//����ǰ�Ĳ�����ͨ��֡����ɾ��,�ͷ��ڴ�
	CARI_CCP_VOS_DEL(common_reqMsg);
	CARI_CCP_VOS_DEL(aInnerReqFrame);
}

/*���ͽ������Ӧ�Ŀͻ���:���ͬ�����첽�����ͳһ�ӿ�
*����������:����ķ��ؽ����֪ͨ���͵���Ϣ����
*/
void CMsgProcess::commonSendRespMsg(inner_ResultResponse_Frame *&inner_respFrame,
									int nSendType)
{
	//����Ҫ������������ظ�client��
	if (!inner_respFrame->isImmediateSend) {
		
		//�����δ�����
		//return;
	}
	bool            bres             = true;
    int             iEndstrlen       = strlen(CARICCP_ENTER_KEY);
	int				iResult		     = CARICCP_SUCCESS_STATE_CODE;
	int				iFrameHeaderSize = sizeof(common_ResultRespFrame_Header); //�������ݵ�ʵ�ʳ���,�ȼ���֡ͷ
	socket3element	element;
	string			strData				= "",strTmp	= ""; //������Ҫ���͵ļ�¼
	int				iPos				= -1;
	bool         	isFirst				= true;
	int				iSingelDataLength	= 0; //������¼�ĳ���
	int				iSendedDataSumLen	= 0; //ÿ�η��͵��ܵ����ݵ��ܳ���
	int				iResultNum			= 0; //��Ҫ���͵ļ�¼����
	int				iMaxBuffer			= CARICCP_MAX_TABLECONTEXT - 1;//��ȥheaderʣ����������ĳ���

	//�������֪ͨ���͵Ľ����Ϣ,��ô�˽��һ���ǵ�ǰ�ͻ��˷��͵��������Ϻ󷵻صĽ��
	if (COMMON_RES_TYPE == nSendType) {
		iResult = CModuleManager::getInstance()->getSocket(inner_respFrame->socketClient, element);
		if (CARICCP_ERROR_STATE_CODE == iResult) {
			//��ӡ������Ϣ
			return;
		}
	}

	//����ʱ���
	getLocalTime(inner_respFrame->header.timeStamp);

	//��Ҫ���ڲ���Ӧ֡ת����ͨ����Ӧ֡,���͸���Ӧ�Ŀͻ���
	common_ResultResponse_Frame	*common_respFrame = CARI_CCP_VOS_NEW(common_ResultResponse_Frame);
	initCommonResponseFrame(common_respFrame);
	//covertRespFrame(inner_respFrame,common_respFrame);

	//�ȿ���"֡ͷ"
	memcpy(common_respFrame, inner_respFrame, sizeof(common_ResultRespFrame_Header));

	//�������ж��Ƿ�Ϊ"֪ͨ����".
	//�����֪ͨ���͵���Ϣ,��ת��"֪ͨ����"����,ֱ�ӷ���֪ͨ����
	if (NOTIFY_RES_TYPE == nSendType) {
		goto norifyMark;
	}

	//���������ɹ�,ֻ���ظ�"���ʹ�����Ŀͻ���".����,֪ͨ���ͷ��صĽ����"��0"
	if (CARICCP_SUCCESS_STATE_CODE != inner_respFrame->header.iResultCode) {
		
		//����ת��
		//convertFrameCode(common_respFrame,"gb2312","utf-8");

		//ֻ��Ҫ������Ϣͷ���ּ���,��Ϣ�岿�ֵ����ݺ���
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

//֪ͨ���͵Ŀ�ʼ�����ʶ
norifyMark:

	//������"��ѯ��"��¼,����,֪ͨ���Ͳ�һ��Ҫ���ز�ѯ���¼,����:reboot����
	if (0 == inner_respFrame->iResultNum 
		&& 0 == inner_respFrame->m_vectTableValue.size()) {
		common_respFrame->iResultNum = inner_respFrame->iResultNum;

		//����ת��
		//convertFrameCode(common_respFrame,"gb2312","utf-8");

		//ֻ��Ҫ������Ϣͷ���ּ���
		//char	sendBuf[sizeof(common_ResultRespFrame_Header)];
		//memset(sendBuf, 0, sizeof(sendBuf));
		//memcpy(sendBuf, common_respFrame, iFrameHeaderSize);

		//�Ƿ�Ϊ"֪ͨ"����
		if (common_respFrame->header.bNotify
			|| DOWN_RES_TYPE == nSendType) {
				char	sendBuf[sizeof(common_ResultRespFrame_Header)];
				memset(sendBuf, 0, sizeof(sendBuf));
				memcpy(sendBuf, common_respFrame, iFrameHeaderSize);
				bres = broadcastRespMsg(sendBuf, iFrameHeaderSize,inner_respFrame->header.strCmdName);//�㲥
				if (!bres){
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Broadcast cmd \"%s\" resp failed!\n",
						common_respFrame->header.strCmdName);
				}
		} else {
			bres = unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize,inner_respFrame->header.strCmdName);    //����
			if (!bres){
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
					common_respFrame->header.strCmdName);
			}
		}

		goto endMark;
	}

	//"��ѯ"��¼��:Ŀǰ��ʱû��Ⱥ���ǳ���,�����ڴ˴�ͳһ���п���

///*֪ͨ���͵���Ϣ*/
//norifyMark:

	//��ͷ�Ŀ���
	memcpy(common_respFrame->strTableName, inner_respFrame->strTableName, sizeof(common_respFrame->strTableName));
	iFrameHeaderSize += sizeof(common_respFrame->iResultNum);   //��¼�����ĳ���
	iFrameHeaderSize += sizeof(common_respFrame->strTableName); //��¼�б����ĳ���

	//���ڲ�ѯ��¼ :��ѯ�ļ�¼���ܴ�,��Ҫ�ֿ鷢��
	//����֪ͨ���͵ļ�¼,��ʱֻ������������
	strData		= "";
	strTmp		= ""; //������Ҫ���͵ļ�¼
	iPos		= -1;
	isFirst		= true;
	iSingelDataLength	= 0; //������¼�ĳ���
	iSendedDataSumLen	= 0; //ÿ�η��͵��ܵ����ݵ��ܳ���
	iResultNum	= 0; //��Ҫ���͵ļ�¼����
	iMaxBuffer	= CARICCP_MAX_TABLECONTEXT - 3;//ʹ��3���ַ���չ

	//1 ��ʼ����Ҫ���صļ�¼���г��ȼ���,���ܳ����涨���ֽ�����
	for (vector< string>::iterator iter = inner_respFrame->m_vectTableValue.begin(); 
		iter != inner_respFrame->m_vectTableValue.end(); ++iter) {
		strTmp = *iter;
		iSingelDataLength = strTmp.length();//������¼�ĳ���
		iSendedDataSumLen += iSingelDataLength;    //�����ܵĳ���
		if (iSendedDataSumLen < iMaxBuffer) {
			//����������Ѿ��Ƿ�װ������,��ʽ����:
			//���м�¼֮��ʹ������ָ��"��"
			//����֮�����ʹ�ûس�����\r\n�ֿ�,�����͸�client�˲����㴦��

			//2 �Ż�����һ��,�鿴�Ƿ��Ѿ���װ��\r\n��ʶ��,�ɴ˴�ͳһ��װ�Ż�һ��,������һ����¼����\r\n�ַ�!!!!!!!!!!!!!!!!!
			iPos = strTmp.find_last_of(CARICCP_ENTER_KEY);
			if (-1 == iPos) {
				iSendedDataSumLen += iEndstrlen;//β��׼������"\r\n"

				//�������͵����ݳ���,ת��next����
				if (iSendedDataSumLen >= iMaxBuffer) {
					iSendedDataSumLen -= iEndstrlen; //�Ѷ����ĳ���Ҫ����
					goto next;
				}

				strTmp.append(CARICCP_ENTER_KEY);//ĩβ����\r\n������
			}

			strData.append(strTmp);   
			iResultNum += 1;

			//���͵��ַ�����û�г�������,���������
			continue;//----------
		}
/**/
next:
		iSendedDataSumLen -= iSingelDataLength;   //�Ѷ����ĳ���Ҫ����,����������֡��ʱ��,heap���������
		common_respFrame->iResultNum = iResultNum;//��Ҫ���͵ļ�¼����

		//���ý�����ʶ
		if (isFirst) {
			isFirst = false;
			common_respFrame->header.isFirst = true;  //�����һ����¼
		} else {
			common_respFrame->header.isFirst = false;
		}
		common_respFrame->header.isLast = false; 	//�������һ����¼

		//�������Ŀ���---------------------------------------------------------------
		memcpy(common_respFrame->strTableContext, strData.c_str(), iSendedDataSumLen);
		//char sendBuf[sizeof(common_ResultResponse_Frame)];
		//memset(sendBuf, 0, sizeof(sendBuf));

		//����ת��
		//convertFrameCode(common_respFrame,"gb2312","utf-8");
		//memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));

		//3 ��ʼ�Դ����������ݽ��зֿ鷢��,�Ƿ���Ҫ֪ͨ����ע��/��¼�Ŀͻ���
		if (NOTIFY_RES_TYPE == nSendType/*common_respFrame->header.bNotify*/
			||DOWN_RES_TYPE == nSendType) {
				char sendBuf[sizeof(common_ResultResponse_Frame)];
				memset(sendBuf, 0, sizeof(sendBuf));
				memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));
				bres = broadcastRespMsg(sendBuf, iFrameHeaderSize + iSendedDataSumLen,inner_respFrame->header.strCmdName);//��ע��ͻ��˵���Ӧ
				if (!bres){
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Broadcast cmd \"%s\" resp failed!\n",
						common_respFrame->header.strCmdName);
				}
		} else {
		  	bres =unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize + iSendedDataSumLen,inner_respFrame->header.strCmdName);//�Ե����ͻ�����Ӧ
			if (!bres){
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
					common_respFrame->header.strCmdName);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//4 ���¼���,��׼��������һ�ֵ����ݷ���
		strData = "";
		iSendedDataSumLen = iSingelDataLength;

		//�Ż�����һ��,�鿴�Ƿ��Ѿ���װ��\r\n��ʶ��
		iPos = strTmp.find_last_of(CARICCP_ENTER_KEY);
		if (-1 == iPos) {
			iSendedDataSumLen += iEndstrlen;//β��׼������"\r\n"

			//�������͵����ݳ���
			if (iSendedDataSumLen >= iMaxBuffer) {
				iSendedDataSumLen -= iEndstrlen;//�Ѷ����ĳ���Ҫ����
				
				continue;//���ٷ��ʹ�����,��������,�����������!!!
			}

			strTmp.append(CARICCP_ENTER_KEY);//ĩβ����\r\n������
		}

		strData.append(strTmp);//Ҫ��֤�˼�¼�����ͳ�ȥ
		iResultNum = 1; 	   //��¼��ֵΪ1

	}//end for���еĲ�ѯ��¼

	//5 ��Ҫ����һ��,����¼ʣ�����󲿷ַ��ͳ�ȥ
	if (0 != iSendedDataSumLen || 0 != iResultNum) {
		common_respFrame->iResultNum = iResultNum;	  //��Ҫ���͵ļ�¼����

		//���ý�����ʶ
		if (isFirst) {
			common_respFrame->header.isFirst = true;  //�����һ����¼
			isFirst = false;
		} else {
		  	common_respFrame->header.isFirst = false; //�ǵ�һ����¼
		}
		common_respFrame->header.isLast = true;		 //�������һ����¼

		//�������Ŀ���---------------------------------------------------------------
		memcpy(common_respFrame->strTableContext, strData.c_str(), iSendedDataSumLen);

		//char sendBuf[sizeof(common_ResultResponse_Frame)];
		//memset(sendBuf, 0, sizeof(sendBuf));
		////����ת��
		////convertFrameCode(common_respFrame,"gb2312","utf-8");
		//memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));

		//�Ƿ���Ҫ�㲥Ⱥ��
		if (NOTIFY_RES_TYPE == nSendType/*common_respFrame->header.bNotify*/
			||DOWN_RES_TYPE == nSendType) {
				char sendBuf[sizeof(common_ResultResponse_Frame)];
				memset(sendBuf, 0, sizeof(sendBuf));

				//����ת��
				//convertFrameCode(common_respFrame,"gb2312","utf-8");
				memcpy(sendBuf, common_respFrame, sizeof(common_ResultResponse_Frame));
				bres = broadcastRespMsg(sendBuf, iFrameHeaderSize + iSendedDataSumLen,inner_respFrame->header.strCmdName);//�㲥֪ͨ
				if (!bres){
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Broadcast cmd \"%s\" resp failed!\n",
						common_respFrame->header.strCmdName);
				}
		} else {
			bres = unicastRespMsg(element, /*sendBuf*/common_respFrame, iFrameHeaderSize + iSendedDataSumLen, inner_respFrame->header.strCmdName); //����֪ͨ
			if (!bres){
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unicast cmd \"%s\" resp failed!\n",
					common_respFrame->header.strCmdName);
			}
		}
	
	}//end (0 !=iSendedDataSumLen ...


endMark:
	/*CARI_CCP_VOS_DEL(inner_respFrame);//�����ڲ���Ӧ֡,�ɵ����߸�������*/
	CARI_CCP_VOS_DEL(common_respFrame);//����ͨ����Ӧ֡
}

/*"����"������Ӧ��Ϣ
 */
bool CMsgProcess::unicastRespMsg(socket3element &element,
								 const char *respMsg, 	  
								 int  bufferSize,
								 char* cmdname)
{
	bool                        berrsend= false;
	
	//�ȷ�������,����ʵ�ʳ��Ƚ��з�������,���͵����ܺ�cpu�й�,��Ҫ�ʵ��ĵ���һ��
	int	sendbytes	= send(element.socketClient, respMsg, bufferSize, 0);
	if (CARICCP_ERROR_STATE_CODE == sendbytes){//SOCKET_ERROR
		//�ͻ��˺�,�ͻ��˵�ַ
		inum++;
		err_unicastsendnum++;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Return response frame failed, cmdname=%s,clientID :%d ,socketID :%d!\n", 
			cmdname,
			element.sClientID,
			element.socketClient);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Cause :%s\n",strerror(errno));
		//fprintf(stderr,"send error:%s \n",strerror(errno)); 
		
		if (10<err_unicastsendnum){
			//�ر�����client��socket
			berrsend = true;
			goto end;
		}

		return false;
	}

	//printf("����֡���� :%d\n",sendbytes);

	//Ϊ�˷�ֹ��������������,�˴�����һ�·��͵�ʱ����,0.5��
	CARI_SLEEP(500);

end:
	//�رղ������ǰ��client socket
	if (berrsend){
		err_unicastsendnum = 0;
		CModuleManager::getInstance()->eraseSocket(element.socketClient);
		
		//�رյ�ǰ�ͻ��˵�socket
		CARI_CLOSE_SOCKET(element.socketClient);
		return false;
	}

	return true;
}

/*�����ع�:�������ؽ����Ϣ
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

	//�˴�ͳһ����web��lmt client
	if (0 == element.sClientID){
		//��ʽ: [cmdID=201,resultcode=0]�����ɹ�.
		string strres = "",strT;
		strres.append("[");
		strres.append("cmdID=");
		strT = intToString(respFrame->header.iCmdCode);strres.append(strT);strres.append(",");
		strres.append("resultcode=");
		strT = intToString(respFrame->header.iResultCode);strres.append(strT);

		//��ѯ��(һ��ͷ�ҳ)
		if (0 !=respFrame->iResultNum){
			//��Ӧ�÷�װ�Ƿ�Ϊ���һ����¼�ı�ʶ,����webҳ���ҳ��ʾʹ��
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
		else{//�ǲ�ѯ��,���·�װ���
			strres.append("]");
			strres.append(respFrame->header.strResuleDesc);
		}

		//�ǳ��ؼ�: java�˲���readLine(),������Ҫ"������"
		//strres.append(CARICCP_ENTER_KEY);
		//strres.append("����TT");
		//strres.append(CARICCP_ENTER_KEY);

		isize = strres.length();
		memcpy(sendBuf, strres.c_str(), isize);

		isize++;//��β�����һ��������'\0'
		isize = CARICCP_MIN(BUFFER_SIZE - 1,isize);

		//java�˲���read(),�ؼ��Ľ�����
		sendBuf[isize] = '\0';
		
	}
	else{//һ���lmt��
		memcpy(sendBuf, respFrame, bufferSize);
	}

	
	bres = unicastRespMsg(element,sendBuf,isize,cmdname);//isize���ò��������web�˽��ղ�����Ϣ֡
	return bres;
}

/*�㲥������Ӧ��Ϣ
 */
bool CMsgProcess::broadcastRespMsg(const char *respMsg, 
								   int bufferSize,
								   char* cmdname)
{
	bool						bResult	= true;
	bool                        berrsend= false;
	int	sendbytes                       = 0;

	//����ע��Ŀͻ���,���ÿ���ͻ������η�����Ϣ
	socket3element						socketClientElement;
	vector< socket3element>				vecTmp	= CModuleManager::getInstance()->getSocketVec();
	vector< socket3element>::iterator	iter	= vecTmp.begin();
	for (; iter != vecTmp.end(); ++iter) {
		//���ע��Ŀͻ��˵�socket�Ŷ���Ҫ�㲥֪ͨ
		socketClientElement = *iter;

		//web������,��Ч��clientID������
		if (0 == socketClientElement.sClientID
			|| CARICCP_MAX_SHORT == socketClientElement.sClientID){
			continue;
		}
		
		//printf("�㲥socketid :%d\n",socketClientElement.socketClient);
		//bResult = unicastRespMsg(socketClient, respMsg, bufferSize,cmdname);//�Ƿ�ɹ�,ÿ���ͻ��˶�Ҫ����һ��
	    sendbytes	= send(socketClientElement.socketClient, respMsg, bufferSize, 0);
		if (CARICCP_ERROR_STATE_CODE == sendbytes){//SOCKET_ERROR
			//�ͻ��˺�,�ͻ��˵�ַ
			inum++;
			err_broadsendnum++;
			bResult = false;
			
			//��ӡʧ����־
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
		//printf("����֡�����ֽ����� :%d\n",sendbytes);
		CARI_SLEEP(100);//�Ż�
	}

	//Ϊ�˷�ֹ��������������,�ͻ��Ƚ��ղ�ͬ��Ϣ.�˴�����һ�·��͵�ʱ����,0.5��
	CARI_SLEEP(500);

	//�ر����е�client socket
	if (berrsend){
		err_broadsendnum = 0;
		CModuleManager::getInstance()->closeAllClientSocket();
	}


	//��ǰ�Ĵ����߼�
	// 	//�ȷ��͵�ǰ�Ŀͻ���
	// 	bResult = unicastRespMsg(element,respMsg,bufferSize,inner_respFrame->header.strCmdName);
	// 	if (!bResult)// ����ʧ��,����ע��Ŀͻ��˲���Ҫ��֪ͨ
	// 	{
	// 		return false;
	// 	}
	// 
	// 	//����ע��������ͻ���,���ÿ���ͻ������η�����Ϣ
	// 	u_int socketClient = element.socketClient;
	// 	socket3element socketTmp;
	// 	vector<socket3element> vecTmp = CModuleManager::getInstance()->getSocketVec();
	// 	vector<socket3element>::iterator iter=vecTmp.begin();
	// 	for( ; iter != vecTmp.end(); ++iter)
	// 	{
	// 		socketTmp = *iter;
	// 		if (socketClient != socketTmp.socketClient)//����ע��Ŀͻ��˷���
	// 		{
	// 			bResult = unicastRespMsg(socketTmp,respMsg,bufferSize,inner_respFrame->header.strCmdName); 	//�Ƿ�ɹ�,ÿ���ͻ��˶�Ҫ����һ��
	// 		}
	// 	}

	return bResult;
}

/*֪ͨ���͵Ĵ���,ĳЩ���͵�����(��/ɾ)����,������Ҫ֪ͨ�����ͻ���ͬ�����Ľ��
*/
void CMsgProcess::notifyResult(inner_CmdReq_Frame *&inner_reqFrame, //ԭ����֡
							   Notify_Type_Code notifyCode,			//֪ͨ��
							   string notifyValue)					//���صļ�¼,�����¼֮��ʹ��������Ũ�����
{
	//��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	//�½�һ��"��Ӧ֡",������һЩ��Ҫ�Ĳ���
	inner_ResultResponse_Frame	*notify_inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//��ʼ���ڲ���Ӧ֡
	/*	memset(notify_inner_respFrame,0,sizeof(inner_ResultResponse_Frame));*/
	initInnerResponseFrame(inner_reqFrame, notify_inner_respFrame);

	notify_inner_respFrame->header.bNotify = true;			  //����Ϊ֪ͨ����
	notify_inner_respFrame->header.iResultCode = notifyCode;  //֪ͨ��

	//�鿴�м�����¼����,�˴��Ĵ������¸���¼֮���CARICCP_ENTER_KEY��ɾ��
	//�ڷ��ͽ����ʱ���ڽ����ж�,���
	splitString(notifyValue, CARICCP_ENTER_KEY, notify_inner_respFrame->m_vectTableValue);

	// 	notify_inner_respFrame->m_vectTableValue.push_back("1\r\n");
	// 	notify_inner_respFrame->m_vectTableValue.push_back("8001");


	//֪ͨ���ͷ��͸�����ע��Ŀͻ���
	commonSendRespMsg(notify_inner_respFrame,NOTIFY_RES_TYPE);

	//�����ڲ���Ӧ֡
	CARI_CCP_VOS_DEL(notify_inner_respFrame);
}

/*֪ͨ���͵Ĵ���,��Ҫ֪ͨ���е�ע��Ŀͻ���
*/
void CMsgProcess::notifyResult(inner_ResultResponse_Frame *&inner_respFrame)  //"�ڲ���Ӧ֡"
{
	//��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	inner_respFrame->header.bNotify = true;  //����Ϊ֪ͨ����

	//֪ͨ���ͷ��͸�����ע��Ŀͻ���
	commonSendRespMsg(inner_respFrame,NOTIFY_RES_TYPE);

	//�����ڲ���Ӧ֡
	CARI_CCP_VOS_DEL(inner_respFrame);
}

/*�����ϱ����͵Ĵ���,��Ҫ���ĳ��ģ��Ľ���ʵʱ�ϱ�
*/
void CMsgProcess::initiativeReport(inner_ResultResponse_Frame*& inner_respFrame)
{

}

/*����ַ�
 */
void CMsgProcess::dispatchCmd()
{
}

/*��������
 */
void CMsgProcess::lock()
{
	// 	InitializeCriticalSection(m_lock); //Initialize the critical section.   
	// 	EnterCriticalSection(m_lock);      //Enter Critical section

	pthread_mutex_lock(&mutex);
}

/*����
 */
void CMsgProcess::unlock()
{
	// 	LeaveCriticalSection(m_lock);     //Leave Critical section
	// 	DeleteCriticalSection(m_lock);    //Delete Critical section

	pthread_mutex_unlock(&mutex);
}

/*��������Ƿ�����
 */
bool CMsgProcess::isFullOfCmdQueue()
{
	if (CARICCP_MAX_CMD_NUM == m_listRequestFrame.size()) {
		return true;
	}

	return false;
}

/*��������Ƿ��ѿ�
 */
bool CMsgProcess::isEmptyOfCmdQueue()
{
	if (0 == m_listRequestFrame.size()) {
		return true;
	}
	return false;
}

/*�������͵�����֡���õ�������,����������,���������.
 *һ������������Ժ�,��֪ͨ�ȴ����߳�ȡ�����ݷ��ͳ�ȥ
 */
int CMsgProcess::pushCmdToQueue(inner_CmdReq_Frame *&reqFrame)
{
	lock();//����
	if (isFullOfCmdQueue())//�ж϶����Ƿ���
	{
		i_full_waiters ++;//��ʶ��������1
		//result = WaitForSingleObject(m_semaphore, INFINITE);//��ǰ�̵߳ȴ�,�����ź���

		pthread_cond_wait(&cond, &mutex);

		i_full_waiters--;//��ʶ������1

		//�Ѿ����ѵ��ǻ�������,�ж��˳�
		if (isFullOfCmdQueue()) {
			unlock();//����
			return CARICCP_ERROR_STATE_CODE;
		}
	}

	m_listRequestFrame.push_back(reqFrame);//���뵽������end

	//�������,�ͷ��ź���
	if (i_empty_waiters) {
		//ReleaseSemaphore(m_semaphore, 1, NULL);//�ͷ��ź���

		pthread_cond_signal(&cond);//ʹ�ÿ�ƽ̨�߳̿�
	}

	//����
	unlock();

	return CARICCP_SUCCESS_STATE_CODE;
}

/*�������͵�����֡���õ�������,����������,���������.
 *һ������������Ժ�,��֪ͨ�ȴ����߳�ȡ�����ݷ��ͳ�ȥ
 */
int CMsgProcess::popCmdFromQueue(inner_CmdReq_Frame *&reqFrame)
{
	lock();//����
	if (isEmptyOfCmdQueue())//��������Ϊ��
	{
		i_empty_waiters++;  //�ձ�ʶ

		//result = WaitForSingleObject(m_semaphore, INFINITE);//��ǰ�̵߳ȴ�,�����ź���
		pthread_cond_wait(&cond, &mutex);//ʹ�ÿ�ƽ̨�߳̿�

		i_empty_waiters--;  //��ʶ������1

		if (isEmptyOfCmdQueue()) {
			unlock();//�߳̽���unlock
			return CARICCP_ERROR_STATE_CODE;
		}
	} 


	reqFrame = m_listRequestFrame.front();//ȡ����������������Ԫ��.��ע :��Ԫ����ʲôʱ������???
	m_listRequestFrame.pop_front();		  //ȡ�������Ҫ��ʱɾ��

	//�˱�ʶ��Ч��ʾ:������ǰ"Ϊ"��,��ΪĿǰ�ո�ȡ����һ������.��ʱpush()��Ӧ���߳�һ����������,��Ҫ����
	if (i_full_waiters) {
		//ReleaseSemaphore(m_semaphore, 1, NULL);//�ͷ��ź���,�����������ڵȴ����߳�(push())

		pthread_cond_signal(&cond);//ʹ�ÿ�ƽ̨�߳̿�
	}

	unlock();//unlock:����

	return CARICCP_SUCCESS_STATE_CODE;
}

/*��������е�Ԫ��
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
