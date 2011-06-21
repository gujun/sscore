//#include "modules\CBaseModule.h"
#include "CBaseModule.h"
#include "CMsgProcess.h"
#include "cari_net_event_process.h"
#include "cari_net_socket.h"

//////////////////////////////////////////////////////////////////////////
/*��������(����)���̵߳����庯��
*/
void* thread_reboot(void *attr)
{
	string strRebootCmd = "";
	bool bServerOrHost = true;
	char *chReboot = "0";//Ĭ��Ϊ�����������
	if (NULL != attr){
		chReboot = (char *)attr;
	}

	//��������������
	if (isEqualStr("1",chReboot)){
#if defined(_WIN32) || defined(WIN32)
		strRebootCmd = "shutdown  /r";
#else
		strRebootCmd = "reboot";//��ǰlinuxϵͳ��֧��shutdown����(shutdown -rt 10)
#endif
	}
	else{//�����������
#if defined(_WIN32) || defined(WIN32)
		//strRebootCmd = "shutdown  /r";
#else
		strRebootCmd = "/sbin/service softswitch restart";//�Լ����õ����������������
#endif
	}

	//����ر����е�socket����
	shutdownSocketAndGarbage();

//����������̻������Ĳ���
#if defined(_WIN32) || defined(WIN32)
	cari_common_excuteSysCmd(strRebootCmd.c_str());
#else
	CARI_SLEEP(3*1000);//�ȴ�һ��
	for (int i=0;i<3;i++){
		cari_common_excuteSysCmd("sync");//�ӿ����ݸ���,���ڴ��е�����д�뵽������,�������ʹ��
	}
	cari_common_excuteSysCmd(strRebootCmd.c_str());
#endif

	
	return NULL;
}


//////////////////////////////////////////////////////////////////////////
CBaseModule::CBaseModule()
{
	m_localIP = NULL;
}


CBaseModule::~CBaseModule()
{
	//�ͷ��ڴ�
	switch_safe_free(m_localIP);
}

/*����ģ����Ҫ�������ڲ�����
*/
int CBaseModule::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	if (NULL == inner_reqFrame) {
		//֡Ϊ��
		return CARICCP_ERROR_STATE_CODE;
	}

	//����֡��"Դģ��"��������Ӧ֡��"Ŀ��ģ��"��
	//����֡��"Ŀ��ģ��"��������Ӧ֡��"Դģ��"��,�����
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;

	int iRes = CARICCP_SUCCESS_STATE_CODE;

	//�ж�������
	int iCmdCode = inner_reqFrame->body.iCmdCode;
	switch (iCmdCode) {
	case CARICCP_REBOOT://����
		reboot(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LOGIN://�û���¼
		iRes = login(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_CLIENTID://��ѯclient id��
		queryClientID(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_SHAKEHAND://����
		shakehand(inner_reqFrame, inner_RespFrame);
		break;

	default:
		break;
	};

	//����ִ�в��ɹ�
	if (CARICCP_SUCCESS_STATE_CODE != iRes) {
		inner_RespFrame->header.bNotify = false;
	} else {
		inner_RespFrame->header.bNotify = inner_reqFrame->body.bNotify;
	}

	//��ӡ��־
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "proc cmd %s end.\n", inner_reqFrame->body.strCmdName);
	return CARICCP_SUCCESS_STATE_CODE;
}

int CBaseModule::sendRespMsg(common_ResultResponse_Frame *&respFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}

int CBaseModule::cmdPro(const inner_CmdReq_Frame *&reqFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}


/************************************************************************/
/*��������ÿ������Ĵ�����                                          */
/************************************************************************/

/**
/*��¼����
*/
int CBaseModule::login(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)//�������,��Ӧ֡
{
	int iRes = CARICCP_ERROR_STATE_CODE;

	//������������漰��������ģ��Ĵ���,���??????
	//�����ڲ�����---------------------------------

	string strParamName, strValue,strDec,strChinaName;
	char* chDec = NULL;

	//��ȡ�û������û�������
	string strUserName,strPwd;
	string strTmp;
	strParamName = "loginName";//�û���
	strChinaName = getValueOfDefinedVar(strParamName);
	strUserName = getValue(strParamName, inner_reqFrame);
	if (0 == strUserName.size()) {
		//�û�����Ϊ��,���ش�����Ϣ
		strDec = getValueOfDefinedVar("PARAM_NULL_ERROR");
		chDec = switch_mprintf(strDec.c_str(), strChinaName.c_str());
		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;

		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

		switch_safe_free(chDec);
		goto end;
	}

	strParamName = "loginPwd";//�û�����
	strChinaName = getValueOfDefinedVar(strParamName);
	strPwd = getValue(strParamName, inner_reqFrame);
	if (0 == strPwd.length()) {
		//�û�����,���ش�����Ϣ
		strDec = getValueOfDefinedVar("PARAM_NULL_ERROR");
		chDec = switch_mprintf(strDec.c_str(), strChinaName.c_str());//switch_mprintf(str_NULL_Error.c_str(),strName)����Ϊʲô????????

		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;
		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

		switch_safe_free(chDec);
		goto end;
	}

	//mod by xxl 2010-5-19 :��ǿ�Բ����û��Ĵ�������
	//�����û��Ƿ����
	if (!isLoginSuc(strUserName,strPwd)){
		strDec = getValueOfDefinedVar("LOGIN_ERROR");
		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;
		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));
		goto end;
	}

	iRes = CARICCP_SUCCESS_STATE_CODE;
	//mod by xxl 2010-5-19 end

	////test--------------------------
	//strDec = "user";
	//strDec.append(strUserName);
	//strDec.append("login succeed.");

	//myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), strlen(strDec.c_str()));

	//inner_RespFrame->iResultNum = 1;

	////����Ωһ�Ŀͻ��˺Ÿ��û�
	//string strVal = "";
	//strVal = "ClientID = 2009";
	//inner_RespFrame->m_vectTableValue.push_back(strVal);

end:
	return iRes;
}

/*��ѯ��ö�Ӧ��client id��,����Ƕ���ͻ���ͬʱ����,���¶�����������ɾ,�������û�м���,�������쳣
*/
int CBaseModule::queryClientID(inner_CmdReq_Frame*& inner_reqFrame, 
							   inner_ResultResponse_Frame*& inner_RespFrame)
{
	//�ڽ����ͻ����̵߳�ʱ��,�Ѿ�������˶�Ӧ��id��,��ʱֱ��ȡ������
	int iRes = CARICCP_SUCCESS_STATE_CODE;
	u_int iSocketID = inner_reqFrame->socketClient;
    u_short iClientID = CARICCP_MAX_SHORT;

	//��У��һ�´�id�Ƿ���Ч,�����������֡
	u_short id = inner_reqFrame->header.sClientID;
	if (CARICCP_MAX_SHORT == id){//���ε�¼����ʼ��ѯ
		iClientID = CModuleManager::getInstance()->assignClientID();//���·���id��
	}
	else{//�鿴���û����Ƿ��ܼ���ʹ��
		bool bUsed = true;
		bUsed = CModuleManager::getInstance()->isUsedID(id);
		if (bUsed){//�Ѿ��������Ŀͻ���ʹ��
			iClientID = CModuleManager::getInstance()->assignClientID();//���·���id��
		}
		else{
			iClientID = id;//����ʹ����ǰ�����id��
			//ͬʱ����������Ҫ������˺���
			CModuleManager::getInstance()->removeClientID(iClientID);
		}
	}

	//��ʼ����˿ͻ��˶�Ӧ��id��,����,�ص�¼��ʱ��,����У����ǰʹ�õ�id�Ƿ���Ч
	socket3element	element;
	CModuleManager::getInstance()->getSocket(iSocketID,element);

	element.sClientID = iClientID;//����˿ͻ��˺�

	//��ɾ����Ԫ��
	CModuleManager::getInstance()->eraseSocket(iSocketID);

	//Ȼ�����������Ӵ�Ԫ��
	CModuleManager::getInstance()->saveSocket(element);

	//�ؼ���Ϣ������
	inner_RespFrame->header.sClientID = iClientID;
	return iRes;
}

/*��������������
*/
int CBaseModule::reboot(inner_CmdReq_Frame*& inner_reqFrame, 
						inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iRes = CARICCP_SUCCESS_STATE_CODE;
	string strDec;

	int notifyCode = CARICCP_NOTIFY_REBOOT;
	string strNotifyCode = "";
	string strClientID = intToString(inner_reqFrame->header.sClientID);
	string strCode = intToString(inner_reqFrame->body.iCmdCode);
	string strCmdName = inner_reqFrame->body.strCmdName;
	char *pChNotifyNode = NULL,*chDec = NULL;
	char *chReboot = "0";

	//�漰��֪ͨ���͵Ĵ���:��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent = NULL;


	string strModel,strParamName,strChinaName;
	string strTmp;

	//model
	strParamName = "model";//model
	strChinaName = getValueOfDefinedVar(strParamName);
	strModel = getValue(strParamName, inner_reqFrame);
	if (0 == strModel.size()) {
		//ģʽΪ��,���ش�����Ϣ
		strDec = getValueOfDefinedVar("PARAM_NULL_ERROR");
		chDec = switch_mprintf(strDec.c_str(), strChinaName.c_str());
		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;

		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

		switch_safe_free(chDec);
		goto end_flag;
	}
	if (isEqualStr("host",strModel)){
		chReboot = "1";
		//������Ϣ
		strDec = getValueOfDefinedVar("HOST_REBOOT");
	}
	else{
		chReboot = "0";
		strDec = getValueOfDefinedVar("SERVER_REBOOT");
	}

	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//��ʱ���ǰ��ճɹ�����
		goto end_flag;
	}

	//֪ͨ��
	notifyCode = CARICCP_NOTIFY_REBOOT;

	//�����ͽ����
	pChNotifyNode = switch_mprintf("%d", notifyCode);
	strNotifyCode = pChNotifyNode;
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   					//֪ͨ��ʶ
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, strNotifyCode.c_str()); //������=֪ͨ��
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CLIENT_ID,          strClientID.c_str()); //�ͻ��˺�
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_CMDCODE,    strCode.c_str());     //
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_CMDNAME,    strCmdName.c_str());
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RESULTDESC, strDec.c_str());	   //������Ϣ

	//����֪ͨ----------------------
	cari_net_event_notify(notifyInfoEvent);

	//�ͷ��ڴ�
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pChNotifyNode);

	//Ϊ�˷��ظ��ͻ�������ʾ������ɫ,�˴��ķ����벻Ϊ0,ΪCARICCP_NOTIFY_REBOOT
	inner_RespFrame->header.iResultCode = CARICCP_NOTIFY_REBOOT;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), strlen(strDec.c_str()));//���ص�������Ϣ


end_flag:

	//Ȼ������������һ���߳�����������,��Ϊ������صĽ����û�й㲥�����е�client,�������Ϳ�ʼ������
	//������������ʱ����,��֤����ܷ��ؼ���.
	pthread_t pth;
	int iRet= pthread_create(&pth,NULL,thread_reboot,(void *)chReboot);
	if(CARICCP_SUCCESS_STATE_CODE != iRet){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Creat reboot thred failed!");
	} 

//#if defined(_WIN32) || defined(WIN32)
//	cari_common_excuteSysCmd("shutdown  /r");
//#else
//	for (int i=0;i<3;i++){
//		cari_common_excuteSysCmd("sync");//�ӿ����ݸ���,���ڴ��е�����д�뵽������,�������ʹ��
//	}
//	cari_common_excuteSysCmd("reboot");//��ǰlinuxϵͳ��֧��shutdown����(shutdown -rt 10)
//#endif

	return iRes;
}

/*����
*/
int CBaseModule::shakehand(inner_CmdReq_Frame*& inner_reqFrame, 
						   inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iRes = CARICCP_SUCCESS_STATE_CODE;
	inner_RespFrame->header.iResultCode = CARICCP_SUCCESS_STATE_CODE;

	return iRes;
}

/************************************************************************/
/* �ṩͨ�õĴ�����                                                   */
/************************************************************************/

/*��ö�Ӧ��domain��,���ڴ�root�ṹ�в���������ֶ�
*<list name="domains" default="deny">
*<node type="allow" domain="192.168.17.123"/>
*</list>
*
*����ʹ��internal profile��������
*<param name="force-register-domain" value="192.168.17.123"/>
*/
char * CBaseModule::getDomainIP()
{
	if (NULL != m_localIP){
		return m_localIP;
	}

	//	char *profile_name = CARICCP_SIP_PROFILES_INTERNAL; //���ݴ�internal.xml�ļ��Ľṹ����
	//	char *domain_name = NULL;//"192.168.17.123";
	//	switch_xml_t x_root, x_section, x_config, x_networklists, x_list, x_node;
	//	x_root = switch_xml_root();
	//	if (!x_root) {
	//		goto mid_flag;
	//	}
	//
	//	//����1 :ͨ�����ʿ����б�ACL���,�������ַ������ɿ�
	//	//<section name="configuration" description="Various Configuration">
	//	x_section = switch_xml_find_child(x_root, "section", "name", "configuration");
	//	if (!x_section) {
	//		goto mid_flag;
	//	}
	//	//<configuration name="acl.conf" description="Network Lists">
	//	x_config = switch_xml_find_child(x_section, "configuration", "name", "acl.conf");
	//	if (!x_config) {
	//		goto mid_flag;
	//	}
	//	// <network-lists>
	//	//  <list name="domains" default="deny">
	//	//	  <node type="allow" domain="192.168.17.123"/>
	//	//	</list>
	//	x_networklists = switch_xml_child(x_config, "network-lists");
	//	if (!x_networklists) {
	//		goto mid_flag;
	//	}
	//	x_list = switch_xml_find_child(x_networklists, "list", "name", "domains");
	//	if (!x_list) {
	//		goto mid_flag;
	//	}
	//	x_node = switch_xml_find_child(x_list, "node", "type", "allow");
	//	if (!x_node) {
	//		goto mid_flag;
	//	}
	//	//<node type="allow" domain="192.168.17.123"/>
	//	domain_name = (char*) switch_xml_attr(x_node, "domain");
	//
	//mid_flag:
	//	if (!switch_strlen_zero(domain_name)) {
	//		goto end_flag;
	//	}

	//����2 :ͨ����ñ�����IP��ַ,�����˫������???
	char guess_ip4[256] = "";
	switch_find_local_ip(guess_ip4, sizeof(guess_ip4),NULL,AF_INET);//���������Ϊ�汾�Ĳ�ͬ����ͬ
	//domain_name = guess_ip4;//��ʱ����,ֱ�Ӹ�ֵ��Ч
	m_localIP = switch_mprintf("%s", guess_ip4);

	//end_flag:
	return /*domain_name*/m_localIP;
}

/*����û��ĵ�ǰdefaultĿ¼�ṹ
*/
char * CBaseModule::getMobUserDefaultXMLPath()
{
	//conf\directory\default,���е��û��������ڴ�Ŀ¼�´��������xml�ļ�
	char *path = switch_mprintf("%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DEFAULT_DIRECTORY, 
		SWITCH_PATH_SEPARATOR);

	return path;
}

/*����û���default/Ŀ¼�µ�Ĭ������xml�ļ�
*/
char * CBaseModule::getMobUserDefaultXMLFile(const char *mobusername)
{
	char *file = getMobUserDefaultXMLPath();
	char *userFile = switch_mprintf("%s%s%s", 
		file, 
		mobusername, 
		CARICCP_XML_FILE_TYPE);

	switch_safe_free(file);
	return userFile;//�δ������ͷ�
}

/*����û�������xml�ļ�
*/
char * CBaseModule::getMobUserGroupXMLFile(const char *groupname, const char *mobusername)
{
	if (NULL == groupname || NULL == mobusername || switch_strlen_zero(groupname) || switch_strlen_zero(mobusername)) {
		return NULL;
	}

	//conf\directory\groups\�û�����
	char *path = switch_mprintf("%s%s%s%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIR_DEFINE_GROUPS,  	//��ĸ�Ŀ¼groups  		  
		SWITCH_PATH_SEPARATOR, 
		groupname, 
		SWITCH_PATH_SEPARATOR, 
		mobusername, 
		CARICCP_XML_FILE_TYPE);

	return path;
}

/*������Ŀ¼
*/
char * CBaseModule::getMobGroupDir(const char *groupname)
{
	//conf\directory\groups\����
	char *path = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIR_DEFINE_GROUPS,  	//��ĸ�Ŀ¼groups  		  
		SWITCH_PATH_SEPARATOR, 
		groupname);

	return path;//�δ��ͷ�
}

/*���conf\directory\default.xml�ļ�
*/
char * CBaseModule::getDirectoryDefaultXMLFile()
{
	//conf\directory\default
	char *defaultXmlFile = switch_mprintf("%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY,
		SWITCH_PATH_SEPARATOR, 
		DEFAULT_DIRECTORY, 
		CARICCP_XML_FILE_TYPE);

	return defaultXmlFile;
}

/*���dia group��xml�ļ�
*/
char * CBaseModule::getDirectoryDialGroupXMLFile()
{
	//conf\dialplan\default\02_group_dial.xml
	char *defaultXmlFile = switch_mprintf("%s%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		DIAPLAN_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DEFAULT_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIAPLAN_GROUP_XMLFILE,  	  //�̶����ļ��� 02_group_dial
		CARICCP_XML_FILE_TYPE);

	return defaultXmlFile;
}

/*���conf\sip_profilesĿ¼
*/
char * CBaseModule::getSipProfileDir()
{
	char *path = switch_mprintf("%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		SIP_PROFILE_DIRECTORY, 
		SWITCH_PATH_SEPARATOR);

	return path;
}

/*���conf\autoload_configs\dispat.conf.xml
*/
char* CBaseModule::getDispatFile()
{
	char *file = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR,
		"autoload_configs",
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_DISPAT_XML_FILE);

	return file;
}


/*login�Ƿ�ɹ�
*/
bool CBaseModule::isLoginSuc(string username,
							 string userpwd)
{
	//��ʱ����:Ŀǰ�����ļ���ȡ
	bool bRes = false;
	switch_xml_t x_NEs,x_ne;
	const char *errmsg[1]={ 0 };

	const char *name=NULL;
	const char *pwd=NULL;
	char *nefile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);

	if (!cari_common_isExistedFile(nefile)){
		goto end;
	}

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//��������
	x_ne = switch_xml_child(x_NEs, "opuser");
	for (; x_ne; x_ne = x_ne->next) {
		name = switch_xml_attr(x_ne, "name");
		pwd = switch_xml_attr(x_ne, "pwd");
		if (isEqualStr(username.c_str(),name)
			&& isEqualStr(userpwd.c_str(),pwd)){

				bRes = true;
				break;
		}	
	}

end:
	switch_safe_free(nefile);
	return bRes;
}
