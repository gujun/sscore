#include "CModule_NEManager.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"

#define MIN_NEID                    1
#define MAX_NEID                    1000

//��ͬ���͵��豸����
#define  CCP                        "CCP"     //cari core platform
#define  SW                         "SW"      //switch
#define  AP                         "AP"      //access point
#define  GATEWAY                    "GATEWAY" //gateway


//��Ԫ�򵥵�xml����
#define NE_XML_SIMPLE_CONTEXT 		"<ne id=\"%s\" type = \"%s\" ip = \"%s\" desc = \"%s\"/>"
#define OPUSER_XML_SIMPLE_CONTEXT 	"<opuser name=\"%s\" pwd = \"%s\" priority = \"%s\"/>"
#define SUPER_USER                  "admin"


CModule_NEManager::CModule_NEManager()
	: CBaseModule()
{
	const char *errmsg[1]={ 0 };
	bool bRes = false;

	//ne��xml�ļ�
	char *nefile = switch_mprintf("%s%s%s", 
								  SWITCH_GLOBAL_dirs.conf_dir, 
								  SWITCH_PATH_SEPARATOR, 
								  CARI_CCP_NE_XML_FILE
								  );
	//char *neContext = "<include></include>";
	//������ļ�������,�����´���,����ֻ��Ҫ����һ�μ���
	if (!cari_common_isExistedFile(nefile)){
		char *neContext = switch_mprintf("%s%s", 
			"<include>", 
			"</include>"
			);
		bRes = cari_common_creatXMLFile(nefile,neContext,errmsg);
		switch_safe_free(neContext);
	}
	if (!bRes){
		//��ӡ��־???
	}

	//�����û���xml�ļ�
	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);
	//������ļ�������,�����´���,����ֻ��Ҫ����һ�μ���
	if (!cari_common_isExistedFile(opuserfile)){
		//������ʼ��¼��Ĭ�ϲ����û�
		char *opuserContext = switch_mprintf("%s%s%s", 
			"<include>", 
			"<opuser name=\"admin\" pwd = \"123456\" priority = \"0\"/>", //�û����������ȼ���0��ʼ,���
			"</include>"
			);
		bRes = cari_common_creatXMLFile(opuserfile,opuserContext,errmsg);
		switch_safe_free(opuserContext);
	}
	if (!bRes){
		//��ӡ��־???
	}

	//�ͷ��ڴ�
	switch_safe_free(nefile);
	switch_safe_free(opuserfile);
}

CModule_NEManager::~CModule_NEManager()
{
}

/*���ݽ��յ���֡,������������,�ַ�������Ĵ�����
*/
int CModule_NEManager::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	if (NULL == inner_reqFrame) {
		//֡Ϊ��
		return CARICCP_ERROR_STATE_CODE;
	}

	//����֡��"Դģ��"��������Ӧ֡��"Ŀ��ģ��"��
	//����֡��"Ŀ��ģ��"��������Ӧ֡��"Դģ��"��,�����
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;

	int				iFunRes		= CARICCP_SUCCESS_STATE_CODE;
	bool	bRes		= true; 

	//�ж����������ж�����(��������Ωһ��),Ҳ���Ը�����������������
	int				iCmdCode	= inner_reqFrame->body.iCmdCode;
	switch (iCmdCode) {
	case CARICCP_ADD_EQUIP_NE:
		iFunRes = addEquipNE(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_EQUIP_NE:
		iFunRes = delEquipNE(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_MOD_EQUIP_NE:
		iFunRes = modEquipNE(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_EQUIP_NE:
		iFunRes = lstEquipNE(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_ADD_OP_USER:
		iFunRes = addOPUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_OP_USER:
		iFunRes = delOPUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_MOD_OP_USER:
		iFunRes = modOPUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_OP_USER:
		iFunRes = lstOPUser(inner_reqFrame, inner_RespFrame);
		break;

	default:
		break;
	};


	//����ִ�в��ɹ�
	if (CARICCP_SUCCESS_STATE_CODE != iFunRes) {
		inner_RespFrame->header.bNotify = false;
	} else {
		inner_RespFrame->header.bNotify = inner_reqFrame->body.bNotify;
	}

	/*--------------------------------------------------------------------------------------------*/
	//������Ľ�����͸���Ӧ�Ŀͻ���
	//������Ľ�����ں�����

	//��ӡ��־
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "proc cmd %s end.\n", inner_reqFrame->body.strCmdName);
	return iFunRes;
}

/*������Ϣ����Ӧ�Ŀͻ���
*/
int CModule_NEManager::sendRespMsg(common_ResultResponse_Frame *&respFrame)
{
	int	iFunRes	= CARICCP_SUCCESS_STATE_CODE;
	return iFunRes;
}

int CModule_NEManager::cmdPro(const inner_CmdReq_Frame *&reqFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}

/*�����豸��Ԫ,��Ҫ֪ͨ����ע���client
*/
int CModule_NEManager::addEquipNE(inner_CmdReq_Frame *&reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strNotifyCode;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc          = NULL,*pNotifyNode = NULL;
	bool	        bRes			= true;

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent      = NULL;
	int notifyCode                       = CARICCP_NOTIFY_ADD_EQUIP_NE;
	char *nefile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_NE_XML_FILE
		);
	switch_xml_t x_NEs=NULL,x_newNE=NULL;
	char *newNEContext=NULL,*nesContext=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strID,strType,strIP,strDesc;
	string			strTmp;
	int				neid=0;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"neid"
	strParamName = "neid";//neid
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//neid�ű�������Ч����
	if (!isNumber(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//��Χ����
	neid = stringToInt(strValue.c_str());
	if (MIN_NEID>neid 
		|| MAX_NEID < neid){
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(),MIN_NEID,MAX_NEID);
		goto end_flag;
	}
	strID = strValue;

	//"NEType"
	strParamName = "netype";//��Ԫ����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue  = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//NEType�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if(!isEqualStr(strValue,AP)
		&& !isEqualStr(strValue,GATEWAY)
		&& !isEqualStr(strValue,"else")){
		strReturnDesc = getValueOfDefinedVar("NE_TYPE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strType = strValue;

	//ip��ַ
	strParamName = "ip";//ip
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//ip�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//��ַ������Ч 
	if (!isValidIPV4(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_IP_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strIP = strValue;

	//desc
	strParamName = "desc";//desc
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	strDesc = strValue;

	//////////////////////////////////////////////////////////////////////////
	//�жϴ���Ԫ�Ƿ��Ѿ�����???
	if (isExistedNE(neid)){
		strReturnDesc = getValueOfDefinedVar("NE_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//��ʼ������Ԫ////////////////////////////////////////////////////////////
	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), nefile);
		goto end_flag;
	}

	//�ȹ���xml���ڴ�ṹ
	newNEContext = NE_XML_SIMPLE_CONTEXT;
	newNEContext = switch_mprintf(newNEContext,
		strID.c_str(), 
		strType.c_str(),
		strIP.c_str(),
		strDesc.c_str());
	x_newNE = switch_xml_parse_str(newNEContext, strlen(newNEContext));
	if (!x_newNE) {
		strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//����������Ԫ�Ľṹ��ӽ���
	if (!switch_xml_insert(x_newNE, x_NEs, INSERT_INTOXML_POS)) {
		strReturnDesc = getValueOfDefinedVar("INNER_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//��ȫ����Ԫ���ڴ�ṹת�����ִ�
	nesContext = switch_xml_toxml(x_NEs, SWITCH_FALSE);
	if (!nesContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//���´�����Ԫ���ļ�
	bRes = cari_common_creatXMLFile(nefile,nesContext,err);
	if (!bRes){
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("ADD_NE_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());

	/************************************************************************/
	/*                             ����֪ͨ                                 */
	/************************************************************************/
	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//��ʱ���ǰ��ճɹ�����
		goto end_flag;
	}
	//֪ͨ��
	notifyCode = CARICCP_NOTIFY_ADD_EQUIP_NE;
	//�����ͽ����
	pNotifyNode = switch_mprintf("%d", notifyCode);
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY,     "1");   		   //֪ͨ��ʶ
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyNode);     //������=֪ͨ��
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "neid",	         strID.c_str());   //֪ͨ�Ľ������
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "netype",           strType.c_str());
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "ip",	             strIP.c_str());
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "desc",	         strDesc.c_str());

	//����֪ͨ----------------------
	cari_net_event_notify(notifyInfoEvent);

	//�ͷ��ڴ�
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pNotifyNode);
	

/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//�ͷ�xml�ṹ
	switch_xml_free(x_NEs);

	switch_safe_free(nefile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*ɾ����Ԫ,��Ҫ֪ͨ����ע���client
*/
int CModule_NEManager::delEquipNE(inner_CmdReq_Frame *&reqFrame, 
								  inner_ResultResponse_Frame *&inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strNotifyCode;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc          = NULL,*pNotifyCode = NULL;
	bool	        bRes			= true;

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent = NULL;
	int            notifyCode       = CARICCP_NOTIFY_DEL_EQUIP_NE;

	char           *nefile          = switch_mprintf("%s%s%s", 
													SWITCH_GLOBAL_dirs.conf_dir, 
													SWITCH_PATH_SEPARATOR, 
													CARI_CCP_NE_XML_FILE
									                );
	switch_xml_t   x_NEs,x_delNE;
	char           *newNEContext    =NULL,*nesContext=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strID;
	string			strTmp;
	int             neid=0;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"neid"
	strParamName = "neid";//neid
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//id�ű�������Ч����
	if (!isNumber(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	neid = stringToInt(strValue.c_str());
	strID = strValue;

	//��Ԫ�Ƿ����
	if (!isExistedNE(neid)){
		strReturnDesc = getValueOfDefinedVar("NE_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//ɾ��������Ԫ�������Ϣ
	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), nefile);
		goto end_flag;
	}

	x_delNE = switch_xml_find_child(x_NEs, "ne", "id", strID.c_str());
	if (!x_delNE) {
		strReturnDesc = getValueOfDefinedVar("NE_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	/************************************************************************/
	//��̬�����ڴ�Ľṹ
	switch_xml_remove(x_delNE);
	/************************************************************************/

	//����д�ļ�
	nesContext = switch_xml_toxml(x_NEs, SWITCH_FALSE);
	if (!nesContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	bRes = cari_common_creatXMLFile(nefile, nesContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
		//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("DEL_NE_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());

	/************************************************************************/
	/*							  ����֪ͨ                                  */
	/************************************************************************/
	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//��ʱ���ǰ��ճɹ�����
		goto end_flag;
	}
	//֪ͨ��
	notifyCode = CARICCP_NOTIFY_DEL_EQUIP_NE;
	//�����ͽ����
	pNotifyCode = switch_mprintf("%d", notifyCode);
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY,     "1");   		 //֪ͨ��ʶ
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode);   //������=֪ͨ��
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "neid",	         strID.c_str()); //֪ͨ�Ľ������

	//����֪ͨ----------------------
	cari_net_event_notify(notifyInfoEvent);

	//�ͷ��ڴ�
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pNotifyCode);


/*-----*/
end_flag :
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(nefile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*�޸���Ԫ����,��Ҫ֪ͨ����ע���client
*/
int CModule_NEManager::modEquipNE(inner_CmdReq_Frame *&reqFrame, 
								  inner_ResultResponse_Frame *&inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strNotifyCode;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc          = NULL,*pNotifyCode = NULL;
	bool	        bRes		    = true,bMod=false;

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent = NULL;
	int            notifyCode       = CARICCP_NOTIFY_MOD_EQUIP_NE;

	char           *nefile          = switch_mprintf("%s%s%s",
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_NE_XML_FILE
		);

	switch_xml_t   x_NEs            = NULL,x_modNE= NULL;
	char           *newNEContext    = NULL,*nesContext=NULL;

	//�漰������
	string			strParamName,strValue,strVar,strChinaName;
	string			strID,strType,strIP,strDesc;
	string			strTmp;
	int				neid = 0;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"neid"
	strParamName = "neid";//neid
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//id�ű�������Ч����
	if (!isNumber(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//��Χ����
	neid = stringToInt(strValue.c_str());
	if (MIN_NEID>neid 
		|| MAX_NEID < neid){
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(),MIN_NEID,MAX_NEID);
		goto end_flag;
	}
	strID = strValue;

	//"NEType"
	strParamName = "netype";//��Ԫ����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//NEType�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if(!isEqualStr(strValue,AP)
		&& !isEqualStr(strValue,GATEWAY)
		&& !isEqualStr(strValue,"else")){
		strReturnDesc = getValueOfDefinedVar("NE_TYPE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strType = strValue;

	//ip��ַ
	strParamName = "ip";//ip
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//ip�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//��ַ������Ч 
	if (!isValidIPV4(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_IP_ERROR");
		strReturnDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strIP = strValue;

	//desc
	strParamName = "desc";//desc
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	strDesc = strValue;

	//////////////////////////////////////////////////////////////////////////
	//�жϴ���Ԫ�Ƿ��Ѿ�����???
	if (!isExistedNE(neid)){
		strReturnDesc = getValueOfDefinedVar("NE_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//��ʼ�޸���Ԫ������
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), nefile);
		goto end_flag;
	}

	x_modNE = switch_xml_find_child(x_NEs, "ne", "id", strID.c_str());
	if (!x_modNE) {
		strReturnDesc = getValueOfDefinedVar("NE_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//������-ֵ��,��: <ne id="1" type = "AP" ip = "127.0.0.1" desc = "ap1"/>
	//����"��Ԫ����"��"��Ԫip"��"������Ϣ"�ж��Ƿ����ǰ����ͬ,��:id�Ų��ܽ����޸�
	strVar	= switch_xml_attr(x_modNE, "type");
	if (!isEqualStr(strVar,strType)){
		switch_xml_set_attr(x_modNE, "type", strType.c_str());
		bMod = true;
	}
	strVar	= switch_xml_attr(x_modNE, "ip");
	if (!isEqualStr(strVar,strIP)){
		switch_xml_set_attr(x_modNE, "ip", strIP.c_str());
		bMod = true;
	}
	strVar	= switch_xml_attr(x_modNE, "desc");
	if (!isEqualStr(strVar,strDesc)){
		switch_xml_set_attr(x_modNE, "desc", strDesc.c_str());
		bMod = true;
	}


	//������������Ľ������޸�,����Ҫ�޸������ļ�,����޸ĵ�ֵ����ǰ�����ֵ��ͬ,�˴�ֱ�ӷ��سɹ���Ϣ����
	//Ҳ����Ҫ�ٽ��й㲥֪ͨ
	if (bMod){
		//����д�ļ�
		nesContext = switch_xml_toxml(x_NEs, SWITCH_FALSE);
		if (!nesContext) {
			strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
		bRes = cari_common_creatXMLFile(nefile, nesContext, err);
		if (!bRes) {
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
			//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

			goto end_flag;
		}
	}
	
	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("MOD_NE_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(),strID.c_str());

	/************************************************************************/
	/*							����֪ͨ                                    */
	/************************************************************************/
	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
	if (bMod){
		//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
		if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
			//��ʱ���ǰ��ճɹ�����
			goto end_flag;
		}
		//֪ͨ��
		notifyCode = CARICCP_NOTIFY_MOD_EQUIP_NE;
		//�����ͽ����
		pNotifyCode = switch_mprintf("%d", notifyCode);
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY,     "1");   		  //֪ͨ��ʶ
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode);    //������=֪ͨ��
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "neid",             strID.c_str());  //֪ͨ�Ľ������
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "netype",           strType.c_str());
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "ip",	             strIP.c_str());
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "desc",	         strDesc.c_str());

		//����֪ͨ----------------------
		cari_net_event_notify(notifyInfoEvent);

		//�ͷ��ڴ�
		switch_event_destroy(&notifyInfoEvent);
		switch_safe_free(pNotifyCode);
	}

/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_NEs);

	switch_safe_free(nefile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*��ѯ�豸NE����Ϣ,��Ҫ��ѯ��Ӧ��id��,����,�Ѿ�ip��ַ
*/
int CModule_NEManager::lstEquipNE(inner_CmdReq_Frame *&reqFrame, 
								  inner_ResultResponse_Frame *&inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strSeq,strRecord;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char *pDesc = NULL,*pNotifyCode = NULL;
	bool	bRes			= true;
	string tableHeader[5] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("NEID"),
		getValueOfDefinedVar("NETYPE"),
		getValueOfDefinedVar("IP"),
		getValueOfDefinedVar("DESC")
	};
	int iRecordCout = 0;//��¼����

	char *nefile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_NE_XML_FILE
		);

	switch_xml_t x_NEs=NULL,x_ne=NULL;
	char *newNEContext=NULL,*nesContext=NULL,*strTableName=NULL;

	//����Ԫ��xml����ת����xml�Ľṹ(ע��:�����ַ�����)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�����������е���Ԫ
	x_ne = switch_xml_child(x_NEs, "ne");
	for (; x_ne; x_ne = x_ne->next) {
		const char *neID   = switch_xml_attr(x_ne, "id");
		const char *netype = switch_xml_attr(x_ne, "type");
		const char *neip   = switch_xml_attr(x_ne, "ip");
		const char *neDesc = switch_xml_attr(x_ne, "desc");
		if (neID && netype && neip){
			char *singleRecord = switch_mprintf("%s%s%s%s%s%s%s%s", 
				neID, 
				CARICCP_SPECIAL_SPLIT_FLAG,
				netype,
				CARICCP_SPECIAL_SPLIT_FLAG,
				neip,
				CARICCP_SPECIAL_SPLIT_FLAG,
				neDesc,
				CARICCP_ENTER_KEY
				);

			//mod by xxl 2010-5-27 :������ŵ���ʾ
			iRecordCout++;//��ŵ���
			strRecord = "";
			strSeq = intToString(iRecordCout);
			strRecord.append(strSeq);
			strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
			strRecord.append(singleRecord);

			//��ż�¼
			inner_RespFrame->m_vectTableValue.push_back(/*singleRecord*/strRecord);
			//mod by xxl 2010-5-27 end

			switch_safe_free(singleRecord);
		}
	}

	//��ǰ��¼Ϊ��
	if (0 ==inner_RespFrame->m_vectTableValue.size()){
		strReturnDesc = getValueOfDefinedVar("NO_NE_RECORD");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("NE_RECORD");

	//���ñ�ͷ�ֶ�
	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s",
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[1].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[2].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[3].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[4].c_str());

	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_NEs);

	switch_safe_free(pDesc);
	switch_safe_free(nefile);
	return iFunRes;
}

/*���Ӳ����û�
*/
int CModule_NEManager::addOPUser(inner_CmdReq_Frame*& reqFrame, 
								 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string	  strReturnDesc	= "",strNotifyCode;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char *pDesc                   = NULL,*pNotifyNode = NULL;
	bool	bRes			= true;

	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);
	switch_xml_t x_opusers=NULL,x_newOpuser=NULL;
	char *newOpuserContext=NULL,*opusersContext=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strUserName,strPwd;
	string			strTmp;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"opusername"
	strParamName = "opusername";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//�û�����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strUserName = strValue;

	//"opuserpwd"
	strParamName = "opuserpwd";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strPwd = strValue;

	//////////////////////////////////////////////////////////////////////////
	//�жϴ�"�����û�"�Ƿ��Ѿ�����???
	if (isExistedOpUser(strUserName)){
		strReturnDesc = getValueOfDefinedVar("OPUSER_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//��ʼ���Ӳ����û�

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_opusers = cari_common_parseXmlFromFile(opuserfile);
	if (!x_opusers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), opuserfile);
		goto end_flag;
	}

	//�ȹ���xml���ڴ�ṹ
	newOpuserContext = OPUSER_XML_SIMPLE_CONTEXT;
	newOpuserContext = switch_mprintf(newOpuserContext,strUserName.c_str(), strPwd.c_str(),"0");//Ĭ��Ϊ�����û�
	x_newOpuser = switch_xml_parse_str(newOpuserContext, strlen(newOpuserContext));
	if (!x_newOpuser) {
		strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�������Ĳ����û��Ľṹ��ӽ���
	if (!switch_xml_insert(x_newOpuser, x_opusers, INSERT_INTOXML_POS)) {
		strReturnDesc = getValueOfDefinedVar("INNER_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//��ȫ�������û����ڴ�ṹת�����ִ�
	opusersContext = switch_xml_toxml(x_opusers, SWITCH_FALSE);
	if (!opusersContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//���´�����Ԫ���ļ�
	bRes = cari_common_creatXMLFile(opuserfile,opusersContext,err);
	if (!bRes){
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("ADD_OPUSER_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());


	/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_opusers);//�ͷ�

	switch_safe_free(opuserfile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*ɾ�������û�
*/
int CModule_NEManager::delOPUser(inner_CmdReq_Frame*& reqFrame, 
								 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strNotifyCode;
	const char		*errArry[1]		= {
		0
	};
	const char		**err				= errArry;
	char *pDesc = NULL;
	bool	bRes			= true;

	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);

	switch_xml_t x_opusers=NULL,x_deOpuser=NULL;
	char *newOpuserContext=NULL,*opusersContext=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strUserName;
	string			strTmp;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"opusername"
	strParamName = "opusername";//opusername
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strUserName = strValue;

	//�жϴ�"�����û�"�Ƿ��Ѿ�����???
	if (!isExistedOpUser(strUserName)){
		strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());
		goto end_flag;
	}

	//�����ж�һ��,�����¼�û���ͬ,������
	if (isEqualStr(strUserName,reqFrame->header.strLoginUserName)){
		strReturnDesc = getValueOfDefinedVar("DEL_OPUSER_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//���Ϊ�̶��û�admin,��һ������ɾ��
	if (isEqualStr(strUserName,SUPER_USER)){
		strReturnDesc = getValueOfDefinedVar("DEL_ADMIN_OPUSER_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str(),SUPER_USER);
		goto end_flag;
	}

	//ɾ�����˲����������Ϣ
	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_opusers = cari_common_parseXmlFromFile(opuserfile);
	if (!x_opusers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), opuserfile);
		goto end_flag;
	}

	x_deOpuser = switch_xml_find_child(x_opusers, "opuser", "name", strUserName.c_str());
	if (!x_deOpuser) {
		strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());
		goto end_flag;
	}

	/************************************************************************/
	//��̬�����ڴ�Ľṹ
	switch_xml_remove(x_deOpuser);
	/************************************************************************/

	//����д�ļ�
	opusersContext = switch_xml_toxml(x_opusers, SWITCH_FALSE);
	if (!opusersContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	bRes = cari_common_creatXMLFile(opuserfile, opusersContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
		//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("DEL_OPUSER_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());


	/*-----*/
end_flag :

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_opusers);//�ͷ�

	switch_safe_free(opuserfile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*�޸Ĳ����û�
*/
int CModule_NEManager::modOPUser(inner_CmdReq_Frame*& reqFrame, 
								 inner_ResultResponse_Frame*& inner_RespFrame)
{

	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strNotifyCode;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char *pDesc = NULL;
	bool	bRes			= true,bMod=false;

	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);

	switch_xml_t x_opusers=NULL,x_modOpuser=NULL;
	char *newOpuserContext=NULL,*opusersContext=NULL;

	//�漰������
	string			strParamName,strValue,strVar,strChinaName;
	string			strOldName,strNewName,strPwd;
	string			strTmp;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"oldopusername"
	strParamName = "oldopusername";//oldopusername
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strOldName = strValue;

	//"newopusername"
	strParamName = "newopusername";//newopusername
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strNewName = strValue;

	//"opuserpwd"
	strParamName = "opuserpwd";//opuserpwd
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strPwd = strValue;


	//���oldname�Ƿ����
	if (!isExistedOpUser(strOldName)){
		strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldName.c_str());
		goto end_flag;
	}

	//oldname��newname����ͬ
	if (!isEqualStr(strOldName,strNewName)){
		if (isEqualStr(strOldName,SUPER_USER)){//admin�û�,�����޸�Ϊ�����û�
			strReturnDesc = getValueOfDefinedVar("MOD_SUPERUSER_FAILED_1");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strOldName.c_str());
			goto end_flag;
		}

		if (isEqualStr(strNewName,SUPER_USER)){//�޸ĵ����û�Ϊadmin,������
			strReturnDesc = getValueOfDefinedVar("MOD_SUPERUSER_FAILED_2");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strNewName.c_str(),strOldName.c_str());
			goto end_flag;
		}

		//���newname�Ƿ����
		if (isExistedOpUser(strNewName)){
			strReturnDesc = getValueOfDefinedVar("OPUSER_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strNewName.c_str());
			goto end_flag;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//��ʼ�޸Ĳ����û�������
	x_opusers = cari_common_parseXmlFromFile(opuserfile);
	if (!x_opusers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), opuserfile);
		goto end_flag;
	}

	x_modOpuser = switch_xml_find_child(x_opusers, "opuser", "name", strOldName.c_str());
	if (!x_modOpuser) {
		strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldName.c_str());
		goto end_flag;
	}

	//������-ֵ��,��: <opuser name=\"%s\" pwd = \"%s\" priority = \"%s\"/>
	switch_xml_set_attr(x_modOpuser, "name", strNewName.c_str());
	switch_xml_set_attr(x_modOpuser, "pwd", strPwd.c_str());

	//����д�ļ�
	opusersContext = switch_xml_toxml(x_opusers, SWITCH_FALSE);
	if (!opusersContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	bRes = cari_common_creatXMLFile(opuserfile, opusersContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
		//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("MOD_OPUSER_SUCCESS");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());


	/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_opusers);//�ͷ�

	switch_safe_free(opuserfile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*��ѯ�����û�
*/
int CModule_NEManager::lstOPUser(inner_CmdReq_Frame*& reqFrame, 
								 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "",strUserName,strParamName,strValue,strChinaName,strSeq,strRecord;
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc          = NULL;
	bool	bRes			= true,bAllOrOne=true;
	string tableHeader[3] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("USER_NAME"),
		getValueOfDefinedVar("USER_PWD")
	};
	int iRecordCout = 0;//��¼����

	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);

	switch_xml_t x_opusers=NULL,x_opuser=NULL;
	char *newOpuserContext=NULL,*opusersContext=NULL,*strTableName=NULL;

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"opusername"
	strParamName = "opusername";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		bAllOrOne = true;
	}
	else{//��ѯ�и�����Ĳ����û���Ϣ
		if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
			//�û�����,���ش�����Ϣ
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
			goto end_flag;
		}

		//�жϴ�"�����û�"�Ƿ����
		if (!isExistedOpUser(strValue)){
			strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
			goto end_flag;
		}

		bAllOrOne = false;
	}
	strUserName = strValue;


	//����Ԫ��xml����ת����xml�Ľṹ(ע��:�����ַ�����)
	x_opusers = cari_common_parseXmlFromFile(opuserfile);
	if (!x_opusers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�����������еĲ����û�
	x_opuser = switch_xml_child(x_opusers, "opuser");
	for (; x_opuser; x_opuser = x_opuser->next) {
		const char *name = switch_xml_attr(x_opuser, "name");
		const char *pwd = switch_xml_attr(x_opuser, "pwd");
		if (name && pwd){
			if (!bAllOrOne){
				if (isEqualStr(strUserName.c_str(),name)){
					goto next;
				}
				continue;
			}

next:
			char *singleRecord = switch_mprintf("%s%s%s%s", 
				name, 
				CARICCP_SPECIAL_SPLIT_FLAG,
				pwd,
				CARICCP_ENTER_KEY
				);

			//mod by xxl 2010-5-27 :������ŵ���ʾ
			iRecordCout++;//��ŵ���
			strRecord = "";
			strSeq = intToString(iRecordCout);
			strRecord.append(strSeq);
			strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
			strRecord.append(singleRecord);

			//��ż�¼
			inner_RespFrame->m_vectTableValue.push_back(/*singleRecord*/strRecord);
			//mod by xxl 2010-5-27 end

			switch_safe_free(singleRecord);
		}
	}

	//��ǰ��¼Ϊ��
	if (0 ==inner_RespFrame->m_vectTableValue.size()){
		strReturnDesc = getValueOfDefinedVar("RECORD_NULL");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("OPUSER_RECORD");
	pDesc = switch_mprintf(strReturnDesc.c_str());

	//���ñ�ͷ�ֶ�
	strTableName = switch_mprintf("%s%s%s%s%s",
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[1].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[2].c_str());

	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);


	/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//�ͷ�xml�ṹ
	switch_xml_free(x_opusers);

	switch_safe_free(pDesc);
	switch_safe_free(opuserfile);
	return iFunRes;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

/*����id�Ž����ж�,�Ƿ���ڴ���Ԫ
*/
bool CModule_NEManager::isExistedNE(int id)
{
	//��ʱ����:Ŀǰ�����ļ���ȡ
	bool bRes = false;
    switch_xml_t x_NEs=NULL,x_ne=NULL;
	const char *errmsg[1]={ 0 };

	const char *neID = NULL;
	string strID = intToString(id);
	char *nefile = switch_mprintf("%s%s%s", 
							SWITCH_GLOBAL_dirs.conf_dir, 
							SWITCH_PATH_SEPARATOR, 
							CARI_CCP_NE_XML_FILE
							);

	if (!cari_common_isExistedFile(nefile)){
		//char *neContext = "<include></include>";
		////���´������ļ�
		//cari_common_creatXMLFile(nefile,neContext,errmsg);

		goto end;
	}

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//��������
	x_ne = switch_xml_child(x_NEs, "ne");
	for (; x_ne; x_ne = x_ne->next) {
		neID = switch_xml_attr(x_ne, "id");
		if (isEqualStr(strID.c_str(),neID)){
			
			bRes = true;
			break;
		}	
	}

end:
	switch_xml_free(x_NEs);
	switch_safe_free(nefile);
	return bRes;
}

/*��Ԫ�Ƿ����,����ip��ַ�����ж�
*/
bool CModule_NEManager::isExistedNE(string ip)
{
	//��ʱ����:Ŀǰ�����ļ���ȡ
	bool bRes = false;
    switch_xml_t x_NEs=NULL,x_ne=NULL;
	const char *errmsg[1]={ 0 };

	const char *neIP=NULL;
	char *nefile = switch_mprintf("%s%s%s", 
							SWITCH_GLOBAL_dirs.conf_dir, 
							SWITCH_PATH_SEPARATOR, 
							CARI_CCP_NE_XML_FILE
							);

	if (!cari_common_isExistedFile(nefile)){
		//char *neContext = "<include></include>";
		////���´������ļ�
		//cari_common_creatXMLFile(nefile,neContext,errmsg);

		goto end;
	}

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//��������
	x_ne = switch_xml_child(x_NEs, "ne");
	for (; x_ne; x_ne = x_ne->next) {
		neIP = switch_xml_attr(x_ne, "ip");
		if (isEqualStr(ip.c_str(),neIP)){
			
			bRes = true;
			break;
		}	
	}

end:
	switch_xml_free(x_NEs);
	switch_safe_free(nefile);
	return bRes;
}


/*�Ƿ���ڴ˲����û�
*/
bool CModule_NEManager::isExistedOpUser(string username)
{
	//��ʱ����:Ŀǰ�����ļ���ȡ
	bool bRes = false;
	switch_xml_t x_NEs,x_ne;
	const char *errmsg[1]={ 0 };

	const char *name=NULL;
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
		if (isEqualStr(username.c_str(),name)){
			bRes = true;
			break;
		}	
	}

end:
	switch_safe_free(nefile);
	return bRes;
}
