//#include "modules\CModule_Route.h"
#include "CModule_Route.h"
//#include "cari_public_interface.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"


//////////////////////////////////////////////////////////////////////////
//mob user��������xml�ļ��ṹ,֧�����ĵ���ʾ
#define  GATEWAY_XML_SIMPLE_CONTEXT	"<include>\n"					\
									"	<gateway name=\"%s\">\n"	\
									"	</gateway>\n"				\
									"   </include>";

#define  GATEWAY_PARAM              "<param name=\"%s\" value=\"%s\"/>"

//out dialplan.xml�ļ�ÿ��·�����ص�������Ϣ
#define  OUT_DIALPLAN_XML_CONTEXT   "<extension name=\"%s\">"                                        \
	                                "<condition field=\"${priority}\" expression=\"^[0123]$\"/>"     \
									"<condition field=\"destination_number\" expression=\"%s\">"     \
									"<action application=\"export\" data=\"dialed_extension=$1\"/>"  \
									"<action application=\"export\" data=\"context_extension=%s\"/>" \
									"<action application=\"set\" data=\"ringback=${us-ring}\"/>"             \
									"<action application=\"set\" data=\"transfer_ringback=$${hold_music}\"/>"\
									"<action application=\"set\" data=\"call_timeout=30\"/>"                 \
									"<action application=\"set\" data=\"continue_on_fail=true\"/>"           \
									"<action application=\"conference\" data=\"bridge:${caller_id_number}-[${uuid}]-${destination_number}@default:sofia/gateway/%s/$1\"/>" \
									"<action application=\"voicetip\" data=\"3\"/>"                          \
									"</condition>"                                                           \
									"</extension>"


#define  INTERNAL_PROFILE_TYPE  	 "internal"
#define  EXTERNAL_PROFILE_TYPE  	 "external"


//////////////////////////////////////////////////////////////////////////
CModule_Route::CModule_Route()
	: CBaseModule()
{
}


CModule_Route::~CModule_Route()
{
}

/*���ݽ��յ���֡,������������,�ַ�������Ĵ�����
*/
int CModule_Route::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
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
	case CARICCP_ADD_GATEWAY:
		iFunRes = addGateWay(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_GATEWAY:
		iFunRes = delGateWay(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_MOD_GATEWAY:
		iFunRes = mobGateWay(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_GATEWAY:
		iFunRes = lstGateWay(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_SEND_MESSAGE:
		iFunRes = sendMessage(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_SET_ROUTER:
		iFunRes = setRouter(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_DEL_ROUTER:
		iFunRes = delRouter(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LST_ROUTER:
		iFunRes = lstRouter(inner_reqFrame, inner_RespFrame);
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
int CModule_Route::sendRespMsg(common_ResultResponse_Frame *&respFrame)
{
	int	iFunRes	= CARICCP_SUCCESS_STATE_CODE;
	return iFunRes;
}

int CModule_Route::cmdPro(const inner_CmdReq_Frame *&reqFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}

//////////////////////////////////////////////////////////////////////////
/*��������,���ص������ļ�������sip_profiles\internal��
*/
int CModule_Route::addGateWay(inner_CmdReq_Frame *&reqFrame, inner_ResultResponse_Frame *&inner_RespFrame, bool bReLoadRoot)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "";
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc                     = NULL;
	bool	        bRes			= true;

	char			*gatewayath		= NULL, *gatewayContext = NULL,*context=NULL;
	switch_xml_t	x_context       =NULL, x_gateway=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strProfileType, strGatewayName, strUsername, strPwd, strRealm, strFromUser, strFromDomain, 
		            strExtension, strProxy, strRegisterProxy, strExpireSeconds, strIsOrNotRegister, strTransportPro,  
					strRetrySeconds, strFromCallerFlag, strContactPrams, strPingSecond;
	string			strTmp;

	////"type"
	//strParamName = "type";//ua����
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//PROFILE�����Ǳ�ѡ����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////ֻ��������������֮һ
	//if (!isEqualStr(strValue, INTERNAL_PROFILE_TYPE) 
	//	&& !isEqualStr(strValue, EXTERNAL_PROFILE_TYPE)) {
	//		strReturnDesc = getValueOfDefinedVar("PROFILE_TYPE_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strProfileType = /*strValue*/INTERNAL_PROFILE_TYPE;//����Ĭ������

	//"gatewayname"
	strParamName = "gatewayname";//gatewayname��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//
		//���ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGatewayName = strValue;

	//"useraccount"
	strParamName = "useraccount";//useraccount��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//username�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û�������
        strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strUsername = strValue;

	//"password"
	strParamName = "password";//password
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//password�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//����
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strPwd = strValue;

	////"realm"
	//strParamName = "realm";//realm
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/*********************************************************/
	//	/* auth realm: *optional* same as gateway name, if blank */
	//	strValue = strGatewayName;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strRealm = strValue;

	////"from-user"
	//strParamName = "from-user";//from-user
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/*********************************************************/
	//	/* username to use in from: *optional* same as  username, if blank */
	//	strValue = strUsername;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strFromUser = strValue;

	////"from-domain"
	//strParamName = "from-domain";//from-domain
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/**************************************************************/
	//	/* domain to use in from: *optional* same as  realm, if blank */
	//	strValue = strRealm;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strFromDomain = strValue;

	////"extension"
	//strParamName = "extension";//extension
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/**************************************************************/
	//	/* extension for inbound calls: *optional* same as username, if blank */
	//	strValue = strUsername;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strExtension = strValue;

	//"proxyip"
	strParamName = "proxyip";//proxyip
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		/**************************************************************/
		/*  proxy host: *optional* same as realm, if blank */
		//strValue = strRealm;

		//����Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	if (!isValidIPV4(strValue)){
		//�Ƿ���Ч��IP��ַ
		strReturnDesc = getValueOfDefinedVar("PARAM_IP_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strProxy = strValue;

	////"register-proxy"
	//strParamName = "register-proxy";//register-proxy
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/**************************************************************/
	//	/* send register to this proxy: *optional* same as proxy, if blank */
	//	strValue = strProxy;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strRegisterProxy = strValue;


	////"expire-seconds"
	//strParamName = "expire-seconds";//expire-seconds
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/************************************************/
	//	/* expire in seconds: *optional* 3600, if blank */
	//	strValue = "3600";
	//}
	//strExpireSeconds = strValue;

	//"register"
	strParamName = "register";//register
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (strcasecmp(strValue.c_str(), "true")) {
		strValue = "false";//��true�������һ����Ϊfalse
	}
	strIsOrNotRegister = strValue;

	//"register-transport"
	strParamName = "register-transport";//register-transport
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "udp";//Ĭ��Ϊudp��ʽ
	//}
	strTransportPro = strValue;

	////"retry-seconds"
	//strParamName = "retry-seconds";//retry-seconds
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "30";//
	//}
	//strRetrySeconds = strValue;

	////"caller-id-in-from"
	//strParamName = "caller-id-in-from";//caller-id-in-from
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (strcasecmp(strValue.c_str(), "true")) {
	//	strValue = "false";//��true�������һ����Ϊfalse
	//}
	//strFromCallerFlag = strValue;

	////"contact-params"
	//strParamName = "contact-params";//contact-params
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "tport=tcp";
	//}
	//strContactPrams = strValue;

	////"ping"
	//strParamName = "ping";//ping
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "10";//Ĭ��
	//}
	//strPingSecond = strValue;

	//�жϴ������Ѿ�����
	if (isExistedGateWay(strProfileType.c_str(), strGatewayName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("GATEWAY_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//�ȹ���xml���ڴ�ṹ
	strTmp = GATEWAY_XML_SIMPLE_CONTEXT;
	context = switch_mprintf(strTmp.c_str(), strGatewayName.c_str());

	//������ת����xml�Ľṹ(ע��:�����ַ�����)
	x_context = switch_xml_parse_str(context, strlen(context));
	//switch_safe_free(context);//��ʱ�ͷŻ�����ڴ�ṹ��������!!~!

	if (!x_context) {
		strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	x_gateway = switch_xml_child(x_context, "gateway");
	if (!x_gateway) {
		strReturnDesc = getValueOfDefinedVar("XML_CHILD_NOTFOUND");
		pDesc = switch_mprintf(strReturnDesc.c_str(), "gateway");
		goto end_flag;
	}

	//���Ӻܶ���ص�����,�˴�ֻ��Ҫ��������,�����Ĳ��ֱ���
	setGatewayAttribute(x_gateway, "username", strUsername.c_str(), true);
	//setGatewayAttribute(x_gateway, "realm", strRealm.c_str(), true);
	//setGatewayAttribute(x_gateway, "from-user", strFromUser.c_str(), true);
	//setGatewayAttribute(x_gateway, "from-domain", strFromDomain.c_str(), true);
	setGatewayAttribute(x_gateway, "password", strPwd.c_str(), true);
	//setGatewayAttribute(x_gateway, "extension", strExtension.c_str(), true);
	setGatewayAttribute(x_gateway, "proxy", strProxy.c_str(), true);
	//setGatewayAttribute(x_gateway, "register-proxy", strRegisterProxy.c_str(), true);
	//setGatewayAttribute(x_gateway, "expire-seconds", strExpireSeconds.c_str(), true);
	setGatewayAttribute(x_gateway, "register", strIsOrNotRegister.c_str(), true);
	setGatewayAttribute(x_gateway, "register-transport", strTransportPro.c_str(), true);
	//setGatewayAttribute(x_gateway, "retry-seconds", strRetrySeconds.c_str(), true);
	//setGatewayAttribute(x_gateway, "caller-id-in-from", strFromCallerFlag.c_str(), true);
	//setGatewayAttribute(x_gateway, "contact-params", strContactPrams.c_str(), true);
	//setGatewayAttribute(x_gateway, "ping", strPingSecond.c_str(), true);

	gatewayContext = switch_xml_toxml(x_context, SWITCH_FALSE);
	if (!gatewayContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�������ص��ļ�
	bRes = creatGateWayXMLFile(strProfileType.c_str(), strGatewayName.c_str(), gatewayContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}  

	//////////////////////////////////////////////////////////////////////////
	//�����ͳһ���¼���һ��root xml�ļ�
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������
		//switch_xml_open_root(1,err);
	}

	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//�������سɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("ADD_GATEWAY_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());

/*-----*/
end_flag :

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml�ṹҪ��ʱ�ͷ�,���������Ե�ʱ��,�������ڴ�
	//switch_xml_free(x_gateway);
	switch_xml_free(x_context);//�ͷ�һ�μ���  

	switch_safe_free(context);
	switch_safe_free(gatewayContext);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*ɾ������
*/
int CModule_Route::delGateWay(inner_CmdReq_Frame *&reqFrame, inner_ResultResponse_Frame *&inner_RespFrame, bool bReLoadRoot)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "";
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char *pDesc                     = NULL;
	bool	bRes			= true;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strProfileType, strGatewayName;
	string			strTmp;

	////"type"
	//strParamName = "type";//ua����
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//type�����Ǳ�ѡ����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////ֻ��������������֮һ
	//if (strcasecmp(strValue.c_str(), INTERNAL_PROFILE_TYPE) && strcasecmp(strValue.c_str(), EXTERNAL_PROFILE_TYPE)) {
	//	strReturnDesc = getValueOfDefinedVar("PROFILE_TYPE_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}
	strProfileType = /*strValue*/INTERNAL_PROFILE_TYPE;//����Ĭ������

	//"gatewayname"
	strParamName = "gatewayname";//gatewayname��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

		goto end_flag;
	}
	strGatewayName = strValue;

	//�жϴ�����û������
	if (!isExistedGateWay(strProfileType.c_str(), strGatewayName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//ɾ�������ļ�
	bRes = delGateWayXMLFile(strProfileType.c_str(), strGatewayName.c_str(), err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�������"out"·���Ѿ��ʹ����ذ�,��ʱ��Ҫ������·��ҲҪһ��ɾ��
	bRes = delOutRouter(true,strGatewayName,pDesc);
	if (!bRes){
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//�����ͳһ���¼���һ��root xml�ļ�
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//ɾ�����سɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("DEL_GATEWAY_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*�޸�����
*/
int CModule_Route::mobGateWay(inner_CmdReq_Frame *&reqFrame, inner_ResultResponse_Frame *&inner_RespFrame, bool bReLoadRoot)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "";
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc          = NULL;
	bool            bRes			= true;

	char			*gatewayath		= NULL, *newgatewayContext = NULL;
	switch_xml_t	x_context=NULL, x_gateway=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strProfileType, strOldGatewayName,strNewGatewayName, strUsername, strPwd, strRealm, strFromUser, strFromDomain, strExtension, 
		            strProxy, strRegisterProxy, strExpireSeconds, strIsOrNotRegister, strTransportPro, strRetrySeconds, strFromCallerFlag, 
		            strContactPrams, strPingSecond;
	string			strTmp;

	////"type"
	//strParamName = "type";//ua����
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//type�����Ǳ�ѡ����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////ֻ��������������֮һ
	//if (!isEqualStr(strValue, INTERNAL_PROFILE_TYPE) 
	//	&& !isEqualStr(strValue, EXTERNAL_PROFILE_TYPE)) {
	//	strReturnDesc = getValueOfDefinedVar("PROFILE_TYPE_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}
	strProfileType = /*strValue*/INTERNAL_PROFILE_TYPE;//����Ĭ������

	//"oldgatewayname"
	strParamName = "oldgatewayname";//oldgatewayname
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//
		//���ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strOldGatewayName = strValue;

	//"newgatewayname"
	strParamName = "newgatewayname";//newgatewayname��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//
		//���ֳ���,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strNewGatewayName = strValue;

	//"newaccount"
	strParamName = "newaccount";//newaccount��
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//username�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}

	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//�û�������
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strUsername = strValue;

	//"newpassword"
	strParamName = "newpassword";//newpassword
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//password�����Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//����
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strPwd = strValue;

	////"realm"
	//strParamName = "realm";//realm
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/*********************************************************/
	//	/* auth realm: *optional* same as gateway name, if blank */
	//	strValue = strNewGatewayName;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strRealm = strValue;

	////"from-user"
	//strParamName = "from-user";//from-user
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/*********************************************************/
	//	/* username to use in from: *optional* same as  username, if blank */
	//	strValue = strUsername;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strFromUser = strValue;

	////"from-domain"
	//strParamName = "from-domain";//from-domain
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/**************************************************************/
	//	/* domain to use in from: *optional* same as  realm, if blank */
	//	strValue = strRealm;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strFromDomain = strValue;

	////"extension"
	//strParamName = "extension";//extension
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/**************************************************************/
	//	/* extension for inbound calls: *optional* same as username, if blank */
	//	strValue = strUsername;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strExtension = strValue;

	//"newproxyip"
	strParamName = "newproxyip";//newproxyip
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		/**************************************************************/
		/*  proxy host: *optional* same as realm, if blank */
		//strValue = strRealm;

		//����Ϊ��,���ش�����Ϣ
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	if (!isValidIPV4(strValue)){
		//�Ƿ���Ч��IP��ַ
		strReturnDesc = getValueOfDefinedVar("PARAM_IP_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strProxy = strValue;

	////"register-proxy"
	//strParamName = "register-proxy";//register-proxy
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/**************************************************************/
	//	/* send register to this proxy: *optional* same as proxy, if blank */
	//	strValue = strProxy;
	//} else if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//����
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	//strRegisterProxy = strValue;


	////"expire-seconds"
	//strParamName = "expire-seconds";//expire-seconds
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	/************************************************/
	//	/* expire in seconds: *optional* 3600, if blank */
	//	strValue = "3600";
	//}
	//strExpireSeconds = strValue;

	//"register"
	strParamName = "register";//register
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (strcasecmp(strValue.c_str(), "true")) {
		strValue = "false";//��true�������һ����Ϊfalse
	}
	strIsOrNotRegister = strValue;


	//"register-transport"
	strParamName = "register-transport";//register-transport
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "udp";//Ĭ��Ϊudp��ʽ
	//}
	strTransportPro = strValue;

	////"retry-seconds"
	//strParamName = "retry-seconds";//retry-seconds
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "30";//
	//}
	//strRetrySeconds = strValue;

	////"caller-id-in-from"
	//strParamName = "caller-id-in-from";//caller-id-in-from
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (strcasecmp(strValue.c_str(), "true")) {
	//	strValue = "false";//��true�������һ����Ϊfalse
	//}
	//strFromCallerFlag = strValue;

	////"contact-params"
	//strParamName = "contact-params";//contact-params
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "tport=tcp";
	//}
	//strContactPrams = strValue;

	////"ping"
	//strParamName = "ping";//ping
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "10";//Ĭ��
	//}
	//strPingSecond = strValue;

	//////////////////////////////////////////////////////////////////////////
	//�жϾ������Ƿ��Ѿ�����,û���������ܽ����޸�
	if (!isExistedGateWay(strProfileType.c_str(), strOldGatewayName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGatewayName.c_str());
		goto end_flag;
	}

	//ֱ�Ӵӵ�ǰ���ص������ļ��л��xml�ṹ,�˴��Ĵ���������"��ͬ"???
	x_context = getGatewayXMLInfo(strProfileType.c_str(), strOldGatewayName.c_str());
	if (!x_context) {
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGatewayName.c_str());
		goto end_flag;
	}

	x_gateway = switch_xml_child(x_context, "gateway");
	if (!x_gateway) {
		strReturnDesc = getValueOfDefinedVar("XML_CHILD_NOTFOUND");
		pDesc = switch_mprintf(strReturnDesc.c_str(), "gateway");
		goto end_flag;
	}

	//�ؼ�:�޸����ص�����
	switch_xml_set_attr(x_gateway, "name", strNewGatewayName.c_str());

	//�޸ĺܶ���ص�����
	setGatewayAttribute(x_gateway, "username", strUsername.c_str(), false);
	//setGatewayAttribute(x_gateway, "realm", strRealm.c_str(), false);
	//setGatewayAttribute(x_gateway, "from-user", strFromUser.c_str(), false);
	//setGatewayAttribute(x_gateway, "from-domain", strFromDomain.c_str(), false);
	setGatewayAttribute(x_gateway, "password", strPwd.c_str(), false);
	//setGatewayAttribute(x_gateway, "extension", strExtension.c_str(), false);
	setGatewayAttribute(x_gateway, "proxy", strProxy.c_str(), false);
	//setGatewayAttribute(x_gateway, "register-proxy", strRegisterProxy.c_str(), false);
	//setGatewayAttribute(x_gateway, "expire-seconds", strExpireSeconds.c_str(), false);
	setGatewayAttribute(x_gateway, "register", strIsOrNotRegister.c_str(), false);
	setGatewayAttribute(x_gateway, "register-transport", strTransportPro.c_str(), false);
	//setGatewayAttribute(x_gateway, "retry-seconds", strRetrySeconds.c_str(), false);
	//setGatewayAttribute(x_gateway, "caller-id-in-from", strFromCallerFlag.c_str(), false);
	//setGatewayAttribute(x_gateway, "contact-params", strContactPrams.c_str(), false);
	//setGatewayAttribute(x_gateway, "ping", strPingSecond.c_str(), false);

	newgatewayContext = switch_xml_toxml(x_context, SWITCH_FALSE);
	if (!newgatewayContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//�޸����ص������ļ�,���µ����ֽ�������,��������ֶ�Ӧ������Ŀǰ�Ѿ�����,������
	//��������������������һ��
	if (!isEqualStr(strOldGatewayName,strNewGatewayName)){
		//���ж�����������Ӧ�������Ƿ��Ѿ�����,�������������
		if (isExistedGateWay(strProfileType.c_str(), strNewGatewayName.c_str())) {
			strReturnDesc = getValueOfDefinedVar("GATEWAY_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strNewGatewayName.c_str());
			goto end_flag;
		}

		//Ȼ��ɾ��ԭ���ľ������ļ�
		bRes = delGateWayXMLFile(strProfileType.c_str(), strOldGatewayName.c_str(), err);
		if (!bRes) {
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

	//�����µ�xml�ļ�
	bRes = creatGateWayXMLFile(strProfileType.c_str(), strNewGatewayName.c_str(), newgatewayContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//����������Ѿ�����Ϊ"����"out����,����Ҫͬ���޸���ص�out·���ļ�
	if (!isEqualStr(strOldGatewayName,strNewGatewayName)){
		bRes = modOutRouter(strOldGatewayName,strNewGatewayName,pDesc);//�޸�out·��
		if (!bRes){
			goto end_flag;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	//�����ͳһ���¼���һ��root xml�ļ�
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//ֱ�ӵ��û��������
		//switch_xml_open_root(1,err);
	}

	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strNewGatewayName.c_str());
		goto end_flag;
	}

	//�޸����سɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//������Ϣ�����·�װһ��
	strReturnDesc = getValueOfDefinedVar("MOD_GATEWAY_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGatewayName.c_str());


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_context);//�ͷ�
	switch_safe_free(pDesc);
	return iFunRes;
}

/*��ѯ������Ϣ
*/
int CModule_Route::lstGateWay(inner_CmdReq_Frame *&reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "",strSeq;
	const char		*err[1]									= {
		0
	};

	char			*chTmp									= NULL, *strTableName=NULL;
	char			*pDesc                                  = NULL;
	
	//ÿ�������CARICCP_MAX_GATEWAY_NUM������
	char			*gatewayInfo[CARICCP_MAX_GATEWAY_NUM]	= {
		0
	};
	char			**allGatewayInfo						= gatewayInfo;
	string			strname;
	int iRecordCout = 0;//��¼����
	string tableHeader[6] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("GATEWAY_NAME"),
		getValueOfDefinedVar("GATEWAY_USERNAME"),
		getValueOfDefinedVar("GATEWAY_PWD"),
		//getValueOfDefinedVar("USER_ACCOUNT"),
		//getValueOfDefinedVar("USER_CID"),
		getValueOfDefinedVar("GATEWAY_IP"),
		//getValueOfDefinedVar("GATEWAY_REGISTER"),
		getValueOfDefinedVar("GATEWAY_TRANSPORT")
	};

	//Ĭ�ϲ�ѯ���е�������Ϣ
	getAllGatewaysFromMem(allGatewayInfo);
	if (!*allGatewayInfo) {
		strReturnDesc = getValueOfDefinedVar("NO_GATEWAY");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("GATEWAY_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

	//����ÿ����
	while (*allGatewayInfo) {
		iRecordCout++;//��ŵ���
		chTmp = *allGatewayInfo;//��ö�Ӧ������Ϣ
		
		//��װ��ѯ�ĸ�ʽ
		strSeq = intToString(iRecordCout);
		strname ="";
		strname.append(strSeq);
		strname.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strname.append(chTmp);

		//��ż�¼
		inner_RespFrame->m_vectTableValue.push_back(strname);

		//һ��Ҫ�ͷ��ڴ�
		switch_safe_free(*allGatewayInfo);

		//��һ���û�
		allGatewayInfo++;
	}

	//���ñ�ͷ�ֶ�
	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s",
		tableHeader[0].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[1].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[2].c_str()	    ,CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[3].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[4].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[5].c_str());
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));

/*-----*/
end_flag :
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));


	switch_safe_free(pDesc);
	switch_safe_free(strTableName);
	return iFunRes;
}

/*����·��
*/
int CModule_Route::setRouter(inner_CmdReq_Frame*& reqFrame, 
							 inner_ResultResponse_Frame*& inner_RespFrame, 
							 bool bReLoadRoot)
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

	//default:"����·��"��Ӧ�������ļ�
	char *diaplanFile = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",//diaplanĿ¼
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_DIAPLAN_DEF_XML_FILE
		);

	//00_out_dial.xml:"����·��"�������ļ�
	char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",           //diaplanĿ¼
		SWITCH_PATH_SEPARATOR,
		"default",            //defaultĿ¼
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_OUT_DIAPLAN_XML_FILE
		);

	switch_xml_t xdiaplan=NULL,xcontext=NULL,xoutdial=NULL,xoutGateway=NULL;
	char *diaplanContext=NULL,*outdialContext=NULL;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strRouteType,strPrefix,strMinBackLen,strMaxBackLen,strGateWay,strLocalId,strDispatchId;
	string			strTmp,strBefore,strExpression,strValidPrefix,strValidPostfix;
	int             iMinLen = 0,iMaxLen = 0,ipos=-1,iType = -1,iPriority=-1;
	vector<string>  vecPrefixs,vecTmp;//���"ǰ׺"������
	char             *outGatewayContext=NULL;     //����Ҫ�ͷ��������

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"type"
	strParamName = "type";//Ĭ��Ϊ"�û�����"·����������
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	trim(strValue);
	if (0 == strValue.size()) {
		//�Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("out",strValue)){       //����
		iPriority = USERPRIORITY_OUT;
	}
	else if (isEqualStr("local",strValue)){//����
		iPriority = USERPRIORITY_LOCAL;
	}
	else if (isEqualStr("to_dispatch_desk",strValue)){//�������̨
		iPriority = ROUTER_TO_DISPATCH_DESK;
	}
	else if (isEqualStr("only_dispatch",strValue)){//ֻ�ܺ�����
		iPriority = USERPRIORITY_ONLY_DISPATCH;
	}
	else{
		//���Ͳ�����,ֵ��Ч
		strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strRouteType = strValue;

	//mod by xxl 2010-10-21 begin :�������͵Ľ�������޸�
	//���ݲ�ͬ��·�����ͷֱ���д���
	if (USERPRIORITY_OUT == iPriority){//����
		//"prefix"
		strParamName = "prefix";//����ǰ׺
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//�Ǳ�ѡ����
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
			//����
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
			goto end_flag;
		}
		strPrefix = strValue;

		//"minbacklen"
		strParamName = "minbacklen";//��С�ĺ������볤��
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//�Ǳ�ѡ����
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iMinLen = stringToInt(strValue.c_str());
		//��Χ����
		if (0 > iMinLen || 16 < iMinLen){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 0,16);
			goto end_flag;
		}
		strMinBackLen = strValue;

		//"maxbacklen"
		strParamName = "maxbacklen";//���ĺ������볤��
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//�Ǳ�ѡ����
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iMaxLen = stringToInt(strValue.c_str());
		//��Χ����
		if (0 > iMaxLen || 16 < iMaxLen){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 0,16);
			goto end_flag;
		}
		strMaxBackLen = strValue;

		//���ֵ����С����Сֵ
		if (iMaxLen<iMinLen){
			strTmp = getValueOfDefinedVar("minbacklen");
			strReturnDesc = getValueOfDefinedVar("PARAM_COMPARE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(),strTmp.c_str());
			goto end_flag;
		}

		//"gateway"
		strParamName = "gateway";
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//�Ǳ�ѡ����
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		//�����Ƿ����
		if (!isExistedGateWay(INTERNAL_PROFILE_TYPE, strValue.c_str())) {
			strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
			goto end_flag;
		}
		strGateWay = strValue;

		//�Ժ���"ǰ׺"������ϸ�Ľ�������
		strValue="";
		splitString(strPrefix,CARICCP_COMMA_FLAG,vecPrefixs);
		for (vector<string>::iterator it = vecPrefixs.begin(); vecPrefixs.end() != it; it++){
			strTmp = *it;
			vecTmp.clear();
			splitString(strTmp,CARICCP_LINK_MARK,vecTmp);

			//�жϸ�ʽ�Ƿ�Ϊ���:7-9
			for (vector<string>::iterator it2 = vecTmp.begin(); vecTmp.end() != it2; it2++){
				//�Ƿ�Ϊ��Ч����
				string str = *it2;
				if (!isNumber(str)){
					strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
					strTmp = getValueOfDefinedVar("ROUTER_PREFIX");
					pDesc = switch_mprintf(strReturnDesc.c_str(), str.c_str());
					goto end_flag;
				}
			}
			if (isEqualStr(strBefore,strTmp)){
				continue;
			}
			strBefore = strTmp;
			strValue.append(strTmp);
			strValue.append(CARICCP_OR_MARK);//"|"
		}
		ipos = strValue.find_last_of(CARICCP_OR_MARK);
		if (0<ipos){
			strValue = strValue.substr(0,ipos);
		}
		strValidPrefix = strValue;//��װ��������Чǰ׺ֵ

		//��װ��ʽ:^ǰ׺(�����)$,��:^7(\d{7,16})$
		strExpression.append("^");      //^ ���ʽ��ʼ�ı�־
		strExpression.append(strValidPrefix);

		strValidPostfix = "";
		strValidPostfix.append("(\\d{");
		strValidPostfix.append(strMinBackLen);
		strValidPostfix.append(CARICCP_COMMA_MARK);
		strValidPostfix.append(strMaxBackLen);
		strValidPostfix.append("})");
		
		strExpression.append(strValidPostfix);
		strExpression.append("$");      //$ ������ʽ�Ľ���
	}
	else if (USERPRIORITY_LOCAL == iPriority){//����
		//"localid"
		strParamName = "localid";//���ֺ����
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//�Ǳ�ѡ����
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
			//����
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
			goto end_flag;
		}
		strLocalId = strValue;

		//��װ��ʽ:^([5-6][0-9][0-9][0-9])$
		strExpression.append("^");           //^ ���ʽ��ʼ�ı�־
		strExpression.append("(");

		vector<string> vecLocalID;
		splitString(strLocalId,CARICCP_COMMA_FLAG,vecLocalID);

		//�Ժ���ν�����ϸ�Ľ�������
		for (vector<string>::iterator it = vecLocalID.begin(); vecLocalID.end() != it; it++){
			strValue = *it;
			//������ϸ�ķ�����ʽ�Ƿ�Ϸ�,���ڷ���
			strExpression.append("[");
			strExpression.append(strValue);
			strExpression.append("]");
		}
		strExpression.append(")");
		strExpression.append("$");          //$ ������ʽ�Ľ���
	}
	else{//�������̨��ֻ�ܺ�����
		//"dispatchid"
		strParamName = "dispatchid";//���ֺ����
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//�Ǳ�ѡ����
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
			//����
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
			goto end_flag;
		}
		//�Ƿ�Ϊ��Ч����
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		strDispatchId = strValue;

		//��װ��ʽ:^����$
		strExpression.append("^");      //^ ���ʽ��ʼ�ı�־
		strExpression.append(strDispatchId);
		strExpression.append("$");      //$ ������ʽ�Ľ���
	}
	//mod by xxl 2010-10-21 end :�������͵Ľ�������޸�

	//////////////////////////////////////////////////////////////////////////
	//"����","ֻ���е���"�Լ�"�������̨"����ͬһ���ļ�,�������Ǿ���Ωһ��
	if (USERPRIORITY_LOCAL == iPriority
		||USERPRIORITY_ONLY_DISPATCH == iPriority
		||ROUTER_TO_DISPATCH_DESK == iPriority){

		//������ת����xml�Ľṹ
		xdiaplan = cari_common_parseXmlFromFile(diaplanFile);
		if (!xdiaplan){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
			goto end_flag;
		}
		xcontext = switch_xml_find_child(xdiaplan, "context", "name", "default");
		if (!xcontext) {
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
			goto end_flag;
		}
		/************************************************************************/
		/*                                                                      */
		/************************************************************************/
		if (USERPRIORITY_LOCAL == iPriority){ //����
			bRes = setDialPlanAttr(xcontext,"local","destination_number",strExpression.c_str());
		}
		if (ROUTER_TO_DISPATCH_DESK == iPriority){ //�������̨
			bRes = setDialPlanAttr(xcontext,"hover","destination_number",strExpression.c_str());
		}
		else if (USERPRIORITY_ONLY_DISPATCH == iPriority){//ֻ������
			bRes = setDialPlanAttr(xcontext,"dispatcher","destination_number",strExpression.c_str());
		}
		if (!bRes){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
			goto end_flag;
		}
		//���ڴ�ṹת�����ִ�
		diaplanContext = switch_xml_toxml(xdiaplan, SWITCH_FALSE);
		if (!diaplanContext) {
			strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
		//���´����ļ�
		bRes = cari_common_creatXMLFile(diaplanFile,diaplanContext,err);
		if (!bRes){
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//"����·��"������Ωһ��,�������,���޸�,����,����
	else{
		//������ת����xml�Ľṹ
		xoutdial = cari_common_parseXmlFromFile(outDialFile);
		if (!xoutdial){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), outDialFile);
			goto end_flag;
		}

		//���ݰ󶨵������������Ҷ�Ӧ��out·��
		xoutGateway = switch_xml_find_child(xoutdial, "extension", "name", strGateWay.c_str());
		if (!xoutGateway) {//out·�ɲ������򴴽�
			//�ȹ���xml���ڴ�ṹ
			outGatewayContext = switch_mprintf(OUT_DIALPLAN_XML_CONTEXT, 
				strGateWay.c_str(),
				strExpression.c_str(),
				strGateWay.c_str(),
				strGateWay.c_str());

			//������ת����xml�Ľṹ
			xoutGateway = switch_xml_parse_str(outGatewayContext, strlen(outGatewayContext));
			//switch_safe_free(outGatewayContext);//�˴��ͷŻ������ڴ�����!!!
			                                      //�ڽ�����д�뵽�ļ����Ժ��ٽ����ͷ�
			if (!xoutGateway) {
				strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
				goto end_flag;
			}
			if (!switch_xml_insert(xoutGateway, xoutdial, INSERT_INTOXML_POS)) {
				goto end_flag;
			}
		}
		else{//out·���Ѿ�����,���޸�,ֻ��Ҫ�޸�condition��expression��
			 //�� :<condition field="destination_number" expression="^9(\d{4,5})$">
			bRes = setOutDialPlanAttr(xoutGateway,"destination_number",strExpression.c_str());
			if (!bRes){
				strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
				goto end_flag;
			}
		}
		//���ڴ�ṹת�����ִ�
		outdialContext = switch_xml_toxml(xoutdial, SWITCH_FALSE);
		if (!outdialContext) {
			strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
		//���´����ļ�
		bRes = cari_common_creatXMLFile(outDialFile,outdialContext,err);
		if (!bRes){
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

    //////////////////////////////////////////////////////////////////////////
	//�������¼���root xml�������ļ�.��ԭʼ�������ļ�freeswitch.xml,�ȼ��ص�bak(����)�ļ���
	iFunRes = cari_common_reset_mainroot(err);
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("ROUTER_SET_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str());

/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(xdiaplan);
	switch_xml_free(xoutdial);

	switch_safe_free(outGatewayContext);//����ͷ�
	switch_safe_free(diaplanFile);
	switch_safe_free(outDialFile);
	switch_safe_free(pDesc);
	return iFunRes;

}

/*ɾ��·��,ֻ��ɾ��"����"·��.
*Ŀǰ"����","�������̨"��"ֻ�ܺ��е���"�ǹ̶���,������ɾ��
 */
int CModule_Route::delRouter(inner_CmdReq_Frame*& reqFrame, 
							 inner_ResultResponse_Frame*& inner_RespFrame, 
							 bool bReLoadRoot)
{
	int				iFunRes			= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode	= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc	= "";
	const char		*errArry[1]		= {
		0
	};
	const char		**err			= errArry;
	char            *pDesc          = NULL;
	bool	        bRes			= true;

	//�漰������
	string			strParamName, strValue, strChinaName;
	string			strRouteType,strGateWay;
	string			strTmp,strBefore,strExpression,strValidPrefix,strValidPostfix;
	int             ipos=-1,iType = -1,iPriority=-1;
	vector<string>  vecPrefixs,vecTmp;//���"ǰ׺"������

	//////////////////////////////////////////////////////////////////////////
	//�Բ������н���
	//"type"
	strParamName = "type";//Ĭ��Ϊ"�û�����"·����������
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	trim(strValue);
	if (0 == strValue.size()) {
		//�Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("out",strValue)){       //����
		iPriority = USERPRIORITY_OUT;
	}
	else{
		//��֧����������·�ɵ�ɾ��
		strReturnDesc = getValueOfDefinedVar("DEL_ROUTER_TYPE_ERR");
		pDesc = (char*)strReturnDesc.c_str();
		goto end_flag;
	}
	strRouteType = strValue;

	//"gateway"
	strParamName = "gateway";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//�Ǳ�ѡ����
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (!isExistedGateWay(INTERNAL_PROFILE_TYPE, strValue.c_str())) {
		//���ز�����
		strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strGateWay = strValue;

	//ɾ��"����"·��
	bRes = delOutRouter(false,strGateWay,pDesc);
	if (!bRes){
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//�������¼���root xml�������ļ�.��ԭʼ�������ļ�freeswitch.xml,�ȼ��ص�bak(����)�ļ���
	iFunRes = cari_common_reset_mainroot(err);
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//�ɹ�
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("ROUTER_DEL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str());

	/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;

}

/*��ѯ·��
*/
int CModule_Route::lstRouter(inner_CmdReq_Frame*& reqFrame, 
							 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnResult = "";
	char *strTableName = "";//��ѯ�ı�ͷ
	char *pDesc                     = NULL;
	string tableHeader[5] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("TYPE"),
		getValueOfDefinedVar("ROUTER_PREFIX"),
		getValueOfDefinedVar("ROUTER_LEN"),
		getValueOfDefinedVar("ROUTER_GATEWAY"),
	};

	//default:��·�ɶ�Ӧ�������ļ�
	char *diaplanFile = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",//diaplanĿ¼
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_DIAPLAN_DEF_XML_FILE
		);
	//00_out_dial.xml:����·�ɵ������ļ�
	char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",           //diaplanĿ¼
		SWITCH_PATH_SEPARATOR,
		"default",            //defaultĿ¼
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_OUT_DIAPLAN_XML_FILE
		);

	switch_xml_t xdiaplan=NULL,xoutdial=NULL,xcontext=NULL;
	char *diaplanContext=NULL,*nesContext=NULL;
	int iRecordCout = 0,iLen=1,ipos=-1;//��¼����
	string strReturnDesc = "",strRecord = "",strType,strLen,strSeq,strRouterInfo,strPrefix;	
	

	//////////////////////////////////////////////////////////////////////////
	//��"����·��"���ļ�����ת����xml�Ľṹ
	xoutdial = cari_common_parseXmlFromFile(outDialFile);
	if (xoutdial){
		//1 ���"����"��·����Ϣ
		getOutRouterInfo(xoutdial,strRouterInfo);
		if (0 != strRouterInfo.length()){
			vector<string> vecSingleOutInfo;
			int iOutSize =0;
			string strSingleInfo;
			vecSingleOutInfo.clear();
			splitString(strRouterInfo,CARICCP_ENTER_KEY,vecSingleOutInfo);
			iOutSize = vecSingleOutInfo.size();
			for (int i=0;i<iOutSize;i++){
				strSingleInfo = getValueOfVec(i,vecSingleOutInfo);
				iRecordCout++;//��ŵ���
				strSeq = intToString(iRecordCout);
				strRecord ="";
				strRecord.append(strSeq);
				strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
				strRecord.append(strSingleInfo);

				//����¼��ŵ�������
				inner_RespFrame->m_vectTableValue.push_back(strRecord);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//��"���ֺ���","�������̨"�Լ�"ֻ�ܺ��е���"�ļ�����ת����xml�Ľṹ
	xdiaplan = cari_common_parseXmlFromFile(diaplanFile);
	if (!xdiaplan){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
		goto end_flag;
	}
	xcontext = switch_xml_find_child(xdiaplan, "context", "name", "default");
	if (!xcontext) {
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
		goto end_flag;
	}

	//2 ���"����"����·����Ϣ
	getLocalRouterInfo(xcontext,strRouterInfo);
	if (0 != strRouterInfo.length()){
		iRecordCout++;//��ŵ���
		strSeq = intToString(iRecordCout);
		strRecord ="";
		strRecord.append(strSeq);
		strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRecord.append(strRouterInfo);

		//����¼��ŵ�������
		inner_RespFrame->m_vectTableValue.push_back(strRecord);
	}

	//3 ���"�������̨"��·����Ϣ
	strRouterInfo="";
	getDispatchRouterInfo(xcontext,ROUTER_TO_DISPATCH_DESK,strRouterInfo);
	if (0 != strRouterInfo.length()){
		iRecordCout++;//��ŵ���
		strSeq = intToString(iRecordCout);
		strRecord ="";
		strRecord.append(strSeq);
		strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRecord.append(strRouterInfo);

		//����¼��ŵ�������
		inner_RespFrame->m_vectTableValue.push_back(strRecord);
	}

	//4 ���"ֻ���е���"��·����Ϣ
	strRouterInfo="";
	getDispatchRouterInfo(xcontext,USERPRIORITY_ONLY_DISPATCH,strRouterInfo);
	if (0 != strRouterInfo.length()){
		iRecordCout++;//��ŵ���
		strSeq = intToString(iRecordCout);
		strRecord ="";
		strRecord.append(strSeq);
		strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRecord.append(strRouterInfo);

		//����¼��ŵ�������
		inner_RespFrame->m_vectTableValue.push_back(strRecord);
	}
	
	//������Ϣ���ͻ���,���������·�����Ϣ
	if (0 == iRecordCout) {
		iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
		if (switch_strlen_zero(strReturnDesc.c_str())) {
			strReturnDesc = getValueOfDefinedVar("RECORD_NULL");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		}
		goto end_flag;
	} 

	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s", //��ͷ
		tableHeader[0].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[1].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[2].c_str()	    ,CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[3].c_str()	    ,CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[4].c_str());

	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("ROUTER_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

	//���ñ�ͷ�ֶ�
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);


/*--goto��ת��--*/
end_flag:

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//�ͷ�xml�ṹ
	switch_xml_free(xoutdial);
	switch_xml_free(xdiaplan);
	
	switch_safe_free(diaplanFile);
	switch_safe_free(outDialFile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*���Ͷ���Ϣ����
*/
int CModule_Route::sendMessage(inner_CmdReq_Frame*& reqFrame, 
							   inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";

	string          strLocalIP                              = "";


	//�漰������
	string			strParamName, strValue;
	string          strFromID,strToID,strMsg;
	string			strTmp;

	char *to_addr=NULL;
	char *from_addr=NULL;
	char *full_from=NULL;
	char *msg = "test";

	from_addr = "";

	//"fromID"
	strParamName = "fromID";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		return iFunRes;
	}
	strFromID = strValue;

	//"toID"
	strParamName = "toID";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		return iFunRes;
	}
	strToID = strValue;

	//"msg"
	strParamName = "msg";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	strMsg = strValue;

	//��װ����sms�Ĺؼ��ֶ�
	msg = (char *)strMsg.c_str();

	strLocalIP = getDomainIP();
	full_from = switch_mprintf("<sip:%s@%s:%s>",
		strFromID.c_str(),
		strLocalIP.c_str(),
		"5060");

	from_addr = switch_mprintf("%s@%s",
		strFromID.c_str(),
		strLocalIP.c_str());

	to_addr = switch_mprintf("%s@%s",
		strToID.c_str(),
		strLocalIP.c_str());

	//���÷��Ͷ��ŵĽӿ�
	switch_core_chat_send(/*proto*/"sip", "sip", from_addr, to_addr, "", msg, NULL, full_from);

	switch_safe_free(full_from);
	switch_safe_free(from_addr);
	switch_safe_free(to_addr);
	return iFunRes;
}

//////////////////////////////////////////////////////////////////////////
/*
 *�鿴�����Ƿ����,���ļ�Ŀ¼�в��һ���ڴ�����,����Ӧ����һ����
*/
bool CModule_Route::isExistedGateWay(const char *profileType, //profile��Ŀ¼
									 const char *gatewayname) //�����ļ���
{
	bool	b1		= false,b2 = false;
	switch_xml_t	x_root, x_section, x_configuration, x_profiles, x_profile, x_gateways, x_gateway;
	const char		*gateway_name;
	char			*path	= getSipProfileDir();
    char *pFile = switch_mprintf("%s%s%s%s%s", path, profileType, SWITCH_PATH_SEPARATOR, gatewayname, CARICCP_XML_FILE_TYPE);
	b1 = cari_common_isExistedFile(pFile);
	switch_safe_free(path);
	switch_safe_free(pFile);

	//���ڴ��в���,��֤�ɿ�
	x_root = switch_xml_root();//ע��:�˺��������,������Ҫ�ͷ�
	if (!x_root) {
		goto end;
	}
	x_section = switch_xml_find_child(x_root, "section", "name", "configuration");
	if (!x_section) {
		goto end;
	}
	x_configuration = switch_xml_find_child(x_section, "configuration", "name", "sofia.conf");
	if (!x_configuration) {
		goto end;
	}
	x_profiles = switch_xml_child(x_configuration, "profiles");
	if (!x_profiles) {
		goto end;
	}
	x_profile = switch_xml_find_child(x_profiles, "profile", "name", profileType);
	if (!x_profile) {
		goto end;
	}
	x_gateways = switch_xml_child(x_profile, "gateways");
	if (!x_gateways) {
		goto end;
	}

	//����ÿ��gateway
	x_gateway = switch_xml_child(x_gateways, "gateway");
	for (; x_gateway; x_gateway = x_gateway->next) {
		//���ҵ���Ӧ����,��ʽ :<group name="default">
		gateway_name = switch_xml_attr(x_gateway, "name");
		if (gateway_name) {
			if (!strcasecmp(gateway_name, gatewayname)) {
				b2 = true;
				break;
			}
		}
	}


end :
	//add by xxl 2010-10-18 begin :��ȡ���root�ڴ�ṹ,RWLOCK������ס,�����Ҫ�ͷ�
	if (x_root){
		switch_xml_free(x_root);//�ͷ���
	}
	//add by xxl 2010-10-18 end

	return (bool) (b1 || b2);
}

/*�������ص����Ի���������
 *ע���ͷ��ڴ�����
*/
bool CModule_Route::setGatewayAttribute(switch_xml_t gatewayinfo, 
										const char *attName,  
										const char *attVal, 	 
										bool bAddattr)
{
	bool	isExistAttr		= false;
	switch_xml_t	x_param, x_newChild;
	char			*newParamContext	= NULL;

	for (x_param = switch_xml_child(gatewayinfo, "param"); x_param; x_param = x_param->next) {
		const char	*var	= switch_xml_attr(x_param, "name");
		const char	*val	= switch_xml_attr(x_param, "value");

		//������ҵ���ͬ������
		if (!strcasecmp(attName, var) && val) {
			//�滻��ǰ������ֵ
			//������-ֵ��,��: <param name="username" value="cluecon"/>
			switch_xml_set_attr(x_param, "value", attVal);

			isExistAttr = true;
			break;
		}
	}

	//��������ӽڵ㲻����,������������,�����´���
	if (!isExistAttr && bAddattr) {
		
		//ע��:�˽ṹ���ͷ�����
		newParamContext = switch_mprintf(GATEWAY_PARAM, attName, attVal);
		x_newChild = switch_xml_parse_str(newParamContext, strlen(newParamContext));

		//�˴����ڴ��Ǻ�xml����󶨵�,�����ͷ�,��������x_newChild�ṹ�쳣
		//Ӧ�ÿ����ͷ�xml�Ľṹ
		//switch_safe_free(newParamContext);
	
		if (!x_newChild) {
			return false;
		}
		if (!switch_xml_insert(x_newChild, gatewayinfo, INSERT_INTOXML_POS)) {
			
			//���õ����
			return false;
		}
	}

	return true;
}

/*���ĳ������
*/
char * CModule_Route::getGatewayParamAttribute(switch_xml_t gatewayXMLinfo, 
											   const char *attName)
{
	switch_xml_t	x_param;
	char			*attVal	= NULL;
	for (x_param = switch_xml_child(gatewayXMLinfo, "param"); x_param; x_param = x_param->next) {
		const char	*var	= switch_xml_attr(x_param, "name");
		const char	*val	= switch_xml_attr(x_param, "value");

		//������ҵ���ͬ������
		if (!strcasecmp(attName, var) && val) {
			attVal = (char*) val;
			return attVal;
		}
	}

	//return NULL;
	return CARICCP_NULL_MARK;
}

/*���profileTypeĿ¼��dateway��xml�ṹ
*/
switch_xml_t CModule_Route::getGatewayXMLInfo(const char *profileType, //profile��Ӧ��Ŀ¼
											  const char *gatewayname)//��������
{
	switch_xml_t	x_gateway		= NULL;
	char			*gateway_xmlfile	= getSipProfileDir();
	gateway_xmlfile = switch_mprintf("%s%s%s%s%s", gateway_xmlfile, profileType, SWITCH_PATH_SEPARATOR, gatewayname, CARICCP_XML_FILE_TYPE);

	if (!gateway_xmlfile) {
		goto end;
	}

	x_gateway = cari_common_parseXmlFromFile(gateway_xmlfile);


end :

	switch_safe_free(gateway_xmlfile);
	return x_gateway;
}

/*����extension -> condition����
*/
bool CModule_Route::setDialPlanAttr(switch_xml_t xcontext, 
								    const char* externName, 
								    const char* condiFieldName, 
								    const char* expressVal)
{
	switch_xml_t xexten,xcondition;
	xexten = switch_xml_find_child(xcontext, "extension", "name", externName);
	if (!xexten) {
		return false;
	}
	xcondition = switch_xml_find_child(xexten, "condition", "field", condiFieldName);
	if (!xcondition) {
		return false;
	}

	//��������
	switch_xml_set_attr(xcondition, "expression", expressVal);

	return true;
}

/*����out����·��
*/
bool CModule_Route::setOutDialPlanAttr(switch_xml_t xextension,
									   const char* condiFieldName, 
									   const char* expressVal)
{
	switch_xml_t xcondition;
	xcondition = switch_xml_find_child(xextension, "condition", "field", condiFieldName);
	if (!xcondition) {
		return false;
	}

	//��������
	switch_xml_set_attr(xcondition, "expression", expressVal);

	return true;
}

/*���ڴ��л�����ص���Ϣ
*ע��:һ��Ҫ�ͷ��ڴ�
*/
void CModule_Route::getAllGatewaysFromMem(char **allGateways)
{
	switch_xml_t	x_root, x_section, x_configuration, x_profiles, x_profile, x_gateways, x_gateway;
	const char		*profilename, *gateway_name;
	char			*username,*pwd,/**realm,*/*proxy,/**reg,*/*transport/*,*contact, *ping*/;


	//���ڴ��в���,��֤�ɿ�
	x_root = switch_xml_root();//ע��:�˺��������,������Ҫ�ͷ�
	if (!x_root) {
		goto end;
	}
	x_section = switch_xml_find_child(x_root, "section", "name", "configuration");
	if (!x_section) {
		goto end;
	}
	x_configuration = switch_xml_find_child(x_section, "configuration", "name", "sofia.conf");
	if (!x_configuration) {
		goto end;
	}
	x_profiles = switch_xml_child(x_configuration, "profiles");
	if (!x_profiles) {
		goto end;
	}

	//����ÿ��profile
	x_profile = switch_xml_child(x_profiles, "profile");
	for (; x_profile; x_profile = x_profile->next) {
		//profile������
		profilename = switch_xml_attr(x_profile, "name");
		x_gateways = switch_xml_child(x_profile, "gateways");

		//����ÿ��gateway
		x_gateway = switch_xml_child(x_gateways, "gateway");
		for (; x_gateway; x_gateway = x_gateway->next) {
			//��������
			gateway_name = switch_xml_attr(x_gateway, "name");

			//���ҵ���Ӧ���������
			username = getGatewayParamAttribute(x_gateway, "username");
			//realm = getGatewayParamAttribute(x_gateway, "realm");
			pwd = getGatewayParamAttribute(x_gateway, "password");
			proxy = getGatewayParamAttribute(x_gateway, "proxy");
			//reg = getGatewayParamAttribute(x_gateway, "register");
			transport = getGatewayParamAttribute(x_gateway, "register-transport");
			//contact = getGatewayParamAttribute(x_gateway, "contact-params");
			//ping = getGatewayParamAttribute(x_gateway, "ping");

			/*********************************************/
			//��װ����,����ͷ��ڴ�???
			*allGateways = switch_mprintf("%s%s%s%s%s%s%s%s%s", 
				gateway_name, CARICCP_SPECIAL_SPLIT_FLAG, 
				username    , CARICCP_SPECIAL_SPLIT_FLAG, 
				pwd         , 	CARICCP_SPECIAL_SPLIT_FLAG, 
				proxy       , CARICCP_SPECIAL_SPLIT_FLAG, 
				transport);

			//λ�ú���һλ
			allGateways++;
		}
	}

end :
	//add by xxl 2010-10-18 begin :��ȡ���root�ڴ�ṹ,RWLOCK������ס,�����Ҫ�ͷ�
	if (x_root){
		switch_xml_free(x_root);//�ͷ���
	}
	//add by xxl 2010-10-18 end

	return;
}

/*�������ص�xml�ļ�
*/
bool CModule_Route::creatGateWayXMLFile(const char *profileType, //profile����,����xml�ļ���ŵ�Ŀ¼
												 const char *gatewayname, //������
												 const char *filecontext, //�ļ�����
												 const char **err)  	 //ɾ������,������Ϣ
{
	char *gatewaypath	= NULL;
	if (!filecontext) {
		return false;
	}

	char *pDir = getSipProfileDir();
	gatewaypath = switch_mprintf("%s%s%s%s%s", pDir, profileType, SWITCH_PATH_SEPARATOR, gatewayname, CARICCP_XML_FILE_TYPE);
	bool bRes = cari_common_creatXMLFile(gatewaypath, (char*) filecontext, err);

	switch_safe_free(pDir);
	switch_safe_free(gatewaypath);
	return bRes;
}

/*ɾ�����ص�xml�ļ�
*/
bool CModule_Route::delGateWayXMLFile(const char *profileType, //profile����,����xml�ļ���ŵ�Ŀ¼   
									  const char *gatewayname, //������
									  const char **err)	   //ɾ������,������Ϣ
{
	char *gatewaypath	= NULL;
	char *pDir = getSipProfileDir();
	gatewaypath = switch_mprintf("%s%s%s%s%s", pDir, profileType, SWITCH_PATH_SEPARATOR, gatewayname, CARICCP_XML_FILE_TYPE);
	bool bRes =  cari_common_delFile(gatewaypath, err);

	switch_safe_free(pDir);
	switch_safe_free(gatewaypath);
	return bRes;
}

/*���"����"·����Ϣ,Ŀǰ����Ωһ��
*/
void CModule_Route::getLocalRouterInfo(switch_xml_t xDialContext, 
									   string &strOut)
{
	switch_xml_t xexten,xcondition;
	const char *routerInfo = NULL;
	string strRouterInfo/*,strPrefix*/;
	int iLen=1,ipos=-1;//��¼����
	string strReturnDesc = "",strRecord = "",strType,strLen;	
	vector<string>  vecPrefixs;//���"ǰ׺"������
	vector<string>::iterator it;

	strOut = "";
	xexten = switch_xml_find_child(xDialContext, "extension", "name", "local");//���Ҵ�����
	if (!xexten) {
		return;
	}
	xcondition = switch_xml_find_child(xexten, "condition", "field", "destination_number");
	if (!xcondition) {
		return;
	}

	strType = getValueOfDefinedVar("ROUTER_LOCAL");
	routerInfo = switch_xml_attr(xcondition, "expression");
	if (switch_strlen_zero(routerInfo)){
		return;
	}

	//���ݴ���Ϣ��ö�Ӧ��"ǰ׺"�ͳ���,�� :^([0-9][0-9][0-9][0-9])$
	strRouterInfo = routerInfo;

	//mod by xxl 2010-10-21 begin :�������͵Ľ�������޸�
	//splitString(strRouterInfo,CARICCP_RIGHT_SQUARE_BRACKETS,vecPrefixs);
	//iLen = vecPrefixs.size()-2;//��ȥǰ׺�ͽ�����2������
	//strLen = intToString(iLen);
	//it = vecPrefixs.begin();
	//strPrefix = *it;
	//ipos = strPrefix.find_first_of(CARICCP_LEFT_SQUARE_BRACKETS);
	//if (-1 != ipos){
	//	strPrefix = strPrefix.substr(ipos+1);
	//}
	////�Ż�һ��:Ϊ�˷�������û��鿴,��"|"��װ��","
	//replace_all(strPrefix,CARICCP_OR_MARK,CARICCP_COMMA_MARK);

	//strOut.append(strType);
	//strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	//strOut.append(strPrefix);
	//strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	//strOut.append(strLen);

	//����һ�°�����ĸ�ʽ���
	//ע:��Ҫ��������⴦��,һ�����ô���,Ҳ���Ը��ݲ�ѯ�Ľ�����в鿴�޸�.
	//����������������Ѷ�λ.
	ipos = strRouterInfo.find_first_of(CARICCP_LEFT_ROUND_BRACKETS);
	if (-1 != ipos){
		strRouterInfo = strRouterInfo.substr(ipos+1);
	}
	ipos = strRouterInfo.find_last_of(CARICCP_RIGHT_ROUND_BRACKETS);
	if (-1 != ipos){
		strRouterInfo = strRouterInfo.substr(0,ipos);
	}
	replace_all(strRouterInfo,CARICCP_LEFT_SQUARE_BRACKETS,CARICCP_EMPTY_MARK);
	replace_all(strRouterInfo,CARICCP_RIGHT_SQUARE_BRACKETS,CARICCP_COMMA_FLAG);
	ipos = strRouterInfo.find_last_of(CARICCP_COMMA_FLAG);
	if ((int)(strRouterInfo.length()-1) == ipos){
		strRouterInfo = strRouterInfo.substr(0,ipos);
	}

	strOut.append(strType);
	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append(strRouterInfo);
	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append("0");
	//mod by xxl 2010-10-21 end :�������͵Ľ�������޸�

	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append("");//����ΪNULL,���漰

}

/*�����"����"�йص�·����Ϣ:ֻ���е��ȺͲ������̨
 *Ŀǰ����Ωһ��
*/
void CModule_Route::getDispatchRouterInfo(switch_xml_t xDialContext,
										  int type,
									      string &strOut)
{
	switch_xml_t xexten,xcondition;
	const char *routerInfo = NULL;
	string strRouterInfo,strPrefix;
	int iLen=1,ipos=-1;//��¼����
	string strReturnDesc = "",strRecord = "",strType,strLen;	
	vector<string>  vecPrefixs;//���"ǰ׺"������
	vector<string>::iterator it;

	strOut = "";
	if (ROUTER_TO_DISPATCH_DESK == type){
		strType = getValueOfDefinedVar("ROUTER_TO_DISPATCH_DESK");
		xexten = switch_xml_find_child(xDialContext, "extension", "name", "hover");
	}
	else{
		strType = getValueOfDefinedVar("ROUTER_DISPATCH");
		xexten = switch_xml_find_child(xDialContext, "extension", "name", "dispatcher");
	}
	if (!xexten) {
		return;
	}
	xcondition = switch_xml_find_child(xexten, "condition", "field", "destination_number");
	if (!xcondition) {
		return;
	}

	
	routerInfo = switch_xml_attr(xcondition, "expression");
	if (switch_strlen_zero(routerInfo)){
		return;
	}

	//���ݴ���Ϣ��ö�Ӧ��"ǰ׺"�ͳ���,�� :^0$ �� ^9(\d{4})$
	strRouterInfo = routerInfo;

	//ȥ����λ��ʶ��
	iLen = strRouterInfo.length();
	strRouterInfo = strRouterInfo.substr(1,iLen-2);
	//�Ƿ���"("
	ipos = strRouterInfo.find_first_of(CARICCP_LEFT_ROUND_BRACKETS);
	if (-1 != ipos){
		strPrefix = strRouterInfo.substr(0,ipos);
		int ipos1 = strRouterInfo.find_first_of(CARICCP_LEFT_BIG_BRACKETS);
		int ipos2 = strRouterInfo.find_first_of(CARICCP_RIGHT_BIG_BRACKETS);
		strLen = strRouterInfo.substr(ipos1+1,ipos2-ipos1-1);
	}
	else{
		strPrefix = strRouterInfo;
		//Ωһֵ,�������볤��Ϊ0
		iLen =0;
		strLen = intToString(iLen);
	}

	strOut.append(strType);
	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append(strPrefix);
	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append(strLen);

	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append("");//����ΪNULL,���漰
}

/*���"����"��·����Ϣ
*/
void CModule_Route::getOutRouterInfo(switch_xml_t xOutDialContext, 
									 string &strOut)
 {
	 switch_xml_t xextension,xcondition;
	 const char *gatewayname = NULL,*field=NULL,*routerInfo=NULL;
	 string strType,strRouterInfo,strPrefix,strLen;
	 int iLen=0,ipos=0;
	 
	 strType =getValueOfDefinedVar("ROUTER_OUT");
	 strOut ="";

	 //����ÿ��"����"·������
	 xextension = switch_xml_child(xOutDialContext, "extension");
	  for (; xextension; xextension = xextension->next) {
		  gatewayname = switch_xml_attr(xextension, "name");//������
		  if (switch_strlen_zero(gatewayname)) {
			  continue;
		  }
		  xcondition = switch_xml_find_child(xextension,"condition","field","destination_number");
		  if (!xcondition){
			  continue;
		  }
		  routerInfo = switch_xml_attr(xcondition, "expression");
		  if (!routerInfo){
			  continue;
		  }

		  //���ݴ���Ϣ��ö�Ӧ��"ǰ׺"�ͳ���,�� :^0$ �� ^9(\d{4})$
		  strRouterInfo = routerInfo;

		  //ȥ����λ��ʶ��
		  iLen = strRouterInfo.length();
		  strRouterInfo = strRouterInfo.substr(1,iLen-2);
		  //�Ƿ���"("
		  ipos = strRouterInfo.find_first_of(CARICCP_LEFT_ROUND_BRACKETS);
		  if (-1 != ipos){
			  strPrefix = strRouterInfo.substr(0,ipos);
			  int ipos1 = strRouterInfo.find_first_of(CARICCP_LEFT_BIG_BRACKETS);
			  int ipos2 = strRouterInfo.find_first_of(CARICCP_RIGHT_BIG_BRACKETS);
			  strLen = strRouterInfo.substr(ipos1+1,ipos2-ipos1-1);
		  }
		  else{
			  strPrefix = strRouterInfo;
			  //Ωһֵ,�������볤��Ϊ0
			  iLen =0;
			  strLen = intToString(iLen);
		  }

		  //��װ��ʽ
		  strOut.append(strType);
		  strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
		  strOut.append(strPrefix);
		  strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
		  strOut.append(strLen);
		  strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
		  strOut.append(gatewayname);//����

		  strOut.append(CARICCP_ENTER_KEY);
	  }
 }

/*ɾ������·��
*/
bool CModule_Route::delOutRouter(bool bSpecial,
								 string gateway,
								 /*string &strErr*/char *&pDesc)//�ڴ���ͷ��ɵ��ú�������
 {
	 bool bRes = false;
	 string strReturnDesc;
	 switch_xml_t xoutdial=NULL,xoutGateway=NULL;
	 char *diaplanContext=NULL,*outdialContext=NULL;
	 const char *errArry[1] = {
		 0
	 };
	 const char **err = errArry;

	 //00_out_dial.xml:����·�ɵ������ļ�
	 char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		 SWITCH_GLOBAL_dirs.conf_dir, 
		 SWITCH_PATH_SEPARATOR, 
		 "dialplan",           //diaplanĿ¼
		 SWITCH_PATH_SEPARATOR,
		 "default",            //defaultĿ¼
		 SWITCH_PATH_SEPARATOR,
		 CARI_CCP_OUT_DIAPLAN_XML_FILE
		 );

	 //������ת����xml�Ľṹ,����ļ�������,��Ҳ��Ϊ�ɹ�!!!
	 xoutdial = cari_common_parseXmlFromFile(outDialFile);
	 if (!xoutdial){
		 if (bSpecial){
			 bRes = true;
		 }
		 else{
			 strReturnDesc = getValueOfDefinedVar("ROUTER_DEL_NOGATEWAY");
			 pDesc = switch_mprintf(strReturnDesc.c_str(),gateway.c_str());
		 }
		 goto end_flag;
	 }
	 xoutGateway = switch_xml_find_child(xoutdial, "extension", "name", gateway.c_str());
	 if (!xoutGateway) {//������û�а�·��,���ش�����Ϣ
		 if (bSpecial){
			 bRes = true;
		 }
		 else{
			 strReturnDesc = getValueOfDefinedVar("ROUTER_DEL_NOGATEWAY");
			 pDesc = switch_mprintf(strReturnDesc.c_str(),gateway.c_str());
		 }
		 goto end_flag;
	 }

	 //out·�ɴ���,��ɾ��
	 /************************************************************************/
	 switch_xml_remove(xoutGateway);
	 /************************************************************************/

	 //���ڴ�ṹת�����ִ�
	 outdialContext = switch_xml_toxml(xoutdial, SWITCH_FALSE);
	 if (!outdialContext) {
		 strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		 pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		 goto end_flag;
	 }
	 //���´����ļ�
	 bRes = cari_common_creatXMLFile(outDialFile,outdialContext,err);
	 if (!bRes){
		 strReturnDesc = *err;
		 pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		 goto end_flag;
	 }

	 //�������ɹ�
	 bRes = true;


end_flag:

     switch_xml_free(xoutdial);//�ͷ�

	 switch_safe_free(outDialFile);
	 return bRes;
 }

/*�޸ĳ���·��
*/
bool CModule_Route::modOutRouter(string oldGateway,
								 string newGateway,
								 /*string &strErr*/char *&pDesc)
{
	bool bRes = false;
	string strReturnDesc;
	switch_xml_t xoutdial=NULL,xoutGateway=NULL,xcondition=NULL,xaction=NULL,xaction_export=NULL,xaction_conference=NULL;
	char *diaplanContext=NULL,*outdialContext=NULL,*chDataExport=NULL,*chDataConference=NULL;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;

	//00_out_dial.xml:����·�ɵ������ļ�
	char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",           //diaplanĿ¼
		SWITCH_PATH_SEPARATOR,
		"default",            //defaultĿ¼
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_OUT_DIAPLAN_XML_FILE
		);

	char *chOldData = switch_mprintf("context_extension=%s", 
		oldGateway.c_str());

	//������ת����xml�Ľṹ,����ļ�������,��Ҳ��Ϊ�ɹ�!!!
	xoutdial = cari_common_parseXmlFromFile(outDialFile);
	if (!xoutdial){
		bRes = true;
		goto end_flag;
	}
	xoutGateway = switch_xml_find_child(xoutdial, "extension", "name", oldGateway.c_str());
	if (!xoutGateway) {//�����ںͳ������ذ�,��Ҳ��Ϊ�ɹ�!!!
		bRes = true;
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//�������Ҷ�Ӧ����������:extension->condition->action
	xcondition = switch_xml_find_child(xoutGateway, "condition", "field", "destination_number");
	if (!xcondition){
		bRes = true;
		goto end_flag;
	}

	//�������ҷ���������������
	xaction = switch_xml_child(xcondition, "action");
	for (; xaction; xaction = xaction->next) {
		// ���Ҷ�Ӧ��2������
		const char *appVal = switch_xml_attr(xaction, "application");
		const char *dataVal = switch_xml_attr(xaction, "data");

		//���Ҷ�Ӧ�Ĺؼ�������,һ�����ǿ�ֵ
		if (switch_strlen_zero(appVal)
			||switch_strlen_zero(dataVal)) {
				
				continue;
		}
		//"export"��"context_extension=��������",2�����������ȷ����Ӧ��xml�ṹ
		if ((!strcasecmp("export",appVal))
			&&(!strcasecmp(chOldData,dataVal))){
				xaction_export = xaction;
				continue;
		}
		//"conference"
		if (!strcasecmp("conference",appVal)){
			xaction_conference = xaction;
			continue;
		}
	}

	//�������漰��������ҲҪ�޸�,xaction_set�漰��2��������Ĳ���,����ķ���������
	//xaction_set        = switch_xml_find_child_multi(xcondition, "action", "application", "set","data",chOldData,NULL);  //action����ĵ�һ��"application="set""����
	//xaction_export     = switch_xml_find_child(xcondition, "action", "application", "export");    //action�����"application="export" "����
	//xaction_conference = switch_xml_find_child(xcondition, "action", "application", "conference");//action�����application="conference"����
	if ((!xaction_export)
		||(!xaction_conference)){
			bRes = true;
			goto end_flag;
	}

	chDataExport = switch_mprintf("context_extension=%s",
		newGateway.c_str());
	chDataConference = switch_mprintf("bridge:${caller_id_number}-[${uuid}]-${destination_number}@default:sofia/gateway/%s/$1",
		newGateway.c_str());

	//��������:���ڵ�extension��Ӧ�����ص����ָ���
	switch_xml_set_attr(xoutGateway, "name", newGateway.c_str());

	//��������:�ӽڵ��漰���������ֵĵط�����Ҫ����
	switch_xml_set_attr(xaction_export    , "data", chDataExport);
	switch_xml_set_attr(xaction_conference, "data", chDataConference);

	//���ڴ�ṹת�����ִ�
	outdialContext = switch_xml_toxml(xoutdial, SWITCH_FALSE);
	if (!outdialContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	//���´����ļ�
	bRes = cari_common_creatXMLFile(outDialFile,outdialContext,err);
	if (!bRes){
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//�������ɹ�
	bRes = true;


end_flag:

    switch_xml_free(xoutdial);//�ͷ�xml�ṹ

	switch_safe_free(outDialFile);
	switch_safe_free(chOldData);
	switch_safe_free(chDataExport);
	switch_safe_free(chDataConference);

	return bRes;
	 
}
