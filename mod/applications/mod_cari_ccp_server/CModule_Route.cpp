//#include "modules\CModule_Route.h"
#include "CModule_Route.h"
//#include "cari_public_interface.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"


//////////////////////////////////////////////////////////////////////////
//mob user的完整的xml文件结构,支持中文的显示
#define  GATEWAY_XML_SIMPLE_CONTEXT	"<include>\n"					\
									"	<gateway name=\"%s\">\n"	\
									"	</gateway>\n"				\
									"   </include>";

#define  GATEWAY_PARAM              "<param name=\"%s\" value=\"%s\"/>"

//out dialplan.xml文件每个路由网关的配置信息
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

/*根据接收到的帧,处理具体的命令,分发到具体的处理函数
*/
int CModule_Route::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	if (NULL == inner_reqFrame) {
		//帧为空
		return CARICCP_ERROR_STATE_CODE;
	}

	//请求帧的"源模块"号则是响应帧的"目的模块"号
	//请求帧的"目的模块"号则是响应帧的"源模块"号,可逆的
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;


	int				iFunRes		= CARICCP_SUCCESS_STATE_CODE;
	bool	bRes		= true; 

	//判断命令码来判断命令(命令码是惟一的),也可以根据命令名进行设置
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


	//命令执行不成功
	if (CARICCP_SUCCESS_STATE_CODE != iFunRes) {
		inner_RespFrame->header.bNotify = false;
	} else {
		inner_RespFrame->header.bNotify = inner_reqFrame->body.bNotify;
	}

	/*--------------------------------------------------------------------------------------------*/
	//将处理的结果发送给对应的客户端
	//在命令的接收入口函数中

	//打印日志
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "proc cmd %s end.\n", inner_reqFrame->body.strCmdName);
	return iFunRes;
}

/*发送信息给对应的客户端
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
/*增加网关,网关的配置文件放置在sip_profiles\internal下
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

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strProfileType, strGatewayName, strUsername, strPwd, strRealm, strFromUser, strFromDomain, 
		            strExtension, strProxy, strRegisterProxy, strExpireSeconds, strIsOrNotRegister, strTransportPro,  
					strRetrySeconds, strFromCallerFlag, strContactPrams, strPingSecond;
	string			strTmp;

	////"type"
	//strParamName = "type";//ua类型
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//PROFILE类型是必选参数
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////只能属于两种类型之一
	//if (!isEqualStr(strValue, INTERNAL_PROFILE_TYPE) 
	//	&& !isEqualStr(strValue, EXTERNAL_PROFILE_TYPE)) {
	//		strReturnDesc = getValueOfDefinedVar("PROFILE_TYPE_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strProfileType = /*strValue*/INTERNAL_PROFILE_TYPE;//设置默认类型

	//"gatewayname"
	strParamName = "gatewayname";//gatewayname名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//
		//名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGatewayName = strValue;

	//"useraccount"
	strParamName = "useraccount";//useraccount名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//username类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户名超长
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
		//password类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//超长
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
	//  	//超长
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
	//  	//超长
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
	//  	//超长
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
	//  	//超长
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

		//名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//超长
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	if (!isValidIPV4(strValue)){
		//是否有效的IP地址
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
	//  	//超长
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
		strValue = "false";//非true的情况下一律视为false
	}
	strIsOrNotRegister = strValue;

	//"register-transport"
	strParamName = "register-transport";//register-transport
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "udp";//默认为udp方式
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
	//	strValue = "false";//非true的情况下一律视为false
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
	//	strValue = "10";//默认
	//}
	//strPingSecond = strValue;

	//判断此网关已经配置
	if (isExistedGateWay(strProfileType.c_str(), strGatewayName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("GATEWAY_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//先构建xml的内存结构
	strTmp = GATEWAY_XML_SIMPLE_CONTEXT;
	context = switch_mprintf(strTmp.c_str(), strGatewayName.c_str());

	//将内容转换成xml的结构(注意:中文字符问题)
	x_context = switch_xml_parse_str(context, strlen(context));
	//switch_safe_free(context);//此时释放会造成内存结构出现问题!!~!

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

	//增加很多相关的属性,此处只需要部分属性,其他的部分保留
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

	//创建网关的文件
	bRes = creatGateWayXMLFile(strProfileType.c_str(), strGatewayName.c_str(), gatewayContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}  

	//////////////////////////////////////////////////////////////////////////
	//最后再统一重新加载一下root xml文件
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞
		//switch_xml_open_root(1,err);
	}

	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//增加网关成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("ADD_GATEWAY_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());

/*-----*/
end_flag :

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml结构要及时释放,在设置属性的时候,分配了内存
	//switch_xml_free(x_gateway);
	switch_xml_free(x_context);//释放一次即可  

	switch_safe_free(context);
	switch_safe_free(gatewayContext);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*删除网关
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

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strProfileType, strGatewayName;
	string			strTmp;

	////"type"
	//strParamName = "type";//ua类型
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//type类型是必选参数
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////只能属于两种类型之一
	//if (strcasecmp(strValue.c_str(), INTERNAL_PROFILE_TYPE) && strcasecmp(strValue.c_str(), EXTERNAL_PROFILE_TYPE)) {
	//	strReturnDesc = getValueOfDefinedVar("PROFILE_TYPE_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}
	strProfileType = /*strValue*/INTERNAL_PROFILE_TYPE;//设置默认类型

	//"gatewayname"
	strParamName = "gatewayname";//gatewayname名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

		goto end_flag;
	}
	strGatewayName = strValue;

	//判断此网关没有配置
	if (!isExistedGateWay(strProfileType.c_str(), strGatewayName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//删除网关文件
	bRes = delGateWayXMLFile(strProfileType.c_str(), strGatewayName.c_str(), err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//如果出局"out"路由已经和此网关绑定,此时需要将出局路由也要一起删除
	bRes = delOutRouter(true,strGatewayName,pDesc);
	if (!bRes){
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//最后再统一重新加载一下root xml文件
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());
		goto end_flag;
	}

	//删除网关成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("DEL_GATEWAY_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGatewayName.c_str());


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*修改网关
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

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strProfileType, strOldGatewayName,strNewGatewayName, strUsername, strPwd, strRealm, strFromUser, strFromDomain, strExtension, 
		            strProxy, strRegisterProxy, strExpireSeconds, strIsOrNotRegister, strTransportPro, strRetrySeconds, strFromCallerFlag, 
		            strContactPrams, strPingSecond;
	string			strTmp;

	////"type"
	//strParamName = "type";//ua类型
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//type类型是必选参数
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////只能属于两种类型之一
	//if (!isEqualStr(strValue, INTERNAL_PROFILE_TYPE) 
	//	&& !isEqualStr(strValue, EXTERNAL_PROFILE_TYPE)) {
	//	strReturnDesc = getValueOfDefinedVar("PROFILE_TYPE_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}
	strProfileType = /*strValue*/INTERNAL_PROFILE_TYPE;//设置默认类型

	//"oldgatewayname"
	strParamName = "oldgatewayname";//oldgatewayname
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//
		//名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strOldGatewayName = strValue;

	//"newgatewayname"
	strParamName = "newgatewayname";//newgatewayname名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//gatewayname类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//
		//名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strNewGatewayName = strValue;

	//"newaccount"
	strParamName = "newaccount";//newaccount名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//username类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}

	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户名超长
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
		//password类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//超长
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
	//  	//超长
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
	//  	//超长
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
	//  	//超长
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
	//  	//超长
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

		//名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
	//  	//超长
	//	strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
	//  	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
	//  	goto end_flag;
	//}
	if (!isValidIPV4(strValue)){
		//是否有效的IP地址
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
	//  	//超长
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
		strValue = "false";//非true的情况下一律视为false
	}
	strIsOrNotRegister = strValue;


	//"register-transport"
	strParamName = "register-transport";//register-transport
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "udp";//默认为udp方式
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
	//	strValue = "false";//非true的情况下一律视为false
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
	//	strValue = "10";//默认
	//}
	//strPingSecond = strValue;

	//////////////////////////////////////////////////////////////////////////
	//判断旧网关是否已经配置,没有配置则不能进行修改
	if (!isExistedGateWay(strProfileType.c_str(), strOldGatewayName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGatewayName.c_str());
		goto end_flag;
	}

	//直接从当前网关的配置文件中获得xml结构,此处的处理和上面的"相同"???
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

	//关键:修改网关的名字
	switch_xml_set_attr(x_gateway, "name", strNewGatewayName.c_str());

	//修改很多相关的属性
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
	//修改网关的配置文件,以新的名字进行命名,如果新名字对应的网关目前已经存在,则不允许
	//旧网关名和新网关名不一致
	if (!isEqualStr(strOldGatewayName,strNewGatewayName)){
		//先判断新网关名对应的配置是否已经存在,如果存在则不允许
		if (isExistedGateWay(strProfileType.c_str(), strNewGatewayName.c_str())) {
			strReturnDesc = getValueOfDefinedVar("GATEWAY_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strNewGatewayName.c_str());
			goto end_flag;
		}

		//然后删除原来的旧网关文件
		bRes = delGateWayXMLFile(strProfileType.c_str(), strOldGatewayName.c_str(), err);
		if (!bRes) {
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

	//创建新的xml文件
	bRes = creatGateWayXMLFile(strProfileType.c_str(), strNewGatewayName.c_str(), newgatewayContext, err);
	if (!bRes) {
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//如果此网关已经设置为"出局"out网关,则需要同步修改相关的out路由文件
	if (!isEqualStr(strOldGatewayName,strNewGatewayName)){
		bRes = modOutRouter(strOldGatewayName,strNewGatewayName,pDesc);//修改out路由
		if (!bRes){
			goto end_flag;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	//最后再统一重新加载一下root xml文件
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞
		//switch_xml_open_root(1,err);
	}

	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strNewGatewayName.c_str());
		goto end_flag;
	}

	//修改网关成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("MOD_GATEWAY_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGatewayName.c_str());


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_context);//释放
	switch_safe_free(pDesc);
	return iFunRes;
}

/*查询网关信息
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
	
	//每个组最多CARICCP_MAX_GATEWAY_NUM个网关
	char			*gatewayInfo[CARICCP_MAX_GATEWAY_NUM]	= {
		0
	};
	char			**allGatewayInfo						= gatewayInfo;
	string			strname;
	int iRecordCout = 0;//记录数量
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

	//默认查询所有的网关信息
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

	//遍历每个组
	while (*allGatewayInfo) {
		iRecordCout++;//序号递增
		chTmp = *allGatewayInfo;//获得对应的组信息
		
		//封装查询的格式
		strSeq = intToString(iRecordCout);
		strname ="";
		strname.append(strSeq);
		strname.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strname.append(chTmp);

		//存放记录
		inner_RespFrame->m_vectTableValue.push_back(strname);

		//一定要释放内存
		switch_safe_free(*allGatewayInfo);

		//下一个用户
		allGatewayInfo++;
	}

	//设置表头字段
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

/*设置路由
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

	//default:"呼叫路由"对应的配置文件
	char *diaplanFile = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",//diaplan目录
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_DIAPLAN_DEF_XML_FILE
		);

	//00_out_dial.xml:"出局路由"的配置文件
	char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",           //diaplan目录
		SWITCH_PATH_SEPARATOR,
		"default",            //default目录
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_OUT_DIAPLAN_XML_FILE
		);

	switch_xml_t xdiaplan=NULL,xcontext=NULL,xoutdial=NULL,xoutGateway=NULL;
	char *diaplanContext=NULL,*outdialContext=NULL;

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strRouteType,strPrefix,strMinBackLen,strMaxBackLen,strGateWay,strLocalId,strDispatchId;
	string			strTmp,strBefore,strExpression,strValidPrefix,strValidPostfix;
	int             iMinLen = 0,iMaxLen = 0,ipos=-1,iType = -1,iPriority=-1;
	vector<string>  vecPrefixs,vecTmp;//存放"前缀"的容器
	char             *outGatewayContext=NULL;     //必须要释放这个变量

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"type"
	strParamName = "type";//默认为"用户号码"路由配置类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	trim(strValue);
	if (0 == strValue.size()) {
		//是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("out",strValue)){       //出局
		iPriority = USERPRIORITY_OUT;
	}
	else if (isEqualStr("local",strValue)){//本局
		iPriority = USERPRIORITY_LOCAL;
	}
	else if (isEqualStr("to_dispatch_desk",strValue)){//拨入调度台
		iPriority = ROUTER_TO_DISPATCH_DESK;
	}
	else if (isEqualStr("only_dispatch",strValue)){//只能呼调度
		iPriority = USERPRIORITY_ONLY_DISPATCH;
	}
	else{
		//类型不符合,值无效
		strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strRouteType = strValue;

	//mod by xxl 2010-10-21 begin :本局类型的结果进行修改
	//根据不同的路由类型分别进行处理
	if (USERPRIORITY_OUT == iPriority){//出局
		//"prefix"
		strParamName = "prefix";//号码前缀
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//是必选参数
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
			//超长
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
			goto end_flag;
		}
		strPrefix = strValue;

		//"minbacklen"
		strParamName = "minbacklen";//最小的后续号码长度
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//是必选参数
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iMinLen = stringToInt(strValue.c_str());
		//范围限制
		if (0 > iMinLen || 16 < iMinLen){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 0,16);
			goto end_flag;
		}
		strMinBackLen = strValue;

		//"maxbacklen"
		strParamName = "maxbacklen";//最大的后续号码长度
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//是必选参数
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iMaxLen = stringToInt(strValue.c_str());
		//范围限制
		if (0 > iMaxLen || 16 < iMaxLen){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 0,16);
			goto end_flag;
		}
		strMaxBackLen = strValue;

		//最大值不能小于最小值
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
			//是必选参数
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		//网关是否存在
		if (!isExistedGateWay(INTERNAL_PROFILE_TYPE, strValue.c_str())) {
			strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
			goto end_flag;
		}
		strGateWay = strValue;

		//对号码"前缀"进行详细的解析处理
		strValue="";
		splitString(strPrefix,CARICCP_COMMA_FLAG,vecPrefixs);
		for (vector<string>::iterator it = vecPrefixs.begin(); vecPrefixs.end() != it; it++){
			strTmp = *it;
			vecTmp.clear();
			splitString(strTmp,CARICCP_LINK_MARK,vecTmp);

			//判断格式是否为如此:7-9
			for (vector<string>::iterator it2 = vecTmp.begin(); vecTmp.end() != it2; it2++){
				//是否为有效数字
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
		strValidPrefix = strValue;//封装完整的有效前缀值

		//封装格式:^前缀(号码段)$,如:^7(\d{7,16})$
		strExpression.append("^");      //^ 表达式开始的标志
		strExpression.append(strValidPrefix);

		strValidPostfix = "";
		strValidPostfix.append("(\\d{");
		strValidPostfix.append(strMinBackLen);
		strValidPostfix.append(CARICCP_COMMA_MARK);
		strValidPostfix.append(strMaxBackLen);
		strValidPostfix.append("})");
		
		strExpression.append(strValidPostfix);
		strExpression.append("$");      //$ 正则表达式的结束
	}
	else if (USERPRIORITY_LOCAL == iPriority){//本局
		//"localid"
		strParamName = "localid";//本局号码段
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//是必选参数
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
			//超长
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
			goto end_flag;
		}
		strLocalId = strValue;

		//封装格式:^([5-6][0-9][0-9][0-9])$
		strExpression.append("^");           //^ 表达式开始的标志
		strExpression.append("(");

		vector<string> vecLocalID;
		splitString(strLocalId,CARICCP_COMMA_FLAG,vecLocalID);

		//对号码段进行详细的解析处理
		for (vector<string>::iterator it = vecLocalID.begin(); vecLocalID.end() != it; it++){
			strValue = *it;
			//不再详细的分析格式是否合法,过于繁琐
			strExpression.append("[");
			strExpression.append(strValue);
			strExpression.append("]");
		}
		strExpression.append(")");
		strExpression.append("$");          //$ 正则表达式的结束
	}
	else{//拨入调度台或只能呼调度
		//"dispatchid"
		strParamName = "dispatchid";//本局号码段
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//是必选参数
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
			//超长
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
			goto end_flag;
		}
		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		strDispatchId = strValue;

		//封装格式:^号码$
		strExpression.append("^");      //^ 表达式开始的标志
		strExpression.append(strDispatchId);
		strExpression.append("$");      //$ 正则表达式的结束
	}
	//mod by xxl 2010-10-21 end :本局类型的结果进行修改

	//////////////////////////////////////////////////////////////////////////
	//"本局","只呼叫调度"以及"拨入调度台"处理同一个文件,并且它们具有惟一性
	if (USERPRIORITY_LOCAL == iPriority
		||USERPRIORITY_ONLY_DISPATCH == iPriority
		||ROUTER_TO_DISPATCH_DESK == iPriority){

		//将内容转换成xml的结构
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
		if (USERPRIORITY_LOCAL == iPriority){ //本局
			bRes = setDialPlanAttr(xcontext,"local","destination_number",strExpression.c_str());
		}
		if (ROUTER_TO_DISPATCH_DESK == iPriority){ //拨入调度台
			bRes = setDialPlanAttr(xcontext,"hover","destination_number",strExpression.c_str());
		}
		else if (USERPRIORITY_ONLY_DISPATCH == iPriority){//只呼调度
			bRes = setDialPlanAttr(xcontext,"dispatcher","destination_number",strExpression.c_str());
		}
		if (!bRes){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
			goto end_flag;
		}
		//将内存结构转换成字串
		diaplanContext = switch_xml_toxml(xdiaplan, SWITCH_FALSE);
		if (!diaplanContext) {
			strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
		//重新创建文件
		bRes = cari_common_creatXMLFile(diaplanFile,diaplanContext,err);
		if (!bRes){
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//"出局路由"不具有惟一性,如果存在,则修改,否则,新增
	else{
		//将内容转换成xml的结构
		xoutdial = cari_common_parseXmlFromFile(outDialFile);
		if (!xoutdial){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), outDialFile);
			goto end_flag;
		}

		//根据绑定的网关名来查找对应的out路由
		xoutGateway = switch_xml_find_child(xoutdial, "extension", "name", strGateWay.c_str());
		if (!xoutGateway) {//out路由不存在则创建
			//先构建xml的内存结构
			outGatewayContext = switch_mprintf(OUT_DIALPLAN_XML_CONTEXT, 
				strGateWay.c_str(),
				strExpression.c_str(),
				strGateWay.c_str(),
				strGateWay.c_str());

			//将内容转换成xml的结构
			xoutGateway = switch_xml_parse_str(outGatewayContext, strlen(outGatewayContext));
			//switch_safe_free(outGatewayContext);//此处释放会引起内存问题!!!
			                                      //在将内容写入到文件中以后再进行释放
			if (!xoutGateway) {
				strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
				goto end_flag;
			}
			if (!switch_xml_insert(xoutGateway, xoutdial, INSERT_INTOXML_POS)) {
				goto end_flag;
			}
		}
		else{//out路由已经存在,则修改,只需要修改condition的expression项
			 //如 :<condition field="destination_number" expression="^9(\d{4,5})$">
			bRes = setOutDialPlanAttr(xoutGateway,"destination_number",strExpression.c_str());
			if (!bRes){
				strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), diaplanFile);
				goto end_flag;
			}
		}
		//将内存结构转换成字串
		outdialContext = switch_xml_toxml(xoutdial, SWITCH_FALSE);
		if (!outdialContext) {
			strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
		//重新创建文件
		bRes = cari_common_creatXMLFile(outDialFile,outdialContext,err);
		if (!bRes){
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

    //////////////////////////////////////////////////////////////////////////
	//考虑重新加载root xml的所有文件.最原始的配置文件freeswitch.xml,先加载到bak(备份)文件中
	iFunRes = cari_common_reset_mainroot(err);
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//成功
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

	switch_safe_free(outGatewayContext);//最后释放
	switch_safe_free(diaplanFile);
	switch_safe_free(outDialFile);
	switch_safe_free(pDesc);
	return iFunRes;

}

/*删除路由,只能删除"出局"路由.
*目前"本局","拨入调度台"和"只能呼叫调度"是固定的,不允许删除
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

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strRouteType,strGateWay;
	string			strTmp,strBefore,strExpression,strValidPrefix,strValidPostfix;
	int             ipos=-1,iType = -1,iPriority=-1;
	vector<string>  vecPrefixs,vecTmp;//存放"前缀"的容器

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"type"
	strParamName = "type";//默认为"用户号码"路由配置类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	trim(strValue);
	if (0 == strValue.size()) {
		//是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("out",strValue)){       //出局
		iPriority = USERPRIORITY_OUT;
	}
	else{
		//不支持其他类型路由的删除
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
		//是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (!isExistedGateWay(INTERNAL_PROFILE_TYPE, strValue.c_str())) {
		//网关不存在
		strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strGateWay = strValue;

	//删除"出局"路由
	bRes = delOutRouter(false,strGateWay,pDesc);
	if (!bRes){
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//考虑重新加载root xml的所有文件.最原始的配置文件freeswitch.xml,先加载到bak(备份)文件中
	iFunRes = cari_common_reset_mainroot(err);
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//成功
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

/*查询路由
*/
int CModule_Route::lstRouter(inner_CmdReq_Frame*& reqFrame, 
							 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnResult = "";
	char *strTableName = "";//查询的表头
	char *pDesc                     = NULL;
	string tableHeader[5] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("TYPE"),
		getValueOfDefinedVar("ROUTER_PREFIX"),
		getValueOfDefinedVar("ROUTER_LEN"),
		getValueOfDefinedVar("ROUTER_GATEWAY"),
	};

	//default:叫路由对应的配置文件
	char *diaplanFile = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",//diaplan目录
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_DIAPLAN_DEF_XML_FILE
		);
	//00_out_dial.xml:出局路由的配置文件
	char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",           //diaplan目录
		SWITCH_PATH_SEPARATOR,
		"default",            //default目录
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_OUT_DIAPLAN_XML_FILE
		);

	switch_xml_t xdiaplan=NULL,xoutdial=NULL,xcontext=NULL;
	char *diaplanContext=NULL,*nesContext=NULL;
	int iRecordCout = 0,iLen=1,ipos=-1;//记录数量
	string strReturnDesc = "",strRecord = "",strType,strLen,strSeq,strRouterInfo,strPrefix;	
	

	//////////////////////////////////////////////////////////////////////////
	//将"出局路由"的文件内容转换成xml的结构
	xoutdial = cari_common_parseXmlFromFile(outDialFile);
	if (xoutdial){
		//1 获得"出局"的路由信息
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
				iRecordCout++;//序号递增
				strSeq = intToString(iRecordCout);
				strRecord ="";
				strRecord.append(strSeq);
				strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
				strRecord.append(strSingleInfo);

				//将记录存放到容器中
				inner_RespFrame->m_vectTableValue.push_back(strRecord);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//将"本局呼叫","拨入调度台"以及"只能呼叫调度"文件内容转换成xml的结构
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

	//2 获得"本局"号码路由信息
	getLocalRouterInfo(xcontext,strRouterInfo);
	if (0 != strRouterInfo.length()){
		iRecordCout++;//序号递增
		strSeq = intToString(iRecordCout);
		strRecord ="";
		strRecord.append(strSeq);
		strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRecord.append(strRouterInfo);

		//将记录存放到容器中
		inner_RespFrame->m_vectTableValue.push_back(strRecord);
	}

	//3 获得"拨入调度台"的路由信息
	strRouterInfo="";
	getDispatchRouterInfo(xcontext,ROUTER_TO_DISPATCH_DESK,strRouterInfo);
	if (0 != strRouterInfo.length()){
		iRecordCout++;//序号递增
		strSeq = intToString(iRecordCout);
		strRecord ="";
		strRecord.append(strSeq);
		strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRecord.append(strRouterInfo);

		//将记录存放到容器中
		inner_RespFrame->m_vectTableValue.push_back(strRecord);
	}

	//4 获得"只呼叫调度"的路由信息
	strRouterInfo="";
	getDispatchRouterInfo(xcontext,USERPRIORITY_ONLY_DISPATCH,strRouterInfo);
	if (0 != strRouterInfo.length()){
		iRecordCout++;//序号递增
		strSeq = intToString(iRecordCout);
		strRecord ="";
		strRecord.append(strSeq);
		strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
		strRecord.append(strRouterInfo);

		//将记录存放到容器中
		inner_RespFrame->m_vectTableValue.push_back(strRecord);
	}
	
	//返回信息给客户端,重新设置下返回信息
	if (0 == iRecordCout) {
		iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
		if (switch_strlen_zero(strReturnDesc.c_str())) {
			strReturnDesc = getValueOfDefinedVar("RECORD_NULL");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		}
		goto end_flag;
	} 

	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s", //表头
		tableHeader[0].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[1].c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		tableHeader[2].c_str()	    ,CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[3].c_str()	    ,CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[4].c_str());

	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("ROUTER_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

	//设置表头字段
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);


/*--goto跳转符--*/
end_flag:

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//释放xml结构
	switch_xml_free(xoutdial);
	switch_xml_free(xdiaplan);
	
	switch_safe_free(diaplanFile);
	switch_safe_free(outDialFile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*发送短信息功能
*/
int CModule_Route::sendMessage(inner_CmdReq_Frame*& reqFrame, 
							   inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";

	string          strLocalIP                              = "";


	//涉及到参数
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

	//封装发送sms的关键字段
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

	//调用发送短信的接口
	switch_core_chat_send(/*proto*/"sip", "sip", from_addr, to_addr, "", msg, NULL, full_from);

	switch_safe_free(full_from);
	switch_safe_free(from_addr);
	switch_safe_free(to_addr);
	return iFunRes;
}

//////////////////////////////////////////////////////////////////////////
/*
 *查看网关是否存在,从文件目录中查找或从内存中找,两者应该是一样的
*/
bool CModule_Route::isExistedGateWay(const char *profileType, //profile的目录
									 const char *gatewayname) //网关文件名
{
	bool	b1		= false,b2 = false;
	switch_xml_t	x_root, x_section, x_configuration, x_profiles, x_profile, x_gateways, x_gateway;
	const char		*gateway_name;
	char			*path	= getSipProfileDir();
    char *pFile = switch_mprintf("%s%s%s%s%s", path, profileType, SWITCH_PATH_SEPARATOR, gatewayname, CARICCP_XML_FILE_TYPE);
	b1 = cari_common_isExistedFile(pFile);
	switch_safe_free(path);
	switch_safe_free(pFile);

	//从内存中查找,保证可靠
	x_root = switch_xml_root();//注意:此函数会加锁,因此最后要释放
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

	//遍历每个gateway
	x_gateway = switch_xml_child(x_gateways, "gateway");
	for (; x_gateway; x_gateway = x_gateway->next) {
		//查找到对应的组,格式 :<group name="default">
		gateway_name = switch_xml_attr(x_gateway, "name");
		if (gateway_name) {
			if (!strcasecmp(gateway_name, gatewayname)) {
				b2 = true;
				break;
			}
		}
	}


end :
	//add by xxl 2010-10-18 begin :读取获得root内存结构,RWLOCK锁被锁住,因此需要释放
	if (x_root){
		switch_xml_free(x_root);//释放锁
	}
	//add by xxl 2010-10-18 end

	return (bool) (b1 || b2);
}

/*设置网关的属性或增加属性
 *注意释放内存问题
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

		//如果查找到相同的属性
		if (!strcasecmp(attName, var) && val) {
			//替换以前的属性值
			//设置名-值对,如: <param name="username" value="cluecon"/>
			switch_xml_set_attr(x_param, "value", attVal);

			isExistAttr = true;
			break;
		}
	}

	//如果属性子节点不存在,并运行新增加,则如下处理
	if (!isExistAttr && bAddattr) {
		
		//注意:此结构的释放问题
		newParamContext = switch_mprintf(GATEWAY_PARAM, attName, attVal);
		x_newChild = switch_xml_parse_str(newParamContext, strlen(newParamContext));

		//此处的内存是和xml结果绑定的,不能释放,否则会造成x_newChild结构异常
		//应该考虑释放xml的结构
		//switch_safe_free(newParamContext);
	
		if (!x_newChild) {
			return false;
		}
		if (!switch_xml_insert(x_newChild, gatewayinfo, INSERT_INTOXML_POS)) {
			
			//放置到最后
			return false;
		}
	}

	return true;
}

/*获得某项属性
*/
char * CModule_Route::getGatewayParamAttribute(switch_xml_t gatewayXMLinfo, 
											   const char *attName)
{
	switch_xml_t	x_param;
	char			*attVal	= NULL;
	for (x_param = switch_xml_child(gatewayXMLinfo, "param"); x_param; x_param = x_param->next) {
		const char	*var	= switch_xml_attr(x_param, "name");
		const char	*val	= switch_xml_attr(x_param, "value");

		//如果查找到相同的属性
		if (!strcasecmp(attName, var) && val) {
			attVal = (char*) val;
			return attVal;
		}
	}

	//return NULL;
	return CARICCP_NULL_MARK;
}

/*获得profileType目录下dateway的xml结构
*/
switch_xml_t CModule_Route::getGatewayXMLInfo(const char *profileType, //profile对应的目录
											  const char *gatewayname)//网关名字
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

/*设置extension -> condition属性
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

	//属性设置
	switch_xml_set_attr(xcondition, "expression", expressVal);

	return true;
}

/*设置out拨号路由
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

	//属性设置
	switch_xml_set_attr(xcondition, "expression", expressVal);

	return true;
}

/*从内存中获得网关的信息
*注意:一定要释放内存
*/
void CModule_Route::getAllGatewaysFromMem(char **allGateways)
{
	switch_xml_t	x_root, x_section, x_configuration, x_profiles, x_profile, x_gateways, x_gateway;
	const char		*profilename, *gateway_name;
	char			*username,*pwd,/**realm,*/*proxy,/**reg,*/*transport/*,*contact, *ping*/;


	//从内存中查找,保证可靠
	x_root = switch_xml_root();//注意:此函数会加锁,因此最后要释放
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

	//遍历每个profile
	x_profile = switch_xml_child(x_profiles, "profile");
	for (; x_profile; x_profile = x_profile->next) {
		//profile的名字
		profilename = switch_xml_attr(x_profile, "name");
		x_gateways = switch_xml_child(x_profile, "gateways");

		//遍历每个gateway
		x_gateway = switch_xml_child(x_gateways, "gateway");
		for (; x_gateway; x_gateway = x_gateway->next) {
			//网关名字
			gateway_name = switch_xml_attr(x_gateway, "name");

			//查找到对应的组的属性
			username = getGatewayParamAttribute(x_gateway, "username");
			//realm = getGatewayParamAttribute(x_gateway, "realm");
			pwd = getGatewayParamAttribute(x_gateway, "password");
			proxy = getGatewayParamAttribute(x_gateway, "proxy");
			//reg = getGatewayParamAttribute(x_gateway, "register");
			transport = getGatewayParamAttribute(x_gateway, "register-transport");
			//contact = getGatewayParamAttribute(x_gateway, "contact-params");
			//ping = getGatewayParamAttribute(x_gateway, "ping");

			/*********************************************/
			//组装数据,如何释放内存???
			*allGateways = switch_mprintf("%s%s%s%s%s%s%s%s%s", 
				gateway_name, CARICCP_SPECIAL_SPLIT_FLAG, 
				username    , CARICCP_SPECIAL_SPLIT_FLAG, 
				pwd         , 	CARICCP_SPECIAL_SPLIT_FLAG, 
				proxy       , CARICCP_SPECIAL_SPLIT_FLAG, 
				transport);

			//位置后移一位
			allGateways++;
		}
	}

end :
	//add by xxl 2010-10-18 begin :读取获得root内存结构,RWLOCK锁被锁住,因此需要释放
	if (x_root){
		switch_xml_free(x_root);//释放锁
	}
	//add by xxl 2010-10-18 end

	return;
}

/*创建网关的xml文件
*/
bool CModule_Route::creatGateWayXMLFile(const char *profileType, //profile类型,网关xml文件存放的目录
												 const char *gatewayname, //网关名
												 const char *filecontext, //文件内容
												 const char **err)  	 //删除参数,错误信息
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

/*删除网关的xml文件
*/
bool CModule_Route::delGateWayXMLFile(const char *profileType, //profile类型,网关xml文件存放的目录   
									  const char *gatewayname, //网关名
									  const char **err)	   //删除参数,错误信息
{
	char *gatewaypath	= NULL;
	char *pDir = getSipProfileDir();
	gatewaypath = switch_mprintf("%s%s%s%s%s", pDir, profileType, SWITCH_PATH_SEPARATOR, gatewayname, CARICCP_XML_FILE_TYPE);
	bool bRes =  cari_common_delFile(gatewaypath, err);

	switch_safe_free(pDir);
	switch_safe_free(gatewaypath);
	return bRes;
}

/*获得"本局"路由信息,目前具体惟一项
*/
void CModule_Route::getLocalRouterInfo(switch_xml_t xDialContext, 
									   string &strOut)
{
	switch_xml_t xexten,xcondition;
	const char *routerInfo = NULL;
	string strRouterInfo/*,strPrefix*/;
	int iLen=1,ipos=-1;//记录数量
	string strReturnDesc = "",strRecord = "",strType,strLen;	
	vector<string>  vecPrefixs;//存放"前缀"的容器
	vector<string>::iterator it;

	strOut = "";
	xexten = switch_xml_find_child(xDialContext, "extension", "name", "local");//查找此子项
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

	//根据此信息获得对应的"前缀"和长度,如 :^([0-9][0-9][0-9][0-9])$
	strRouterInfo = routerInfo;

	//mod by xxl 2010-10-21 begin :本局类型的结果进行修改
	//splitString(strRouterInfo,CARICCP_RIGHT_SQUARE_BRACKETS,vecPrefixs);
	//iLen = vecPrefixs.size()-2;//除去前缀和结束符2个部分
	//strLen = intToString(iLen);
	//it = vecPrefixs.begin();
	//strPrefix = *it;
	//ipos = strPrefix.find_first_of(CARICCP_LEFT_SQUARE_BRACKETS);
	//if (-1 != ipos){
	//	strPrefix = strPrefix.substr(ipos+1);
	//}
	////优化一下:为了方便操作用户查看,将"|"封装成","
	//replace_all(strPrefix,CARICCP_OR_MARK,CARICCP_COMMA_MARK);

	//strOut.append(strType);
	//strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	//strOut.append(strPrefix);
	//strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	//strOut.append(strLen);

	//处理一下按下面的格式输出
	//注:不要过多的特殊处理,一旦配置错误,也可以根据查询的结果进行查看修改.
	//否则可能造成问题较难定位.
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
	//mod by xxl 2010-10-21 end :本局类型的结果进行修改

	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append("");//网关为NULL,不涉及

}

/*获得与"调度"有关的路由信息:只呼叫调度和拨入调度台
 *目前具体惟一项
*/
void CModule_Route::getDispatchRouterInfo(switch_xml_t xDialContext,
										  int type,
									      string &strOut)
{
	switch_xml_t xexten,xcondition;
	const char *routerInfo = NULL;
	string strRouterInfo,strPrefix;
	int iLen=1,ipos=-1;//记录数量
	string strReturnDesc = "",strRecord = "",strType,strLen;	
	vector<string>  vecPrefixs;//存放"前缀"的容器
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

	//根据此信息获得对应的"前缀"和长度,如 :^0$ 或 ^9(\d{4})$
	strRouterInfo = routerInfo;

	//去除首位标识符
	iLen = strRouterInfo.length();
	strRouterInfo = strRouterInfo.substr(1,iLen-2);
	//是否含有"("
	ipos = strRouterInfo.find_first_of(CARICCP_LEFT_ROUND_BRACKETS);
	if (-1 != ipos){
		strPrefix = strRouterInfo.substr(0,ipos);
		int ipos1 = strRouterInfo.find_first_of(CARICCP_LEFT_BIG_BRACKETS);
		int ipos2 = strRouterInfo.find_first_of(CARICCP_RIGHT_BIG_BRACKETS);
		strLen = strRouterInfo.substr(ipos1+1,ipos2-ipos1-1);
	}
	else{
		strPrefix = strRouterInfo;
		//惟一值,后续号码长度为0
		iLen =0;
		strLen = intToString(iLen);
	}

	strOut.append(strType);
	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append(strPrefix);
	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append(strLen);

	strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
	strOut.append("");//网关为NULL,不涉及
}

/*获得"出局"的路由信息
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

	 //遍历每个"出局"路由设置
	 xextension = switch_xml_child(xOutDialContext, "extension");
	  for (; xextension; xextension = xextension->next) {
		  gatewayname = switch_xml_attr(xextension, "name");//网关名
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

		  //根据此信息获得对应的"前缀"和长度,如 :^0$ 或 ^9(\d{4})$
		  strRouterInfo = routerInfo;

		  //去除首位标识符
		  iLen = strRouterInfo.length();
		  strRouterInfo = strRouterInfo.substr(1,iLen-2);
		  //是否含有"("
		  ipos = strRouterInfo.find_first_of(CARICCP_LEFT_ROUND_BRACKETS);
		  if (-1 != ipos){
			  strPrefix = strRouterInfo.substr(0,ipos);
			  int ipos1 = strRouterInfo.find_first_of(CARICCP_LEFT_BIG_BRACKETS);
			  int ipos2 = strRouterInfo.find_first_of(CARICCP_RIGHT_BIG_BRACKETS);
			  strLen = strRouterInfo.substr(ipos1+1,ipos2-ipos1-1);
		  }
		  else{
			  strPrefix = strRouterInfo;
			  //惟一值,后续号码长度为0
			  iLen =0;
			  strLen = intToString(iLen);
		  }

		  //封装格式
		  strOut.append(strType);
		  strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
		  strOut.append(strPrefix);
		  strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
		  strOut.append(strLen);
		  strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
		  strOut.append(gatewayname);//网关

		  strOut.append(CARICCP_ENTER_KEY);
	  }
 }

/*删除出局路由
*/
bool CModule_Route::delOutRouter(bool bSpecial,
								 string gateway,
								 /*string &strErr*/char *&pDesc)//内存的释放由调用函数负责
 {
	 bool bRes = false;
	 string strReturnDesc;
	 switch_xml_t xoutdial=NULL,xoutGateway=NULL;
	 char *diaplanContext=NULL,*outdialContext=NULL;
	 const char *errArry[1] = {
		 0
	 };
	 const char **err = errArry;

	 //00_out_dial.xml:出局路由的配置文件
	 char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		 SWITCH_GLOBAL_dirs.conf_dir, 
		 SWITCH_PATH_SEPARATOR, 
		 "dialplan",           //diaplan目录
		 SWITCH_PATH_SEPARATOR,
		 "default",            //default目录
		 SWITCH_PATH_SEPARATOR,
		 CARI_CCP_OUT_DIAPLAN_XML_FILE
		 );

	 //将内容转换成xml的结构,如果文件不存在,则也视为成功!!!
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
	 if (!xoutGateway) {//此网关没有绑定路由,返回错误信息
		 if (bSpecial){
			 bRes = true;
		 }
		 else{
			 strReturnDesc = getValueOfDefinedVar("ROUTER_DEL_NOGATEWAY");
			 pDesc = switch_mprintf(strReturnDesc.c_str(),gateway.c_str());
		 }
		 goto end_flag;
	 }

	 //out路由存在,则删除
	 /************************************************************************/
	 switch_xml_remove(xoutGateway);
	 /************************************************************************/

	 //将内存结构转换成字串
	 outdialContext = switch_xml_toxml(xoutdial, SWITCH_FALSE);
	 if (!outdialContext) {
		 strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		 pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		 goto end_flag;
	 }
	 //重新创建文件
	 bRes = cari_common_creatXMLFile(outDialFile,outdialContext,err);
	 if (!bRes){
		 strReturnDesc = *err;
		 pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		 goto end_flag;
	 }

	 //处理结果成功
	 bRes = true;


end_flag:

     switch_xml_free(xoutdial);//释放

	 switch_safe_free(outDialFile);
	 return bRes;
 }

/*修改出局路由
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

	//00_out_dial.xml:出局路由的配置文件
	char *outDialFile = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		"dialplan",           //diaplan目录
		SWITCH_PATH_SEPARATOR,
		"default",            //default目录
		SWITCH_PATH_SEPARATOR,
		CARI_CCP_OUT_DIAPLAN_XML_FILE
		);

	char *chOldData = switch_mprintf("context_extension=%s", 
		oldGateway.c_str());

	//将内容转换成xml的结构,如果文件不存在,则也视为成功!!!
	xoutdial = cari_common_parseXmlFromFile(outDialFile);
	if (!xoutdial){
		bRes = true;
		goto end_flag;
	}
	xoutGateway = switch_xml_find_child(xoutdial, "extension", "name", oldGateway.c_str());
	if (!xoutGateway) {//不存在和出局网关绑定,则也视为成功!!!
		bRes = true;
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//继续查找对应的树的子项:extension->condition->action
	xcondition = switch_xml_find_child(xoutGateway, "condition", "field", "destination_number");
	if (!xcondition){
		bRes = true;
		goto end_flag;
	}

	//遍历查找符合条件的属性项
	xaction = switch_xml_child(xcondition, "action");
	for (; xaction; xaction = xaction->next) {
		// 查找对应的2个属性
		const char *appVal = switch_xml_attr(xaction, "application");
		const char *dataVal = switch_xml_attr(xaction, "data");

		//查找对应的关键的子项,一定不是空值
		if (switch_strlen_zero(appVal)
			||switch_strlen_zero(dataVal)) {
				
				continue;
		}
		//"export"和"context_extension=旧网关名",2个属性项才能确定对应的xml结构
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

	//子项中涉及的网关名也要修改,xaction_set涉及到2个属性项的查找,下面的方法不合适
	//xaction_set        = switch_xml_find_child_multi(xcondition, "action", "application", "set","data",chOldData,NULL);  //action子项的第一个"application="set""属性
	//xaction_export     = switch_xml_find_child(xcondition, "action", "application", "export");    //action子项的"application="export" "属性
	//xaction_conference = switch_xml_find_child(xcondition, "action", "application", "conference");//action子项的application="conference"属性
	if ((!xaction_export)
		||(!xaction_conference)){
			bRes = true;
			goto end_flag;
	}

	chDataExport = switch_mprintf("context_extension=%s",
		newGateway.c_str());
	chDataConference = switch_mprintf("bridge:${caller_id_number}-[${uuid}]-${destination_number}@default:sofia/gateway/%s/$1",
		newGateway.c_str());

	//属性设置:父节点extension对应的网关的名字更改
	switch_xml_set_attr(xoutGateway, "name", newGateway.c_str());

	//属性设置:子节点涉及到网关名字的地方都需要更改
	switch_xml_set_attr(xaction_export    , "data", chDataExport);
	switch_xml_set_attr(xaction_conference, "data", chDataConference);

	//将内存结构转换成字串
	outdialContext = switch_xml_toxml(xoutdial, SWITCH_FALSE);
	if (!outdialContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	//重新创建文件
	bRes = cari_common_creatXMLFile(outDialFile,outdialContext,err);
	if (!bRes){
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//处理结果成功
	bRes = true;


end_flag:

    switch_xml_free(xoutdial);//释放xml结构

	switch_safe_free(outDialFile);
	switch_safe_free(chOldData);
	switch_safe_free(chDataExport);
	switch_safe_free(chDataConference);

	return bRes;
	 
}
