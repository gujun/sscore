//#include "modules\CModule_User.h"
#include "CModule_User.h"

//#include "cari_public_interface.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"


/************************************************************************/
/* ������windows������,·�����ļ�������Ŀǰ���ܻ��,��ע��...           */
/************************************************************************/


//////////////////////////////////////////////////////////////////////////
//xml�ĵ�ע�����Ļ�������
//"<?xml   version=\"1.0\"   encoding=\"gb2312\"?>\n"  
//<?xml version="1.0" encoding="utf-8" ?>

//mob user��������xml�ļ��ṹ,֧�����ĵ���ʾ
//#define MOB_USER_XML_CONTEXT  "<include>\n"															\
//	"	<user id=\"%s\" mailbox=\"%s\">\n"															\
//	"		<params>\n"																				\
//	"			<param name=\"password\" value=\"%s\"/>\n"											\
//	"			<param name=\"vm-password\" value=\"%s\"/>\n"										\
//	"			</params>\n"																		\
//	"		<variables>\n"																			\
//	"			<variable name=\"toll_allow\" value=\"%s\"/>\n"										\
//	"			<variable name=\"accountcode\" value=\"%s\"/>\n"									\
//	"			<variable name=\"user_context\" value=\"default\"/>\n"								\
//	"			<variable name=\"effective_caller_id_name\" value=\"Extension %s\"/>\n"				\
//	"			<variable name=\"effective_caller_id_number\" value=\"%s\"/>\n"						\
//	"			<variable name=\"outbound_caller_id_name\" value=\"$${outbound_caller_name}\"/>\n"	\
//	"			<variable name=\"outbound_caller_id_number\" value=\"$${outbound_caller_id}\"/>\n"  \
//	"			<variable name=\"callgroup\" value=\"%s\"/>\n"										\
//	"		</variables>\n"																			\
//	"	</user>\n"																					\
//	"</include>\n";


#define MOB_USER_XML_CONTEXT  "<include>"		\
	"	<user id=\"%s\" mailbox=\"%s\">"		\
	"		<params>"							\
	"		</params>"							\
	"		<variables>"						\
	"		</variables>"						\
	"	</user>"								\
	"</include>";

//mob user ��<params>����
#define MOB_USER_XML_PARAMS		"<params>"		\
								"</params>"

//mob user ��<variables>����
#define MOB_USER_XML_VARIABLES	"<variables>"	\
								"</variables>"

//mob user�򵥵�xml�ļ�,��Ҫ������ŵ�"��"Ŀ¼��ʹ��
#define SIMPLE_MOB_USER_XML_CONTEXT "<include>"\
									"<user id=\"%s\" type=\"pointer\"/>"\
									"</include>"

//mob user param��������
#define MOB_USER_XML_PARAM			"<param name=\"%s\" value=\"%s\">"  	\
									"</param>"

//mob user variable��������
#define MOB_USER_XML_VARATTR		"<variable name=\"%s\" value=\"%s\">"   \
									"</variable>"

//���ӻ�ɾ�����xml��Ϣ.ע�����Ļ�����
//��������ӵ�directory��default.xml�ļ���
#define MOB_GROUP_XML_DEFAULT_CONTEXT	"<group name=\"%s\" desc =\"%s\">"					\
										"<users>"														\
										"<X-PRE-PROCESS cmd=\"include\" data=\"groups/%s/*.xml\"/>"		\
										"</users>"														\
										"</group>"

//��·�������йص����diaplan call group��xml�ļ���Ϣ
#define MOB_GROUP_XML_DIAL_CONTEXT  "<extension name=\"group_dial_%s\">"	\
	                                "<condition field=\"destination_number\" expression=\"^%d$\">"		        \
                                    "<action application=\"bridge\" data=\"group/%s@${domain_name}\"/>"	\
                                    "</condition>"																\
                                    "</extension>"		

//����Ա��������
#define DISPAT_USER_XML_VARATTR		"<dispatcher id=\"%s\">"   \
									"</dispatcher>"
																																																																																//ʹ�õ����ݱ�,�ȽϹ̶�
#define AUTHUSERS					"authusers"

//////////////////////////////////////////////////////////////////////////
//˽�еĺ궨��,ֻ�����ڱ�ģ���ڲ�
#define MIN_GROUP_ID                          1
#define MAX_GROUP_ID						  10	

/*�Ƿ�Ϊ��Ч��ʱ���,��ʽΪ:12:00-13:30
*/
static bool isValidTimeSlot(string timeslot1)
{
	string strTime1,strTime2,strHour1,strMinute1,strHour2,strMinute2;
	int iHour1,iMinute1,iHour2,iMinute2;
	strTime1 = getValueOfStr(CARICCP_LINK_MARK,0,timeslot1);
	strTime2 = getValueOfStr(CARICCP_LINK_MARK,1,timeslot1);
	if (0 == strTime1.length()
		|| 0 == strTime2.length()){
			return false;
	}
	strHour1   = getValueOfStr(CARICCP_COLON_MARK,0,strTime1);
	strMinute1 = getValueOfStr(CARICCP_COLON_MARK,1,strTime1);
	strHour2   = getValueOfStr(CARICCP_COLON_MARK,0,strTime2);
	strMinute2 = getValueOfStr(CARICCP_COLON_MARK,1,strTime2);
	if (0 == strHour1.length()
		|| 0 == strMinute1.length()
		|| 0 == strHour2.length()
		|| 0 == strMinute2.length()){
			return false;
	}
	iHour1   = stringToInt(strHour1.c_str());
	iMinute1 = stringToInt(strMinute1.c_str());
	iHour2   = stringToInt(strHour2.c_str());
	iMinute2 = stringToInt(strMinute2.c_str());
	if (0 > iHour1 
		|| 23 < iHour1
		|| 0  > iHour2
		|| 23 < iHour2
		|| 0  > iMinute1
		|| 59 < iMinute1
		|| 0  > iMinute2
		|| 59 < iMinute2){
			return false;
	}
	if (iHour1>iHour2){
		return false;
	}
	if (iHour1==iHour2
		&& iMinute1 > iMinute2){
		return false;
	}

	return true;
}


//������������漰��������ģ��Ĵ���,���?????????????????????????????????????????????????????????????????
//�����ڲ�����---------------------------------

//////////////////////////////////////////////////////////////////////////
CModule_User::CModule_User():CBaseModule()
{

}


CModule_User::~CModule_User()
{

}

/*���ݽ��յ���֡,������������,�ַ�������Ĵ�����
*/
int CModule_User::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, 
								inner_ResultResponse_Frame *&inner_RespFrame)
{
	if (NULL == inner_reqFrame) {
		//֡Ϊ��
		return CARICCP_ERROR_STATE_CODE;
	}

	//����֡��"Դģ��"��������Ӧ֡��"Ŀ��ģ��"��
	//����֡��"Ŀ��ģ��"��������Ӧ֡��"Դģ��"��,�����
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;


	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	bool bRes = true; 

	//�ж����������ж�����(��������Ωһ��),Ҳ���Ը�����������������
	int iCmdCode = inner_reqFrame->body.iCmdCode;
	switch (iCmdCode) {
	case CARICCP_ADD_MOB_USER:
		iFunRes = addMobUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_MOB_USER:
		iFunRes = delMobUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_MOD_MOB_USER:
		iFunRes = mobMobUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_MOB_USER:
		iFunRes = lstMobUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_MOB_USER:
		//�ڲ�����,��LST_MOBUSER��ͬ,ֻ��Ҫ�������е��û���
		iFunRes = queryMobUser(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_ADD_MOB_GROUP:
		iFunRes = addMobGroup(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_MOB_GROUP:
		iFunRes = delMobGroup(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_MOD_MOB_GROUP:
		iFunRes = modMobGroup(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_MOB_GROUP:
		iFunRes = lstMobGroup(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_SET_MOB_GROUP:
		iFunRes = setMobGroup(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_USER_CALL_TRANSFER:
		iFunRes = lstTransferCallInfo(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_SET_USER_CALL_TRANSFER:
		iFunRes = setTransferCall(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_UNBIND_CALL_TRANSFER:
		iFunRes = unbindTransferCall(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_ADD_DISPATCHER:
		iFunRes = addDispatcher(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_DISPATCHER:
		iFunRes = delDispatcher(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_DISPATCHER:
		iFunRes = lstDispatcher(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_ADD_RECORD:
		iFunRes = addRecord(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_RECORD:
		iFunRes = delRecord(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_RECORD:
		iFunRes = lstRecord(inner_reqFrame, inner_RespFrame);
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


	//test ���������Ŀ�ĺ���
	//switch_core_thread_session_end(NULL);

	//��ӡ��־
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "proc cmd %s end.\n", inner_reqFrame->body.strCmdName);
	return iFunRes;
}


/*������Ϣ����Ӧ�Ŀͻ���
 */
int CModule_User::sendRespMsg(common_ResultResponse_Frame *&respFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	return iFunRes;
}



int CModule_User::cmdPro(const inner_CmdReq_Frame *&reqFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}


/*-----------------------------��������ÿ������Ĵ�����-------------------------*/
//���ǵ��������ܵĹ���,�˴��ľ����߼�Ӧ���ɾ����SW������.��ǰ����SW����ģ��,����û����
//����֡�Ĺؼ���Ϣ�ϰ�����NE����ؼ��ֶ�.
int CModule_User::addMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame, //�������,��Ӧ֡
							 bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";

	//�û�����й���Ϣ
	char *validusergroups = "";
	char *tmpch = "";
	int groupNum = 0;
	char *groups[CARICCP_MAX_GROUPS_NUM] = {
		0
	}; //���CARICCP_MAX_GROUPS_NUM����

	char *new_default_xmlfile = NULL , *new_group_xmlfile = NULL;
	char *xmlfilecontext = NULL , *xmltemplate = NULL , *group_xmlfilecontext = NULL ;
	bool bRes = true;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	char *pDesc = NULL,*pFile = NULL;
	switch_xml_t x_context= NULL;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	int iPriority = DEFAULT_USERPRIORITY;
	string strModel,strUserID, strPwd, strPriority, strDomain, strGroups, strAllows, strContext, strAccountcode, strEffectiveCallerIdName, 
		strEffectiveCallerIdNumber, strOutboundCallerIdName, strOutboundCallerIdNumber,strRecordAllow,strUserDesc;
	string strTmp,strHeader="";
	char *pUserID;
	int iUserCount = 1,iStartUserID = 0,iAddedStep = 1,iOriLen = 0;
	vector<string> vecSucUserID,vecFailUserID;
	vector<string>vecValidGroup;

	//֪ͨ��
	int notifyCode = CARICCP_NOTIFY_ADD_MOBUSER;
	string strNotifyCode = "";
	char *pNotifyCode                     = NULL;
	switch_status_t dbStatus = SWITCH_STATUS_SUCCESS;

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent = NULL;
	char *sql = NULL;
	switch_event_t *outErrorMsgEvent = NULL;
	char *errorMsg = NULL;
	switch_stream_handle_t stream = {
		0
	};
	switch_event_t *resultEvent = NULL;
	//string strResult = "";
	//const char **outMsg = NULL;
	string strSendCmd;

	//"addmodel"
	strParamName = "addmodel";//�����û���ģʽ:single��multi
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//Ĭ��Ϊ��������
		strValue = "single";
	}
	if ((!isEqualStr(strValue,"single"))
		&&(!isEqualStr(strValue,"multi"))){
		//����ֵ��Ч,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strModel = strValue;

	//���ز����Ĵ���,�漰��"����"��"����"����
	if (isEqualStr(strModel,"multi")){
		//"usercount "
		strParamName = "usercount";//�û�����
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//Ĭ��Ϊ����һ���û�
			strValue = "1";
		}

		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iUserCount = stringToInt(strValue.c_str());
		//��Χ����
		if (0>=iUserCount || 100<iUserCount){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,100);
			goto end_flag;
		}

		//"userstep"
		strParamName = "userstep";//�û����ӵĲ���
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//Ĭ��Ϊ1
			strValue = "1";
		}
		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iAddedStep = stringToInt(strValue.c_str());
		//��Χ����
		if (0>=iAddedStep || 10<iAddedStep){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,10);
			goto end_flag;
		}

		//��ʼ���û���
		//"startuserid"
		strParamName = "startuserid";//��ʼ���û���
	}
	else{
		//"userid"
		strParamName = "userid";//�û���
		iUserCount = 1;
	}
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û��ų���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//�û��Ų�����������,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strUserID = strValue;
	pUserID = (char*)strUserID.c_str();

	////add by xxl 2010-5-18: �����ַ��Ĵ�������
	////                      unsigned long,  �ֳ�Ϊ4�ֽڹ�32λ��������, ���ķ�Χ��0~4294967295
	//if (MAX_INTLENGTH < strUserID.length()){
	//	strHeader = strUserID.substr(0,strUserID.length() - MAX_INTLENGTH);
	//	strUserID = strUserID.substr(strUserID.length() - MAX_INTLENGTH);//���������ַ�
	//}
	//else{
	//	strHeader = "";
	//}
	////add by xxl 2010-5-18 end
	//iStartUserID = stringToInt(strUserID.c_str());//�������"0"��ͷ������,��:007,�ᶪʧ!!!
	//iOriLen = strUserID.length();

	//password
	strParamName = "password";//�û�����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.length()) {
		//�û�����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());//switch_mprintf(str_NULL_Error.c_str(),strName)����,Ϊʲô????????
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û����볬��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//�Ƿ��к�������
	bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	if (bRes) {
		strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strPwd = strValue;

	//priority
	iPriority = DEFAULT_USERPRIORITY;
	strParamName = "priority";//�û���Ȩ��,��ѡ����,����û�û����д,Ĭ��Ϊ3,��ͨ��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		iPriority = DEFAULT_USERPRIORITY;
	}
	iPriority = stringToInt(strValue.c_str());
	if (USERPRIORITY_INTERNATIONAL > iPriority || USERPRIORITY_ONLY_DISPATCH < iPriority) { //�û�Ȩ��,����ԽС,����Խ��
		//�û����ȼ���ȡֵ������Χ
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), USERPRIORITY_ONLY_DISPATCH, USERPRIORITY_INTERNATIONAL);
		goto end_flag;
	}
	strPriority = strValue;

	//allows�ֶ�
	//strParamName = "allows";
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "";
	//}
	//strAllows = strValue;
	//strAllows = "international,domestic,local";

	//�Ż�����һ��
	/*  
	��Ϊ5������: 1,2,3,4,5 ,����ԽС,����Խ��,Ŀǰֻ��3,4,5����,����

	1: ���ʳ�;(international)
	2: ����(domestic)
	3: ����(out)
	4: ����(local)
	5: ֻ�ܲ������(diapatch)

	����û���ʱ��,Ĭ�ϼ���Ϊ:4
	*/
	switch (iPriority)
	{
	case USERPRIORITY_INTERNATIONAL:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_INTERNATIONAL");
		break;

	case USERPRIORITY_DOMESTIC:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_DOMESTIC");
		break;

	case USERPRIORITY_OUT:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_OUT");
		break;

	case USERPRIORITY_LOCAL:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_LOCAL");
		break;

	case USERPRIORITY_ONLY_DISPATCH:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_ONLY_DISPATCH");
		break;

	default:
		break;
	}
	if (0 == strAllows.size()) {
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_DOMESTIC");
	}

	//domain
	strParamName = "domain";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strDomain = strValue;

	//�û���groups
	strParamName = "groups";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//�Ƿ��к�������
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strGroups = strValue;

	//desc�ֶ�
	strParamName = "desc";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	};
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strUserDesc = strValue;

	//context�ֶ�
	strParamName = "context";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "default";//Ĭ��Ϊdefault·�ɷ�ʽ
	}
	strContext = strValue;

	//accountcode�ֶ�
	strParamName = "accountcode";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strAccountcode = strValue;

	//effective_caller_id_name�ֶ�,cidname,������ʾ��
	strParamName = "cidname";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//strValue.append("Extension ");
		strValue.append(strHeader);
		strValue.append(strUserID);//Ĭ�Ϸ�ʽΪ�û���
	}
	strEffectiveCallerIdName = strValue;

	//recordallow�ֶ�
	strParamName = "recordallow";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (!isEqualStr(strValue,"yes")){
		strValue = "no";//Ĭ��ֵΪno,��¼��
	}
	strRecordAllow = strValue;

	strEffectiveCallerIdNumber.append(strHeader);
	strEffectiveCallerIdNumber.append(strUserID);

	strOutboundCallerIdName	  = "$${outbound_caller_name}";//��ʱʹ�ô˷�ʽ���д���!!!
	strOutboundCallerIdNumber = "$${outbound_caller_id}";

	//mod by xxl 2010-5-18: �����Ч�ж�����,��ͳһ�����ж�
	//�����û���,�����֮��ʹ��','�Ž�������
	if (!switch_strlen_zero(strGroups.c_str())) {
		int iInvalidGNum = 0;
		string strInvalidG = "";
		groupNum = switch_separate_string((char*) strGroups.c_str(), CARICCP_CHAR_COMMA_MARK, groups, (sizeof(groups) / sizeof(groups[0])));

		//���δ��������Ϣ
		for (int j = 0; j < groupNum; j++) {
			//������Ч����
			if (isExistedGroup(groups[j])) {
				vecValidGroup.push_back(groups[j]);
			}
			else{
				iInvalidGNum ++;
				strInvalidG.append(groups[j]);
                strInvalidG.append(CARICCP_COMMA_MARK);
			}
		}
		//������в�����Ч����,����Ϊʧ��
		if (0 < strInvalidG.length()){
			strInvalidG = strInvalidG.substr(0,strInvalidG.length()-1);//ȥ�����ķ���","
			strReturnDesc = getValueOfDefinedVar("MOB_GROUP_INVALID");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strInvalidG.c_str());
			goto end_flag;
		}
	}
	//mod by xxl 2010-5-18 end

	///***************************************************************/
	//�ռ�����Ҫ���ӵ��û�,�˴�ͳһ������������
	for (int i=0;i<iUserCount;i++){
		string strID = "";
		char strRes[MAX_INTLENGTH];
		string strStep = intToString(i * iAddedStep);

		//mod by xxl  2010-6-1 begin
		//���������"0"��ͷ������,Ҫ����׼ȷ.
		pUserID = (char*)strUserID.c_str();
		addLongInteger(pUserID,(char*)strStep.c_str(),strRes);
		strID = strRes;
		//mod by xxl  2010-6-1 end

		//������ID����ص�����
		strEffectiveCallerIdName = strID;
		strEffectiveCallerIdNumber = strID;
	
		////////////////////////////////////////////////////////////////////////////
		//������ģ��Ľ�������,����ϵͳ����ģ��Ķ���ӿں���
		//switch_stream_handle_t stream = { 0 };
		//switch_event_t *resultEvent = NULL;
		////string strResult = "";
		//char *pResultcode;
		//*string strResultCode;
		//��ʼͬ��ִ����ص�api����
		//SWITCH_STANDARD_STREAM(stream);
		//iCmdReturnCode = switch_api_execute(CARI_NET_SOFIA_CMD_HANDLER, strSendCmd.c_str(), NULL, &stream);
		//outMsg  = (const char **)stream.data;//����Ĵ�����Ϣ

		////����ִ�в��ɹ�
		//if (CARICCP_SUCCESS_STATE_CODE != iCmdReturnCode)
		//{
		//	strReturnDesc = *outMsg;
		//	goto end_flag;
		//}
		////////////////////////////////////////////////////////////////////////////
		//�����һЩ�ֲ�char *ָ��������,�����������
		//memset(tmpch,0,sizeof(char *));
		//memset(validusergroups,0,sizeof(char *));

		//�ж��û��Ƿ��Ѿ�����
		if (bReLoadRoot) {
			bRes = isExistedMobUser(strID.c_str());
			if (bRes) {
				/*iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
				strReturnDesc = getValueOfDefinedVar("MO_USER_EXIST");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
				goto end_flag;*/
				continue;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//��defaultĿ¼�½���������һ��xml�û���Ϣ�ļ�,Ȼ��鿴�����Ϣ,�ڸ���������
		//����һ��simple��xml�ļ�,���ǵ������Ϣ����ЧУ��,˳���ϻ�һ��

		//�����û���,�����֮��ʹ��','�Ž�������
		//if (!switch_strlen_zero(strGroups.c_str())) {
		//	groupNum = switch_separate_string((char*) strGroups.c_str(), CARICCP_CHAR_COMMA_MARK, groups, (sizeof(groups) / sizeof(groups[0])));

		//	//���δ��������Ϣ
		//	string strG = "";
		//	for (int j = 0; j < groupNum; j++) {
		//		//��������,��Ҫ�ڴ����Ӧ��Ŀ¼������mobuser��simple xml�ļ�
		//		//����鲻����,���˵�,��������,ͬʱ��������Ч��У��
		//		if (isExistedGroup(groups[j])) {
		//			//���������û������Ϣ,������Ч���û���,�˴�����ͳһ�ı���
		//			char *pch = switch_mprintf("%s%s", groups[j], CARICCP_COMMA_MARK);
		//			strG.append(pch);
		//			switch_safe_free(pch);

		//			new_group_xmlfile = getMobUserGroupXMLFile(groups[j], strID.c_str());
		//			if (!new_group_xmlfile) {
		//				continue;
		//			}

		//			xmltemplate = SIMPLE_MOB_USER_XML_CONTEXT;
		//			group_xmlfilecontext = switch_mprintf(xmltemplate, strID.c_str());

		//			/************************************************************************/
		//			cari_common_creatXMLFile(new_group_xmlfile, group_xmlfilecontext, err);
		//			switch_safe_free(new_group_xmlfile);
		//			switch_safe_free(group_xmlfilecontext);
		//			/************************************************************************/
		//		}
		//	}

		//	//ȥ�����һ������Ķ���
		//	tmpch = (char *)strG.c_str();
		//	if (!switch_strlen_zero(tmpch)) {
		//		//strncpy(validusergroups,tmpch,strlen(tmpch) -1); //��������CARICCP_COMMA_MARK�Ĵ�С
		//		strValue = tmpch;
		//		strValue = strValue.substr(0, strlen(tmpch) - 1);
		//		validusergroups = (char*) strValue.c_str();
		//	}
		//}

		//mod by xxl 2010-5-18: ����޸�,����Ч��ֱ�ӽ��д���
		//�������û������Ϣ
		if (0 < vecValidGroup.size()){
			string strG = "",strGName="";
			for (vector<string>::iterator it = vecValidGroup.begin(); vecValidGroup.end() != it; it++){
				char *pch;
				strGName = *it;
				pch = switch_mprintf("%s%s", strGName.c_str(), CARICCP_COMMA_MARK);
				strG.append(pch);
				switch_safe_free(pch);
				new_group_xmlfile = getMobUserGroupXMLFile(strGName.c_str(), strID.c_str());
				if (!new_group_xmlfile) {
					continue;
				}
				xmltemplate = SIMPLE_MOB_USER_XML_CONTEXT;
				group_xmlfilecontext = switch_mprintf(xmltemplate, strID.c_str());

				/************************************************************************/
				cari_common_creatXMLFile(new_group_xmlfile, group_xmlfilecontext, err);
				switch_safe_free(new_group_xmlfile);
				switch_safe_free(group_xmlfilecontext);
				/************************************************************************/
			}

			//ȥ�����һ������Ķ���
			tmpch = (char *)strG.c_str();
			if (!switch_strlen_zero(tmpch)) {
				//strncpy(validusergroups,tmpch,strlen(tmpch) -1); //��������CARICCP_COMMA_MARK�Ĵ�С
				strValue = tmpch;
				strValue = strValue.substr(0, strlen(tmpch) - 1);
				validusergroups = (char*) strValue.c_str();
			}
		}
		//mod by xxl 2010-5-18 end

		//////////////////////////////////////////////////////////////////////////
		//��Ҫ���û���������Ϣxml�ļ���defaultĿ¼����һ��
		//��װ�û��ļ���������Ŀ¼�ṹ
		pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			new_default_xmlfile = switch_mprintf("%s%s.xml", pFile, strID.c_str());
			switch_safe_free(pFile);
		}
		if (!new_default_xmlfile) {
			//strReturnDesc = switch_mprintf("Cannot get user %s xml path ", strID.c_str());
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;
			vecFailUserID.push_back(strID);
			continue;
		}

		//��ʼд������,ÿ�м�¼�Ľ���������ʹ��\r\n,ֻ��ʹ������һ��-------------
		xmltemplate = MOB_USER_XML_CONTEXT;
		xmlfilecontext = switch_mprintf(xmltemplate, strID.c_str(), strID.c_str());

		//������ת����xml�Ľṹ(ע��:�����ַ�����)
		x_context = switch_xml_parse_str(xmlfilecontext, strlen(xmlfilecontext));
		if (!x_context) {
			//strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);
			switch_safe_free(xmlfilecontext);//��Ҫ�ͷ��ڴ�
			switch_safe_free(new_default_xmlfile);//�ͷ��ڴ�
			
			continue;
		}

		//���Ӻܶ���ص�����
		//params�ֶ�����
		setMobUserParamAttr(x_context, "password", strPwd.c_str(), true);
		setMobUserParamAttr(x_context, "vm-password", strPwd.c_str(), true);

		//variables�ֶ���������
		setMobUserVarAttr(x_context, "priority"					  , strPriority.c_str(), true);
		//setMobUserVarAttr(x_context, "toll_allow"				  , strAllows.c_str(), true); //Ŀǰ��ʹ�ô��ֶ�
		setMobUserVarAttr(x_context, "accountcode"				  , strID.c_str(), true);
		setMobUserVarAttr(x_context, "user_context"				  , strContext.c_str(), true);
		setMobUserVarAttr(x_context, "effective_caller_id_name"	  , strEffectiveCallerIdName.c_str(), true);
		setMobUserVarAttr(x_context, "effective_caller_id_number" , strEffectiveCallerIdNumber.c_str(), true);
		setMobUserVarAttr(x_context, "outbound_caller_id_name"	  , strOutboundCallerIdName.c_str(), true);
		setMobUserVarAttr(x_context, "outbound_caller_id_number"  , strOutboundCallerIdNumber.c_str(), true);
		setMobUserVarAttr(x_context, "callgroup"				  , validusergroups, true);
		setMobUserVarAttr(x_context, "record_allow"				  , strRecordAllow.c_str(), true);
		setMobUserVarAttr(x_context, "desc"						  , strUserDesc.c_str(), true);

		xmltemplate = switch_xml_toxml(x_context, SWITCH_FALSE);
		if (!xmltemplate) {
			//strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);
			switch_safe_free(xmlfilecontext);     //��Ҫ�ͷ��ڴ�
			switch_safe_free(new_default_xmlfile);//�ͷ��ڴ�
			switch_xml_free(x_context);           //xml�ṹҲҪ�ͷ�
			
			continue;
		}

		/************************************************************************/
		//����(O_CREAT)�µ�MOB�û��ļ�
		bRes = cari_common_creatXMLFile(new_default_xmlfile, xmltemplate, err);
		if (!bRes) {
			//strReturnDesc = *err;
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);
			switch_safe_free(xmlfilecontext);     //��Ҫ�ͷ��ڴ�
			switch_safe_free(new_default_xmlfile);//�ͷ�
			switch_xml_free(x_context);           //xml�ṹҲҪ�ͷ�
			
			continue;
		}
		/************************************************************************/
		
		vecSucUserID.push_back(strID);//�������ӳɹ����û��ĺ���

		switch_safe_free(xmlfilecontext);     //��Ҫ�ͷ�
		switch_safe_free(new_default_xmlfile);//�ͷ�
		switch_xml_free(x_context);           //xml�ṹҲҪ�ͷ�
		
	}//end for (int i=0;i<iUserCount;i++)�����û�

	/*************************************************************/
	//���û��һ���û����ӳɹ�,����Ϊʧ��,�����û����ӳɹ�,��Ϊ�ɹ�
	if (iUserCount == (int)vecFailUserID.size()){
		strReturnDesc = getValueOfDefinedVar("ADD_MOBUSER_FAILE");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}
	//����û�ȫ���Ѿ�����,����Ϊ�ɹ�
	if (0 == vecSucUserID.size() 
		&& 0 == vecFailUserID.size()){
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("MO_USER_ALL_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//֪ͨ��
	notifyCode = CARICCP_NOTIFY_ADD_MOBUSER;
	//�����ͽ����
	pNotifyCode = switch_mprintf("%d", notifyCode);
	for (vector<string>::iterator it = vecSucUserID.begin(); vecSucUserID.end() != it; it++){
		string strID = *it;

		//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
		//if(SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM)) {
		if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
			//��ʱ���ǰ��ճɹ�����
			continue;
		}
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   			//֪ͨ��ʶ
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode); //������=֪ͨ��
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_MOBUSERNAME, strID.c_str());	//֪ͨ�Ľ������

		//����֪ͨ----------------------
		cari_net_event_notify(notifyInfoEvent);

		//�ͷ��ڴ�
		switch_event_destroy(&notifyInfoEvent);
    }
	switch_safe_free(pNotifyCode);

	//////////////////////////////////////////////////////////////////////////
	//�������¼���root xml�������ļ�.��ԭʼ�������ļ�freeswitch.xml,�ȼ��ص�bak(����)�ļ���
	//���´���root xml�ļ��Ľṹ
	//����,����д���ڴ�ķ�ʽ,���������¼����ļ�,Ч������.ʹ��switch_xml_move()����������һ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (bReLoadRoot) {
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
	}

	/************************************************************************/
	////ʹ���ڴ�д��ķ�ʽ����,�漰�����ڴ�ṹ���ݶ�����ʱ����????
	/************************************************************************/

	//�����û�������ִ�гɹ�,��Ҫ֪ͨ����ע��Ŀͻ���
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("ADD_MOBUSER_ALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str());

/*--goto��ת��--*/
end_flag:

	//ֻ��Ҫ�ͷ�һ�μ���
	//switch_safe_free(stream.data);
	//switch_event_destroy(&outMsg); //��Ҫ�ظ��ͷ�,��������

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	//switch_safe_free(new_default_xmlfile);
	return iFunRes;
}

/*ɾ���û�
*/
int CModule_User::delMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame, //�������,��Ӧ֡
							 bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";

	bool bRes;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	char *pDesc                     = NULL;
	char *pNotifyCode               = NULL;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	string strModel,strUserID;
	char*pUserID;
	int iUserCount=1,iStartUserID=0,iDeledStep=1,iOriLen = 0;
	vector<string> vecSucUserID,vecFailUserID;

	//֪ͨ��
	int notifyCode = CARICCP_NOTIFY_DEL_MOBUSER;
	string strNotifyCode = "",strHeader="";
	switch_status_t dbStatus = SWITCH_STATUS_SUCCESS;

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent = NULL;
	switch_stream_handle_t stream = {
		0
	};
	switch_event_t *resultEvent = NULL;
	const char **outMsg = NULL;
	string strSendCmd;
	char *user_default_file = NULL , *chTmp,*pFile = NULL;

	//"delmodel"
	strParamName = "delmodel";//ɾ���û���ģʽ
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//Ĭ��Ϊ����ɾ��
		strValue = "single";
	}
	if ((!isEqualStr(strValue,"single"))
		&&(!isEqualStr(strValue,"multi"))){
			//����ֵ��Ч,���ش�����Ϣ
			strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
	}
	strModel = strValue;

	//���ز����Ĵ���,�漰��"����"��"����"����
	if (isEqualStr(strModel,"multi")){
		//"usercount "
		strParamName = "usercount";//�û�����
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//Ĭ��Ϊɾ��һ���û�
			strValue = "1";
		}
		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iUserCount = stringToInt(strValue.c_str());
		//��Χ����
		if (0>=iUserCount || 100<iUserCount){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,100);
			goto end_flag;
		}

		//"userstep"
		strParamName = "userstep";//ɾ���û��Ĳ���
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//Ĭ��Ϊ1
			strValue = "1";
		}
		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iDeledStep = stringToInt(strValue.c_str());
		//��Χ����
		if (0>=iDeledStep || 10<iDeledStep){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,10);
			goto end_flag;
		}

		//��ʼ���û���
		//"startuserid"
		strParamName = "startuserid";//��ʼ���û���
	}
	else{

		//"userid"
		strParamName = "userid";//�û���
		iUserCount = 1;
	}
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û��ų���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//�û��Ų�����������,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strUserID = strValue;
	pUserID = (char*)strUserID.c_str();


	///***************************************************************/
	//�ռ�����Ҫɾ�����û�,�˴�ͳһ���д���
	for (int i=0;i<iUserCount;i++){
		char *groups[CARICCP_MAX_GROUPS_NUM] = {
			0
		}; //���CARICCP_MAX_GROUPS_NUM����
		char **mobusergoup = groups;

		string strID = "";
		char strRes[MAX_INTLENGTH];
		string strStep = intToString(i * iDeledStep);

		//mod by xxl  2010-6-1 begin
		//���������"0"��ͷ������,Ҫ����׼ȷ.
		pUserID = (char*)strUserID.c_str();
		addLongInteger(pUserID,(char*)strStep.c_str(),strRes);
		strID = strRes;
		//mod by xxl  2010-6-1 end

		//�ж��û��Ƿ��Ѿ�����
		bRes = isExistedMobUser(strID.c_str());
		if (!bRes){//�û�������
			/*iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
			//���ش������Ϣ��ʾ�����û�:�ն��û�������
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
			goto end_flag;*/

			continue;
		}

		//��ɾ��defaultĿ¼�¶�Ӧ���û��ļ�,�����ȸ��ݴ�xml�ļ����Ҷ�Ӧ�������Ϣ,
		//���߱���root xml�ṹ���Ҷ�Ӧ�������Ϣ(���ô˷���)
		pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_file = switch_mprintf("%s%s.xml", pFile, strID.c_str());
			switch_safe_free(pFile);
		}
		if (!user_default_file) {
			//pDesc = switch_mprintf("Cannot get user %s xml path ", strID.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);//��ŵ�ʧ�ܵ�������
			continue;
		}

		//ɾ��defultĿ¼�µ�mob�û���xml�ļ�
		bRes = cari_common_delFile(user_default_file, err);
		if (!bRes) {
			//strReturnDesc = *err;
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);   //��ŵ�ʧ�ܵ�������
			switch_safe_free(user_default_file);//�ͷ�
			continue;
		}

		//Ȼ����ɾ����Ŀ¼�¶�Ӧ���ļ�
		getMobGroupOfUserFromMem(strID.c_str(), groups);
		while (*mobusergoup) {
			chTmp = *mobusergoup;
			char *group_file = getMobUserGroupXMLFile(chTmp, strID.c_str());
			if (group_file) {
				cari_common_delFile(group_file, err);
				switch_safe_free(group_file);
			}
			mobusergoup++;
		}

		vecSucUserID.push_back(strID);//����ɾ���ɹ����û��ĺ���
		switch_safe_free(user_default_file);

	}//end forɾ�����е��û�

	/*************************************************************/
	//���û��һ���û�ɾ���ɹ�,����Ϊʧ��,�����û�ɾ���ɹ�,��Ϊ�ɹ�
	if (iUserCount == (int)vecFailUserID.size()){
		strReturnDesc = getValueOfDefinedVar("DEL_MOBUSER_FAILE");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//����û�ȫ��������,����Ϊʧ��
	if (0 == vecSucUserID.size() 
		&& 0 == vecFailUserID.size()){
			iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
			strReturnDesc = getValueOfDefinedVar("MO_USER_ALL_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
	}

	/*---�漰��dispat.conf.xml�ļ�������---*/
	//��Ҫ�鿴���û��Ƿ�Ϊ����Ա
	{
		switch_xml_t x_dispatchers = NULL,x_dispatcher = NULL;
		int userNum = 0,validUserNum = 0,existedDiapatNum = 0;
		char *xmlfilecontext = NULL,*dispatFile = NULL;
		//switch_event_t *event;
        //�˽ṹ��Ҫ�ͷ�
		x_dispatchers = getAllDispatchersXMLInfo();
		if (!x_dispatchers){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), CARI_CCP_DISPAT_XML_FILE);
			goto reboot_flag;
		}

		//��ɾ���ɹ����û�,����ǵ���Ա,Ҳ��Ҫɾ��
		for (vector<string>::iterator it = vecSucUserID.begin(); vecSucUserID.end() != it; it++){
			string strID = *it;

			//�鿴���û��Ƿ��Ѿ��ǵ���Ա
			x_dispatcher = getDispatcherXMLInfo(strID.c_str(),x_dispatchers);
			if (x_dispatcher){
				existedDiapatNum++;	//�Ѿ����ڵĵ����û�����
				
				//ɾ���˵���Ա
				switch_xml_remove(x_dispatcher);
			}
		}

		if (0 != existedDiapatNum){
			//���½��ṹд�뵽�ļ�
			dispatFile = getDispatFile();
			if (dispatFile) {
				xmlfilecontext = switch_xml_toxml(x_dispatchers, SWITCH_FALSE);
				bRes = cari_common_creatXMLFile(dispatFile, xmlfilecontext, err);
				switch_safe_free(dispatFile);//�����ͷ�
				if (!bRes) {
					strReturnDesc = *err;
					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto end_flag;
				}
			}
			////���ں͵���ģ���໥����,�˴������¼�
			//if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
			//	if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
			//		switch_event_destroy(&event);
			//	}
			//}
		}

		//�ͷ�xml�ṹ
		switch_xml_free(x_dispatchers);
	}

/*--------*/
reboot_flag:
	//////////////////////////////////////////////////////////////////////////
	//���´���root xml�ļ��Ľṹ
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//ɾ���ն��û��ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("DEL_MOBUSER_ALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str());

	//////////////////////////////////////////////////////////////////////////
	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
	//�����ͽ����
	notifyCode = CARICCP_NOTIFY_DEL_MOBUSER;
	pNotifyCode = switch_mprintf("%d", notifyCode);
	for (vector<string>::iterator it = vecSucUserID.begin(); vecSucUserID.end() != it; it++){

		//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
		//if(SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM)) {
		if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
			//��ʱ���ǰ��ճɹ�����
			continue;
		}
		string strID = *it;
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   			//֪ͨ��ʶ
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode); //������=֪ͨ��
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_MOBUSERNAME, strID.c_str());	//֪ͨ�Ľ������

		//����֪ͨ----------------------
		cari_net_event_notify(notifyInfoEvent);

		//�ͷ��ڴ�
		switch_event_destroy(&notifyInfoEvent);
	}
	switch_safe_free(pNotifyCode);


/*--goto��ת��--*/
end_flag:

	//ֻ��Ҫ�ͷ�һ�μ���
	//switch_safe_free(stream.data);
	//switch_event_destroy(&outMsg); //��Ҫ�ظ��ͷ�,��������

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*�޸��ն��û�
*/
int CModule_User::mobMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame, //�������,��Ӧ֡
							 bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";

	bool bModGroupFlag = false;//Ĭ��Ϊ�޸���ı�־
	bool bModGroup = true;
	char *groups[CARICCP_MAX_GROUPS_NUM] = {
		0
	}; //���CARICCP_MAX_GROUPS_NUM����
	char **mobusergoup = groups;//��ֵ;

	char *beforeusergroups = "";
	char *chTmp = "", *tmpG = "",*user_default_xmlfile = NULL,*xmltemplate = NULL, 
		*group_xmlfilecontext = NULL, *userDefaultXmlContext = NULL,*pFile = NULL;

	bool bRes = true;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	char *pDesc                     = NULL;
	char *pNotifyCode               = NULL;
	switch_xml_t x_user = NULL;
	char *userDefaultXMLPath = NULL;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	int iPriority = DEFAULT_USERPRIORITY;
	string strUserID, strPwd, strPriority, strDomain, strGroups, strAllows, strContext, strAccountcode, strGroupModFlag,
		strEffectiveCallerIdName,strRecordAllow,strUserDesc;
	string strTmp;

	//֪ͨ��
	int notifyCode = CARICCP_NOTIFY_MOD_MOBUSER;
	string strNotifyCode = "";
	switch_status_t dbStatus = SWITCH_STATUS_SUCCESS;

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ�����ͻ���
	switch_event_t *notifyInfoEvent = NULL;

	char *sql = NULL;
	switch_event_t *outErrorMsgEvent = NULL;
	char *errorMsg = NULL;

	switch_stream_handle_t stream = {
		0
	};
	switch_event_t *resultEvent = NULL;
	//string strResult = "";
	const char **outMsg = NULL;
	string strSendCmd;
	vector< string> addGroupVec, delGroupVec;
	int iOriLen = 0;

	//"userid"
	strParamName = "userid";//�û���
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û��ų���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//�û��Ų�����������,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strUserID = strValue;
	iOriLen = strUserID.length();

	//password
	strParamName = "password";//�û�����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.length()) {
		//�û�����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û����볬��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	//�Ƿ��к�������
	bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	if (bRes) {
		strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strPwd = strValue;

	//priority
	iPriority = DEFAULT_USERPRIORITY;
	strParamName = "priority";//�û���Ȩ��,�Ǳ�ѡ����,����û�û����д,Ĭ��Ϊ0,��ͼ�
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		iPriority = DEFAULT_USERPRIORITY;
	}
	iPriority = stringToInt(strValue.c_str());
	if (USERPRIORITY_INTERNATIONAL > iPriority || USERPRIORITY_ONLY_DISPATCH < iPriority) {
		//�û����ȼ���ȡֵ������Χ
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), USERPRIORITY_INTERNATIONAL, USERPRIORITY_ONLY_DISPATCH);
		goto end_flag;
	}
	strPriority = strValue;

	////allows�ֶ�
	//strParamName = "allows";
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "";
	//}
	//strAllows = strValue;

	//�Ż�����һ��
	/*  
	��Ϊ5������: 1,2,3,4,5 ,����ԽС,����Խ��,Ŀǰֻ��3,4,5����,����

	1: ���ʳ�;(international)
	2: ����(domestic)
	3: ����(out)
	4: ����(local)
	5: ֻ�ܲ������(diapatch)

	����û���ʱ��,Ĭ�ϼ���Ϊ:4
	*/
	switch (iPriority)
	{
	case USERPRIORITY_INTERNATIONAL:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_INTERNATIONAL");
		break;

	case USERPRIORITY_DOMESTIC:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_DOMESTIC");
		break;

	case USERPRIORITY_OUT:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_OUT");
		break;

	case USERPRIORITY_LOCAL:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_LOCAL");
		break;

	case USERPRIORITY_ONLY_DISPATCH:
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_ONLY_DISPATCH");
		break;

	default:
		break;
	}
	if (0 == strAllows.size()) {
		strAllows = getValueOfDefinedVar("MOBUSER_PRIORITY_DOMESTIC");
	}

	//domain
	strParamName = "domain";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strDomain = strValue;

	//�û���groups
	strParamName = "groups";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strGroups = strValue;

	//context�ֶ�
	strParamName = "context";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "default";//Ĭ��Ϊdefault��ʽ
	}
	strContext = strValue;

	//accountcode�ֶ�
	strParamName = "accountcode";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strAccountcode = strValue;

	//effective_caller_id_name�ֶ�,cidname,������ʾ��
	strParamName = "cidname";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {//ʹ��Ĭ�ϵķ�ʽ
		//strValue.append("Extension ");
		strValue.append(strUserID);
	}
	//else{
	//	//У�鳤��
	//	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
	//		//�û�������,���ش�����Ϣ
	//		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
	//		goto end_flag;
	//	}
	//}
	strEffectiveCallerIdName = strValue;

	//groupmodflag 
	strParamName = "groupmodflag";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strGroupModFlag = strValue;

	//recordallow�ֶ�
	strParamName = "recordallow";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (!isEqualStr(strValue,"yes")){
		strValue = "no";//Ĭ��ֵΪno,��¼��
	}
	strRecordAllow = strValue;

	//desc�ֶ�
	strParamName = "desc";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	};
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strUserDesc = strValue;

	//�ж��û��Ƿ��Ѿ�����
	bRes = isExistedMobUser(strUserID.c_str());
	if (!bRes) {
		iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
		//���ش������Ϣ��ʾ�����û�:�ն��û�������
		strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());
		goto end_flag;
	}

	//������޸��û���,����Ҫ����ǰ���û�����Ϣ��ȡ��������һ�º��ʹ��,��Ϊ�����ɾ����������ɶ�ʧ
	//ֻҪȡֵ��yes,���޸�
	if (isEqualStr("yes",strGroupModFlag)) {
		bModGroupFlag = true;
	}
	if (bModGroupFlag) {
		//mod by xxl 2010-5-18: �����Ч�ж�����,��ͳһ�����ж�
		//�����û���,�����֮��ʹ��','�Ž�������
		int groupNum=0;
		if (!switch_strlen_zero(strGroups.c_str())) {
			int iInvalidGNum = 0;
			string strInvalidG = "";
			groupNum = switch_separate_string((char*) strGroups.c_str(), CARICCP_CHAR_COMMA_MARK, groups, (sizeof(groups) / sizeof(groups[0])));

			//���δ��������Ϣ
			for (int j = 0; j < groupNum; j++) {
				if (isExistedGroup(groups[j])) {
					addGroupVec.push_back(groups[j]);
				}
				else{//��Ч�����ݱ���
					iInvalidGNum ++;
					strInvalidG.append(groups[j]);
					strInvalidG.append(CARICCP_COMMA_MARK);
				}
			}

			//������в�����Ч����,����Ϊʧ��
			if (0 < strInvalidG.length()){
				strInvalidG = strInvalidG.substr(0,strInvalidG.length()-1);//ȥ�����ķ���","
				strReturnDesc = getValueOfDefinedVar("MOB_GROUP_INVALID");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strInvalidG.c_str());
				goto end_flag;
			}
		}
		//mod by xxl 2010-5-18 end

		//��Ҫ������ǰ���û���,����ɾ����ʱ��������Ϣ��ʧ
		getMobGroupOfUserFromMem(strUserID.c_str(), mobusergoup);
		//mobusergoup = groups;//ֱ�Ӹ�ֵ������???
		string strG="";
		while (*mobusergoup) {
			chTmp = *mobusergoup;
			char *pch = switch_mprintf("%s%s", chTmp, CARICCP_COMMA_MARK);
			strG.append(pch);
			switch_safe_free(pch);//��ʱ�ͷ��ڴ�

			mobusergoup++;
			//��������
			delGroupVec.push_back(chTmp);
		}
		bModGroup = false;
		//ȥ�����һ������Ķ���
		tmpG = (char*)strG.c_str();
		if (!switch_strlen_zero(tmpG)) {
			//C++����ִ�г�������
			//strncpy(beforeusergroups,tmpG,strlen(tmpG) -1); //��������CARICCP_COMMA_MARK�Ĵ�С
			strValue = tmpG;
			strValue = strValue.substr(0, strlen(tmpG) - 1);
			beforeusergroups = (char*) strValue.c_str();
		}
	}
	//ֱ�ӿ������Ե��޸�,��Ҫ�漰����/����Ϣ���޸�
	x_user = getMobDefaultXMLInfo(strUserID.c_str());
	if (!x_user) {
		//���ش������Ϣ��ʾ�����û�:�ն��û�������
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());
		goto end_flag;
	}

	//�漰�����Ե��޸�,�漰���ֶβ�ͬ,ʹ�õķ�����ͬ
	//params�ֶ�����
	setMobUserParamAttr(x_user, "password", strPwd.c_str());
	setMobUserParamAttr(x_user, "vm-password", strPwd.c_str());
	
	//variables�ֶ�����
	setMobUserVarAttr(x_user, "priority", strPriority.c_str());
	//setMobUserVarAttr(x_user, "toll_allow", strAllows.c_str()); //Ŀǰ��ʹ�ô��ֶ�
	setMobUserVarAttr(x_user, "effective_caller_id_name", strEffectiveCallerIdName.c_str());
	setMobUserVarAttr(x_user, "record_allow", strRecordAllow.c_str());
	if (0 != strUserDesc.length()){
		setMobUserVarAttr(x_user, "desc", strUserDesc.c_str());
	}

	if (bModGroupFlag) {
		//��Ҫ�鿴��������Щ��,��ɾ��������Щ��,Ϊ�˲�������,�ȶ�beforeusergroups��ͳһ���,Ȼ���ٶ�strGroupsͳһ����
		vector<string>::iterator iter = delGroupVec.begin();
		for (; iter != delGroupVec.end(); ++iter) {
			strTmp = *iter;
			char *group_file = getMobUserGroupXMLFile(strTmp.c_str(), strUserID.c_str());
			if (group_file) {
				cari_common_delFile(group_file, err);
				switch_safe_free(group_file);
			}
		}

		//�ٽ�������
		string strValidGroup = "";
		int index = -1;
		xmltemplate = SIMPLE_MOB_USER_XML_CONTEXT;
		group_xmlfilecontext = switch_mprintf(xmltemplate, strUserID.c_str());
		//splitString(strGroups, CARICCP_COMMA_MARK, addGroupVec);//ǰ���Ѿ�����
		iter = addGroupVec.begin();
		for (; iter != addGroupVec.end(); ++iter) {
			strTmp = *iter;

			////�ж��Ƿ���Ч����,ǰ���Ѿ��������жϴ���
			//if (!isExistedGroup(strTmp.c_str())){
			//	continue;
			//}
			strValidGroup.append(strTmp);
			strValidGroup.append(CARICCP_COMMA_MARK);

			char *group_file = getMobUserGroupXMLFile(strTmp.c_str(), strUserID.c_str());
			if (group_file) {
				cari_common_creatXMLFile(group_file,group_xmlfilecontext,err);
				switch_safe_free(group_file);
			}
		}
		index =  strValidGroup.find_last_of(CARICCP_COMMA_MARK);
		if(0 <= index){
			strValidGroup = strValidGroup.substr(0, index);
		}
		strGroups = strValidGroup;
		//��������������
		setMobUserVarAttr(x_user, "callgroup", strGroups.c_str());
	}

	//���ݶ�̬���ĵ��ڴ�����ݸ�д��xml�����ļ���
	userDefaultXmlContext = switch_xml_toxml(x_user, SWITCH_FALSE);
	if (!userDefaultXmlContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//ֱ�Ӹ�����ǰ���ļ�
	pFile = getMobUserDefaultXMLPath();
	if (pFile) {
		user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, strUserID.c_str());
		switch_safe_free(pFile);
	}
	if (!user_default_xmlfile) {
		goto end_flag;
	}
	bRes = cari_common_creatXMLFile(user_default_xmlfile, userDefaultXmlContext, err);
	if (!bRes) {
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//�����ͳһ���¼���һ��root xml�ļ�
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), user_default_xmlfile);
		goto end_flag;
	}

	//�����û�������ִ�гɹ�,��Ҫ֪ͨ����ע��Ŀͻ���
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("MOD_MOBUSER_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());

	//�漰��֪ͨ���͵Ĵ��� :��������֪ͨ����,��Ҫ�㲥֪ͨ����ע���client
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//��ʱ���ǰ��ճɹ�����
		goto end_flag;
	}

	//֪ͨ��
	notifyCode = CARICCP_NOTIFY_MOD_MOBUSER;

	//�����ͽ����
	pNotifyCode = switch_mprintf("%d", notifyCode);
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   					//֪ͨ��ʶ
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode);          //������=֪ͨ��
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_MOBUSERNAME, strUserID.c_str());	//֪ͨ�Ľ������

	//����֪ͨ----------------------
	cari_net_event_notify(notifyInfoEvent);

	//�ͷ��ڴ�
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pNotifyCode);


/*--goto��ת��--*/
end_flag:

	//ֻ��Ҫ�ͷ�һ�μ���
	//switch_safe_free(stream.data);
	//switch_event_destroy(&outMsg); //��Ҫ�ظ��ͷ�,��������

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//��Ҫ�ͷ�xml�ڴ�
	switch_xml_free(x_user);

	switch_safe_free(pDesc);
	switch_safe_free(user_default_xmlfile);
	switch_safe_free(group_xmlfilecontext);
	return iFunRes;
}

/*������������
 *��ѯ��ǰ���û�:������ϸ���û���Ϣ,
*/
int CModule_User::lstMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnResult = "";
	char *strTableName = "";//��ѯ�ı�ͷ
	bool bRes;
	switch_xml_t userXMLinfo = NULL,groupXMLinfo = NULL,x_users = NULL, x_user = NULL;
	//char *strResult = "";	//�����˶�����������,�ڴ�û���ͷŵ�.���ļ�C++ʹ��memset()���׳�����
	//						//Ҳ�����鸳ֵΪNULL,����Ҫֱ�Ӳ���,������

	const char *userid = NULL;
	//const char **userMsg = NULL;
	const char *err[1] = {
		0
	};
	const char **outMsg = err;
	char *pDesc                     = NULL;
	switch_event_t *resultEvent = NULL;
	string strSendCmd;

	string tableHeader[6] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("USER_ID"),
		getValueOfDefinedVar("USER_PWD"),
		//getValueOfDefinedVar("USER_ACCOUNT"),
		//getValueOfDefinedVar("USER_CID"),
		getValueOfDefinedVar("USER_PRIORITY"),
		getValueOfDefinedVar("USER_GROUP"),
		getValueOfDefinedVar("USER_DESC")
	};
	int iRecordCout = 0;//��¼����

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	string strUserID;

	string strReturnDesc = "",strResult,strRecord = "",strSeq;	//�����˶�����������,�ڴ�û���ͷŵ�.ʹ��memset()
	//Ҳ�����鸳ֵΪNULL,����Ҫֱ�Ӳ���,������

	//userid
	strParamName = "userid";//�û���
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);

	//�û��ų���,���ش�����Ϣ
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	strUserID = strValue;

	//�����ĳЩchar *�ֲ������������,�����ڴ��������
	//memset(strResult, 0, sizeof(char));

	//����Ϊ�ղ�ѯ���е��û�,���ֲ�Ϊ��,��ѯ�˵����û�
	if (!switch_strlen_zero(strUserID.c_str())) {
		//�ж��û��Ƿ��Ѿ�����
		bRes = isExistedMobUser(strUserID.c_str());
		if (!bRes) {
			//�û�������
			iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
			//���ش������Ϣ��ʾ�����û�:�ն��û�������
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());
			goto end_flag;
		}

		//��ѯ�����û�����ϢuserXMLinfo
		userXMLinfo = getMobUserXMLInfoFromMem(strUserID.c_str(), userXMLinfo);
		if (userXMLinfo) {
			//��װ�û�����ϸ��Ϣ
			/*
				<variables>
				    <variable name="toll_allow" value="domestic,international,local"/>
					<variable name="accountcode" value="1005"/>
					<variable name="user_context" value="default"/>
					<variable name="effective_caller_id_name" value="Extension 1005"/>
					<variable name="effective_caller_id_number" value="1005"/>
					<variable name="outbound_caller_id_name" value="FreeSWITCH"/>
					<variable name="outbound_caller_id_number" value="0000000000"/>
					<variable name="callgroup" value="techsupport"/>
					</variables>
			*/
			/*----------------------------*/
			strResult="";
			getSingleMobInfo(userXMLinfo, strResult);

			//strResult =  *userMsg;//ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����
			//strcat(strResult,*userMsg);
			//strcat(strResult,CARICCP_ENTER_KEY);

			//mod by xxl 2010-5-27 :������ŵ���ʾ
			if (0 != strResult.length()){
				iRecordCout++;//��ŵ���
				strRecord = "";
				strSeq = intToString(iRecordCout);
				strRecord.append(strSeq);
				strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
				strRecord.append(strResult);

				//��ż�¼
				inner_RespFrame->m_vectTableValue.push_back(/*strResult*/strRecord);
			}
			//mod by xxl 2010-5-27 end

			iFunRes = CARICCP_SUCCESS_STATE_CODE;
			iCmdReturnCode = SWITCH_STATUS_SUCCESS;
		}
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//��ѯdefault���Ӧ�������û�����Ϣ
	strReturnDesc = "";
	groupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, groupXMLinfo, strReturnDesc);
	if (groupXMLinfo) {
		//�������е��û�
		//����users�ڵ�
		if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {
			
			//����ÿ���û�
			x_user = switch_xml_child(x_users, "user");
			for (; x_user; x_user = x_user->next) {
				userid = switch_xml_attr(x_user, "id");
				if (switch_strlen_zero(userid)) {
					continue;
				}

				//�Ƿ�Ϊ��Ч����
				if (switch_is_number(userid)) {
					/*-----------------------*/
					strResult ="";
					getSingleMobInfo(x_user, strResult);
					if (0 != strResult.length()){
						iRecordCout++;//��ŵ���
						strRecord = "";
						strSeq = intToString(iRecordCout);
						strRecord.append(strSeq);
						strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
						strRecord.append(strResult);

						//��ż�¼
						inner_RespFrame->m_vectTableValue.push_back(/*strResult*/strRecord);
					}
				}
			}//����users
		}
	}
	else {
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	}
	

/*--goto��ת��--*/
end_flag:

	//������Ϣ���ͻ���,���������·�����Ϣ
	if (0 == inner_RespFrame->m_vectTableValue.size()) {
		iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
		if (switch_strlen_zero(strReturnDesc.c_str())) {
			strReturnDesc = getValueOfDefinedVar("RECORD_NULL");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		}
	} 
	else {
	  	//�����ͷ,��Ҫ��xml�������ļ�һ��
	  	//�û�xml�ļ�����ϸ��Ϣ
	  	/*
	  		<variables>
	  		 <variable name="toll_allow" value="domestic,international,local"/>
	  		 <variable name="accountcode" value="1005"/>
	  		 <variable name="user_context" value="default"/>
	  		 <variable name="effective_caller_id_name" value="Extension 1005"/>
			<variable name="effective_caller_id_number" value="1005"/>
			<variable name="outbound_caller_id_name" value="FreeSWITCH"/>
			<variable name="outbound_caller_id_number" value="0000000000"/>
			<variable name="callgroup" value="techsupport"/>
			</variables>
		*/


	  	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s", //��ͷ
			tableHeader[0].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
			tableHeader[1].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
			tableHeader[2].c_str()	    ,CARICCP_SPECIAL_SPLIT_FLAG,
			tableHeader[3].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
			tableHeader[4].c_str()      ,CARICCP_SPECIAL_SPLIT_FLAG, 
			tableHeader[5].c_str());

	  	iFunRes = SWITCH_STATUS_SUCCESS;
	  	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	  	strReturnDesc = getValueOfDefinedVar("MOBUSER_RECORD");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());

		//���ñ�ͷ�ֶ�
		myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
		switch_safe_free(strTableName);
	}

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*��ѯ��ǰ���û�:ֻ�������е��û����Լ�״̬(�����),�����ڲ�����
*���ģ���ʼ��¼��ʱ��ʹ��
*/
int CModule_User::queryMobUser(inner_CmdReq_Frame *&reqFrame, 
							   inner_ResultResponse_Frame *&inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnResult = "";
	const char *userMsgArry[1] = {
		0
	};
	//const char **outMsg = userMsgArry;

	//char resbuf_userid[CARICCP_STRING_LENGTH_64];
	string strSingleRes = "";
	char *pDesc          = NULL;
	const char *userid = NULL;

	switch_xml_t groupXMLinfo = NULL,x_users = NULL, x_user = NULL;
	//char *strResult = "";	//�����˶�����������,�ڴ�û���ͷŵ�.ʹ��memset()
	string strResult;
	//Ҳ�����鸳ֵΪNULL,����Ҫֱ�Ӳ���,������
	const char **userMsg = userMsgArry;
	//userMsg = malloc(sizeof(char *)); //�������ڴ�ռ�
	//switch_assert(userMsg);
	//memset(userMsg, 0, sizeof(char *));

	//�����ĳЩchar *�ֲ������������,�����ڴ��������
	//memset(strResult, 0, sizeof(char *));

	//////////////////////////////////////////////////////////////////////////
	//��ѯdefault���Ӧ�������û�����Ϣ,ֻ��Ҫ��ѯmob ���뼴��
	groupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, groupXMLinfo, strReturnResult);
	if (groupXMLinfo) {
		//�������е��û�����users�ڵ�
		if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {

			//��Ҫ�����ݿ��в鿴���û��Ƿ��Ѿ�ע��???
			switch_core_db_t *db = NULL;
			char *dbFileName = "sofia_reg_internal";//Ŀǰ�������ݿ��ļ��鿴

#ifdef SWITCH_HAVE_ODBC
#else
#endif

			if (!(db = switch_core_db_open_file(dbFileName))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", dbFileName);
				db = NULL;
			}

			//����ÿ���û�
			x_user = switch_xml_child(x_users, "user");
			for (; x_user; x_user = x_user->next) {
				userid = switch_xml_attr(x_user, "id");
				if (switch_strlen_zero(userid)) {
					continue;
				}

				//�Ƿ�Ϊ��Ч����
				if (switch_is_number(userid)) {
					strResult = (char*)userid;			 //ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����
					strResult.append(CARICCP_COLON_MARK);//����֮��ʹ��":"����
					char  *mobUserState	 = NULL;
					
					//�Ƿ��Ѿ�ע����
					bool bReg = isRegisteredMobUser(userid,db);
					if (bReg){
						mobUserState = switch_mprintf("%d", /*CARICCP_MOB_USER_INIT*/1);
					}
					else{
						mobUserState = switch_mprintf("%d", /*CARICCP_MOB_USER_NEW*/0);
					}

					strResult.append(mobUserState);

					//��ż�¼
					inner_RespFrame->m_vectTableValue.push_back(strResult);
					switch_safe_free(mobUserState);
				}
			}//����users

			//�ͷſ������
			if (db) {
				switch_core_db_close(db);
			}
		}//end x_users

		//������Ϣ���ͻ���,���������·�����Ϣ
		iFunRes = CARICCP_SUCCESS_STATE_CODE;
		iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
		strReturnResult = getValueOfDefinedVar("PRIVATE_MOBUSER_INFO_RESULT");
	}
	else{
		iFunRes = SWITCH_STATUS_FALSE;
		if (0 == strReturnResult.length()) {
			//��ǰ�ļ�¼Ϊnull
			strReturnResult = getValueOfDefinedVar("RECORD_NULL");
		}
	}

	pDesc = switch_mprintf("%s",strReturnResult.c_str());
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/*�����û���
*/
int CModule_User::addMobGroup(inner_CmdReq_Frame *&reqFrame, 
							  inner_ResultResponse_Frame *&inner_RespFrame, 
							  bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	const char *err[1] = {
		0
	};

	char *pDesc                     = NULL;
	char *groupPath = NULL, *default_xmlfile = NULL, *diagroup_xmlfile = NULL;
	char *newGroupsContext = NULL, *oldGroupsContext = NULL, *diagroupcontext = NULL;
	switch_xml_t x_newgroup = NULL,x_allgroups = NULL,x_domain = NULL,x_groups = NULL,x_diagroup = NULL,x_diaallgroups = NULL;
	bool bRes = false;
	int iGroupPos = 0;
	char output[100]={0};

	vector< string> vec;
	string newStr;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//���������������Ϣ
	string strGroupName, strGroupDesc, strGroupID;
	int iGroupId = -1;

	//"groupName"
	strParamName = "groupName";//����
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (0 == strValue.size()) {
		//������Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//��������
		//�����ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//�Ƿ��к�������
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strGroupName = strValue;

	////"groupID"
	//strParamName = "groupID";//���
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//������Ϊ��,���ش�����Ϣ
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////��ŵķ�Χ[1-10]
	//iGroupId = stringToInt(strValue.c_str());
	//if (MIN_GROUP_ID > iGroupId || MAX_GROUP_ID < iGroupId) {
	//	//��ŵķ�Χ����
	//	strReturnDesc = getValueOfDefinedVar("GROUPID_RANGE_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), MIN_GROUP_ID, MAX_GROUP_ID);

	//	goto end_flag;
	//}
	//strGroupID = strValue;

	//"desc"
	strParamName = "desc";//��������Ϣ
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//��������Ϣ����
		//��������Ϣ����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	////�Ƿ��к�������
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes){
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(),
	//		strChinaName.c_str());
	//	goto end_flag;
	//}
	strGroupDesc = strValue;

	//�жϴ����Ƿ����,�����ڴ��нṹ�����ж�
	if (isExistedGroup(strGroupName.c_str(), iGroupId)) {
		strReturnDesc = getValueOfDefinedVar("GROUP_NAMEID_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		goto end_flag;
	}
	strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
	strReturnDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

	//���������Ϣ.����:
	/*
	<group name="strGroupName " id = "groupid" desc ="strGroupDesc">
	<users>
	<X-PRE-PROCESS cmd="include" data="groups/strGroupName/*.xml"/>
	</users>
	</group>
	*/
	newGroupsContext = switch_mprintf(MOB_GROUP_XML_DEFAULT_CONTEXT, 
		(char*) strGroupName.c_str(), 
		/*(char*) strGroupID.c_str(),*/ 
		(char*) strGroupDesc.c_str(), 
		(char*) strGroupName.c_str());

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_newgroup = switch_xml_parse_str(newGroupsContext, strlen(newGroupsContext));
	
	//conf\directory\default.xml
	default_xmlfile = getDirectoryDefaultXMLFile();//��Ҫ�ͷ�

	//���ļ��ж�ȡgroups���ڴ�ṹ
	x_allgroups = cari_common_parseXmlFromFile(default_xmlfile);
	if (!x_allgroups || !x_newgroup) {
		strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());
		goto end_flag;
	}

	//װ�����ַ���,����ʹ��,����Ŀǰxml������
	oldGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);

	//���Ҷ�Ӧ��groups�ڵ�
	x_domain = switch_xml_child(x_allgroups, "domain");
	if (!x_domain) {
		goto end_flag;
	}
	x_groups = switch_xml_child(x_domain, "groups");
	if (!x_groups) {
		goto end_flag;
	}

	//��̬�����ڴ�Ľṹ,��������group�ڵ�ϲ��������Ľṹ��
	//���λ�÷��õ����
	iGroupPos = getGroupNumFromMem();
	if (!switch_xml_insert(x_newgroup, x_groups, /*iGroupPos*/INSERT_INTOXML_POS)){//���λ��,���õ����
		goto end_flag;
	}
	//�ϲ�����ɵ�default.xml�ṹ,����д�ļ�
	newGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);
	if (!newGroupsContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	//ֱ�Ӹ�����ǰ��conf\directory\default.xml�ļ�
	bRes = cari_common_creatXMLFile(default_xmlfile, newGroupsContext, err);
	if (!bRes) {
		//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
		//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//��conf\directory\groupsĿ¼�´�����Ӧ�������Ŀ¼
	groupPath = getMobGroupDir(strGroupName.c_str());
	if (!cari_common_creatDir(groupPath, err)) {
		//���Ŀ¼�Ѿ�������,��cari_common_creatDir()���Ѿ��ж���,�˴���������ʵ��ʧ�����
		strReturnDesc = getValueOfDefinedVar("CREAT_DIR_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), groupPath);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());

		//ֱ�ӷ���,��Ҫ����ֱ�����¼���һ��???
		goto end_flag;
	}

	////////////////////////////////////////////////////////////////////////////
	////�漰�޸ĵ��ļ�:conf\directory\default.xml,conf\dialplan\default\02_group_dial.xml
	////������Ӧ��diaplan group��xml��Ϣ
	//diagroupcontext = switch_mprintf(MOB_GROUP_XML_DIAL_CONTEXT, 
	//	(char*) strGroupName.c_str(), 
	//	iGroupId,
	//	(char*) strGroupName.c_str());

	////������ת����xml�Ľṹ
	//x_diagroup = switch_xml_parse_str(diagroupcontext, strlen(diagroupcontext));
	//diagroup_xmlfile = getDirectoryDialGroupXMLFile();
	//x_diaallgroups = cari_common_parseXmlFromFile(diagroup_xmlfile);
	//if (!x_diagroup || !x_diaallgroups) {
	//	strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());
	//	goto end_flag;
	//}
	//if (!switch_xml_insert(x_diagroup, x_diaallgroups, /*iGroupPos*/INSERT_INTOXML_POS)){//���λ��
	//	goto end_flag;
	//}
	////�ϲ�����ɵ�default xml�ṹ,����д�ļ�
	//diagroupcontext = switch_xml_toxml(x_diaallgroups, SWITCH_FALSE);
	//if (!diagroupcontext) {
	//	strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
	//	pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	//	goto end_flag;
	//}

	////ֱ�Ӹ�����ǰ���ļ�
	//bRes = cari_common_creatXMLFile(diagroup_xmlfile, diagroupcontext, err);
	//if (!bRes) {
	//	//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
	//	//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

	//	goto end_flag;
	//}
	
	//////////////////////////////////////////////////////////////////////////
	//���¼��ض�Ӧ���ڴ�,�˲����Ƿ�Ϊ��Ҫ???,��Ϊ������Ƿ���ڵ�ʱ��ʹ��,Ҳ����
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diagroup_xmlfile);
		goto end_flag;
	}

	//������ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("ADD_MOBGROUP_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

/*-----*/
end_flag:



	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml�ṹ���ڴ��ͷ�
	switch_xml_free(x_allgroups);

	switch_safe_free(pDesc);
	switch_safe_free(groupPath);
	switch_safe_free(default_xmlfile);
	return iFunRes;
}

/*ɾ����,����Ѵ������û�����ϢҲɾ����
*/
int CModule_User::delMobGroup(inner_CmdReq_Frame *&reqFrame, 
							  inner_ResultResponse_Frame *&inner_RespFrame, 
							  bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	const char *err[1] = {
		0
	};
	char *pDesc                     = NULL,*pCh = NULL;

	char *mobusers[CARICCP_MAX_MOBUSER_IN_GROUP_NUM] = {
		0
	}; //ÿ�������CARICCP_MAX_MOBUSER_IN_GROUP_NUM��mob�û�
	char **allMobUsersInGroup = mobusers;
	char *groupPath = NULL, *default_xmlfile = NULL, *chTmp = NULL, *user_default_xmlfile = NULL, *diagroup_xmlfile = NULL;
	char *groupInfo = NULL, *xmlfilecontext = NULL;
	char *newGroupsContext = NULL, *oldGroupsContext = NULL;
	switch_xml_t x_delgroup = NULL,x_allgroups = NULL,x_domain = NULL,x_groups = NULL,x_user = NULL,x_extension = NULL;
	bool bRes = false;

	vector< string> vec;
	string newStr, strTmp;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	string strGroupName;

	//"groupName"
	strParamName = "groupName";//����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//������Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){ //��������
		//�����ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGroupName = strValue;

	//�жϴ����Ƿ����,�����ڵ����
	if (!isExistedGroup(strGroupName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		goto end_flag;
	}

	//�������ص�������Ϣ
	strReturnDesc = getValueOfDefinedVar("MO_DEL_GROUP_FAILED");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

	default_xmlfile = getDirectoryDefaultXMLFile();//��Ҫ�ͷ�
	x_allgroups = cari_common_parseXmlFromFile(default_xmlfile);
	if (!x_allgroups) {
		goto end_flag;
	}

	//װ�����ַ���,����ʹ��,����Ŀǰxml������
	oldGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);

	//���Ҷ�Ӧ��groups�ڵ�
	x_domain = switch_xml_child(x_allgroups, "domain");
	if (!x_domain) {
		goto end_flag;
	}
	x_groups = switch_xml_child(x_domain, "groups");
	if (!x_groups) {
		goto end_flag;
	}
	//���Ҷ�Ӧ��group�Ľṹ,���ݾ��������������
	x_delgroup = switch_xml_find_child(x_groups, "group", "name", strGroupName.c_str());
	if (!x_delgroup) {
		goto end_flag;
	}

	/************************************************************************/
	//��̬�����ڴ�Ľṹ,����group�ڵ�ɾ��,��ʱx_allgroups������Ҳ�ᶯ̬�����仯
	switch_xml_remove(x_delgroup);
	/************************************************************************/

	//����д�ļ�
	newGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);
	if (!newGroupsContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	bRes = cari_common_creatXMLFile(default_xmlfile, newGroupsContext, err);
	if (!bRes) {
		//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
		//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

		goto end_flag;
	}
	//����ɾ����Ӧ�����Ŀ¼
	groupPath = getMobGroupDir(strGroupName.c_str());
	if (!cari_common_delDir(groupPath, err)) {
		//���۳ɹ�ʧ��,��ʱ������

		//iFunRes = CARICCP_SUCCESS_STATE_CODE;
		//iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
		//strReturnDesc = getValueOfDefinedVar("ADD_MOBGROUP_SUCCESS");
		//strReturnDesc = switch_mprintf(strReturnDesc.c_str(),strGroupName.c_str());

		//goto end_flag;
	}
	switch_safe_free(groupPath);

	//���¼��ض�Ӧ���ڴ�,�˲����Ƿ�Ϊ��Ҫ???,��Ϊ������Ƿ���ڵ�ʱ��ʹ��,Ҳ����
	//�ȴ����ӵ�һ�û���ʱ�������¼����ڴ�ṹ

	//���Ҵ����µ�����mob�û�,��root xml���ڴ�ṹ�н��в���
	getMobUsersOfGroupFromMem(strGroupName.c_str(), allMobUsersInGroup);
	while (*allMobUsersInGroup) {
		//char * ch1,*ch2;
		chTmp = *allMobUsersInGroup;//��ö�Ӧ���û�id��

		//�޸�defaultĿ¼�¶�Ӧ���û��ŵ�xml�ļ����������
		//����(O_CREAT)�µ�MOB�û��ļ�
		char *pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, chTmp);
			switch_safe_free(pFile);
		}

		if (!user_default_xmlfile) {
			continue;
		}

		//�˽ṹ��Ҫ�ͷ�
		x_user = getMobDefaultXMLInfo(chTmp);
		if (!x_user) {
			continue;
		}

		//���Ҷ�Ӧ��callgroup������
		groupInfo = NULL;
		groupInfo = getMobUserVarAttr(x_user, "callgroup");
		if (!groupInfo) {
			allMobUsersInGroup++;
			continue;
		}

		//ȥ�������Ϣ,���Ƿ��õ��м������λ�õ��������
		//groupInfo = switch_mprintf("%s%s", strGroupName.c_str(), ",");
		//ch1 = switch_string_replace(chTmp, groupInfo, "");
		//ch2 = switch_string_replace(ch1, strGroupName.c_str(), "");
	
		string strG=getNewStr(groupInfo,strGroupName.c_str());

		//�޸Ĵ��û�������ֵ,x_user�����ݻᱻ�޸�
		setMobUserVarAttr(x_user, "callgroup", strG.c_str());

		//�����ͷ�chTmp,switch_string_replace()�����������µ��ڴ�ռ�
		//ע��:�˷�����������������???????????????????????????
		//switch_safe_free(groupInfo);
		//switch_safe_free(ch1);
		//switch_safe_free(ch2);

		//���´����޸ĺ���û���Ϣ�ļ�
		xmlfilecontext = switch_xml_toxml(x_user, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(user_default_xmlfile, xmlfilecontext, err);
		
		switch_safe_free(user_default_xmlfile);//�˴��ͷ�

		//�ͷ�xml�ṹ
		switch_xml_free(x_user);

		if (!bRes) {
			allMobUsersInGroup++;
			continue;
		}

		//��һ���û�
		allMobUsersInGroup++;
	}//end while

	//////////////////////////////////////////////////////////////////////////////
	//////diaplan group xml��Ϣ�ĸ���: conf/dialplan/default/02_group_dial.xml
	////diagroup_xmlfile = getDirectoryDialGroupXMLFile();
	////x_extension = cari_common_parseXmlFromFile(diagroup_xmlfile);
	////if (!x_extension) {
	////	strReturnDesc = getValueOfDefinedVar("MO_DEL_GROUP_FAILED_NO_DIALXML");
	////	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
	////	goto end_flag;
	////}

	//////���Ҷ�Ӧ��group�Ľṹ,���ݾ��������������
	////pCh = switch_mprintf("%s%s", "group_dial_", strGroupName.c_str());
	////x_delgroup = switch_xml_find_child(x_extension, "extension", "name", pCh);
	////switch_safe_free(pCh);
	////if (!x_delgroup) {
	////	goto end_flag;
	////}

	/////************************************************************************/
	//////��̬�����ڴ�Ľṹ,����group�ڵ�ɾ��,��ʱx_allgroups������Ҳ�ᶯ̬�����仯
	////switch_xml_remove(x_delgroup);
	/////************************************************************************/

	//////����д�ļ�
	////newGroupsContext = switch_xml_toxml(x_extension, SWITCH_FALSE);
	////if (!newGroupsContext) {
	////	strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
	////	goto end_flag;
	////}

	////bRes = cari_common_creatXMLFile(diagroup_xmlfile, newGroupsContext, err);
	////pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	////if (!bRes) {
	////	//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
	////	//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

	////	goto end_flag;
	////}

	/************************************************************************/
	//////////////////////////////////////////////////////////////////////////
	//�������¼���root xml�������ļ�.��ԭʼ�������ļ�freeswitch.xml,�ȼ��ص�bak(����)�ļ���
	//���´���root xml�ļ��Ľṹ
	//����,����д���ڴ�ķ�ʽ,���������¼����ļ�,Ч������.ʹ��switch_xml_move()����������һ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diagroup_xmlfile);
		goto end_flag;
	}

	//ɾ����ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("DEL_MOBGROUPR_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

/*-----*/
end_flag:

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml�ṹ���ڴ��ͷ�
	switch_xml_free(x_allgroups);

	switch_safe_free(pDesc);
	switch_safe_free(default_xmlfile);
	return iFunRes;
}

/*�޸��û���,��Ҫ���޸����ƺ���ص�������Ϣ
*/
int CModule_User::modMobGroup(inner_CmdReq_Frame *&reqFrame, 
							  inner_ResultResponse_Frame *&inner_RespFrame, 
							  bool bReLoadRoot)
{
	//�û�������ֽ����޸�,�漰����default.xml�ļ���Ҫ�޸Ķ�Ӧ������
	//�ļ�Ŀ¼��������Ҫ�޸�
	//�漰�������е��û���callgroup������Ҫ�޸�
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	const char *err[1] = {
		0
	};
	char *pDesc                     = NULL;
	char *oldGroupPath = NULL, *newGroupPath = NULL, *default_xmlfile = NULL, *mobuserid = NULL, *mobuserattr = NULL, *diagroup_xmlfile = NULL;
	char *newGroupsContext = NULL, *oldGroupsContext = NULL, *xmlfilecontext = NULL, *user_default_xmlfile = NULL, *diagroupcontext = NULL;
	switch_xml_t x_newgroup = NULL,x_allgroups = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_user = NULL,x_delgroup = NULL,x_diagroup = NULL,
				 x_extension = NULL,x_diaallgroups = NULL;
	bool bRes = false;

	char *mobusers[CARICCP_MAX_MOBUSER_IN_GROUP_NUM] = {
		0
	}; //ÿ�������CARICCP_MAX_MOBUSER_IN_GROUP_NUM��mob�û�
	char **allMobUsersInGroup = mobusers;
	int iGroupPos = 0;

	vector<string> vec;
	string newStr, strTmp;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//���������������Ϣ
	string strOldGroupName, strNewGroupName, strGroupDesc, /*strGroupID,*/ strModFlag="no";
	//int iGroupId = 0;

	//"oldgroupname"
	strParamName = "oldgroupname";//����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//������Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){
		//�����ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//�Ƿ��к�������
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strOldGroupName = strValue;

	//"newgroupname"
	strParamName = "newgroupname";//����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//������Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){
		//�����ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//�Ƿ��к�������
	bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strNewGroupName = strValue;

	////"modFlag"
	//strParamName = "modFlag";//�Ƿ��޸������ı�ʶ
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//strModFlag = strValue;
	//if (isEqualStr(strModFlag,"yes")){
	//	//"groupID"
	//	strParamName = "groupID";//���
	//	strValue = getValue(strParamName, reqFrame);
	//	trim(strValue);
	//	if (0 == strValue.size()) {
	//		//������Ϊ��,���ش�����Ϣ
	//		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//		goto end_flag;
	//	}
	//	//��ŵķ�Χ[1-10]
	//	iGroupId = stringToInt(strValue.c_str());
	//	if (MIN_GROUP_ID > iGroupId 
	//		|| MAX_GROUP_ID < iGroupId) {
	//		//��ŵķ�Χ����
	//		strReturnDesc = getValueOfDefinedVar("GROUPID_RANGE_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), MIN_GROUP_ID, MAX_GROUP_ID);
	//		goto end_flag;
	//	}
	//	strGroupID = strValue;
	//}

	//"desc"
	strParamName = "desc";//��������Ϣ
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//��������Ϣ����
		//��������Ϣ����,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//�Ƿ��к�������
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes)
	//{
	//strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	strReturnDesc = switch_mprintf(strr,
	//		strChinaName.c_str());

	//	goto end_flag;
	//}
	strGroupDesc = strValue;

	//�жϴ����Ƿ����,�����ڴ��нṹ�����ж�,���������,������
	if (!isExistedGroup(strOldGroupName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
		goto end_flag;
	}

	//�Ƿ��޸����
	if (isEqualStr(strModFlag,"yes")){//yes
		////���"������"��"�����"����ǰ����ͬ,��ֱ�ӷ��سɹ�
		//int iOldGID = -1;
		//strTmp = getGroupID(strOldGroupName.c_str());
		//iOldGID = stringToInt(strTmp.c_str());
		////�������޸�
		//if (isEqualStr(strOldGroupName, strNewGroupName)){
		//	if (iOldGID == iGroupId){
		//		goto success_flag;
		//	}
		//	else{//����Ƿ��Ѿ���������ʹ��
		//		if(isExistedGroup("", iGroupId)){
		//			strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_3");
		//			pDesc = switch_mprintf(strReturnDesc.c_str(),iGroupId);
		//			goto end_flag;
		//		}
		//	}
		//}
		////�����޸�
		//else{
		//	//�ж��޸ĺ������(���ͺ�)�Ƿ����,�������,������
		//	if (iOldGID != iGroupId){
		//		if (isExistedGroup(strNewGroupName.c_str(), iGroupId)){
		//			strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_2");
		//			pDesc = switch_mprintf(strReturnDesc.c_str());
		//			goto end_flag;
		//		}
		//	}
		//	//�����Ų���,ֻ�޸�����,������.
		//}
	}
	else{//no
		//��������ľ�/��������ͬ,��ò���Ҫ�޸��û�������,����Ĳ����Ͳ���Ҫ��
		if (isEqualStr(strOldGroupName,strNewGroupName)){
			//�û����������,�Լ����Ŀ¼����Ҫ�ٽ����޸�,ֱ�ӷ���
			goto success_flag;
		}
		////��ȡ���ڵ����,��������һ��
		//strGroupID = getGroupID(strOldGroupName.c_str());
		//iGroupId = stringToInt(strGroupID.c_str());
	}

	strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED");
	strReturnDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());

	//�޸�conf/directory/default.xml�ļ�,�޸������Ϣ.����:
	/*
	<group name="strNewGroupName " id = "iGroupId" desc ="strGroupDesc">
	<users>
	<X-PRE-PROCESS cmd="include" data="groups/strNewGroupName/*.xml"/>
	</users>
	</group>
	*/
	newGroupsContext = switch_mprintf(MOB_GROUP_XML_DEFAULT_CONTEXT, 
		(char*) strNewGroupName.c_str(), 
		/*(char*) strGroupID.c_str(),*/ 
		(char*) strGroupDesc.c_str(), 
		(char*) strNewGroupName.c_str());

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_newgroup = switch_xml_parse_str(newGroupsContext, strlen(newGroupsContext));

	default_xmlfile = getDirectoryDefaultXMLFile();
	x_allgroups = cari_common_parseXmlFromFile(default_xmlfile);
	if (!x_allgroups || !x_newgroup) {
		strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
		goto end_flag;
	}

	//���Ҷ�Ӧ��groups�ڵ�
	x_domain = switch_xml_child(x_allgroups, "domain");
	if (!x_domain) {
		goto end_flag;
	}

	x_groups = switch_xml_child(x_domain, "groups");
	if (!x_groups) {
		goto end_flag;
	}

	//��̬�����ڴ�Ľṹ,��������group�ڵ�ϲ��������Ľṹ��
	//��ʱx_allgroups������Ҳ�ᶯ̬�����仯
	x_group = switch_xml_find_child(x_groups, "group", "name", strOldGroupName.c_str());
	if (!x_group) {
		goto end_flag;
	}

	/************************************************************************/
	//�ȼ���ڵ�
	iGroupPos = getGroupNumFromMem();
	if (!switch_xml_insert(x_newgroup, x_groups, /*iGroupPos*/INSERT_INTOXML_POS)) {
		goto end_flag;
	}

	//�����µĽڵ�ɹ���,��ɾ��ԭ���Ľڵ�
	switch_xml_remove(x_group);
	/************************************************************************/

	//�ϲ�����ɵ�default xml�ṹ,����д�ļ�
	newGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);
	if (!newGroupsContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//ֱ�Ӹ�����ǰ���ļ�
	bRes = cari_common_creatXMLFile(default_xmlfile, newGroupsContext, err);
	if (!bRes) {
		//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
		//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//�����޸Ķ�Ӧ�����Ŀ¼
	oldGroupPath = getMobGroupDir(strOldGroupName.c_str());
	newGroupPath = getMobGroupDir(strNewGroupName.c_str());
	if (!cari_common_modDir(oldGroupPath, newGroupPath, err)) {
		switch_safe_free(oldGroupPath);
		switch_safe_free(newGroupPath);
		goto end_flag;
	}
	switch_safe_free(oldGroupPath);
	switch_safe_free(newGroupPath);

    //////////////////////////////////////////////////////////////////////////
	//���ԭ��������û���callgroup�����Խ����޸�
	//���Ҵ����µ�����mob�û�,��root xml���ڴ�ṹ�н��в���
	getMobUsersOfGroupFromMem(strOldGroupName.c_str(), allMobUsersInGroup);
	while (*allMobUsersInGroup) {
		mobuserid = *allMobUsersInGroup;//��ö�Ӧ���û�id��

		//�޸�defaultĿ¼�¶�Ӧ���û��ŵ�xml�ļ����������
		char *pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, mobuserid);
			switch_safe_free(pFile);
		}
		if (!user_default_xmlfile) {
			continue;
		}

		//�˽ṹ��Ҫ�ͷ�
		x_user = getMobDefaultXMLInfo(mobuserid);
		if (!x_user) {
			continue;
		}

		//���Ҷ�Ӧ��callgroup������
		mobuserattr = NULL;
		mobuserattr = getMobUserVarAttr(x_user, "callgroup");
		if (!mobuserattr) {
			allMobUsersInGroup++;
			continue;
		}

		//�滻�����Ϣ,ʹ���µ�������ɵ�����Ϣ
		mobuserattr = switch_string_replace(mobuserattr, strOldGroupName.c_str(), strNewGroupName.c_str());

		//�޸Ĵ��û�������ֵ,x_user�����ݻᱻ�޸�
		setMobUserVarAttr(x_user, "callgroup", mobuserattr);

		//���´����޸ĺ���û���Ϣ�ļ�
		xmlfilecontext = switch_xml_toxml(x_user, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(user_default_xmlfile, xmlfilecontext, err);

		//�����ͷ�switch_string_replace()�����������µ��ڴ�ռ�
		//ע��:�˷�������ϵͳ������������???????????????????????????
		switch_safe_free(mobuserattr);
		switch_safe_free(user_default_xmlfile);//�����ͷ�

		//�ͷ�xml�ṹ
		switch_xml_free(x_user);

		if (!bRes) {
			allMobUsersInGroup++;
			continue;
		}

		//��һ���û�
		allMobUsersInGroup++;
	}

	////////////////////////////////////////////////////////////////////////////
	////�޸�conf/dialplan/default/02_group_dial.xml����Ϣ
	//diagroup_xmlfile = getDirectoryDialGroupXMLFile();
	//x_extension = cari_common_parseXmlFromFile(diagroup_xmlfile);
	//if (!x_extension) {
	//	strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_4");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
	//	goto end_flag;
	//}

	////���Ҷ�Ӧ��extension�Ľṹ,���ݾ��������������
	//pDesc = switch_mprintf("%s%s", "group_dial_", strOldGroupName.c_str());
	//x_delgroup = switch_xml_find_child(x_extension, "extension", "name", pDesc);
	//switch_safe_free(pDesc);
	//if (!x_delgroup) {
	//	strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_4");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
	//	goto end_flag;
	//}
	//diagroupcontext = switch_mprintf(MOB_GROUP_XML_DIAL_CONTEXT, 
	//	(char*) strNewGroupName.c_str(),
	//	iGroupId,
	//	(char*) strNewGroupName.c_str());

	////������ת����xml�Ľṹ
	//x_diagroup = switch_xml_parse_str(diagroupcontext, strlen(diagroupcontext));
	///************************************************************************/
	////�Ƚ�������
	//if (!switch_xml_insert(x_diagroup, x_extension, /*iGroupPos*/INSERT_INTOXML_POS)){//���λ��
	//	strReturnDesc = getValueOfDefinedVar("XML_INSERT_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
	//	goto end_flag;
	//}

	////�����µĽڵ�ɹ���,��ɾ��ԭ���Ľڵ�
	//switch_xml_remove(x_delgroup);
	///************************************************************************/

	////�ϲ�����ɵ�degault xml�ṹ,����д�ļ�
	//diagroupcontext = switch_xml_toxml(x_extension, SWITCH_FALSE);
	//if (!diagroupcontext) {
	//	strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
	//	pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	//	goto end_flag;
	//}

	////ֱ�Ӹ�����ǰ���ļ� :conf/dialplan/default/02_group_dial.xml
	//bRes = cari_common_creatXMLFile(diagroup_xmlfile, diagroupcontext, err);
	//switch_safe_free(diagroupcontext);//�������ļ����ͷŴ�ָ��
	//if (!bRes) {
	//	//�Ƿ��ǻָ���ǰ��ģ��ṹ!!!!!!!!!!!!!!!
	//	//������ִ���,��һ������д���ݵ�ʱ�����,�˴����ٿ��ǻ�������.

	//	goto end_flag;
	//}

	//���¼��ض�Ӧ���ڴ�
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
	}

	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diagroup_xmlfile);
		goto end_flag;
	}

/*---------*/
success_flag:

	//�޸���ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("MOD_MOBGROUP_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());

/*-----*/
end_flag:

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml�ṹ���ڴ��ͷ�
	switch_xml_free(x_allgroups);

	switch_safe_free(pDesc);
	switch_safe_free(default_xmlfile);
	switch_safe_free(diagroup_xmlfile);
	return iFunRes;
}

/*��ѯmob��,�����������,���ѯ�����������û�,����,��ѯ���������Ϣ
*/
int CModule_User::lstMobGroup(inner_CmdReq_Frame *&reqFrame, 
							  inner_ResultResponse_Frame *&inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "",strname,strRecord,strSeq;
	const char *err[1] = {
		0
	};
	char *mobInfo[CARICCP_MAX_MOBUSER_IN_GROUP_NUM] = {
		0
	}; //ÿ�������CARICCP_MAX_MOBUSER_IN_GROUP_NUM��mob�û�

	string tableHeader[4] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("GROUP_NAME"),
		getValueOfDefinedVar("GROUP_DESC"),

		//�������ֲ�ͬ����ı�ͷ
		getValueOfDefinedVar("USER_ID"),
	};
	int iRecordCout = 0;//��¼����

	char **allMobInfo = mobInfo;
	char *chTmp = NULL, *strTableName;
	char *pDesc = NULL;
	string strAll = "";

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ���������������Ϣ
	string strGroupName;

	//"groupName"
	strParamName = "groupName";//����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);

	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//��������
		//�����ֳ���,���ش�����Ϣ
	    strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGroupName = strValue;

	//�������������,���ѯ���������е��û���Ϣ
	//���û����������,����ʾ���е���
	if (!switch_strlen_zero(strGroupName.c_str())) {
		//�жϴ����Ƿ����,�����ڵ����
		if (!isExistedGroup(strGroupName.c_str())) {
			strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
			goto end_flag;
		}
		//////////////////////////////////////////////////////////////////////////
		//���1:��ѯ���������е�"�û�"��Ϣ
		getMobUsersOfGroupFromMem(strGroupName.c_str(), allMobInfo);
		if (!*allMobInfo) {
			strReturnDesc = getValueOfDefinedVar("PRIVATE_NO_USER_IN_GROUP");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}

		//��ͷ��Ϣ����
		strTableName = switch_mprintf("%s%s%s", 
			tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
			tableHeader[3].c_str());

		iFunRes = SWITCH_STATUS_SUCCESS;
		iCmdReturnCode = SWITCH_STATUS_SUCCESS;
		strReturnDesc = getValueOfDefinedVar("PRIVATE_USER_IN_GROUP");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());

		//���ñ�ͷ�ֶ�
		myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
		switch_safe_free(strTableName);

		//�������е�ÿ���û�
		while (*allMobInfo) {
			chTmp = *allMobInfo;//��ö�Ӧ���û�id��
			char *name = switch_mprintf("%s%s", chTmp, CARICCP_ENTER_KEY);//ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����

			//mod by xxl 2010-5-27 :������ŵ���ʾ
			iRecordCout++;//��ŵ���
			strRecord = "";
			strSeq = intToString(iRecordCout);
			strRecord.append(strSeq);
			strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
			strRecord.append(name);

			//��ż�¼
			inner_RespFrame->m_vectTableValue.push_back(/*name*/strRecord);
			//mod by xxl 2010-5-27 end

			switch_safe_free(name);

			//��һ���û�
			allMobInfo++;
		}
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//���2:��ѯ����"��"����Ϣ
	getAllGroupFromMem(strAll);
	if (0 == strAll.length()) {
		strReturnDesc = getValueOfDefinedVar("PRIVATE_NO_GROUP");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	//*allMobInfo = (char*)strAll.c_str();

	//��ͷ��Ϣ����
	strTableName = switch_mprintf("%s%s%s%s%s", 
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, /*TABLE_HEADER_GROUP_ID, CARICCP_SPECIAL_SPLIT_FLAG,*/ 
		tableHeader[1].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[2].c_str());

	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("MOBGROUP_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

    splitString(strAll,CARICCP_ENTER_KEY,inner_RespFrame->m_vectTableValue);

	////����ÿ����
	//while (*allMobInfo) {
	//	chTmp = *allMobInfo;//��ö�Ӧ������Ϣ
	//	char *name = switch_mprintf("%s%s", chTmp, CARICCP_ENTER_KEY);//ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����

	//	//��ż�¼
	//	inner_RespFrame->m_vectTableValue.push_back(name);
	//	switch_safe_free(name);

	//	//��һ���û�
	//	allMobInfo++;
	//}

	//���ñ�ͷ�ֶ�
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);


/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));
	switch_safe_free(pDesc);

	return iFunRes;
}

/*�����û���,����ĳЩ�û����ӵ�������.����û�֮��ʹ��,����
*/
int CModule_User::setMobGroup(inner_CmdReq_Frame *&reqFrame, 
							  inner_ResultResponse_Frame *&inner_RespFrame, 
							  bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	bool bRes = false;
	const char *err[1] = {
		0
	};
	const char **outMsg = err;

	switch_xml_t x_group = NULL,x_userInGroup = NULL,x_user = NULL;

	int userNum = 0,validUserNum = 0,existedUserNum = 0;//����������û�����Ч���������Ѿ������д��ڵ��û�����
	char *users[CARICCP_MAX_MOBUSER_IN_GROUP_NUM] = {
		0
	}; //���CARICCP_MAX_MOBUSER_IN_GROUP_NUM���û�
	char *default_xmlfile = NULL, *groupInfo = NULL, *user_default_xmlfile = NULL;
	char *xmlfilecontext = NULL;
	char *pDesc = NULL;
	int i = 0,iMaxLen	;
	bool bAddOrDel = true;//���ӻ���ɾ���û��ı�ʶ,Ĭ��Ϊ�����û�������

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	string strGroupName, strAddOrDelFlag, strAllUserName;

	//"groupName"
	strParamName = "groupName";//����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//������Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}

	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�����ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGroupName = strValue;

	//"addOrdel"
	strParamName = "addOrdel";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	iMaxLen = CARICCP_MAX_STR_LEN("del_user_from_group", strValue.c_str());
	if (!strncasecmp("del_user_from_group", strValue.c_str(), iMaxLen)) {
		bAddOrDel = false; //��ʱ��Ҫ������ɾ���û�
	}
	strAddOrDelFlag = strValue;

	//"userid"
	strParamName = "userid";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//������Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}

	if (CARICCP_STRING_LENGTH_512 < strValue.length()){//��������
		//�����ֳ���,���ش�����Ϣ
	    strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_512);
		goto end_flag;
	}
	strAllUserName = strValue;

	//�жϴ����Ƿ����,�����ڴ��нṹ�����ж�
	if (!isExistedGroup(strGroupName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		goto end_flag;
	}

	strReturnDesc = "";
	x_group = getGroupXMLInfoFromMem(strGroupName.c_str(), x_group, strReturnDesc);
	if (!x_group) {
		if (0 == strReturnDesc.length()){
			strReturnDesc = getValueOfDefinedVar("MO_GROUP_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		}
		else{
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		}
		
		goto end_flag;
	}

	//����������û���,�ֽ�ÿ���û�
	userNum = switch_separate_string((char*) strAllUserName.c_str(), CARICCP_CHAR_COMMA_MARK, users, (sizeof(users) / sizeof(users[0])));

	//���ζ�ÿ���û����д��������Ϣ
	for (i = 0; i < userNum; i++) {
		string newStr;
		char *new_group_xmlfile = NULL,*pFile;
		//�鿴�������Ƿ��Ѿ����ڴ��û�???
		x_userInGroup = NULL;
		x_userInGroup = getMobUserXMLInfoInGroup(users[i], x_group, x_userInGroup);
		//����������û�������
		if (bAddOrDel) {
			if (x_userInGroup) {
				validUserNum++;		//��Ч���û�����
				existedUserNum++;	//�Ѿ����ڵ��û�����

				continue; //�������Ѿ����ڴ��û�
			}
		} 
		else {
			//������ɾ���û�
			if (!x_userInGroup) {
				//���������û�����Ч,������,ֱ�ӽ����ж���һ���û�
				continue;
			}

			validUserNum++;		//��Ч���û�����
			existedUserNum++;	//�Ѿ����ڵ��û�����
		}
		////�ٲ鿴���û��Ƿ���Ч
		//bRes = isExistedMobUser(users[i]);
		//if (!bRes)
		//{
		//	continue;
		//}
		pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, users[i]);
			switch_safe_free(pFile);//�˴��ͷ�
		}
		if (!user_default_xmlfile) {
			continue;
		}

		//��Ҫ�ͷŴ˽ṹ
        x_user = NULL;
		x_user = cari_common_parseXmlFromFile(user_default_xmlfile);

		//�û�����Ч,���������,�����ش���!!!
		if (!x_user) {
			goto for_end;
		}
		//��Ч���û����� :����������û�������
		if (bAddOrDel) {
			validUserNum++;
		} 
		else {
			existedUserNum++;	//�Ѿ����ڵ��û�����
		}

		new_group_xmlfile = getMobUserGroupXMLFile((char*) strGroupName.c_str(), users[i]);
		if (!new_group_xmlfile) {
			goto for_end;
		}
		/************************************************************************/
		//����������û�������,�����ĵ������Ŀ¼��
		if (bAddOrDel) {
			char *group_xmlfilecontext = switch_mprintf(SIMPLE_MOB_USER_XML_CONTEXT, users[i]);
			bRes = cari_common_creatXMLFile(new_group_xmlfile, group_xmlfilecontext, err);
			switch_safe_free(group_xmlfilecontext);
		} 
		else {//����Ǵ�����ɾ���û�,����Ҫ�����Ŀ¼��ɾ�����û�������ļ�
			bRes = cari_common_delFile(new_group_xmlfile, err);
		}
		switch_safe_free(new_group_xmlfile);
		/************************************************************************/

		if (!bRes) {
			goto for_end;
		}

		//ͬʱ����Ҫ�޸�defaultĿ¼���û���xml�ļ���callgroup��������Ϣ
		//�޸�defaultĿ¼�¶�Ӧ���û��ŵ�xml�ļ����������
		groupInfo = NULL;
		groupInfo = getMobUserVarAttr(x_user, "callgroup");
		if (!groupInfo) {
			goto for_end;
		}
		//���������
		if (bAddOrDel){
			//���������Ϣ,���������ǰû�м�����,�����ǵ�һ��
			if (switch_strlen_zero(groupInfo)) {
				groupInfo = switch_mprintf("%s", strGroupName.c_str());
			} 
			else {
				//�����ж��ִ����Ƿ��Ѿ�����???
			  	groupInfo = switch_mprintf("%s%s%s", groupInfo, ",", strGroupName.c_str());
			}
		} 
		else {
			string strNewG = getNewStr(groupInfo,strGroupName.c_str());
			//test
			//groupInfo =  (char *)strNewG.c_str();//��˸�ֵ,����strNewG�Ǿֲ�����,������������,groupInfoֵ�����쳣,xml�ļ�����!!!
			//printf("����1: %s\n",groupInfo);
			groupInfo = switch_mprintf("%s", strNewG.c_str());
		}
		////test 
		//if (!bAddOrDel) {
		//	printf("����2: %s\n",groupInfo);
		//}

		//�޸Ĵ��û�������ֵ,x_user�����ݻᱻ�޸�
		setMobUserVarAttr(x_user, "callgroup", groupInfo);

		//���´����޸ĺ���û���Ϣ�ļ�,����
		xmlfilecontext = switch_xml_toxml(x_user, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(user_default_xmlfile, xmlfilecontext, err);
		
		//�������ͷ�
		switch_safe_free(groupInfo);
		if (!bRes) {
			//������
		}

for_end:
		//�ͷ�xml�ṹ
		switch_xml_free(x_user);
		
		//���ͳһ�ͷ�
		switch_safe_free(user_default_xmlfile);

	}//�������е��û�,�������

	//�û��Ŷ�����Ч��
	if (0 == validUserNum) {
		if (bAddOrDel) {
			strReturnDesc = getValueOfDefinedVar("PRIVATE_SETGROUP_ADD_ALLUSER_INVALID");
		}
		else{
			strReturnDesc = getValueOfDefinedVar("PRIVATE_SETGROUP_DEL_ALLUSER_INVALID");
		}
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�����û�������
	if (bAddOrDel) {
		//���������û��Ŷ��Ѿ������д�����,��û�б�Ҫ�ٽ������¼���root xml�Ľṹ,�ṩЧ��
		if (validUserNum == existedUserNum) {
			goto no_reload;
		}
	} 
	else{//������ɾ���û�
		//���������û��Ŷ�����Ч��,��ʱ����Ҫ����
		if (0 == existedUserNum) {
			goto no_reload;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//���¼��ض�Ӧ���ڴ�,�˲����Ƿ�Ϊ��Ҫ???,��Ϊ������Ƿ���ڵ�ʱ��ʹ��,Ҳ����
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), user_default_xmlfile);
		goto end_flag;
	}

/*------*/
no_reload:

	//�����û�������ִ�гɹ�,��Ҫ֪ͨ����ע��Ŀͻ���
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("SET_MOBGROUP_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*���ú���ĺ���ת��,����ǰת/æʱ,��ʱ�ĺ���ת�Ƶ����ú�����
*/
int CModule_User::setTransferCall(inner_CmdReq_Frame *&reqFrame, 
								  inner_ResultResponse_Frame *&inner_RespFrame, 
								  bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	char *default_xmlfile = NULL;
	char *chFailover = NULL, *beforeMainID=NULL,*account=NULL,*transerid=NULL;
	bool bRes = true;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	char *pDesc                     = NULL;
	switch_xml_t x_first_user=NULL, x_second_user=NULL, x_beformain_user=NULL;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;
	//��ȡ�û��ź��û�������:��ѡ����
	string strTranscallType,strFirstUserName, strSecondUserName,strSecondUserType,strGateway;
	string strTimeslot1,strTimeslot2,strTimeslot3,strValidUser,strTransferUser;
	string strTmp;
	vector<string> vecTransferID;
	int ipos = 0;

	//"transcalltype"
	strParamName = "transcalltype";//ת������
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strTranscallType = strValue;

	//"userfirstname"
	strParamName = "userfirstname";//�û���
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û��ų���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//�û��Ų�����������,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strFirstUserName = strValue;

	//"transcallusertype"
	strParamName = "transcallusertype";//ת�����û�����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strSecondUserType = strValue;

	//���ص�����
	if (isEqualStr("out",strSecondUserType)){
		//"gateway"
		strParamName = "gateway";//����
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//Ϊ��,���ش�����Ϣ
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		strGateway = strValue;

		//�����Ƿ����
		CModule_Route moduleRoute;
		if (!moduleRoute.isExistedGateWay("internal", strGateway.c_str())) {
			strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strGateway.c_str());
			goto end_flag;
		}
	}

	//"usersecname"
	strParamName = "usersecname";//ת����
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û��ų���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strValue = filterSameStr(strValue,CARICCP_COMMA_MARK);//������ͬ�ĺ�
	if (0 == strValue.size()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strSecondUserName = strValue;
	
	//add by xxl 2010-10-22 begin :�����"ҹ��"����,������ʱ��ν�������,
	//����ʱ���1,ʱ���2��ʱ���3�Ǳ�ѡ.
	if (isEqualStr("night",strTranscallType)){
		//"timeslot1"
		strParamName = "timeslot1";//ʱ���1
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		strTimeslot1 = strValue;
		//���ʱ���ʽ�Ƿ���ȷ
		if (0 != strTimeslot1.length()){
			if(!isValidTimeSlot(strTimeslot1)){
				//ʱ��θ�ʽ��Ч
				strReturnDesc = getValueOfDefinedVar("PARAM_FORMAT_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
				goto end_flag;
			}
		}

		//"timeslot2"
		strParamName = "timeslot2";//ʱ���2
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		strTimeslot2 = strValue;
		//���ʱ���ʽ�Ƿ���ȷ
		if (0 != strTimeslot2.length()){
			if(!isValidTimeSlot(strTimeslot2)){
				//ʱ��θ�ʽ��Ч
				strReturnDesc = getValueOfDefinedVar("PARAM_FORMAT_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
				goto end_flag;
			}

			//��װʱ���ʽ
			if (0 != strTimeslot1.length()){
				strTimeslot1.append(CARICCP_COMMA_MARK);
			}
			strTimeslot1.append(strTimeslot2);
		}

		//"timeslot3"
		strParamName = "timeslot3";//ʱ���3
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		strTimeslot3 = strValue;
		//���ʱ���ʽ�Ƿ���ȷ
		if (0 != strTimeslot3.length()){
			if(!isValidTimeSlot(strTimeslot3)){
				//ʱ��θ�ʽ��Ч
				strReturnDesc = getValueOfDefinedVar("PARAM_FORMAT_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
				goto end_flag;
			}

			//��װʱ���ʽ
			if (0 != strTimeslot1.length()){
				strTimeslot1.append(CARICCP_COMMA_MARK);
			}
			strTimeslot1.append(strTimeslot3);
		}
	}
	//add by xxl 2010-10-22 end

	//�ж�"����"�Ƿ����
	bRes = isExistedMobUser(strFirstUserName.c_str());
	if (!bRes) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("FIR_NUM_NOT_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());
		goto end_flag;
	}

	//�˽ṹ��Ҫ�ͷ�
	x_first_user = getMobDefaultXMLInfo(strFirstUserName.c_str());
	if (!x_first_user) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("MOBUSER_DEFAULT_XMLFILE_NOEXIST");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//��ǰ�����߼� :"æʱ"ת��
	if (isEqualStr("busy",strTranscallType)){
		//���һ�������Ƿ�Ϊ��������ı�ת��
		account = getMobUserVarAttr(x_first_user, "accountcode");
		if (account
			&&(!switch_strlen_zero(account))) {
				//accountcode��id�����,˵��������Ϊת����,��Ҫ�ٽ�������,������ܵ���ѭ�� 
				if (strcasecmp(account, strFirstUserName.c_str())) {
					strReturnDesc = getValueOfDefinedVar("FIR_IS_TRANSFER");
					pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str(),account);
					goto end_flag;
				}
		}
		//mod by xxl 2010-10-26 begin :����̫���ӵĴ���,��"ҹ��"������һ��.
		////���һ������Ŀǰ�Ƿ��Ѿ�������ת����
		//transerid = getMobUserVarAttr(x_first_user, "fail_over");
		//if (transerid
		//	&&(!switch_strlen_zero(transerid))) {
		//		strTmp = transerid;

		//		////�͵�ǰ������һ��,���������Ϊ�ɹ�
		//		//if (!strcasecmp(transerid, strSecondUserName.c_str())) {
		//		//	goto succuss_flag;
		//		//}
		//		//�����ʽΪ:voicemail:1001,���������,��Ϊ�������
		//		ipos = strTmp.find("voicemail");
		//		if (0 > ipos){
		//			//������ʾ:�����Ƚ����ǰ��ת��(�漰���ܶ��ļ����޸�!!!�˴�������̫���ӵĴ���)
		//			strReturnDesc = getValueOfDefinedVar("FIR_HAS_TRANSFER");
		//			pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str(),transerid);
		//			goto end_flag;
		//		}
		//}
		//mod by xxl 2010-10-26 end
	}

	strReturnDesc = getValueOfDefinedVar("OP_FAILED");

	//mod by xxl 2010-06-02 begin :��֧��"�༶"ת������,����֧��"���"ת������
	//mod by xxl 2010-10-22 begin :������������ :��"����"��ת���Ž�����ϸ���жϴ���
	//                             "����"���û�������ƽ̨���޷�ʶ��,ֱ����Ӽ���.
	splitString(strSecondUserName,CARICCP_COMMA_MARK,vecTransferID);

	//����ת��
	if (isEqualStr("local",strSecondUserType)){
		//"���"ת���Ŵ���
		for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
			string strID = *it;
			
			//�û����Ƿ�Ϊ��������
			if (!isNumber(strID)) {
				strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
				goto end_flag;
			}
			//���ź�ת�����Ƿ���ͬ
			if (!strcasecmp(strFirstUserName.c_str(), strID.c_str())) {
				strReturnDesc = getValueOfDefinedVar("FIR_SEC_EQUAL_ERROR");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto end_flag;
			}
			//�ж�"ת����"�Ƿ����
			bRes = isExistedMobUser(strID.c_str());
			if (!bRes) {
				iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
				strReturnDesc = getValueOfDefinedVar("SEC_NUM_NOT_EXIST");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
				goto end_flag;
			}

			//////////////////////////////////////////////////////////////////////////
			//"æʱ"����
			if (isEqualStr("busy",strTranscallType)){

				//mod by xxl 2010-10-26 begin :����̫���ӵĴ���,��"ҹ��"������һ��.
				/*
				x_second_user = getMobDefaultXMLInfo(strID.c_str());
				if (!x_second_user){
					continue;
				}
				beforeMainID = getMobUserVarAttr(x_second_user, "accountcode");//��ʱ������ȡ��accountcode��
				if (beforeMainID &&
					(!strcasecmp(beforeMainID, strFirstUserName.c_str()))) {//���ֲ���
						switch_xml_free(x_second_user);
						continue;
				}
				//��ת�����Ƿ��Ѿ�������������ת����???��֧�ֶ༶!!!!!!!!!!!
				char *chAttr = getMobUserVarAttr(x_second_user, "fail_over");
				if (chAttr){
					strTmp = chAttr;

					//�����ʽΪ:voicemail:1001,���������,��Ϊ�������
					ipos = strTmp.find("voicemail");
					if (0 > ipos){
						switch_xml_free(x_second_user);

						//������ʾ:�����Ƚ����ǰ��ת��(�漰���ܶ��ļ����޸�!!!�˴�������̫���ӵĴ���)
						strReturnDesc = getValueOfDefinedVar("FIR_HAS_TRANSFER_2");
						pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str(),chAttr);
						goto end_flag;
					}
				}

				//ת�������/�޸����������
				//<variable name="accountcode" value="����"/>
				//<variable name="max_calls" value="1"/>
				//<variable name="fail_over" value="voicemail:����"/>
				chFailover = switch_mprintf("voicemail:%s", strFirstUserName.c_str());
				setMobUserVarAttr(x_second_user, "accountcode", strFirstUserName.c_str(), true); //accountcode���Ա���ͬ����һ��
				setMobUserVarAttr(x_second_user, "max_calls", "1", true);
				setMobUserVarAttr(x_second_user, "fail_over", chFailover, true);

				//��д"ת����"���ļ�
				bRes = rewriteMobUserXMLFile(strID.c_str(),x_second_user,strReturnDesc);
				if (!bRes) {
					switch_xml_free(x_second_user);

					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto end_flag;
				}
				switch_safe_free(chFailover);

				//�˴�����Ҫע��:�����ת����������ǰ��������ʹ��(��:������Ŷ�Ӧͬһ��ת����),����Ҫ����ǰ���ŵ�"fail_over"����????
				if (!beforeMainID
					||switch_strlen_zero(beforeMainID)
					|| isEqualStr("",beforeMainID)) {
						switch_xml_free(x_second_user);
						continue;
				}
				//�����ǰû�б�����ת��,���ٴ���.
				if (!strcasecmp(beforeMainID, strID.c_str())) {
					switch_xml_free(x_second_user);
					continue;
				}

				//����Ҫ����ǰ���ŵ�"fail_over"����,�����������Ŷ�Ӧͬһ��ת����
				x_beformain_user = getMobDefaultXMLInfo(beforeMainID);
				if (!x_beformain_user) {
					switch_xml_free(x_second_user);
					continue;
				}
				chAttr = getMobUserVarAttr(x_beformain_user,"fail_over");
				if (chAttr){
					string strAttr = chAttr;
					replace_all(strAttr,strID,"");
					chFailover = switch_mprintf("%s", strAttr.c_str());
				}
				else{
					chFailover = switch_mprintf("voicemail:%s", beforeMainID);
				}
				setMobUserVarAttr(x_beformain_user, "fail_over", chFailover);

				//��дת����֮ǰ���󶨵����ŵ�xml�ļ�
				bRes = rewriteMobUserXMLFile(beforeMainID,x_beformain_user,strReturnDesc);
				switch_safe_free(chFailover);//�����ͷ�
				if (!bRes) {
					//�ͷ�xml�ṹ
					switch_xml_free(x_beformain_user);
					switch_xml_free(x_second_user);

					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto end_flag;
				}

				//����ͷ�xml�ṹ
				switch_xml_free(x_beformain_user);
				switch_xml_free(x_second_user);
				*/
				//mod by xxl 2010-10-26 end

			}//end æʱ����
			//////////////////////////////////////////////////////////////////////////
			//"ҹ��"����
			else{
				strValidUser.append("internal/");
			}

			strValidUser.append(strID);
			strValidUser.append(CARICCP_COMMA_MARK);

		}//end for ��������

		if (0!= strValidUser.length()){
			strValidUser = strValidUser.substr(0,strValidUser.length()-1);//ȥ�����Ķ��ŷ���
		}
		strSecondUserName = strValidUser;
	}
	// "����"���û�ת������,���Ժ��������Ч�ж�,ֱ��д��
	else{
		for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
			string strID = *it;
			strTransferUser.append("gateway/");
			strTransferUser.append(strGateway);
			strTransferUser.append("/");
			strTransferUser.append(strID);
			strTransferUser.append(CARICCP_COMMA_MARK);
		}
		if (0!= strTransferUser.length()){
			strTransferUser = strTransferUser.substr(0,strTransferUser.length()-1);
		}
		strSecondUserName = strTransferUser;
	}
	//mod by xxl 2010-06-02 end
	//mod by xxl 2010-10-22 end

	//�������/�޸�����
	//"æ��"
	if (isEqualStr("busy",strTranscallType)){
		setMobUserVarAttr(x_first_user, "vm_extension", strFirstUserName.c_str(), true);
		setMobUserVarAttr(x_first_user, "max_calls", "1", true);
		setMobUserVarAttr(x_first_user, "fail_over", strSecondUserName.c_str(), true);//�˺������ʧ��ת�Ƶ�second������
	}
	//"ҹ��"
	else{
		setMobUserVarAttr(x_first_user, "direct_transfer", strSecondUserName.c_str(), true);
		setMobUserVarAttr(x_first_user, "direct_transfer_period", strTimeslot1.c_str(), true);
	}

	//��д�����ļ�
	bRes = rewriteMobUserXMLFile(strFirstUserName.c_str(),x_first_user,strReturnDesc);
	if (!bRes) {
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//���¼���root xml�ļ��Ľṹ
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), default_xmlfile);
			goto end_flag;
		}
	}

	//����ת�ƺ��гɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("SET_TRANSFERCALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());


/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//�ͷ�xml�ṹ
	switch_xml_free(x_first_user);
	//switch_xml_free(x_second_user);

	switch_safe_free(pDesc);
	switch_safe_free(chFailover);
	return iFunRes;
}

/*�������ĺ���ת������
*/
int CModule_User::unbindTransferCall(inner_CmdReq_Frame *&reqFrame, 
									 inner_ResultResponse_Frame *&inner_RespFrame, 
									 bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	char *default_xmlfile = NULL;
	char *chFailover = NULL, *chTransferID;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	char *pDesc      = NULL;
	bool bRes = false,bBusy=false;
	switch_xml_t x_first_user= NULL, x_second_user= NULL;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;

	//��ȡ�û��ź��û�������:��ѡ����
	string strTranscallType,strFirstUserName,strTransferID;
	string strTmp,strSegName;
	vector<string> vecTransferID;
	int ipos = 0;

	//"transcalltype"
	strParamName = "transcalltype";//ת������
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	//if (0 == strValue.size()) {
	//	//�û���Ϊ��,���ش�����Ϣ
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strTranscallType = "all";//Ŀǰ�����а󶨶����

	//"userfirstname"
	strParamName = "userfirstname";//�û���
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//�û���Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û��ų���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//�û��Ų�����������,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strFirstUserName = strValue;

	//�ж�"����"�Ƿ����
	bRes = isExistedMobUser(strFirstUserName.c_str());
	if (!bRes) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("FIR_NUM_NOT_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());
		goto end_flag;
	}
	
	//�˽ṹ��Ҫ�ͷ�
	x_first_user = getMobDefaultXMLInfo(strFirstUserName.c_str());
	if (!x_first_user) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("MOBUSER_DEFAULT_XMLFILE_NOEXIST");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//mod by xxl 2010-10-22 begin :��Բ�ͬ�����ͷֱ���
	//�ȴ���"æʱ"ת��
	if (isEqualStr("busy",strTranscallType)
		|| isEqualStr("all",strTranscallType)){
			strSegName = "fail_over";

			//�鿴"����"�Ƿ��Ѿ��а󶨵ĺ���
			chTransferID = getMobUserVarAttr(x_first_user, strSegName.c_str());
			if (!chTransferID) {
				strReturnDesc = getValueOfDefinedVar("NOT_BIND_SECNUM");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto night;//��ת��"ҹ��"����
			}
		
			//�����ʽΪ:voicemail:1001,����Ϊû�а�
			strTmp = chTransferID;
			ipos = strTmp.find("voicemail");
			if (0 <= ipos){
				strReturnDesc = getValueOfDefinedVar("NOT_BIND_SECNUM");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto night;//��ת��"ҹ��'����
			}

			//�������ת���ŵ��ļ������޸�
			splitString(chTransferID,CARICCP_COMMA_MARK,vecTransferID);
			for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
				string strID = *it;
				
				//�жϰ󶨵ĺ��Ƿ���Ч
				bRes = isExistedMobUser(strID.c_str());
				if (!bRes) {
					continue;
				}

				//1 �Ƚ��"ת����"��Ӧ�İ󶨹�ϵ,�޸�ת���ŵ�xml�ļ�
				x_second_user = getMobDefaultXMLInfo(strID.c_str());//��������ж���Щ�ظ���!!!
				if (!x_second_user) {
					//ת���ŵ�xml�ļ�������,ֱ�ӵ��ɹ�����
					continue;
				}

				//2 �޸�"ת����"��xml�ļ�����
				chFailover = switch_mprintf("voicemail:%s", strID.c_str());
				setMobUserVarAttr(x_second_user, "accountcode", strID.c_str(), false); //accountcode���Ա���ͬ����һ��
				setMobUserVarAttr(x_second_user, "fail_over", chFailover, false);

				//3 ��д"ת����"��xml�ļ�
				bRes = rewriteMobUserXMLFile(strID.c_str(),x_second_user,strReturnDesc);
				switch_safe_free(chFailover);//�����ͷ�
				if (!bRes){
					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto night;//��ת��"ҹ��'����
				}
				
				//�ͷ�xml�ṹ
				switch_xml_free(x_second_user);

			}//end for

			//3 �����"����"��Ӧ�İ󶨺�,�޸����ŵ�xml�ļ�.�������ת���ŵ��߼�����,��ҪӰ�����ŵ��޸Ĳ���
			chFailover = switch_mprintf("voicemail:%s", strFirstUserName.c_str());
			setMobUserVarAttr(x_first_user, "fail_over", chFailover, false);
						
			bBusy = true;
	}

night:

	//�ٶ�"ҹ��"���ͽ��д���,�򵥴��� :ֱ�Ӷ����ŵ��й��ֶν������
	if (isEqualStr("night",strTranscallType)
		|| isEqualStr("all",strTranscallType)){
			strSegName = "direct_transfer";

			//�鿴"����"�Ƿ��Ѿ��а󶨵ĺ���
			chTransferID = getMobUserVarAttr(x_first_user, strSegName.c_str());
			if (!chTransferID
				&& !bBusy) {
				strReturnDesc = getValueOfDefinedVar("NOT_BIND_SECNUM");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto end_flag;
			}

			//setMobUserVarAttr(x_first_user, "direct_transfer"       , "", false);
			//setMobUserVarAttr(x_first_user, "direct_transfer_period", "", false);

			/************************************************************************/
			//ֱ�ӽ��������Զ�ɾ����,���ٱ���.
			//��̬�����ڴ�Ľṹ,����direct�ڵ�ɾ��,��ʱx_first_user������Ҳ�ᶯ̬�����仯
			switch_xml_t x_user= NULL, x_variables= NULL, x_direct_transfer= NULL, x_direct_transfer_period= NULL;
			x_user = switch_xml_child(x_first_user, "user");
			if (!x_user){
				goto end_flag;
			}
			x_variables = switch_xml_child(x_user, "variables");
			if (!x_variables){
				goto end_flag;
			}
			x_direct_transfer = switch_xml_find_child(x_variables, "variable", "name", "direct_transfer");
			if (x_direct_transfer) {
				switch_xml_remove(x_direct_transfer);
			}
			x_direct_transfer_period = switch_xml_find_child(x_variables, "variable", "name", "direct_transfer_period");
			if (x_direct_transfer_period) {
				switch_xml_remove(x_direct_transfer_period);
			}
	}
	//mod by xxl 2010-10-22 end


	//��д"����"��xml�ļ�
	bRes = rewriteMobUserXMLFile(strFirstUserName.c_str(),x_first_user,strReturnDesc);
	if (!bRes){
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	switch_safe_free(chFailover);

	//////////////////////////////////////////////////////////////////////////
	//���¼���root xml�ļ��Ľṹ
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), default_xmlfile);
			goto end_flag;
		}
	}

	//����ת�ƺ��гɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("UNBIND_TRANSFERCALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//�ͷ�xml�ṹ
	switch_xml_free(x_first_user);

	switch_safe_free(pDesc);
	switch_safe_free(chFailover);
	return iFunRes;
}

/*��ѯ��ǰ����ת�Ƶ���Ϣ
*/
int CModule_User::lstTransferCallInfo(inner_CmdReq_Frame*& reqFrame, 
									  inner_ResultResponse_Frame*& inner_RespFrame)
{
    int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	char *strTableName = "";//��ѯ�ı�ͷ
	switch_xml_t defaultGroupXMLinfo = NULL,x_users = NULL, x_user = NULL;

	const char *userid = NULL,*accountcode=NULL,*busycode=NULL,*nightcode=NULL,*timeseg=NULL;
	const char *outMsg[1] = { 0 };
	char *pDesc = NULL,*result=NULL;
	string strType,strTransUserType,strTimeSlot,strGateway;
	string strValue,strRecord,strTmp;
	string strTransferUser,strTimeSeg;
	vector<string> vecTransferInfo,vec;
	int ipos = -1;
	string tableHeader[6] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("transcalltype"),      //ת������
		getValueOfDefinedVar("userfirstname"),      //����
		getValueOfDefinedVar("usersecname"),        //ת����
		getValueOfDefinedVar("USER_TRANS_USERTYPE"),//ת�����û�����
		getValueOfDefinedVar("TIMESLOT")            //ʱ���
	};
	int iRecordCout = 0;//��¼����

	//////////////////////////////////////////////////////////////////////////
	//��ѯdefault���Ӧ�������û�����Ϣ
	defaultGroupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, defaultGroupXMLinfo, strReturnDesc);
	if (defaultGroupXMLinfo) {
		//�������е��û�,����users�ڵ�
		if ((x_users = switch_xml_child(defaultGroupXMLinfo, "users"))) {
			//����ÿ���û�
			x_user = switch_xml_child(x_users, "user");
			for (; x_user; x_user = x_user->next) {
				//�ֱ��ѯ"æʱ"ת����"ҹ��"��������
				
				//////////////////////////////////////////////////////////////////////////
				//1 ����"æת"��¼
				//����1 :���Ը���id��accountcode�����ж��Ƿ�󶨵ĺ���ת��,Ŀǰ���ô��ַ�ʽ���д���
				userid = switch_xml_attr(x_user, "id");
				accountcode = getMobUserVarAttr(x_user,"accountcode",true);
				if (switch_strlen_zero(userid)) {
					continue;
				}
				if (switch_strlen_zero(accountcode)) {
					continue;
				}
				////���id��accountcode����ͬ,��϶���"æ��"ת�ư�
				////���ִ���ʽ�Ƚϼ�.
				//if (strcasecmp(userid,accountcode)){
				//	strTranscallType = getValueOfDefinedVar("USER_BUSY_TRANSFER");

				//	//���ź�ת����
				//	result = switch_mprintf("%s%s%s%s%s%s%s%s%s%s", 
				//		accountcode,   
				//		CARICCP_SPECIAL_SPLIT_FLAG,
				//		strTranscallType.c_str(),
				//		CARICCP_SPECIAL_SPLIT_FLAG,
				//		userid,
				//		CARICCP_SPECIAL_SPLIT_FLAG,
				//		"",
				//		CARICCP_SPECIAL_SPLIT_FLAG,
				//		"",
				//		CARICCP_ENTER_KEY);//ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����

				//	//mod by xxl 2010-5-27 :������ŵ���ʾ
				//	iRecordCout++;//��ŵ���
				//	strRecord = "";
				//	strValue = intToString(iRecordCout);
				//	strRecord.append(strValue);
				//	strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);

				//	strRecord.append(result);

				//	//��ż�¼
				//	inner_RespFrame->m_vectTableValue.push_back(/*result*/strRecord);
				//	//mod by xxl 2010-5-27 end

				//	//�ͷ��ڴ�
				//	switch_safe_free(result);
				//}

				//����2 :���Ը���id��fail_over�����Խ����ж�

				//////////////////////////////////////////////////////////////////////////
				//1 �Ȳ���æʱת��
				busycode = getMobUserVarAttr(x_user,"fail_over",true);
				if (switch_strlen_zero(busycode)) {
					goto night;//��ת��"ҹת"�鿴��
				}
				strTmp = busycode;
				//�����ʽΪ:voicemail:1001,����æʱת�������
				ipos = strTmp.find("voicemail");
				if (0 <= ipos){
					goto night;//��ת��"ҹת"�鿴��
				}

				strTimeSeg="";
				vecTransferInfo.clear();
				getTransferInfo(accountcode,busycode,0,vecTransferInfo);
				for (vector<string>::iterator it = vecTransferInfo.begin(); vecTransferInfo.end() != it; it++){
					string strInfo = *it;

					//���ź�ת����
					result = switch_mprintf("%s%s%s%s", 
						strInfo.c_str(),
						CARICCP_SPECIAL_SPLIT_FLAG,
						strTimeSeg.c_str(),
						CARICCP_ENTER_KEY);//ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����


					iRecordCout++;//��ŵ���
					strRecord = "";
					strValue = intToString(iRecordCout);
					strRecord.append(strValue);
					strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
					strRecord.append(result);

					//��ż�¼
					inner_RespFrame->m_vectTableValue.push_back(/*result*/strRecord);

					//�ͷ��ڴ�
					switch_safe_free(result);
				}

night:
				//////////////////////////////////////////////////////////////////////////
				//2 �ٲ���"ҹת"��¼
				nightcode = getMobUserVarAttr(x_user,"direct_transfer",true);
				if (switch_strlen_zero(nightcode)) {
					continue;
				}
				
				//ʱ���
				timeseg = getMobUserVarAttr(x_user,"direct_transfer_period",true);
				if (switch_strlen_zero(timeseg)) {
					strTimeSeg="";
				}
				else{
					strTimeSeg=timeseg;
				}

				vecTransferInfo.clear();
				getTransferInfo(accountcode,nightcode,1,vecTransferInfo);
				for (vector<string>::iterator it = vecTransferInfo.begin(); vecTransferInfo.end() != it; it++){
					string strInfo = *it;

					//���ź�ת����
					result = switch_mprintf("%s%s%s%s", 
						strInfo.c_str(),
						CARICCP_SPECIAL_SPLIT_FLAG,
						strTimeSeg.c_str(),
						CARICCP_ENTER_KEY);//ÿ�м�¼֮��ʹ��CARICCP_ENTER_KEY����


					iRecordCout++;//��ŵ���
					strRecord = "";
					strValue = intToString(iRecordCout);
					strRecord.append(strValue);
					strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
					strRecord.append(result);

					//��ż�¼
					inner_RespFrame->m_vectTableValue.push_back(/*result*/strRecord);

					//�ͷ��ڴ�
					switch_safe_free(result);
				}

			}//����users
		}
	}
	else{
		if (0 != strReturnDesc.length()){
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

	//û�м�¼���ڵ������
	if (0 == inner_RespFrame->m_vectTableValue.size()){
		strReturnDesc = getValueOfDefinedVar("FIR_SEC_BIND_NULL");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s", //��ͷ
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[1].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[2].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[3].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[4].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[5].c_str());

	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("FIR_SEC_BINDINFO");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

	//���ñ�ͷ�ֶ�
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);

/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/************************************************************************/
/* ���ӵ���Ա                                                           */
/************************************************************************/
int CModule_User::addDispatcher(inner_CmdReq_Frame*& reqFrame, 
								inner_ResultResponse_Frame*& inner_RespFrame,
								bool bReLoadRoot)
{

	simpleParamStruct paramStruct;
	paramStruct.strParaName="dispatopt";
	paramStruct.strParaValue="add_dispat";//��ӱ�ʶ

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setDispatcher(reqFrame,inner_RespFrame,true);
}

/************************************************************************/
/* ɾ������Ա                                                           */
/************************************************************************/
int CModule_User::delDispatcher(inner_CmdReq_Frame*& reqFrame, 
								inner_ResultResponse_Frame*& inner_RespFrame,
								bool bReLoadRoot)
{

	simpleParamStruct paramStruct;
	paramStruct.strParaName="dispatopt";
	paramStruct.strParaValue="del_dispat";//ɾ����ʶ

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setDispatcher(reqFrame,inner_RespFrame,true);

}

/*���õ���Ա,���ӻ�ɾ��
*/
int CModule_User::setDispatcher(inner_CmdReq_Frame*& reqFrame, 
								inner_ResultResponse_Frame*& inner_RespFrame,
								bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	switch_xml_t x_dispatchers = NULL,x_dispatcher = NULL/*,x_user*/;
	int userNum = 0,validUserNum = 0,existedDiapatNum = 0;//����������û�����Ч���������Ѿ��ǵ���Ա
	char *users[100] = {
		0
	};
	char *pDesc = NULL,*xmlfilecontext = NULL,*dispatFile = NULL;
	bool bRes,bAddOrDel;
	int i = 0;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	switch_event_t *event;
	//vector<string>vecPriUser;
	//int iDispatcherPriority = USERPRIORITY_SPECIAL; //����Ա��Ȩ��
	//string strDispatcherPriority="";

	//�Ƚ��в�����У��
	string strParamName,strValue/*,strUser*/,strChinaName;

	//��ѡ����
	string strDispatchopt,strDispatcherID;

	//"dispatopt"
	strParamName = "dispatopt";//���ӻ���ɾ������
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//����Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("add_dispat",strValue)){
		bAddOrDel = true;
	}
	else{
		bAddOrDel = false;//��������¶���ʶΪ"ɾ��"
	}
	strDispatchopt= strValue;

	//"dispatcherID"
	strParamName = "dispatcherID";//����Ա��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//����Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//���ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strDispatcherID = strValue;

	//����������û���,�ֽ�ÿ���û�
	userNum = switch_separate_string((char*) strDispatcherID.c_str(), CARICCP_CHAR_COMMA_MARK, users, (sizeof(users) / sizeof(users[0])));
	x_dispatchers = getAllDispatchersXMLInfo();//��õ�ǰ�Ľṹ
	if (!x_dispatchers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), CARI_CCP_DISPAT_XML_FILE);
		goto end_flag;
	}
	//���ζ�ÿ���û����д���
	for (i = 0; i < userNum; i++) {
		//�Ȳ鿴���û��Ƿ��Ѿ�����
		if (!isExistedMobUser(users[i])){
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST_1");
			pDesc = switch_mprintf(strReturnDesc.c_str(), users[i]);
			goto end_flag;
		}
		validUserNum++;

		//�鿴���û��Ƿ��Ѿ��ǵ���Ա
		x_dispatcher = getDispatcherXMLInfo(users[i],x_dispatchers);
		if (x_dispatcher){
			existedDiapatNum++;	//�Ѿ����ڵ��û�����
			if (!bAddOrDel){//�����ɾ������
				
				//ɾ���˵���Ա
				switch_xml_remove(x_dispatcher);

				//mod by xxl 2010-6-24:���û���Ȩ��ҲҪͬ���޸�,����Ϊһ��Ȩ��:USERPRIORITY_OUT
				//vecPriUser.push_back(users[i]);
				//mod by xxl 2010-6-24 end
			}
			continue;
		}

		if (bAddOrDel){//��������Ӳ���
			//�����û�Ϊ����Ա
			setdispatcherVarAttr(x_dispatchers,users[i],true);

			//mod by xxl 2010-6-24:����Ա��Ȩ��ҲҪͬ���޸�,����Ϊ���Ȩ��:USERPRIORITY_SPECIAL
			//vecPriUser.push_back(users[i]);
			//mod by xxl 2010-6-24 end
		}

	}//�������е��û�,�������

	if (bAddOrDel){//��������Ӳ���
		//�û��Ŷ�����Ч��
		if (0 == validUserNum) {
			strReturnDesc = getValueOfDefinedVar("ADD_DISPAT_FAILED");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
		//���������û��Ŷ��Ѿ������д�����,ֱ�ӷ��سɹ�
		if (validUserNum == existedDiapatNum) {
			goto no_reload;
		}
	}
	else{
		//���������û��Ŷ����ǵ���Ա
		if (0 == existedDiapatNum) {
			strReturnDesc = getValueOfDefinedVar("DEL_DISPAT_FAILED");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
	}
	//���½��ṹд�뵽�ļ�
	dispatFile = getDispatFile();
	if (dispatFile) {
		xmlfilecontext = switch_xml_toxml(x_dispatchers, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(dispatFile, xmlfilecontext, err);
		switch_safe_free(dispatFile);//�����ͷ�
		if (!bRes) {
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

	//mod by xxl 2010-6-24:����Ա��Ȩ��ҲҪͬ���޸�,�޸��û���"�����ļ�",���������,����Ϊ���Ȩ��,
	//�����ɾ��,����Ϊһ���Ȩ��
	//if (bAddOrDel){
	//	strDispatcherPriority = intToString(USERPRIORITY_SPECIAL);
	//}
	//else{
	//	strDispatcherPriority = intToString(USERPRIORITY_OUT);
	//}
	//for (vector<string>::iterator it = vecPriUser.begin();it != vecPriUser.end();it++){
	//	strUser = *it;

	//	//�޸�defaultĿ¼�¶�Ӧ���û��ŵ�xml�ļ���Ȩ������
	//	x_user = getMobDefaultXMLInfo(strUser.c_str());
	//	if (!x_user) {
	//		continue;
	//	}
	//	//���ö�Ӧ��"priority"������,��߼���"USERPRIORITY_OUT",x_user�����ݻᱻ�޸�
	//	setMobUserVarAttr(x_user, "priority", strDispatcherPriority.c_str(), true);

	//	//��д�ļ�
	//	bRes = rewriteMobUserXMLFile(strUser.c_str(),x_user,strReturnDesc);

	//	//�ͷ�
	//	switch_xml_free(x_user);
	//}
	//mod by xxl 2010-6-24 end


	//mod by xxl 2009-5-18: ������������
	//���¼���root xml�ļ�.��Ϊ����ģ���Ǵ��ڴ���ֱ�Ӷ�ȡ
	if (bReLoadRoot) {
		//ֱ�ӵ��û��������,��Ϊû��ֱ���ͷ�!!!
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
	}
	//mod by xxl 2009-5-18 end

	//////////////////////////////////////////////////////////////////////////
	//���ں͵���ģ���໥����,�˴������¼�
	if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
		if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
			switch_event_destroy(&event);
		}
	}

	/*------*/
no_reload:

	//����ִ�гɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	if (bAddOrDel){
		strReturnDesc = getValueOfDefinedVar("ADD_DISPAT_SUCCESS");
	}
	else{
		strReturnDesc = getValueOfDefinedVar("DEL_DISPAT_SUCCESS");
	}
	pDesc = switch_mprintf(strReturnDesc.c_str());

	/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//�ͷ�xml�ṹ
	if (bAddOrDel){//��������Ӳ���
		switch_xml_free(x_dispatchers);
	}
	switch_safe_free(pDesc);
	return iFunRes;

}
/*��ѯ����Ա
*/
int CModule_User::lstDispatcher(inner_CmdReq_Frame*& reqFrame,
								inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "",strname ;

	char *pDesc = NULL;
	char *strTableName = "";//��ѯ�ı�ͷ;
	string strAll = "";
	string tableHeader[2] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("USER_ID"),
	};

	//������еĵ���Ա�ִ���Ϣ
	getAllDispatchers(strAll);
	if (0 == strAll.length()) {
		strReturnDesc = getValueOfDefinedVar("DISPAT_RECORD_NULL");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	splitString(strAll,CARICCP_ENTER_KEY,inner_RespFrame->m_vectTableValue);

	//��ͷ��Ϣ����
	strTableName = switch_mprintf("%s%s%s", 
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[1].c_str());
	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("DISPAT_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

	//���ñ�ͷ�ֶ�
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);
	
/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/************************************************************************/
/*����¼���û�                                                          */
/************************************************************************/
	 
int CModule_User::addRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot)
{
	simpleParamStruct paramStruct;
	paramStruct.strParaName="recordopt";
	paramStruct.strParaValue="add_record";//���ӱ�ʶ

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setRecord(reqFrame,inner_RespFrame);
}

/************************************************************************/
/*ɾ��¼���û�                                                          */
/************************************************************************/
int CModule_User::delRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot)
{
	simpleParamStruct paramStruct;
	paramStruct.strParaName="recordopt";
	paramStruct.strParaValue="del_record";//ɾ����ʶ

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setRecord(reqFrame,inner_RespFrame);
}

/*����¼���û�
*/
int CModule_User::setRecord(inner_CmdReq_Frame*& reqFrame, 
							inner_ResultResponse_Frame*& inner_RespFrame,
							bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	switch_xml_t x_user = NULL;
	int userNum = 0;
	char *users[100] = {
		0
	};
	char *pDesc = NULL,*xmlfilecontext = NULL,*dispatFile = NULL;
	bool bRes,bSetOrCancle;
	string strFlag = "no";
	int i = 0,ivalidnum = 0;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;

	//�Ƚ��в�����У��
	string strParamName, strValue, strChinaName;//������������ʾ��

	//��ѡ����
	string strRecordopt,strRecordID;

	//"recordopt"
	strParamName = "recordopt";//����¼������ȡ��¼��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//����Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("add_record",strValue)){
		strFlag = "yes";//����¼��
		bSetOrCancle = true;
	}
	else{
		strFlag = "no";//ȡ��¼��
		bSetOrCancle = false;
	}
	strRecordopt = strValue;

	//"recorduser"
	strParamName = "recorduser";//¼���û�
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//����Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_256 < strValue.length()) {
		//���ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strRecordID = strValue;

	//����������û���,�ֽ�ÿ���û�
	userNum = switch_separate_string((char*) strRecordID.c_str(), CARICCP_CHAR_COMMA_MARK, users, (sizeof(users) / sizeof(users[0])));
	//���ζ�ÿ���û����д���
	for (i = 0; i < userNum; i++) {
		char *recordallow = NULL;

		//�˽ṹ��Ҫ�ͷ�
		x_user = getMobDefaultXMLInfo(users[i]);
		//�Ȳ鿴���û��Ƿ��Ѿ�����
		if (!x_user){
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), users[i]);
			goto end_flag;
		}

		//�鿴�������Ƿ���������Ҫ�޸�
		recordallow = getMobUserVarAttr(x_user,"record_allow");
		if (!switch_strlen_zero(recordallow)
			&&isEqualStr(recordallow,strFlag)){

			continue;
		}
	
		//�������/�޸����������
		setMobUserVarAttr(x_user, "record_allow", strFlag.c_str(),true);//��ʶΪyes��no

		//��д�û���xml�ļ�
		bRes = rewriteMobUserXMLFile(users[i],x_user,strReturnDesc);
		if (!bRes) {
			continue;
		}

		ivalidnum++;//��Ч�����޸�

		//xml�ṹ�ͷ�
		switch_xml_free(x_user);

	}//�������е��û�,�������

	//////////////////////////////////////////////////////////////////////////
	//���¼���root xml�ļ��Ľṹ
	if (0 != ivalidnum 
		&&bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strRecordID.c_str());//���û�������ļ���
			goto end_flag;
		}
	}

	//����¼���û��ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//������Ϣ�����·�װһ��
	if (bSetOrCancle){
		strReturnDesc = getValueOfDefinedVar("SET_RECORD_SUCCESS");
	}
	else{
		strReturnDesc = getValueOfDefinedVar("CANCLE_RECORD_SUCCESS");
	}
	
	pDesc = switch_mprintf(strReturnDesc.c_str());

	/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*��ѯ¼���û�
*/
int CModule_User::lstRecord(inner_CmdReq_Frame*& reqFrame,
							inner_ResultResponse_Frame*& inner_RespFrame)
 {
	 int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	 int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;

	 //�������
	 char *pDesc          = NULL,*strTableName=NULL;
	 const char *userid = NULL;
	 switch_xml_t groupXMLinfo = NULL,x_users = NULL, x_user = NULL;
	 //char *strResult = "";	//�����˶�����������,�ڴ�û���ͷŵ�.ʹ��memset()
	 string strResult = "",strReturnDesc,strSeq;
	 string tableHeader[2] = {
		 getValueOfDefinedVar("SEQUENCE"),
		 getValueOfDefinedVar("USER_ID")
	 };
	 int iRecordCout = 0;//��¼����

	 //////////////////////////////////////////////////////////////////////////
	 //��ѯdefault���Ӧ�������û�����Ϣ
	 groupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, groupXMLinfo, strReturnDesc);
	 if (groupXMLinfo) {
		 if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {
			 //����ÿ���û�
			 x_user = switch_xml_child(x_users, "user");
			 for (; x_user; x_user = x_user->next) {
				 char *recordallow = NULL;
				 userid = switch_xml_attr(x_user, "id");
				 recordallow = getMobUserVarAttr(x_user,"record_allow",true);
				 if (!switch_strlen_zero(userid)
					 &&NULL != recordallow 
					 &&isEqualStr(recordallow,"yes")){

						 //mod by xxl 2010-5-27 :������ŵ���ʾ
						 iRecordCout++;//��ŵ���
						 strSeq = intToString(iRecordCout);
						 strResult.append(strSeq);
						 strResult.append(CARICCP_SPECIAL_SPLIT_FLAG);
						 //mod by xxl 2010-5-27 end

						 strResult.append(userid);
						 strResult.append(CARICCP_ENTER_KEY);
				 }
			 }
		 }
	 }
	 else{
		 pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		 goto end_flag;
	 }
	 if (0 == strResult.length()){
		 strReturnDesc = getValueOfDefinedVar("RECORD_RECORD_NULL");
		 pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		 goto end_flag;
	 }
	 splitString(strResult,CARICCP_ENTER_KEY,inner_RespFrame->m_vectTableValue);

	 //������Ϣ���ͻ���,���������·�����Ϣ
	 //��ͷ��Ϣ����
	 strTableName = switch_mprintf("%s%s%s", 
		 tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		 tableHeader[1].c_str());
	 iFunRes = CARICCP_SUCCESS_STATE_CODE;
	 iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	 strReturnDesc = getValueOfDefinedVar("RECORD_RECORD");
	 pDesc = switch_mprintf(strReturnDesc.c_str());

	 //���ñ�ͷ�ֶ�
	 myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	 switch_safe_free(strTableName);

/*-----*/
end_flag : 
	 inner_RespFrame->header.iResultCode = iCmdReturnCode;
	 myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	 switch_safe_free(pDesc);
	 return iFunRes;
 }

/************************************************************************/
/*								�������� 								*/
/************************************************************************/

/*�����û���ID���ж��Ƿ�(ע��)��Ч
*/
bool CModule_User::isLocalMobUser(string strUserID)
{
#ifdef CARI_ODBC

	//	switch_odbc_handle_t *odbc_handle = NULL;
	//	SQLHSTMT stmt  = NULL;
	//	char *sql =NULL;
	//	char *errorMsg = NULL;
	//
	//	SQLINTEGER mobcount=0;
	//
	//	odbc_handle = cari_net_getOdbcHandle();
	//	if (NULL == odbc_handle)
	//	{
	//		return false;
	//	}
	//
	//	/*string sql = switch_mprintf("%s%s","select count(userid) from authusers where userid = ",userid);*/
	//	sql =  
	//		switch_mprintf("select count(userid) from %s where userid = '%s'",
	//		AUTHUSERS,
	//		strUserID.c_str());//����ʹ��c_str(),�������
	//
	//
	//	//��������ݿ�����Ӿ��profile->master_odbcʧ��,�Ƿ�������???
	//	if (SWITCH_ODBC_SUCCESS != switch_odbc_handle_exec(odbc_handle, sql, &stmt)) 
	//	{
	//		errorMsg = switch_odbc_handle_get_error(odbc_handle, stmt);
	//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, switch_str_nil(errorMsg));
	//
	//		//�ͷ��ڴ�
	//		switch_safe_free(errorMsg);
	//		goto end;
	//	}
	//
	//	while(SQL_NO_DATA!= SQLFetch(stmt)) 
	//	{
	//		SQLGetData(stmt, 1, SQL_C_SLONG,  &mobcount, 0, NULL);
	//		break;
	//	}  
	//
	//
	//end:
	//	//�ͷ��ڴ�,�ɵ����߸����ͷ�
	//	if (NULL != stmt)
	//	{
	//		SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	//	}
	//
	//	if (0 == mobcount)
	//	{
	//		return false;
	//	}

#else


#endif

	return true;
}

/*�鿴���û��Ƿ��Ѿ�����
*@param userid :�û�����(��)
*/
bool CModule_User::isExistedMobUser(const char *userid)
{
	switch_status_t iFunRes = SWITCH_STATUS_SUCCESS;

	//��β����û�����Ϣ???���������?����������,��β���?
	const char *domain_name, *group_name, *group_id,*mobuserid;
	switch_xml_t xml = NULL, group = NULL;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users,x_user;
	switch_event_t *params = NULL;
	bool bExsited = false;

	//�û���(��)�ж�,�Ǳ�Ҫ
	if (switch_strlen_zero(userid)) {
		goto end;
	}
	//����profile�����Ի�ö�Ӧ������
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//ע�� :���������������Ǻ��ϸ�,��ø���default�������в����û��Ƿ����
	//ʹ������ķ���,�������������"��/ɾ"�û�������??!!!why???
	//iFunRes = switch_xml_locate_user("id", userid, domain_name, NULL, &xml, &domain, &user, &group, params);

	//�Ȳ���domain,�˴��ĵ��û�����ڴ�,����Ҫ�ͷ�
	iFunRes = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		goto end;
	}
	//����xml�ļ�,�ּ����Ҷ�Ӧ����Ϣdomain-->groups-->group->users->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		//����ÿ����
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");
			group_id = switch_xml_attr(x_group, "id");
			//default��
			if (NULL != group_name 
				&& isEqualStr(group_name, DEFAULT_GROUP_NAME)){
				
				//����users�ڵ�
				if ((x_users = switch_xml_child(x_group, "users"))) {
					//����ÿ���û�
					x_user = switch_xml_child(x_users, "user");
					for (; x_user; x_user = x_user->next) {
						mobuserid = switch_xml_attr(x_user, "id");
						if (switch_strlen_zero(userid)) {
							continue;
						}
						//���ڴ��û�
						if (!strcasecmp(userid, mobuserid)) {
							bExsited = true;
							goto end;
						}
					}//����users
				}
			}
		}
	}
	
end:

	//��ȡ���root�ڴ�ṹ,RWLOCK������ס,�����Ҫ�ͷ�
	//�˴�ʵ�������ͷ�����RWLOCK,�������������ͷ���root��xml�ṹ
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain()����,��˱���Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	//����ֵΪbool����
	return bExsited;
}

/*�鿴���û��Ƿ��Ѿ�ע��
*/
bool CModule_User::isRegisteredMobUser(const char* userID,
								  switch_core_db_t *db)
{
	bool ret = false;

	//Ŀǰʹ��������ʽ���в�ѯ
	char sql[256];
	switch_snprintf(sql, sizeof(sql),
		"select count(sip_user) from sip_registrations where sip_user = '%s'",
		userID);

	char *errmsg = NULL;
	char **resultp = NULL;
	 vector<string> resultVec;
	int nrow;
	int ncolumn;

	if (NULL == userID 
		|| NULL == db){
		return ret;
	}

#ifdef SWITCH_HAVE_ODBC
#else
#endif

	int iRes = getRecordTableFromDB(db,sql,resultVec,&nrow,&ncolumn, NULL);
	if (CARICCP_SUCCESS_STATE_CODE != iRes){
		return ret;
	}

	//���ݲ�ѯ�ļ�¼�鿴�Ƿ���ڴ�"ע��"�û�
	string strRes = getValueOfVec(0,resultVec);
	int iNum = stringToInt(strRes.c_str());
	if (0 != iNum){
		ret = true;
	}

	return ret;
}

/*�鿴���Ƿ����,��Ҫ��root xml�ڴ��н��в���
*/
bool CModule_User::isExistedGroup(const char *mobgroupname, 
								  int groupid)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	bool bExsited = false;
	switch_event_t *params = NULL;
	const char *domain_name, *group_name, *group_id;
	int mobgroupid = -1;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;
	int iMaxLen = 0;
	if (!switch_strlen_zero(mobgroupname)) {
		iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, mobgroupname);
		//�����Ĭ�ϵ��鲻��Ҫ����
		if (!strncasecmp(DEFAULT_GROUP_NAME, mobgroupname, iMaxLen)) {
			goto end;
		}
	}
	
	//����
	domain_name = getDomainIP();

	//�Ȳ���domain,�˴��ĵ��û�����ڴ�,����Ҫ�ͷ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ����Ϣdomain-->groups-->group
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		//����ÿ����
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");
			group_id = switch_xml_attr(x_group, "id");

			if (NULL != group_name 
				&& (!switch_strlen_zero(mobgroupname))) {
				if (isEqualStr(group_name, mobgroupname)){
				//if (!strcasecmp(group_name, mobgroupname)) {//������ͬ,�����
					bExsited = true;
					break;
				}
			}
			//���ʹ��Ĭ��ֵ,�򲻿�����ŵ�����
			if (-1 == groupid) {
				continue;
			}
			if (NULL != group_id) {
				mobgroupid = stringToInt(group_id);        //�����ͬ,�����
				if (mobgroupid == groupid) {
					bExsited = true;
					break;
				}
			}//end ������ͬ����
		}
	}

end:
	//��ȡ���root�ڴ�ṹ,RWLOCK������ס,�����Ҫ�ͷ�
	//�˴�ʵ�������ͷ�����RWLOCK,�������������ͷ���root��xml�ṹ
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return bExsited;
}

/*������
*/
string CModule_User::getGroupID(const char* mobgroupname)
{
	string strGID = "";
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;
	const char *domain_name, *group_name, *group_id;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;
	int iMaxLen = 0;

	if (switch_strlen_zero(mobgroupname)) {
		goto end;
	}
	iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, mobgroupname);
	//�����Ĭ�ϵ��鲻��Ҫ����
	if (!strncasecmp(DEFAULT_GROUP_NAME, mobgroupname, iMaxLen)) {//isEqualStr()
		goto end;
	}

	//����
	domain_name = getDomainIP();

	//�Ȳ���domain,����Ҫ�ͷŷ�����ڴ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}
	//����xml�ļ�,�ּ����Ҷ�Ӧ����Ϣdomain-->groups-->group
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		//����ÿ����
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");
			group_id = switch_xml_attr(x_group, "id");
			if (NULL != group_name) {
				if (isEqualStr(group_name, mobgroupname)){
					//if (!strcasecmp(group_name, mobgroupname)) {//������ͬ,�����
					strGID = group_id;
					break;
				}
			}
		}
	}

end:
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return strGID;

}

/*�ַ�����ȥ��delStr��ʣ�µ��ִ�
*/
string CModule_User::getNewStr(const char* oldStr,const char* delStr)
{
	if (NULL == oldStr){
		return "";
	}

	vector<string> vecG;
	vecG.clear();
	splitString(oldStr,",",vecG);

	//�����Ƿ���ڴ���
	string strNewG;
	string strT;
	for (vector<string>::iterator it = vecG.begin();it != vecG.end();it++){
		strT = *it;
		if (isEqualStr(strT,delStr)){
			continue;
		}
		strNewG.append(strT);
		strNewG.append(",");
	}
	if (0 !=strNewG.length()){
		strNewG = strNewG.substr(0,strNewG.length()-1);
	}

	return strNewG;
}

/*����û��������Ϣ,Ĭ�ϵ��鲻��Ҫ����
*/
void CModule_User::getMobGroupOfUserFromMem(const char *mobuserid, 
											char **allMobGroup)//������� :�û������Ϣ
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, *userid;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users = NULL,x_user = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(mobuserid)) {
		goto end;
	}

	//����
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//�Ȳ���domain
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ���û�domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			//default����Ҫ����
			iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, group_name);

			//�����Ĭ��default���鲻��Ҫ����
			if (!strncasecmp(DEFAULT_GROUP_NAME, group_name, iMaxLen)) {
				continue;
			}

			//����users�ڵ�
			if ((x_users = switch_xml_child(x_group, "users"))) {
				//����ÿ���û�
				x_user = switch_xml_child(x_users, "user");
				for (; x_user; x_user = x_user->next) {
					userid = switch_xml_attr(x_user, "id");
					if (switch_strlen_zero(userid)) {
						continue;
					}

					//���ڴ��û�
					if (!strcasecmp(userid, mobuserid)) {
						//�����Ϣ��������
						//*allMobGroup = (char *)switch_xml_attr(x_group,"name");

						//��ʱ�����Ƿ��Ӱ��ֵ???
						*allMobGroup = (char*) group_name;//��Ϊ��ֵ�Ǵ��ڴ���ȡ��,������Ч

						//λ�ú��ƶ�һλ
						allMobGroup ++;

						break;
					}
				}//����users
			}
		}//������groups
	}

	end:
	if (root) {
		switch_xml_free(root);//����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return;
}

/*���ĳ����������û�����Ϣ,��root xml�Ľṹ�н��в���
*/
void CModule_User::getMobUsersOfGroupFromMem(const char *mobgroupname, 
											 char **allMobUsersInGroup)//������� :ĳ����������û���Ϣ
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, *userid;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users = NULL,x_user = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(mobgroupname)) {
		goto end;
	}

	//����
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//�Ȳ���domain,��Ϊ�������ڴ�,����Ҫ�ͷ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ���û�domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			iMaxLen = CARICCP_MAX_STR_LEN(mobgroupname, group_name);

			//���ҵ���Ӧ����,�����鲻����
			if (strncasecmp(mobgroupname, group_name, iMaxLen)) {
				continue;
			}

			//����users�ڵ�
			if ((x_users = switch_xml_child(x_group, "users"))) {
				//����ÿ���û�
				x_user = switch_xml_child(x_users, "user");
				for (; x_user; x_user = x_user->next) {
					userid = switch_xml_attr(x_user, "id");//ʵ���ϴ��ڴ��л��
					if (switch_strlen_zero(userid)) {
						continue;
					}

					//���������û���id��
					*allMobUsersInGroup = (char*) userid;

					//λ�ú��ƶ�һλ
					allMobUsersInGroup ++;
				}//����users
			}
		}//������groups
	}


end:
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return;
}

/*������������Ϣ,��ע:default�鲻����
*/
void CModule_User::getAllGroupFromMem(string &allGroups,//�������
									  bool bshowSeq)//���صļ�¼�Ƿ�Ҫ�����
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, /**group_id,*/ *group_desc;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;

	int iMaxLen = 0;
	int groupNum = 0;
	int iRecordCout = 0;//��¼����
	string strSeq,strRecord;

	//����
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//�Ȳ���domain,��Ϊ�������ڴ�,����Ҫ�ͷ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ���û�domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="g1" desc ="">
			group_name = switch_xml_attr(x_group, "name");
			//group_id = switch_xml_attr(x_group, "id");
			group_desc = switch_xml_attr(x_group, "desc");

			iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, group_name);

			//���ҵ���Ӧ����,default�����
			if (!strncasecmp(DEFAULT_GROUP_NAME, group_name, iMaxLen)) {
				continue;
			}

			//��NULLУ��
			if (group_name) {
				/* *allGroups = (char *)group_name; */

				char *gInfo = NULL;
				//��װ�����Ϣ
				if (group_desc) {
					gInfo/**allGroups*/ = switch_mprintf("%s%s%s", group_name, CARICCP_SPECIAL_SPLIT_FLAG, /*group_id, CARICCP_SPECIAL_SPLIT_FLAG,*/ group_desc);
				} else {
				  	gInfo = switch_mprintf("%s%s%s", group_name, CARICCP_SPECIAL_SPLIT_FLAG, /*group_id, CARICCP_SPECIAL_SPLIT_FLAG,*/ "");
				}

				//mod by xxl 2010-5-27 :������ŵ���ʾ
				if (bshowSeq){
					iRecordCout++;//��ŵ���
					strRecord = "";
					strSeq = intToString(iRecordCout);
					allGroups.append(strSeq);
					allGroups.append(CARICCP_SPECIAL_SPLIT_FLAG);
				}
				//mod by xxl 2010-5-27 end

				allGroups.append(gInfo);
				allGroups.append(CARICCP_ENTER_KEY);
				switch_safe_free(gInfo);//��ʱ�ͷ��ڴ�

				//�������
				groupNum++;
			}
		}//������groups
	}

end:
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return;
}

/*��õ�ǰ�����������.��ע:default�������Ҫ����
*/
int CModule_User::getGroupNumFromMem()
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;

	int iMaxLen = 0;
	int groupNum = 0;

	//����
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//�Ȳ���domain,��Ϊ�������ڴ�,����Ҫ�ͷ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ���û�domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="g1" desc ="">
			group_name = switch_xml_attr(x_group, "name");

			////���ҵ���Ӧ����,default�����
			//iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME,group_name);
			//if (!strncasecmp(DEFAULT_GROUP_NAME,group_name,iMaxLen))
			//{
			//	continue;
			//}

			//��NULLУ��
			if (group_name) {
				//�������
				groupNum++;
			}
		}//������groups
	}

	end:
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return groupNum;
}

/*��������Ϣ,���ڴ��л��
*/
switch_xml_t CModule_User::getGroupXMLInfoFromMem(const char *groupName, 
												  switch_xml_t groupXMLinfo, //������� :���xml��Ϣ
												  string &err)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;
	string strReturnDesc = "";

	const char *domain_name, *group_name;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(groupName)) {
		goto end;
	}

	//����
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		err = getValueOfDefinedVar("INVALID_DOMOMAIN");
		goto end;
	}

	//�Ȳ���domain,��Ϊ�������ڴ�,����Ҫ�ͷ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		err = getValueOfDefinedVar("DOMOMAIN_NO_EXIST");//�򲻴���
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ���û�domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			//ֻ��Ҫ����default������,���������е��û���Ϣ
			iMaxLen = CARICCP_MAX_STR_LEN(groupName, group_name);

			if (!strncasecmp(groupName, group_name, iMaxLen)) {
				groupXMLinfo = x_group;

				break;
			}

			continue;
		}//������groups
	}

end:
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return groupXMLinfo;
}

/*����û���xml��Ϣ
*/
switch_xml_t CModule_User::getMobUserXMLInfoFromMem(const char *mobuserid, 
													switch_xml_t userXMLinfo) //������� :�û���xml��Ϣ
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, *userid;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users = NULL,x_user = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(mobuserid)) {
		goto end;
	}

	//����
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//�Ȳ���domain,��Ϊ�������ڴ�,����Ҫ�ͷ�
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//����xml�ļ�,�ּ����Ҷ�Ӧ���û�domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//���ҵ���Ӧ����,��ʽ :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			//ֻ��Ҫ����default������,���������е��û���Ϣ
			iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, group_name);

			//�����Ĭ��default���鲻��Ҫ����
			if (!strncasecmp(DEFAULT_GROUP_NAME, group_name, iMaxLen)) {
				//����users�ڵ�
				if ((x_users = switch_xml_child(x_group, "users"))) {
					//����ÿ���û�
					x_user = switch_xml_child(x_users, "user");
					for (; x_user; x_user = x_user->next) {
						userid = switch_xml_attr(x_user, "id");

						//���ڴ��û�
						if (!strcasecmp(userid, mobuserid)) {
							userXMLinfo = x_user;

							break;
						}
					}//����users
				}

				break;
			}

			continue;
		}//������groups
	}

end:
	if (root) {
		switch_xml_free(root);//��Ϊ������switch_xml_locate_domain����,����Ҫ�ͷ�(�ͷ��߳���???),�����ؼ��ص�ʱ�������
		root = NULL;
	}

	return userXMLinfo;
}

/*��ĳ�����л��ĳ���û�����Ϣ
*/
switch_xml_t CModule_User::getMobUserXMLInfoInGroup(const char *mobuserid, //�û���
													switch_xml_t groupXMLinfo, //���xml��Ϣ
													switch_xml_t userXMLinfo)  //������� :�û���xml��Ϣ
{
	switch_xml_t x_users = NULL, x_user = NULL;
	const char *userid;

	//����users�ڵ�
	if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {
		//����ÿ���û�
		x_user = switch_xml_child(x_users, "user");
		for (; x_user; x_user = x_user->next) {
			userid = switch_xml_attr(x_user, "id");
			if (switch_strlen_zero(userid)) {
				continue;
			}
			//���ڴ��û�
			if (!strcasecmp(userid, mobuserid)) {
				userXMLinfo = x_user;

				break;
			}
		}//����users
	}

	return userXMLinfo;
}

/*���defaultĿ¼���û�Ĭ�ϵ�xml�ṹ,��xml�ļ���ֱ�ӻ��
 *���ص�xml�ṹ�����ͷ�,�ɵ��õĺ�������
*/
switch_xml_t CModule_User::getMobDefaultXMLInfo(const char *mobuserid)
{
	switch_xml_t x_user = NULL;
	char *user_default_xmlfile = NULL,*file = NULL;

	//�޸�defaultĿ¼�¶�Ӧ���û��ŵ�xml�ļ����������
	//����(O_CREAT)�µ�MOB�û��ļ�
	file = getMobUserDefaultXMLPath();
	if (file) {
		user_default_xmlfile = switch_mprintf("%s%s%s", file, mobuserid, CARICCP_XML_FILE_TYPE);
		switch_safe_free(file);
	}
	if (!user_default_xmlfile) {
		goto end;
	}

	//�˽ṹ����Ҫ�ͷ�,�ɵ��ú�������
	x_user = cari_common_parseXmlFromFile(user_default_xmlfile);
	switch_safe_free(user_default_xmlfile);

end : 
	return x_user;
}

/*���ļ��л�����õ���Ա��xml��Ϣ
 *���ص�xml�ṹҪ�ͷ�.
*/
switch_xml_t CModule_User::getAllDispatchersXMLInfo()
{
	switch_xml_t x_dispatusers = NULL;
	char *file = getDispatFile();
	if (file) {
		//�˽ṹ����Ҫ�ͷ�,�ɵ��ú�������
		x_dispatusers = cari_common_parseXmlFromFile(file);
		switch_safe_free(file);//�ͷ�
	}
	return x_dispatusers;
}

/*���һ��������Ա��xml�ṹ
*/
switch_xml_t CModule_User::getDispatcherXMLInfo(const char* dispatcherid,
												switch_xml_t x_all)
{
	switch_xml_t x_dispatusers = NULL, x_dispatuser = NULL;
	const char *userid = NULL;

	//����ÿ��������Ա
	x_dispatusers = switch_xml_child(x_all, "dispatchers");
	if (!x_dispatusers){
		goto end;
	}
	x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
	for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
		userid = switch_xml_attr(x_dispatuser, "id");
		if (switch_strlen_zero(userid)) {
			continue;
		}
		//���ڴ˵�����Ա
		if (!strcasecmp(userid, dispatcherid)) {
			goto end;
		}
	}//����users

end:
	return x_dispatuser;
}

/*������õĵ���Ա��Ϣ
*/
void CModule_User::getAllDispatchers(string &allDispatchers,
									 bool bshowSeq)//�Ƿ�Ҫ��ʾ���
{
	switch_xml_t x_all = NULL, x_dispatusers = NULL, x_dispatuser = NULL;
	const char *userid = NULL;
	string strSeq;
	int iRecordCout = 0;

	//�˽ṹҪ�ͷ�
	x_all = getAllDispatchersXMLInfo();
	if (!x_all){
		return;
	}
	x_dispatusers = switch_xml_child(x_all, "dispatchers");
	if (!x_dispatusers){
		return;
	}

	//����ÿ��������Ա
	x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
	for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
		userid = switch_xml_attr(x_dispatuser, "id");
		if (switch_strlen_zero(userid)) {
			continue;
		}

		//mod by xxl 2010-5-27 :������ŵ���ʾ
		if (bshowSeq){
			iRecordCout++;//��ŵ���
			strSeq = intToString(iRecordCout);
			allDispatchers.append(strSeq);
			allDispatchers.append(CARICCP_SPECIAL_SPLIT_FLAG);
		}
		//mod by xxl 2010-5-27 end

		//���ڴ˵�����Ա
		allDispatchers.append(userid);
		allDispatchers.append(CARICCP_ENTER_KEY);

	}//����users

	//�ͷ�xml�ṹ
	switch_xml_free(x_all);
}

/*�Ƿ���ڵ���Ա
*/
bool CModule_User::isExistedDispatcher(const char* dispatcherid)
{
	switch_xml_t x_dispatuser = NULL,x_dispatusers = NULL,x_all = NULL;
	const char *userid = NULL;
	bool blexist = false;

	//�˽ṹҪ�ͷ�
	x_all = getAllDispatchersXMLInfo();
	if (!x_all) {
		goto end;
	}

	//�����Ƿ���ڴ˵�����Ա
	if ((x_dispatusers = switch_xml_child(x_all, "dispatchers"))) {
		//����ÿ��������Ա
		x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
		for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
			userid = switch_xml_attr(x_dispatuser, "id");
			if (switch_strlen_zero(userid)) {
				continue;
			}
			//���ڴ˵�����Ա
			if (!strcasecmp(userid, dispatcherid)) {
				blexist = true;
				goto end;
			}
		}//����users
	}

end : 
	//�ͷ�xml�ṹ
	switch_xml_free(x_all);

	return blexist;
}

bool CModule_User::isExistedDispatcher(const char* dispatcherid,
									   switch_xml_t x_all)
{
	switch_xml_t x_dispatuser = NULL,x_dispatusers = NULL;
	const char *userid = NULL;
	bool blexist = false;
	if (!x_all) {
		goto end;
	}

	//�����Ƿ���ڴ˵�����Ա
	if ((x_dispatusers = switch_xml_child(x_all, "dispatchers"))) {
		//����ÿ��������Ա
		x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
		for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
			userid = switch_xml_attr(x_dispatuser, "id");
			if (switch_strlen_zero(userid)) {
				continue;
			}
			//���ڴ˵�����Ա
			if (!strcasecmp(userid, dispatcherid)) {
				blexist = true;
				goto end;
			}
		}//����users
	}

end : 
	return blexist;

}

/*����û�����ϸ��Ϣ,�Ѿ��Ƿ�װ�õı�����Ϣ
*/
void CModule_User::getSingleMobInfo(switch_xml_t userXMLinfo,  string &strOut) //������� :�û�����Ϣ
{
	switch_xml_t x_variables, x_variable, x_params, x_param;
	//�û�xml�ļ�����ϸ��Ϣ
	/*
	<params>
		<param name="password" value="$${default_password}" /> 
		<param name="vm-password" value="1001" /> 
	</params>
	<variables>
		<variable name="toll_allow" value="domestic,international,local"/>
		<variable name="accountcode" value="1005"/>
		<variable name="user_context" value="default"/>
		<variable name="effective_caller_id_name" value="Extension 1005"/>
		<variable name="effective_caller_id_number" value="1005"/>
		<variable name="outbound_caller_id_name" value="FreeSWITCH"/>
		<variable name="outbound_caller_id_number" value="0000000000"/>
		<variable name="callgroup" value="techsupport"/>
	</variables>
	*/

	//��װ����ϢӦ�ú������ļ�����һ��
	//userid  domain  groups  allows context accountcode
	char *usrInfo = "", *var, *val;
	char *infoarray[4] = {//����4���ֶ�
		"","","",""
	}; //ֻ��Ҫ�鿴��ǰ�û���3�����Լ���
	char **tmpInfo = infoarray; //�����׵�ַ,���ַ������ɿ�,���ȱ��ĳ���ֶ�,�����¼������!!!
	int i = 0,ilength = sizeof(infoarray) / sizeof(infoarray[0]);

	/*Ȩ�޵�������Ϣ
	*/
	string chPriDesc[USERPRIORITY_ONLY_DISPATCH + 1] = {
		"",                                                    //��Ч����
		getValueOfDefinedVar("MOBUSER_PRIORITY_INTERNATIONAL"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_DOMESTIC"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_OUT"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_LOCAL"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_ONLY_DISPATCH")
	};

	if (userXMLinfo) {
		//������Ҫ�û���
		usrInfo = (char*) switch_xml_attr_soft(userXMLinfo, "id");//����id��ö�Ӧ���û���

		//params --> param
		if ((x_params = switch_xml_child(userXMLinfo, "params"))) {
			x_param = switch_xml_child(x_params, "param");
			for (; x_param; x_param = x_param->next) {
				var = (char*) switch_xml_attr_soft(x_param, "name");
				val = (char*) switch_xml_attr_soft(x_param, "value");

				if (!strcasecmp("password", var)) {
					//*tmpInfo = val;
					//tmpInfo ++;

					infoarray[0] = val;
					continue;
				}
			}
		}

		//variables --> variable
		if ((x_variables = switch_xml_child(userXMLinfo, "variables"))) {
			x_variable = switch_xml_child(x_variables, "variable");
			for (; x_variable; x_variable = x_variable->next) {
				var = (char*) switch_xml_attr_soft(x_variable, "name");
				val = (char*) switch_xml_attr_soft(x_variable, "value");

				//if (!strcasecmp("toll_allow", var)) {
				//	*tmpInfo = val;
				//	tmpInfo ++;
				//	continue;
				//}

				if (!strcasecmp("priority", var)) {
					//mod by xxl 2010-6-24 :����ʹ��toll_allow�ֶ�
					//��Ҫ����ת��һ��:
					int iPriority = stringToInt(val);
					if (USERPRIORITY_INTERNATIONAL > iPriority 
						|| USERPRIORITY_ONLY_DISPATCH < iPriority){

							iPriority =0;
					}
					//mod by xxl 2010-6-24 end

					//*tmpInfo = (char *)(chPriDesc[iPriority].c_str());
					//tmpInfo ++;
					infoarray[1] = (char *)(chPriDesc[iPriority].c_str());
					continue;
				}

				if (!strcasecmp("callgroup", var)) {
					//*tmpInfo = val;
					//tmpInfo ++;

					infoarray[2] = val;
					continue;
				}
				if (!strcasecmp("desc",var))
				{
					//*tmpInfo = val;
					//tmpInfo ++;//�Ƿ���������Խ��???

					infoarray[3] = val;
					continue;
				}
			}
		}

		//��װ��������Ϣ
		//tmpInfo = infoarray;//���¸���ʼλ��
		//����ķ������ɿ�,�����0xcccccccc(tmpInfo)�ڴ��������
		//if (tmpInfo)
		//{
		//	while(*tmpInfo)
		//	{
		//		usrInfo = switch_mprintf("%s%s%s",usrInfo,CARICCP_SPECIAL_SPLIT_FLAG,*tmpInfo);

		//		tmpInfo++;
		//	}
		//}

		//��װ��ѯ���û���Ϣ
		strOut.append(usrInfo);
		for (; i < ilength; i++) {
			strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
			if (NULL != infoarray[i]){
				strOut.append(infoarray[i]);
			}
		}

		//�������
		//*info = usrInfo;
	}
}

/*���ת����Ϣ
*/
void CModule_User::getTransferInfo(const char* firstid,
								   const char* transferinfo,
								   int type,
								   vector<string> &vecrecord)
{
	vector<string>vecTransferID,vec;
	string strTranscallType,strType,strGateway,strID,strTransUserType,strTimeSeg,strRes;
	int idIndex=0;
	if (0 == type){
		strTranscallType = getValueOfDefinedVar("USER_BUSY_TRANSFER");
		idIndex=0;
	}
	else{
		strTranscallType = getValueOfDefinedVar("USER_NIGHT_TRANSFER");
		idIndex=1;
	}

	//�������ĸ�ʽ
	vecTransferID.clear();
	splitString(transferinfo,CARICCP_SEMICOLON_MARK,vecTransferID);//��";"Ϊ�ָ��???

	//"����"ת���Ŵ���,Ŀǰֻ����һ�����
	for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
		string strID = *it;
		vec.clear();
		strRes="";

		splitString(strID,"/",vec);

		strType    = getValueOfVec(0,vec);//ת���ŵ���������

		//�����"��������","æת"��"ҹת"ͳһ����
		if (isEqualStr("gateway",strType)){//
			strGateway       = getValueOfVec(1,vec);//ĿǰĬ���Ƕ�ת��������ͬһ������
			strID            = getValueOfVec(2,vec);
			strTransUserType = getValueOfDefinedVar("ROUTER_OUT");
			strTransUserType.append("(");
			strTransUserType.append(getValueOfDefinedVar("ROUTER_GATEWAY"));
			strTransUserType.append(":");
			strTransUserType.append(strGateway);
			strTransUserType.append(")");
		}
		//�����"��������","æת"��"ҹת"�����idIndex���ִ���
		else{
			strGateway       = "";
			strID            = getValueOfVec(idIndex,vec);//����
			strTransUserType = getValueOfDefinedVar("ROUTER_LOCAL");
		}

		strRes.append(strTranscallType);
		strRes.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRes.append(firstid);
		strRes.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRes.append(strID);
		strRes.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRes.append(strTransUserType);

		vecrecord.push_back(strRes);
	}
}

/*��xml�ļ��л��mob user��ĳ������,
*��default���ļ�Ŀ¼���û���xml�ļ�variables�ӽڵ��л���û���ĳЩ����ֵ
*/
char * CModule_User::getMobUserVarAttr(switch_xml_t userXMLinfo, 
									   const char *attName,
									   bool bContainedUserNode) //�Ƿ񻹰���user��,false:��Ҫ��һ����user�ڵ���ȡxml�ṹ
									                            //                 true:ֱ��ʹ��
{
	switch_xml_t x_user, x_variables, x_var;
	char *attVal = NULL;

	//��user�ӽڵ㿪ʼ
	if (!bContainedUserNode){//�����xml�ļ���ֱ�Ӳ��һ��,����Ҫ�˲���
		x_user = switch_xml_child(userXMLinfo, "user");//��ȡuser��xml�ṹ
	}
	else{//��ʱuserXMLinfo����user�ӽڵ�xml�ṹ
		x_user = userXMLinfo;
	}

	if (!x_user){
		return NULL;	
	}

	//��Ҫ�Ǵ�"variables"�ڵ�������
	/*if (x_user = switch_xml_child(userXMLinfo, "user")) {*/
		if ((x_variables = switch_xml_child(x_user, "variables"))) {
			for (x_var = switch_xml_child(x_variables, "variable"); x_var; x_var = x_var->next) {
				const char *var = switch_xml_attr(x_var, "name");
				const char *val = switch_xml_attr(x_var, "value");

				//������ҵ���ͬ������
				if (!strcasecmp(attName, var) && val) {
					attVal = (char*) val;
					return attVal;
				}
			}
		}
	/*}*/

	return NULL;
}

/*����xml�ļ��������û��޸�mob user��params����,��Ҫ�����default/Ŀ¼�µ�Ĭ�������ļ������޸�
*������Բ�����,������
*/
bool CModule_User::setMobUserParamAttr(switch_xml_t userXMLinfo, //xml node
									   const char *attName, //������
									   const char *attVal, //����ֵ
									   bool bAddattr)  	//������Բ������Ƿ���Ҫ����,Ĭ��Ϊ������
{
	bool isExistAttr = false;
	switch_xml_t x_user, x_params, x_param, x_newChild;
	int iPos = 0;
	char *newVariableContext = "";

	//��user�ӽڵ㿪ʼ
	if (x_user = switch_xml_child(userXMLinfo, "user")) {
		if ((x_params = switch_xml_child(x_user, "params"))) {
			for (x_param = switch_xml_child(x_params, "param"); x_param; x_param = x_param->next) {
				iPos++;
				const char *var = switch_xml_attr(x_param, "name");
				const char *val = switch_xml_attr(x_param, "value");

				//������ҵ���ͬ������
				if (!strcasecmp(attName, var) && val) {
					//�滻��ǰ������ֵ
					//������-ֵ��,��: <variable name="attName" value="g2" /> 
					switch_xml_set_attr(x_param, "value", attVal);
					isExistAttr = true;
					break;
				}
			}

			//��������ӽڵ㲻����,������������,�����´���
			if (!isExistAttr && bAddattr) {
				newVariableContext = switch_mprintf(MOB_USER_XML_PARAM, attName, attVal);
				
				//��Ҫ�ͷ�
				x_newChild = switch_xml_parse_str(newVariableContext, strlen(newVariableContext));
				if (!x_newChild) {
					return false;
				}
				if (!switch_xml_insert(x_newChild, x_params, INSERT_INTOXML_POS)){//���õ����
					return false;
				}
				
				//�˴����ڴ��Ǻ�xml����󶨵�,�����ͷ�,��������x_newChild�ṹ�쳣
				//Ӧ�ÿ����ͷ�xml�Ľṹ,�ɵ��ú�������
				//switch_safe_free(newVariableContext);
			}
		}
	}

	return true;
}

/*���û��޸�mob user��variables����,��Ҫ�����default/Ŀ¼�µ�Ĭ�������ļ������޸�
*������Բ�����,������
*/
bool CModule_User::setMobUserVarAttr(switch_xml_t userXMLinfo, //xml node 
									 const char *attName,      //������
									 const char *attVal,       //����ֵ 
									 bool bAddattr)	           //������Բ������Ƿ���Ҫ����,Ĭ��Ϊ������false
{
	bool isExistAttr = false;
	switch_xml_t x_user, x_variables, x_var, x_newChild;
	int iPos = 0;
	char *newVariableContext = "";

	//��user�ӽڵ㿪ʼ
	if (x_user = switch_xml_child(userXMLinfo, "user")) {
		if ((x_variables = switch_xml_child(x_user, "variables"))) {
			for (x_var = switch_xml_child(x_variables, "variable"); x_var; x_var = x_var->next) {
				iPos++;
				const char *var = switch_xml_attr(x_var, "name");
				const char *val = switch_xml_attr(x_var, "value");

				//������ҵ���ͬ������
				if (!strcasecmp(attName, var) && val) {
					//�滻��ǰ������ֵ
					//������-ֵ��,��: <variable name="attName" value="g2" /> 
					switch_xml_set_attr(x_var, "value", attVal);

					isExistAttr = true;
					break;
				}
			}

			//��������ӽڵ㲻����,������������,�����´���
			if (!isExistAttr && bAddattr) {
				newVariableContext = switch_mprintf(MOB_USER_XML_VARATTR, attName, attVal);

				//��Ҫ�ͷ�
				x_newChild = switch_xml_parse_str(newVariableContext, strlen(newVariableContext));
				if (!x_newChild) {
					return false;
				}

				if (!switch_xml_insert(x_newChild, x_variables, INSERT_INTOXML_POS)){//���õ����
					return false;
				}
				//�˴����ڴ��Ǻ�xml����󶨵�,�����ͷ�,��������x_newChild�ṹ�쳣
				//Ӧ�ÿ����ͷ�xml�Ľṹ,�ɵ��ú�������
				//switch_safe_free(newVariableContext);
			}
		}
	}

	return true;
}

/*���õ���Ա����
*/
bool CModule_User::setdispatcherVarAttr(switch_xml_t dispathcersXml,
										const char* attVal,
										bool bAddattr)
{
	bool isExistAttr = false;
	switch_xml_t x_dispatchers, x_newChild;
	int iPos = 0;
	char *newVariableContext = "";
	if (bAddattr){
		if (x_dispatchers = switch_xml_child(dispathcersXml, "dispatchers")) {
			newVariableContext = switch_mprintf(DISPAT_USER_XML_VARATTR, attVal);
			
			//��Ҫ�ͷ�
			x_newChild = switch_xml_parse_str(newVariableContext, strlen(newVariableContext));
			if (!x_newChild) {
				return false;
			}
			//��ӵ��˽ṹ��
			if (!switch_xml_insert(x_newChild, x_dispatchers, INSERT_INTOXML_POS)){//���õ����
				return false;
			}
			//�˴����ڴ��Ǻ�xml����󶨵�,�����ͷ�,��������x_newChild�ṹ�쳣
			//switch_safe_free(newVariableContext);
		}
	}
	//	else{
	//		//��dispatchers�ӽڵ㿪ʼ
	//		if (x_dispatchers = switch_xml_child(dispathcersXml, "dispatchers")) {
	//			for (x_dispatcher = switch_xml_child(x_dispatchers, "dispatcher"); x_dispatcher; x_dispatcher = x_dispatcher->next) {
	//				const char *varid = switch_xml_attr(x_dispatcher, "id");

	//				//������ҵ���ͬ������
	//				if (!strcasecmp(attVal, varid)) {
	//					//ɾ��������ֵ
	//					switch_xml_set_attr(x_dispatcher, "value", attVal);
	//					isExistAttr = true;
	//					break;
	//				}
	//			}

	//	}
	//}

	return true;
}

/*��дmob user��xml�ļ�����
*/
bool CModule_User::rewriteMobUserXMLFile(const char *mobuserid, 
										 switch_xml_t x_mobuser,
										 string &err)
{
	char *default_xmlfile=NULL, *xmlfilecontext;
	string strcontext,strtmp;
	bool bRes = false;
	const char *eMsg[1]={0};

	xmlfilecontext = switch_xml_toxml(x_mobuser, SWITCH_FALSE);
	if (!xmlfilecontext) {
		err = getValueOfDefinedVar("XMLFILE_TO_STR_ERROR");//�ļ�ת������
		goto end_flag;
	}
	default_xmlfile = getMobUserDefaultXMLFile(mobuserid);
	bRes = cari_common_creatXMLFile(default_xmlfile, xmlfilecontext, eMsg);
	switch_safe_free(default_xmlfile);
	if (!bRes) {
		err = getValueOfDefinedVar("CREAT_DIR_ERROR");
		goto end_flag;
	}

/*---*/
end_flag : 
	
	err = "operation sucess.";//�����ɹ�
	return bRes;
}
