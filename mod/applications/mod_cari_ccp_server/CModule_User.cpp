//#include "modules\CModule_User.h"
#include "CModule_User.h"

//#include "cari_public_interface.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"


/************************************************************************/
/* 运行在windows环境下,路径和文件含中文目前不能获得,请注意...           */
/************************************************************************/


//////////////////////////////////////////////////////////////////////////
//xml文档注意中文化的问题
//"<?xml   version=\"1.0\"   encoding=\"gb2312\"?>\n"  
//<?xml version="1.0" encoding="utf-8" ?>

//mob user的完整的xml文件结构,支持中文的显示
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

//mob user 的<params>属性
#define MOB_USER_XML_PARAMS		"<params>"		\
								"</params>"

//mob user 的<variables>属性
#define MOB_USER_XML_VARIABLES	"<variables>"	\
								"</variables>"

//mob user简单的xml文件,主要用来存放到"组"目录下使用
#define SIMPLE_MOB_USER_XML_CONTEXT "<include>"\
									"<user id=\"%s\" type=\"pointer\"/>"\
									"</include>"

//mob user param变量属性
#define MOB_USER_XML_PARAM			"<param name=\"%s\" value=\"%s\">"  	\
									"</param>"

//mob user variable变量属性
#define MOB_USER_XML_VARATTR		"<variable name=\"%s\" value=\"%s\">"   \
									"</variable>"

//增加或删除组的xml信息.注意中文化问题
//此内容添加到directory的default.xml文件中
#define MOB_GROUP_XML_DEFAULT_CONTEXT	"<group name=\"%s\" desc =\"%s\">"					\
										"<users>"														\
										"<X-PRE-PROCESS cmd=\"include\" data=\"groups/%s/*.xml\"/>"		\
										"</users>"														\
										"</group>"

//与路由配置有关的组呼diaplan call group的xml文件信息
#define MOB_GROUP_XML_DIAL_CONTEXT  "<extension name=\"group_dial_%s\">"	\
	                                "<condition field=\"destination_number\" expression=\"^%d$\">"		        \
                                    "<action application=\"bridge\" data=\"group/%s@${domain_name}\"/>"	\
                                    "</condition>"																\
                                    "</extension>"		

//调度员变量属性
#define DISPAT_USER_XML_VARATTR		"<dispatcher id=\"%s\">"   \
									"</dispatcher>"
																																																																																//使用的数据表,比较固定
#define AUTHUSERS					"authusers"

//////////////////////////////////////////////////////////////////////////
//私有的宏定义,只适用于本模块内部
#define MIN_GROUP_ID                          1
#define MAX_GROUP_ID						  10	

/*是否为有效的时间段,格式为:12:00-13:30
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


//如果此命令又涉及到其他子模块的处理,如何?????????????????????????????????????????????????????????????????
//当成内部命令---------------------------------

//////////////////////////////////////////////////////////////////////////
CModule_User::CModule_User():CBaseModule()
{

}


CModule_User::~CModule_User()
{

}

/*根据接收到的帧,处理具体的命令,分发到具体的处理函数
*/
int CModule_User::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, 
								inner_ResultResponse_Frame *&inner_RespFrame)
{
	if (NULL == inner_reqFrame) {
		//帧为空
		return CARICCP_ERROR_STATE_CODE;
	}

	//请求帧的"源模块"号则是响应帧的"目的模块"号
	//请求帧的"目的模块"号则是响应帧的"源模块"号,可逆的
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;


	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	bool bRes = true; 

	//判断命令码来判断命令(命令码是惟一的),也可以根据命令名进行设置
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
		//内部命令,与LST_MOBUSER不同,只需要返回所有的用户号
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

	//命令执行不成功
	if (CARICCP_SUCCESS_STATE_CODE != iFunRes) {
		inner_RespFrame->header.bNotify = false;
	} else {
		inner_RespFrame->header.bNotify = inner_reqFrame->body.bNotify;
	}

	/*--------------------------------------------------------------------------------------------*/
	//将处理的结果发送给对应的客户端
	//在命令的接收入口函数中


	//test 调用其他的库的函数
	//switch_core_thread_session_end(NULL);

	//打印日志
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "proc cmd %s end.\n", inner_reqFrame->body.strCmdName);
	return iFunRes;
}


/*发送信息给对应的客户端
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


/*-----------------------------具体的针对每条命令的处理函数-------------------------*/
//考虑到集成网管的功能,此处的具体逻辑应该由具体的SW来处理.当前属于SW的子模块,处理没问题
//命令帧的关键信息上包含了NE这个关键字段.
int CModule_User::addMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame, //输出参数,响应帧
							 bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";

	//用户组的有关信息
	char *validusergroups = "";
	char *tmpch = "";
	int groupNum = 0;
	char *groups[CARICCP_MAX_GROUPS_NUM] = {
		0
	}; //最多CARICCP_MAX_GROUPS_NUM个组

	char *new_default_xmlfile = NULL , *new_group_xmlfile = NULL;
	char *xmlfilecontext = NULL , *xmltemplate = NULL , *group_xmlfilecontext = NULL ;
	bool bRes = true;
	const char *errArry[1] = {
		0
	};
	const char **err = errArry;
	char *pDesc = NULL,*pFile = NULL;
	switch_xml_t x_context= NULL;

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	int iPriority = DEFAULT_USERPRIORITY;
	string strModel,strUserID, strPwd, strPriority, strDomain, strGroups, strAllows, strContext, strAccountcode, strEffectiveCallerIdName, 
		strEffectiveCallerIdNumber, strOutboundCallerIdName, strOutboundCallerIdNumber,strRecordAllow,strUserDesc;
	string strTmp,strHeader="";
	char *pUserID;
	int iUserCount = 1,iStartUserID = 0,iAddedStep = 1,iOriLen = 0;
	vector<string> vecSucUserID,vecFailUserID;
	vector<string>vecValidGroup;

	//通知码
	int notifyCode = CARICCP_NOTIFY_ADD_MOBUSER;
	string strNotifyCode = "";
	char *pNotifyCode                     = NULL;
	switch_status_t dbStatus = SWITCH_STATUS_SUCCESS;

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知其他客户端
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
	strParamName = "addmodel";//增加用户的模式:single或multi
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//默认为单步增加
		strValue = "single";
	}
	if ((!isEqualStr(strValue,"single"))
		&&(!isEqualStr(strValue,"multi"))){
		//参数值无效,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strModel = strValue;

	//开关参数的处理,涉及到"步长"和"数量"参数
	if (isEqualStr(strModel,"multi")){
		//"usercount "
		strParamName = "usercount";//用户数量
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//默认为增加一个用户
			strValue = "1";
		}

		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iUserCount = stringToInt(strValue.c_str());
		//范围限制
		if (0>=iUserCount || 100<iUserCount){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,100);
			goto end_flag;
		}

		//"userstep"
		strParamName = "userstep";//用户增加的步长
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//默认为1
			strValue = "1";
		}
		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iAddedStep = stringToInt(strValue.c_str());
		//范围限制
		if (0>=iAddedStep || 10<iAddedStep){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,10);
			goto end_flag;
		}

		//起始的用户号
		//"startuserid"
		strParamName = "startuserid";//起始的用户号
	}
	else{
		//"userid"
		strParamName = "userid";//用户号
		iUserCount = 1;
	}
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户号超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//用户号不是整型数字,返回错误信息
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strUserID = strValue;
	pUserID = (char*)strUserID.c_str();

	////add by xxl 2010-5-18: 超长字符的处理问题
	////                      unsigned long,  字长为4字节共32位二进制数, 数的范围是0~4294967295
	//if (MAX_INTLENGTH < strUserID.length()){
	//	strHeader = strUserID.substr(0,strUserID.length() - MAX_INTLENGTH);
	//	strUserID = strUserID.substr(strUserID.length() - MAX_INTLENGTH);//保留最后的字符
	//}
	//else{
	//	strHeader = "";
	//}
	////add by xxl 2010-5-18 end
	//iStartUserID = stringToInt(strUserID.c_str());//如果是以"0"开头的数字,如:007,会丢失!!!
	//iOriLen = strUserID.length();

	//password
	strParamName = "password";//用户密码
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.length()) {
		//用户密码,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());//switch_mprintf(str_NULL_Error.c_str(),strName)出错,为什么????????
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户密码超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//是否含有汉字问题
	bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	if (bRes) {
		strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strPwd = strValue;

	//priority
	iPriority = DEFAULT_USERPRIORITY;
	strParamName = "priority";//用户的权限,必选参数,如果用户没有填写,默认为3,普通级
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		iPriority = DEFAULT_USERPRIORITY;
	}
	iPriority = stringToInt(strValue.c_str());
	if (USERPRIORITY_INTERNATIONAL > iPriority || USERPRIORITY_ONLY_DISPATCH < iPriority) { //用户权限,数字越小,级别越高
		//用户优先级的取值超出范围
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), USERPRIORITY_ONLY_DISPATCH, USERPRIORITY_INTERNATIONAL);
		goto end_flag;
	}
	strPriority = strValue;

	//allows字段
	//strParamName = "allows";
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "";
	//}
	//strAllows = strValue;
	//strAllows = "international,domestic,local";

	//优化处理一下
	/*  
	分为5个级别: 1,2,3,4,5 ,数字越小,级别越高,目前只用3,4,5三种,其中

	1: 国际长途(international)
	2: 国内(domestic)
	3: 出局(out)
	4: 本地(local)
	5: 只能拨打调度(diapatch)

	添加用户的时候,默认级别为:4
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

	//用户组groups
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
	//是否含有汉字问题
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strGroups = strValue;

	//desc字段
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

	//context字段
	strParamName = "context";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "default";//默认为default路由方式
	}
	strContext = strValue;

	//accountcode字段
	strParamName = "accountcode";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strAccountcode = strValue;

	//effective_caller_id_name字段,cidname,来电显示号
	strParamName = "cidname";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//strValue.append("Extension ");
		strValue.append(strHeader);
		strValue.append(strUserID);//默认方式为用户号
	}
	strEffectiveCallerIdName = strValue;

	//recordallow字段
	strParamName = "recordallow";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (!isEqualStr(strValue,"yes")){
		strValue = "no";//默认值为no,不录音
	}
	strRecordAllow = strValue;

	strEffectiveCallerIdNumber.append(strHeader);
	strEffectiveCallerIdNumber.append(strUserID);

	strOutboundCallerIdName	  = "$${outbound_caller_name}";//暂时使用此方式进行处理!!!
	strOutboundCallerIdNumber = "$${outbound_caller_id}";

	//mod by xxl 2010-5-18: 组的有效判断问题,先统一进行判断
	//解析用户组,多个组之间使用','号进行连接
	if (!switch_strlen_zero(strGroups.c_str())) {
		int iInvalidGNum = 0;
		string strInvalidG = "";
		groupNum = switch_separate_string((char*) strGroups.c_str(), CARICCP_CHAR_COMMA_MARK, groups, (sizeof(groups) / sizeof(groups[0])));

		//依次处理组的信息
		for (int j = 0; j < groupNum; j++) {
			//保存有效的组
			if (isExistedGroup(groups[j])) {
				vecValidGroup.push_back(groups[j]);
			}
			else{
				iInvalidGNum ++;
				strInvalidG.append(groups[j]);
                strInvalidG.append(CARICCP_COMMA_MARK);
			}
		}
		//如果含有部分无效的组,则认为失败
		if (0 < strInvalidG.length()){
			strInvalidG = strInvalidG.substr(0,strInvalidG.length()-1);//去除最后的符号","
			strReturnDesc = getValueOfDefinedVar("MOB_GROUP_INVALID");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strInvalidG.c_str());
			goto end_flag;
		}
	}
	//mod by xxl 2010-5-18 end

	///***************************************************************/
	//收集所需要增加的用户,此处统一进行配置增加
	for (int i=0;i<iUserCount;i++){
		string strID = "";
		char strRes[MAX_INTLENGTH];
		string strStep = intToString(i * iAddedStep);

		//mod by xxl  2010-6-1 begin
		//如果号码以"0"开头或长整数,要计算准确.
		pUserID = (char*)strUserID.c_str();
		addLongInteger(pUserID,(char*)strStep.c_str(),strRes);
		strID = strRes;
		//mod by xxl  2010-6-1 end

		//其他与ID号相关的属性
		strEffectiveCallerIdName = strID;
		strEffectiveCallerIdNumber = strID;
	
		////////////////////////////////////////////////////////////////////////////
		//和其他模块的交互处理,调用系统其他模块的对外接口函数
		//switch_stream_handle_t stream = { 0 };
		//switch_event_t *resultEvent = NULL;
		////string strResult = "";
		//char *pResultcode;
		//*string strResultCode;
		//开始同步执行相关的api函数
		//SWITCH_STANDARD_STREAM(stream);
		//iCmdReturnCode = switch_api_execute(CARI_NET_SOFIA_CMD_HANDLER, strSendCmd.c_str(), NULL, &stream);
		//outMsg  = (const char **)stream.data;//输出的错误信息

		////命令执行不成功
		//if (CARICCP_SUCCESS_STATE_CODE != iCmdReturnCode)
		//{
		//	strReturnDesc = *outMsg;
		//	goto end_flag;
		//}
		////////////////////////////////////////////////////////////////////////////
		//必须对一些局部char *指针进行清除,否则出现问题
		//memset(tmpch,0,sizeof(char *));
		//memset(validusergroups,0,sizeof(char *));

		//判断用户是否已经存在
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
		//在default目录下建立完整的一份xml用户信息文件,然后查看组的信息,在各个组下面
		//建立一份simple的xml文件,考虑到组的信息的有效校验,顺序上换一下

		//解析用户组,多个组之间使用','号进行连接
		//if (!switch_strlen_zero(strGroups.c_str())) {
		//	groupNum = switch_separate_string((char*) strGroups.c_str(), CARICCP_CHAR_COMMA_MARK, groups, (sizeof(groups) / sizeof(groups[0])));

		//	//依次处理组的信息
		//	string strG = "";
		//	for (int j = 0; j < groupNum; j++) {
		//		//如果组存在,需要在此组对应的目录下增加mobuser的simple xml文件
		//		//如果组不存在,过滤掉,不做处理,同时进行了有效的校验
		//		if (isExistedGroup(groups[j])) {
		//			//重新设置用户组的信息,建立有效的用户组,此处进行统一的保存
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

		//	//去除最后一个多余的逗号
		//	tmpch = (char *)strG.c_str();
		//	if (!switch_strlen_zero(tmpch)) {
		//		//strncpy(validusergroups,tmpch,strlen(tmpch) -1); //减掉最后的CARICCP_COMMA_MARK的大小
		//		strValue = tmpch;
		//		strValue = strValue.substr(0, strlen(tmpch) - 1);
		//		validusergroups = (char*) strValue.c_str();
		//	}
		//}

		//mod by xxl 2010-5-18: 组的修改,把有效组直接进行处理
		//重设置用户组的信息
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

			//去除最后一个多余的逗号
			tmpch = (char *)strG.c_str();
			if (!switch_strlen_zero(tmpch)) {
				//strncpy(validusergroups,tmpch,strlen(tmpch) -1); //减掉最后的CARICCP_COMMA_MARK的大小
				strValue = tmpch;
				strValue = strValue.substr(0, strlen(tmpch) - 1);
				validusergroups = (char*) strValue.c_str();
			}
		}
		//mod by xxl 2010-5-18 end

		//////////////////////////////////////////////////////////////////////////
		//需要把用户的完整信息xml文件在default目录建立一份
		//封装用户文件的完整的目录结构
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

		//开始写入数据,每行记录的结束符不能使用\r\n,只能使用其中一个-------------
		xmltemplate = MOB_USER_XML_CONTEXT;
		xmlfilecontext = switch_mprintf(xmltemplate, strID.c_str(), strID.c_str());

		//将内容转换成xml的结构(注意:中文字符问题)
		x_context = switch_xml_parse_str(xmlfilecontext, strlen(xmlfilecontext));
		if (!x_context) {
			//strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);
			switch_safe_free(xmlfilecontext);//需要释放内存
			switch_safe_free(new_default_xmlfile);//释放内存
			
			continue;
		}

		//增加很多相关的属性
		//params字段属性
		setMobUserParamAttr(x_context, "password", strPwd.c_str(), true);
		setMobUserParamAttr(x_context, "vm-password", strPwd.c_str(), true);

		//variables字段属性设置
		setMobUserVarAttr(x_context, "priority"					  , strPriority.c_str(), true);
		//setMobUserVarAttr(x_context, "toll_allow"				  , strAllows.c_str(), true); //目前不使用此字段
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
			switch_safe_free(xmlfilecontext);     //需要释放内存
			switch_safe_free(new_default_xmlfile);//释放内存
			switch_xml_free(x_context);           //xml结构也要释放
			
			continue;
		}

		/************************************************************************/
		//创建(O_CREAT)新的MOB用户文件
		bRes = cari_common_creatXMLFile(new_default_xmlfile, xmltemplate, err);
		if (!bRes) {
			//strReturnDesc = *err;
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);
			switch_safe_free(xmlfilecontext);     //需要释放内存
			switch_safe_free(new_default_xmlfile);//释放
			switch_xml_free(x_context);           //xml结构也要释放
			
			continue;
		}
		/************************************************************************/
		
		vecSucUserID.push_back(strID);//保存增加成功的用户的号码

		switch_safe_free(xmlfilecontext);     //需要释放
		switch_safe_free(new_default_xmlfile);//释放
		switch_xml_free(x_context);           //xml结构也要释放
		
	}//end for (int i=0;i<iUserCount;i++)增加用户

	/*************************************************************/
	//如果没有一个用户增加成功,则认为失败,部分用户增加成功,视为成功
	if (iUserCount == (int)vecFailUserID.size()){
		strReturnDesc = getValueOfDefinedVar("ADD_MOBUSER_FAILE");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}
	//如果用户全部已经存在,则认为成功
	if (0 == vecSucUserID.size() 
		&& 0 == vecFailUserID.size()){
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("MO_USER_ALL_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//通知码
	notifyCode = CARICCP_NOTIFY_ADD_MOBUSER;
	//结果码和结果集
	pNotifyCode = switch_mprintf("%d", notifyCode);
	for (vector<string>::iterator it = vecSucUserID.begin(); vecSucUserID.end() != it; it++){
		string strID = *it;

		//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
		//if(SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM)) {
		if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
			//此时还是按照成功处理
			continue;
		}
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   			//通知标识
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode); //返回码=通知码
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_MOBUSERNAME, strID.c_str());	//通知的结果数据

		//发送通知----------------------
		cari_net_event_notify(notifyInfoEvent);

		//释放内存
		switch_event_destroy(&notifyInfoEvent);
    }
	switch_safe_free(pNotifyCode);

	//////////////////////////////////////////////////////////////////////////
	//考虑重新加载root xml的所有文件.最原始的配置文件freeswitch.xml,先加载到bak(备份)文件中
	//重新创建root xml文件的结构
	//另外,考虑写入内存的方式,而不是重新加载文件,效率问题.使用switch_xml_move()方法来测试一下!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (bReLoadRoot) {
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
	}

	/************************************************************************/
	////使用内存写入的方式进行,涉及到的内存结构数据都是临时变量????
	/************************************************************************/

	//增加用户的命令执行成功,需要通知所有注册的客户端
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("ADD_MOBUSER_ALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str());

/*--goto跳转符--*/
end_flag:

	//只需要释放一次即可
	//switch_safe_free(stream.data);
	//switch_event_destroy(&outMsg); //不要重复释放,出现问题

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	//switch_safe_free(new_default_xmlfile);
	return iFunRes;
}

/*删除用户
*/
int CModule_User::delMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame, //输出参数,响应帧
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

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	string strModel,strUserID;
	char*pUserID;
	int iUserCount=1,iStartUserID=0,iDeledStep=1,iOriLen = 0;
	vector<string> vecSucUserID,vecFailUserID;

	//通知码
	int notifyCode = CARICCP_NOTIFY_DEL_MOBUSER;
	string strNotifyCode = "",strHeader="";
	switch_status_t dbStatus = SWITCH_STATUS_SUCCESS;

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知其他客户端
	switch_event_t *notifyInfoEvent = NULL;
	switch_stream_handle_t stream = {
		0
	};
	switch_event_t *resultEvent = NULL;
	const char **outMsg = NULL;
	string strSendCmd;
	char *user_default_file = NULL , *chTmp,*pFile = NULL;

	//"delmodel"
	strParamName = "delmodel";//删除用户的模式
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//默认为单步删除
		strValue = "single";
	}
	if ((!isEqualStr(strValue,"single"))
		&&(!isEqualStr(strValue,"multi"))){
			//参数值无效,返回错误信息
			strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
	}
	strModel = strValue;

	//开关参数的处理,涉及到"步长"和"数量"参数
	if (isEqualStr(strModel,"multi")){
		//"usercount "
		strParamName = "usercount";//用户数量
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//默认为删除一个用户
			strValue = "1";
		}
		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iUserCount = stringToInt(strValue.c_str());
		//范围限制
		if (0>=iUserCount || 100<iUserCount){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,100);
			goto end_flag;
		}

		//"userstep"
		strParamName = "userstep";//删除用户的步长
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//默认为1
			strValue = "1";
		}
		//是否为有效数字
		if (!isNumber(strValue)){
			strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		iDeledStep = stringToInt(strValue.c_str());
		//范围限制
		if (0>=iDeledStep || 10<iDeledStep){
			strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), 1,10);
			goto end_flag;
		}

		//起始的用户号
		//"startuserid"
		strParamName = "startuserid";//起始的用户号
	}
	else{

		//"userid"
		strParamName = "userid";//用户号
		iUserCount = 1;
	}
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户号超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//用户号不是整型数字,返回错误信息
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strUserID = strValue;
	pUserID = (char*)strUserID.c_str();


	///***************************************************************/
	//收集所需要删除的用户,此处统一进行处理
	for (int i=0;i<iUserCount;i++){
		char *groups[CARICCP_MAX_GROUPS_NUM] = {
			0
		}; //最多CARICCP_MAX_GROUPS_NUM个组
		char **mobusergoup = groups;

		string strID = "";
		char strRes[MAX_INTLENGTH];
		string strStep = intToString(i * iDeledStep);

		//mod by xxl  2010-6-1 begin
		//如果号码以"0"开头或长整数,要计算准确.
		pUserID = (char*)strUserID.c_str();
		addLongInteger(pUserID,(char*)strStep.c_str(),strRes);
		strID = strRes;
		//mod by xxl  2010-6-1 end

		//判断用户是否已经存在
		bRes = isExistedMobUser(strID.c_str());
		if (!bRes){//用户不存在
			/*iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
			//返回错误的信息提示操作用户:终端用户不存在
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
			goto end_flag;*/

			continue;
		}

		//先删除default目录下对应的用户文件,可以先根据此xml文件查找对应的组的信息,
		//或者遍历root xml结构查找对应的组的信息(采用此方法)
		pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_file = switch_mprintf("%s%s.xml", pFile, strID.c_str());
			switch_safe_free(pFile);
		}
		if (!user_default_file) {
			//pDesc = switch_mprintf("Cannot get user %s xml path ", strID.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);//存放到失败的容器中
			continue;
		}

		//删除defult目录下的mob用户的xml文件
		bRes = cari_common_delFile(user_default_file, err);
		if (!bRes) {
			//strReturnDesc = *err;
			//pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			//goto end_flag;

			vecFailUserID.push_back(strID);   //存放到失败的容器中
			switch_safe_free(user_default_file);//释放
			continue;
		}

		//然后再删除组目录下对应的文件
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

		vecSucUserID.push_back(strID);//保存删除成功的用户的号码
		switch_safe_free(user_default_file);

	}//end for删除所有的用户

	/*************************************************************/
	//如果没有一个用户删除成功,则认为失败,部分用户删除成功,视为成功
	if (iUserCount == (int)vecFailUserID.size()){
		strReturnDesc = getValueOfDefinedVar("DEL_MOBUSER_FAILE");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//如果用户全部不存在,则认为失败
	if (0 == vecSucUserID.size() 
		&& 0 == vecFailUserID.size()){
			iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
			strReturnDesc = getValueOfDefinedVar("MO_USER_ALL_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
	}

	/*---涉及到dispat.conf.xml文件的设置---*/
	//还要查看此用户是否为调度员
	{
		switch_xml_t x_dispatchers = NULL,x_dispatcher = NULL;
		int userNum = 0,validUserNum = 0,existedDiapatNum = 0;
		char *xmlfilecontext = NULL,*dispatFile = NULL;
		//switch_event_t *event;
        //此结构需要释放
		x_dispatchers = getAllDispatchersXMLInfo();
		if (!x_dispatchers){
			strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), CARI_CCP_DISPAT_XML_FILE);
			goto reboot_flag;
		}

		//将删除成功的用户,如果是调度员,也需要删除
		for (vector<string>::iterator it = vecSucUserID.begin(); vecSucUserID.end() != it; it++){
			string strID = *it;

			//查看此用户是否已经是调度员
			x_dispatcher = getDispatcherXMLInfo(strID.c_str(),x_dispatchers);
			if (x_dispatcher){
				existedDiapatNum++;	//已经存在的调度用户数量
				
				//删除此调度员
				switch_xml_remove(x_dispatcher);
			}
		}

		if (0 != existedDiapatNum){
			//重新将结构写入到文件
			dispatFile = getDispatFile();
			if (dispatFile) {
				xmlfilecontext = switch_xml_toxml(x_dispatchers, SWITCH_FALSE);
				bRes = cari_common_creatXMLFile(dispatFile, xmlfilecontext, err);
				switch_safe_free(dispatFile);//立即释放
				if (!bRes) {
					strReturnDesc = *err;
					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto end_flag;
				}
			}
			////由于和调度模块相互关联,此处产生事件
			//if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
			//	if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
			//		switch_event_destroy(&event);
			//	}
			//}
		}

		//释放xml结构
		switch_xml_free(x_dispatchers);
	}

/*--------*/
reboot_flag:
	//////////////////////////////////////////////////////////////////////////
	//重新创建root xml文件的结构
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//删除终端用户成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("DEL_MOBUSER_ALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str());

	//////////////////////////////////////////////////////////////////////////
	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
	//结果码和结果集
	notifyCode = CARICCP_NOTIFY_DEL_MOBUSER;
	pNotifyCode = switch_mprintf("%d", notifyCode);
	for (vector<string>::iterator it = vecSucUserID.begin(); vecSucUserID.end() != it; it++){

		//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
		//if(SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM)) {
		if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
			//此时还是按照成功处理
			continue;
		}
		string strID = *it;
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   			//通知标识
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode); //返回码=通知码
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_MOBUSERNAME, strID.c_str());	//通知的结果数据

		//发送通知----------------------
		cari_net_event_notify(notifyInfoEvent);

		//释放内存
		switch_event_destroy(&notifyInfoEvent);
	}
	switch_safe_free(pNotifyCode);


/*--goto跳转符--*/
end_flag:

	//只需要释放一次即可
	//switch_safe_free(stream.data);
	//switch_event_destroy(&outMsg); //不要重复释放,出现问题

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*修改终端用户
*/
int CModule_User::mobMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame, //输出参数,响应帧
							 bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";

	bool bModGroupFlag = false;//默认为修改组的标志
	bool bModGroup = true;
	char *groups[CARICCP_MAX_GROUPS_NUM] = {
		0
	}; //最多CARICCP_MAX_GROUPS_NUM个组
	char **mobusergoup = groups;//赋值;

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

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	int iPriority = DEFAULT_USERPRIORITY;
	string strUserID, strPwd, strPriority, strDomain, strGroups, strAllows, strContext, strAccountcode, strGroupModFlag,
		strEffectiveCallerIdName,strRecordAllow,strUserDesc;
	string strTmp;

	//通知码
	int notifyCode = CARICCP_NOTIFY_MOD_MOBUSER;
	string strNotifyCode = "";
	switch_status_t dbStatus = SWITCH_STATUS_SUCCESS;

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知其他客户端
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
	strParamName = "userid";//用户号
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户号超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//用户号不是整型数字,返回错误信息
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strUserID = strValue;
	iOriLen = strUserID.length();

	//password
	strParamName = "password";//用户密码
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.length()) {
		//用户密码,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户密码超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	//是否含有汉字问题
	bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	if (bRes) {
		strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strPwd = strValue;

	//priority
	iPriority = DEFAULT_USERPRIORITY;
	strParamName = "priority";//用户的权限,非必选参数,如果用户没有填写,默认为0,最低级
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		iPriority = DEFAULT_USERPRIORITY;
	}
	iPriority = stringToInt(strValue.c_str());
	if (USERPRIORITY_INTERNATIONAL > iPriority || USERPRIORITY_ONLY_DISPATCH < iPriority) {
		//用户优先级的取值超出范围
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), USERPRIORITY_INTERNATIONAL, USERPRIORITY_ONLY_DISPATCH);
		goto end_flag;
	}
	strPriority = strValue;

	////allows字段
	//strParamName = "allows";
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	strValue = "";
	//}
	//strAllows = strValue;

	//优化处理一下
	/*  
	分为5个级别: 1,2,3,4,5 ,数字越小,级别越高,目前只用3,4,5三种,其中

	1: 国际长途(international)
	2: 国内(domestic)
	3: 出局(out)
	4: 本地(local)
	5: 只能拨打调度(diapatch)

	添加用户的时候,默认级别为:4
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

	//用户组groups
	strParamName = "groups";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strGroups = strValue;

	//context字段
	strParamName = "context";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "default";//默认为default方式
	}
	strContext = strValue;

	//accountcode字段
	strParamName = "accountcode";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		strValue = "";
	}
	strAccountcode = strValue;

	//effective_caller_id_name字段,cidname,来电显示号
	strParamName = "cidname";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {//使用默认的方式
		//strValue.append("Extension ");
		strValue.append(strUserID);
	}
	//else{
	//	//校验长度
	//	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
	//		//用户名超长,返回错误信息
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

	//recordallow字段
	strParamName = "recordallow";
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (!isEqualStr(strValue,"yes")){
		strValue = "no";//默认值为no,不录音
	}
	strRecordAllow = strValue;

	//desc字段
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

	//判断用户是否已经存在
	bRes = isExistedMobUser(strUserID.c_str());
	if (!bRes) {
		iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
		//返回错误的信息提示操作用户:终端用户不存在
		strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());
		goto end_flag;
	}

	//如果不修改用户组,则需要把以前的用户组信息提取出来保持一下后继使用,因为下面的删除操作会造成丢失
	//只要取值是yes,则修改
	if (isEqualStr("yes",strGroupModFlag)) {
		bModGroupFlag = true;
	}
	if (bModGroupFlag) {
		//mod by xxl 2010-5-18: 组的有效判断问题,先统一进行判断
		//解析用户组,多个组之间使用','号进行连接
		int groupNum=0;
		if (!switch_strlen_zero(strGroups.c_str())) {
			int iInvalidGNum = 0;
			string strInvalidG = "";
			groupNum = switch_separate_string((char*) strGroups.c_str(), CARICCP_CHAR_COMMA_MARK, groups, (sizeof(groups) / sizeof(groups[0])));

			//依次处理组的信息
			for (int j = 0; j < groupNum; j++) {
				if (isExistedGroup(groups[j])) {
					addGroupVec.push_back(groups[j]);
				}
				else{//无效的组暂保存
					iInvalidGNum ++;
					strInvalidG.append(groups[j]);
					strInvalidG.append(CARICCP_COMMA_MARK);
				}
			}

			//如果含有部分无效的组,则认为失败
			if (0 < strInvalidG.length()){
				strInvalidG = strInvalidG.substr(0,strInvalidG.length()-1);//去除最后的符号","
				strReturnDesc = getValueOfDefinedVar("MOB_GROUP_INVALID");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strInvalidG.c_str());
				goto end_flag;
			}
		}
		//mod by xxl 2010-5-18 end

		//需要查找以前的用户组,否则删除的时候会造成信息丢失
		getMobGroupOfUserFromMem(strUserID.c_str(), mobusergoup);
		//mobusergoup = groups;//直接赋值有问题???
		string strG="";
		while (*mobusergoup) {
			chTmp = *mobusergoup;
			char *pch = switch_mprintf("%s%s", chTmp, CARICCP_COMMA_MARK);
			strG.append(pch);
			switch_safe_free(pch);//及时释放内存

			mobusergoup++;
			//保存起来
			delGroupVec.push_back(chTmp);
		}
		bModGroup = false;
		//去除最后一个多余的逗号
		tmpG = (char*)strG.c_str();
		if (!switch_strlen_zero(tmpG)) {
			//C++编译执行出现问题
			//strncpy(beforeusergroups,tmpG,strlen(tmpG) -1); //减掉最后的CARICCP_COMMA_MARK的大小
			strValue = tmpG;
			strValue = strValue.substr(0, strlen(tmpG) - 1);
			beforeusergroups = (char*) strValue.c_str();
		}
	}
	//直接考虑属性的修改,主要涉及密码/组信息的修改
	x_user = getMobDefaultXMLInfo(strUserID.c_str());
	if (!x_user) {
		//返回错误的信息提示操作用户:终端用户不存在
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());
		goto end_flag;
	}

	//涉及到属性的修改,涉及的字段不同,使用的方法不同
	//params字段属性
	setMobUserParamAttr(x_user, "password", strPwd.c_str());
	setMobUserParamAttr(x_user, "vm-password", strPwd.c_str());
	
	//variables字段属性
	setMobUserVarAttr(x_user, "priority", strPriority.c_str());
	//setMobUserVarAttr(x_user, "toll_allow", strAllows.c_str()); //目前不使用此字段
	setMobUserVarAttr(x_user, "effective_caller_id_name", strEffectiveCallerIdName.c_str());
	setMobUserVarAttr(x_user, "record_allow", strRecordAllow.c_str());
	if (0 != strUserDesc.length()){
		setMobUserVarAttr(x_user, "desc", strUserDesc.c_str());
	}

	if (bModGroupFlag) {
		//需要查看增加了哪些组,又删除掉了哪些组,为了操作方便,先对beforeusergroups的统一清除,然后再对strGroups统一增加
		vector<string>::iterator iter = delGroupVec.begin();
		for (; iter != delGroupVec.end(); ++iter) {
			strTmp = *iter;
			char *group_file = getMobUserGroupXMLFile(strTmp.c_str(), strUserID.c_str());
			if (group_file) {
				cari_common_delFile(group_file, err);
				switch_safe_free(group_file);
			}
		}

		//再进行增加
		string strValidGroup = "";
		int index = -1;
		xmltemplate = SIMPLE_MOB_USER_XML_CONTEXT;
		group_xmlfilecontext = switch_mprintf(xmltemplate, strUserID.c_str());
		//splitString(strGroups, CARICCP_COMMA_MARK, addGroupVec);//前面已经保存
		iter = addGroupVec.begin();
		for (; iter != addGroupVec.end(); ++iter) {
			strTmp = *iter;

			////判断是否有效的组,前面已经进行了判断处理
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
		//最后进行属性设置
		setMobUserVarAttr(x_user, "callgroup", strGroups.c_str());
	}

	//根据动态更改的内存的内容改写到xml配置文件中
	userDefaultXmlContext = switch_xml_toxml(x_user, SWITCH_FALSE);
	if (!userDefaultXmlContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//直接覆盖以前的文件
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
	//最后再统一重新加载一下root xml文件
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), user_default_xmlfile);
		goto end_flag;
	}

	//增加用户的命令执行成功,需要通知所有注册的客户端
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("MOD_MOBUSER_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//此时还是按照成功处理
		goto end_flag;
	}

	//通知码
	notifyCode = CARICCP_NOTIFY_MOD_MOBUSER;

	//结果码和结果集
	pNotifyCode = switch_mprintf("%d", notifyCode);
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   					//通知标识
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode);          //返回码=通知码
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_MOBUSERNAME, strUserID.c_str());	//通知的结果数据

	//发送通知----------------------
	cari_net_event_notify(notifyInfoEvent);

	//释放内存
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pNotifyCode);


/*--goto跳转符--*/
end_flag:

	//只需要释放一次即可
	//switch_safe_free(stream.data);
	//switch_event_destroy(&outMsg); //不要重复释放,出现问题

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//需要释放xml内存
	switch_xml_free(x_user);

	switch_safe_free(pDesc);
	switch_safe_free(user_default_xmlfile);
	switch_safe_free(group_xmlfilecontext);
	return iFunRes;
}

/*属于配置命令
 *查询当前的用户:返回详细的用户信息,
*/
int CModule_User::lstMobUser(inner_CmdReq_Frame *&reqFrame, 
							 inner_ResultResponse_Frame *&inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnResult = "";
	char *strTableName = "";//查询的表头
	bool bRes;
	switch_xml_t userXMLinfo = NULL,groupXMLinfo = NULL,x_users = NULL, x_user = NULL;
	//char *strResult = "";	//如果如此定义会出现问题,内存没有释放掉.此文件C++使用memset()容易出问题
	//						//也不建议赋值为NULL,下面要直接操作,不合适

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
	int iRecordCout = 0;//记录数量

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	string strUserID;

	string strReturnDesc = "",strResult,strRecord = "",strSeq;	//如果如此定义会出现问题,内存没有释放掉.使用memset()
	//也不建议赋值为NULL,下面要直接操作,不合适

	//userid
	strParamName = "userid";//用户号
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);

	//用户号超长,返回错误信息
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	strUserID = strValue;

	//必须对某些char *局部变量进行清除,否则内存出现问题
	//memset(strResult, 0, sizeof(char));

	//名字为空查询所有的用户,名字不为空,查询此单个用户
	if (!switch_strlen_zero(strUserID.c_str())) {
		//判断用户是否已经存在
		bRes = isExistedMobUser(strUserID.c_str());
		if (!bRes) {
			//用户不存在
			iCmdReturnCode = CARICCP_MOBUSER_NOEXIST_CODE;
			//返回错误的信息提示操作用户:终端用户不存在
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strUserID.c_str());
			goto end_flag;
		}

		//查询单个用户的信息userXMLinfo
		userXMLinfo = getMobUserXMLInfoFromMem(strUserID.c_str(), userXMLinfo);
		if (userXMLinfo) {
			//封装用户的详细信息
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

			//strResult =  *userMsg;//每行记录之间使用CARICCP_ENTER_KEY区分
			//strcat(strResult,*userMsg);
			//strcat(strResult,CARICCP_ENTER_KEY);

			//mod by xxl 2010-5-27 :增加序号的显示
			if (0 != strResult.length()){
				iRecordCout++;//序号递增
				strRecord = "";
				strSeq = intToString(iRecordCout);
				strRecord.append(strSeq);
				strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
				strRecord.append(strResult);

				//存放记录
				inner_RespFrame->m_vectTableValue.push_back(/*strResult*/strRecord);
			}
			//mod by xxl 2010-5-27 end

			iFunRes = CARICCP_SUCCESS_STATE_CODE;
			iCmdReturnCode = SWITCH_STATUS_SUCCESS;
		}
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//查询default组对应的所有用户的信息
	strReturnDesc = "";
	groupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, groupXMLinfo, strReturnDesc);
	if (groupXMLinfo) {
		//遍历所有的用户
		//查找users节点
		if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {
			
			//遍历每个用户
			x_user = switch_xml_child(x_users, "user");
			for (; x_user; x_user = x_user->next) {
				userid = switch_xml_attr(x_user, "id");
				if (switch_strlen_zero(userid)) {
					continue;
				}

				//是否为有效数字
				if (switch_is_number(userid)) {
					/*-----------------------*/
					strResult ="";
					getSingleMobInfo(x_user, strResult);
					if (0 != strResult.length()){
						iRecordCout++;//序号递增
						strRecord = "";
						strSeq = intToString(iRecordCout);
						strRecord.append(strSeq);
						strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
						strRecord.append(strResult);

						//存放记录
						inner_RespFrame->m_vectTableValue.push_back(/*strResult*/strRecord);
					}
				}
			}//遍历users
		}
	}
	else {
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	}
	

/*--goto跳转符--*/
end_flag:

	//返回信息给客户端,重新设置下返回信息
	if (0 == inner_RespFrame->m_vectTableValue.size()) {
		iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
		if (switch_strlen_zero(strReturnDesc.c_str())) {
			strReturnDesc = getValueOfDefinedVar("RECORD_NULL");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		}
	} 
	else {
	  	//结果表头,需要和xml的配置文件一致
	  	//用户xml文件的详细信息
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


	  	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s", //表头
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

		//设置表头字段
		myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
		switch_safe_free(strTableName);
	}

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*查询当前的用户:只返回所有的用户号以及状态(必须的),属于内部命令
*监控模块初始登录的时候使用
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
	//char *strResult = "";	//如果如此定义会出现问题,内存没有释放掉.使用memset()
	string strResult;
	//也不建议赋值为NULL,下面要直接操作,不合适
	const char **userMsg = userMsgArry;
	//userMsg = malloc(sizeof(char *)); //分配了内存空间
	//switch_assert(userMsg);
	//memset(userMsg, 0, sizeof(char *));

	//必须对某些char *局部变量进行清除,否则内存出现问题
	//memset(strResult, 0, sizeof(char *));

	//////////////////////////////////////////////////////////////////////////
	//查询default组对应的所有用户的信息,只需要查询mob 号码即可
	groupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, groupXMLinfo, strReturnResult);
	if (groupXMLinfo) {
		//遍历所有的用户查找users节点
		if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {

			//需要从数据库中查看此用户是否已经注册???
			switch_core_db_t *db = NULL;
			char *dbFileName = "sofia_reg_internal";//目前采用数据库文件查看

#ifdef SWITCH_HAVE_ODBC
#else
#endif

			if (!(db = switch_core_db_open_file(dbFileName))) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", dbFileName);
				db = NULL;
			}

			//遍历每个用户
			x_user = switch_xml_child(x_users, "user");
			for (; x_user; x_user = x_user->next) {
				userid = switch_xml_attr(x_user, "id");
				if (switch_strlen_zero(userid)) {
					continue;
				}

				//是否为有效数字
				if (switch_is_number(userid)) {
					strResult = (char*)userid;			 //每行记录之间使用CARICCP_ENTER_KEY区分
					strResult.append(CARICCP_COLON_MARK);//属性之间使用":"隔开
					char  *mobUserState	 = NULL;
					
					//是否已经注册了
					bool bReg = isRegisteredMobUser(userid,db);
					if (bReg){
						mobUserState = switch_mprintf("%d", /*CARICCP_MOB_USER_INIT*/1);
					}
					else{
						mobUserState = switch_mprintf("%d", /*CARICCP_MOB_USER_NEW*/0);
					}

					strResult.append(mobUserState);

					//存放记录
					inner_RespFrame->m_vectTableValue.push_back(strResult);
					switch_safe_free(mobUserState);
				}
			}//遍历users

			//释放库的连接
			if (db) {
				switch_core_db_close(db);
			}
		}//end x_users

		//返回信息给客户端,重新设置下返回信息
		iFunRes = CARICCP_SUCCESS_STATE_CODE;
		iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
		strReturnResult = getValueOfDefinedVar("PRIVATE_MOBUSER_INFO_RESULT");
	}
	else{
		iFunRes = SWITCH_STATUS_FALSE;
		if (0 == strReturnResult.length()) {
			//当前的记录为null
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
/*增加用户组
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

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//组名和组的描述信息
	string strGroupName, strGroupDesc, strGroupID;
	int iGroupId = -1;

	//"groupName"
	strParamName = "groupName";//组名
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (0 == strValue.size()) {
		//组名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//组名超长
		//组名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//是否含有汉字问题
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strGroupName = strValue;

	////"groupID"
	//strParamName = "groupID";//组号
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//if (0 == strValue.size()) {
	//	//组名字为空,返回错误信息
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());

	//	goto end_flag;
	//}

	////组号的范围[1-10]
	//iGroupId = stringToInt(strValue.c_str());
	//if (MIN_GROUP_ID > iGroupId || MAX_GROUP_ID < iGroupId) {
	//	//组号的范围超出
	//	strReturnDesc = getValueOfDefinedVar("GROUPID_RANGE_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), MIN_GROUP_ID, MAX_GROUP_ID);

	//	goto end_flag;
	//}
	//strGroupID = strValue;

	//"desc"
	strParamName = "desc";//组描述信息
	strValue = getValue(strParamName, reqFrame);
	strChinaName = getValueOfDefinedVar(strParamName);
	trim(strValue);
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//组描述信息超长
		//组描述信息超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	////是否含有汉字问题
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes){
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(),
	//		strChinaName.c_str());
	//	goto end_flag;
	//}
	strGroupDesc = strValue;

	//判断此组是否存在,根据内存中结构进行判断
	if (isExistedGroup(strGroupName.c_str(), iGroupId)) {
		strReturnDesc = getValueOfDefinedVar("GROUP_NAMEID_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		goto end_flag;
	}
	strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
	strReturnDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

	//增加组的信息.如下:
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

	//将内容转换成xml的结构(注意:中文字符问题)
	x_newgroup = switch_xml_parse_str(newGroupsContext, strlen(newGroupsContext));
	
	//conf\directory\default.xml
	default_xmlfile = getDirectoryDefaultXMLFile();//需要释放

	//从文件中读取groups的内存结构
	x_allgroups = cari_common_parseXmlFromFile(default_xmlfile);
	if (!x_allgroups || !x_newgroup) {
		strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());
		goto end_flag;
	}

	//装换成字符串,回退使用,保存目前xml的内容
	oldGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);

	//查找对应的groups节点
	x_domain = switch_xml_child(x_allgroups, "domain");
	if (!x_domain) {
		goto end_flag;
	}
	x_groups = switch_xml_child(x_domain, "groups");
	if (!x_groups) {
		goto end_flag;
	}

	//动态更改内存的结构,将新增的group节点合并到以往的结构中
	//组的位置放置到最后
	iGroupPos = getGroupNumFromMem();
	if (!switch_xml_insert(x_newgroup, x_groups, /*iGroupPos*/INSERT_INTOXML_POS)){//组的位置,放置到最后
		goto end_flag;
	}
	//合并成完成的default.xml结构,重新写文件
	newGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);
	if (!newGroupsContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	//直接覆盖以前的conf\directory\default.xml文件
	bRes = cari_common_creatXMLFile(default_xmlfile, newGroupsContext, err);
	if (!bRes) {
		//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
		//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//在conf\directory\groups目录下创建对应的组的子目录
	groupPath = getMobGroupDir(strGroupName.c_str());
	if (!cari_common_creatDir(groupPath, err)) {
		//如果目录已经创建了,再cari_common_creatDir()中已经判断了,此处代表了真实的失败情况
		strReturnDesc = getValueOfDefinedVar("CREAT_DIR_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), groupPath);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());

		//直接返回,还要考虑直接重新加载一下???
		goto end_flag;
	}

	////////////////////////////////////////////////////////////////////////////
	////涉及修改的文件:conf\directory\default.xml,conf\dialplan\default\02_group_dial.xml
	////创建对应的diaplan group的xml信息
	//diagroupcontext = switch_mprintf(MOB_GROUP_XML_DIAL_CONTEXT, 
	//	(char*) strGroupName.c_str(), 
	//	iGroupId,
	//	(char*) strGroupName.c_str());

	////将内容转换成xml的结构
	//x_diagroup = switch_xml_parse_str(diagroupcontext, strlen(diagroupcontext));
	//diagroup_xmlfile = getDirectoryDialGroupXMLFile();
	//x_diaallgroups = cari_common_parseXmlFromFile(diagroup_xmlfile);
	//if (!x_diagroup || !x_diaallgroups) {
	//	strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, strReturnDesc.c_str());
	//	goto end_flag;
	//}
	//if (!switch_xml_insert(x_diagroup, x_diaallgroups, /*iGroupPos*/INSERT_INTOXML_POS)){//组的位置
	//	goto end_flag;
	//}
	////合并成完成的default xml结构,重新写文件
	//diagroupcontext = switch_xml_toxml(x_diaallgroups, SWITCH_FALSE);
	//if (!diagroupcontext) {
	//	strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
	//	pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	//	goto end_flag;
	//}

	////直接覆盖以前的文件
	//bRes = cari_common_creatXMLFile(diagroup_xmlfile, diagroupcontext, err);
	//if (!bRes) {
	//	//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
	//	//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

	//	goto end_flag;
	//}
	
	//////////////////////////////////////////////////////////////////////////
	//重新加载对应的内存,此步骤是否为必要???,因为检查组是否存在的时候使用,也可以
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diagroup_xmlfile);
		goto end_flag;
	}

	//增加组成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("ADD_MOBGROUP_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

/*-----*/
end_flag:



	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml结构的内存释放
	switch_xml_free(x_allgroups);

	switch_safe_free(pDesc);
	switch_safe_free(groupPath);
	switch_safe_free(default_xmlfile);
	return iFunRes;
}

/*删除组,必须把此组下用户的信息也删除掉
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
	}; //每个组最多CARICCP_MAX_MOBUSER_IN_GROUP_NUM个mob用户
	char **allMobUsersInGroup = mobusers;
	char *groupPath = NULL, *default_xmlfile = NULL, *chTmp = NULL, *user_default_xmlfile = NULL, *diagroup_xmlfile = NULL;
	char *groupInfo = NULL, *xmlfilecontext = NULL;
	char *newGroupsContext = NULL, *oldGroupsContext = NULL;
	switch_xml_t x_delgroup = NULL,x_allgroups = NULL,x_domain = NULL,x_groups = NULL,x_user = NULL,x_extension = NULL;
	bool bRes = false;

	vector< string> vec;
	string newStr, strTmp;

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	string strGroupName;

	//"groupName"
	strParamName = "groupName";//组名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//组名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){ //组名超长
		//组名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGroupName = strValue;

	//判断此组是否存在,不存在的情况
	if (!isExistedGroup(strGroupName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
		goto end_flag;
	}

	//操作返回的描述信息
	strReturnDesc = getValueOfDefinedVar("MO_DEL_GROUP_FAILED");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

	default_xmlfile = getDirectoryDefaultXMLFile();//需要释放
	x_allgroups = cari_common_parseXmlFromFile(default_xmlfile);
	if (!x_allgroups) {
		goto end_flag;
	}

	//装换成字符串,回退使用,保存目前xml的内容
	oldGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);

	//查找对应的groups节点
	x_domain = switch_xml_child(x_allgroups, "domain");
	if (!x_domain) {
		goto end_flag;
	}
	x_groups = switch_xml_child(x_domain, "groups");
	if (!x_groups) {
		goto end_flag;
	}
	//查找对应的group的结构,根据具体的属性来查找
	x_delgroup = switch_xml_find_child(x_groups, "group", "name", strGroupName.c_str());
	if (!x_delgroup) {
		goto end_flag;
	}

	/************************************************************************/
	//动态更改内存的结构,将的group节点删除,此时x_allgroups的内容也会动态发生变化
	switch_xml_remove(x_delgroup);
	/************************************************************************/

	//重新写文件
	newGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);
	if (!newGroupsContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	bRes = cari_common_creatXMLFile(default_xmlfile, newGroupsContext, err);
	if (!bRes) {
		//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
		//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

		goto end_flag;
	}
	//考虑删除对应的组的目录
	groupPath = getMobGroupDir(strGroupName.c_str());
	if (!cari_common_delDir(groupPath, err)) {
		//无论成功失败,暂时不考虑

		//iFunRes = CARICCP_SUCCESS_STATE_CODE;
		//iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
		//strReturnDesc = getValueOfDefinedVar("ADD_MOBGROUP_SUCCESS");
		//strReturnDesc = switch_mprintf(strReturnDesc.c_str(),strGroupName.c_str());

		//goto end_flag;
	}
	switch_safe_free(groupPath);

	//重新加载对应的内存,此步骤是否为必要???,因为检查组是否存在的时候使用,也可以
	//等待增加第一用户的时候再重新加载内存结构

	//查找此组下的所有mob用户,从root xml的内存结构中进行查找
	getMobUsersOfGroupFromMem(strGroupName.c_str(), allMobUsersInGroup);
	while (*allMobUsersInGroup) {
		//char * ch1,*ch2;
		chTmp = *allMobUsersInGroup;//获得对应的用户id号

		//修改default目录下对应的用户号的xml文件的组的属性
		//创建(O_CREAT)新的MOB用户文件
		char *pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, chTmp);
			switch_safe_free(pFile);
		}

		if (!user_default_xmlfile) {
			continue;
		}

		//此结构需要释放
		x_user = getMobDefaultXMLInfo(chTmp);
		if (!x_user) {
			continue;
		}

		//查找对应的callgroup的属性
		groupInfo = NULL;
		groupInfo = getMobUserVarAttr(x_user, "callgroup");
		if (!groupInfo) {
			allMobUsersInGroup++;
			continue;
		}

		//去除组的信息,考虑放置到中间和最后等位置的特殊情况
		//groupInfo = switch_mprintf("%s%s", strGroupName.c_str(), ",");
		//ch1 = switch_string_replace(chTmp, groupInfo, "");
		//ch2 = switch_string_replace(ch1, strGroupName.c_str(), "");
	
		string strG=getNewStr(groupInfo,strGroupName.c_str());

		//修改此用户的属性值,x_user的内容会被修改
		setMobUserVarAttr(x_user, "callgroup", strG.c_str());

		//考虑释放chTmp,switch_string_replace()函数分配了新的内存空间
		//注意:此方法曾出现致命错误???????????????????????????
		//switch_safe_free(groupInfo);
		//switch_safe_free(ch1);
		//switch_safe_free(ch2);

		//重新创建修改后的用户信息文件
		xmlfilecontext = switch_xml_toxml(x_user, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(user_default_xmlfile, xmlfilecontext, err);
		
		switch_safe_free(user_default_xmlfile);//此处释放

		//释放xml结构
		switch_xml_free(x_user);

		if (!bRes) {
			allMobUsersInGroup++;
			continue;
		}

		//下一个用户
		allMobUsersInGroup++;
	}//end while

	//////////////////////////////////////////////////////////////////////////////
	//////diaplan group xml信息的更改: conf/dialplan/default/02_group_dial.xml
	////diagroup_xmlfile = getDirectoryDialGroupXMLFile();
	////x_extension = cari_common_parseXmlFromFile(diagroup_xmlfile);
	////if (!x_extension) {
	////	strReturnDesc = getValueOfDefinedVar("MO_DEL_GROUP_FAILED_NO_DIALXML");
	////	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
	////	goto end_flag;
	////}

	//////查找对应的group的结构,根据具体的属性来查找
	////pCh = switch_mprintf("%s%s", "group_dial_", strGroupName.c_str());
	////x_delgroup = switch_xml_find_child(x_extension, "extension", "name", pCh);
	////switch_safe_free(pCh);
	////if (!x_delgroup) {
	////	goto end_flag;
	////}

	/////************************************************************************/
	//////动态更改内存的结构,将的group节点删除,此时x_allgroups的内容也会动态发生变化
	////switch_xml_remove(x_delgroup);
	/////************************************************************************/

	//////重新写文件
	////newGroupsContext = switch_xml_toxml(x_extension, SWITCH_FALSE);
	////if (!newGroupsContext) {
	////	strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
	////	goto end_flag;
	////}

	////bRes = cari_common_creatXMLFile(diagroup_xmlfile, newGroupsContext, err);
	////pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	////if (!bRes) {
	////	//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
	////	//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

	////	goto end_flag;
	////}

	/************************************************************************/
	//////////////////////////////////////////////////////////////////////////
	//考虑重新加载root xml的所有文件.最原始的配置文件freeswitch.xml,先加载到bak(备份)文件中
	//重新创建root xml文件的结构
	//另外,考虑写入内存的方式,而不是重新加载文件,效率问题.使用switch_xml_move()方法来测试一下!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diagroup_xmlfile);
		goto end_flag;
	}

	//删除组成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("DEL_MOBGROUPR_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

/*-----*/
end_flag:

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml结构的内存释放
	switch_xml_free(x_allgroups);

	switch_safe_free(pDesc);
	switch_safe_free(default_xmlfile);
	return iFunRes;
}

/*修改用户组,主要是修改名称和相关的描述信息
*/
int CModule_User::modMobGroup(inner_CmdReq_Frame *&reqFrame, 
							  inner_ResultResponse_Frame *&inner_RespFrame, 
							  bool bReLoadRoot)
{
	//用户组的名字进行修改,涉及到的default.xml文件需要修改对应的属性
	//文件目录的名字需要修改
	//涉及到此组中的用户的callgroup属性需要修改
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
	}; //每个组最多CARICCP_MAX_MOBUSER_IN_GROUP_NUM个mob用户
	char **allMobUsersInGroup = mobusers;
	int iGroupPos = 0;

	vector<string> vec;
	string newStr, strTmp;

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//组名和组的描述信息
	string strOldGroupName, strNewGroupName, strGroupDesc, /*strGroupID,*/ strModFlag="no";
	//int iGroupId = 0;

	//"oldgroupname"
	strParamName = "oldgroupname";//组名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//组名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){
		//组名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//是否含有汉字问题
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strOldGroupName = strValue;

	//"newgroupname"
	strParamName = "newgroupname";//组名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//组名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){
		//组名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//是否含有汉字问题
	bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes) {
	//	strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strNewGroupName = strValue;

	////"modFlag"
	//strParamName = "modFlag";//是否修改组号码的标识
	//strChinaName = getValueOfDefinedVar(strParamName);
	//strValue = getValue(strParamName, reqFrame);
	//trim(strValue);
	//strModFlag = strValue;
	//if (isEqualStr(strModFlag,"yes")){
	//	//"groupID"
	//	strParamName = "groupID";//组号
	//	strValue = getValue(strParamName, reqFrame);
	//	trim(strValue);
	//	if (0 == strValue.size()) {
	//		//组名字为空,返回错误信息
	//		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//		goto end_flag;
	//	}
	//	//组号的范围[1-10]
	//	iGroupId = stringToInt(strValue.c_str());
	//	if (MIN_GROUP_ID > iGroupId 
	//		|| MAX_GROUP_ID < iGroupId) {
	//		//组号的范围超出
	//		strReturnDesc = getValueOfDefinedVar("GROUPID_RANGE_ERROR");
	//		pDesc = switch_mprintf(strReturnDesc.c_str(), MIN_GROUP_ID, MAX_GROUP_ID);
	//		goto end_flag;
	//	}
	//	strGroupID = strValue;
	//}

	//"desc"
	strParamName = "desc";//组描述信息
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//组描述信息超长
		//组描述信息超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	//是否含有汉字问题
	//bRes = cari_common_isExsitedChinaChar(strValue.c_str());
	//if (bRes)
	//{
	//strReturnDesc = getValueOfDefinedVar("PARAM_CHINA_ERROR");
	//	strReturnDesc = switch_mprintf(strr,
	//		strChinaName.c_str());

	//	goto end_flag;
	//}
	strGroupDesc = strValue;

	//判断此组是否存在,根据内存中结构进行判断,如果不存在,则不允许
	if (!isExistedGroup(strOldGroupName.c_str())) {
		strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
		goto end_flag;
	}

	//是否修改组号
	if (isEqualStr(strModFlag,"yes")){//yes
		////如果"新组名"和"新组号"与以前的相同,则直接返回成功
		//int iOldGID = -1;
		//strTmp = getGroupID(strOldGroupName.c_str());
		//iOldGID = stringToInt(strTmp.c_str());
		////组名不修改
		//if (isEqualStr(strOldGroupName, strNewGroupName)){
		//	if (iOldGID == iGroupId){
		//		goto success_flag;
		//	}
		//	else{//组号是否已经被其他组使用
		//		if(isExistedGroup("", iGroupId)){
		//			strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_3");
		//			pDesc = switch_mprintf(strReturnDesc.c_str(),iGroupId);
		//			goto end_flag;
		//		}
		//	}
		//}
		////组名修改
		//else{
		//	//判断修改后的新组(名和号)是否存在,如果存在,则不允许
		//	if (iOldGID != iGroupId){
		//		if (isExistedGroup(strNewGroupName.c_str(), iGroupId)){
		//			strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_2");
		//			pDesc = switch_mprintf(strReturnDesc.c_str());
		//			goto end_flag;
		//		}
		//	}
		//	//如果组号不变,只修改组名,则允许.
		//}
	}
	else{//no
		//如果参数的旧/新组名相同,获得不需要修改用户的名字,下面的操作就不需要了
		if (isEqualStr(strOldGroupName,strNewGroupName)){
			//用户的组的属性,以及组的目录不需要再进行修改,直接返回
			goto success_flag;
		}
		////提取现在的组号,重新设置一下
		//strGroupID = getGroupID(strOldGroupName.c_str());
		//iGroupId = stringToInt(strGroupID.c_str());
	}

	strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED");
	strReturnDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());

	//修改conf/directory/default.xml文件,修改组的信息.如下:
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

	//将内容转换成xml的结构(注意:中文字符问题)
	x_newgroup = switch_xml_parse_str(newGroupsContext, strlen(newGroupsContext));

	default_xmlfile = getDirectoryDefaultXMLFile();
	x_allgroups = cari_common_parseXmlFromFile(default_xmlfile);
	if (!x_allgroups || !x_newgroup) {
		strReturnDesc = getValueOfDefinedVar("MO_CREAT_GROUP_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
		goto end_flag;
	}

	//查找对应的groups节点
	x_domain = switch_xml_child(x_allgroups, "domain");
	if (!x_domain) {
		goto end_flag;
	}

	x_groups = switch_xml_child(x_domain, "groups");
	if (!x_groups) {
		goto end_flag;
	}

	//动态更改内存的结构,将新增的group节点合并到以往的结构中
	//此时x_allgroups的内容也会动态发生变化
	x_group = switch_xml_find_child(x_groups, "group", "name", strOldGroupName.c_str());
	if (!x_group) {
		goto end_flag;
	}

	/************************************************************************/
	//先加入节点
	iGroupPos = getGroupNumFromMem();
	if (!switch_xml_insert(x_newgroup, x_groups, /*iGroupPos*/INSERT_INTOXML_POS)) {
		goto end_flag;
	}

	//加入新的节点成功后,在删除原来的节点
	switch_xml_remove(x_group);
	/************************************************************************/

	//合并成完成的default xml结构,重新写文件
	newGroupsContext = switch_xml_toxml(x_allgroups, SWITCH_FALSE);
	if (!newGroupsContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//直接覆盖以前的文件
	bRes = cari_common_creatXMLFile(default_xmlfile, newGroupsContext, err);
	if (!bRes) {
		//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
		//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//考虑修改对应的组的目录
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
	//针对原来的组的用户的callgroup的属性进行修改
	//查找此组下的所有mob用户,从root xml的内存结构中进行查找
	getMobUsersOfGroupFromMem(strOldGroupName.c_str(), allMobUsersInGroup);
	while (*allMobUsersInGroup) {
		mobuserid = *allMobUsersInGroup;//获得对应的用户id号

		//修改default目录下对应的用户号的xml文件的组的属性
		char *pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, mobuserid);
			switch_safe_free(pFile);
		}
		if (!user_default_xmlfile) {
			continue;
		}

		//此结构需要释放
		x_user = getMobDefaultXMLInfo(mobuserid);
		if (!x_user) {
			continue;
		}

		//查找对应的callgroup的属性
		mobuserattr = NULL;
		mobuserattr = getMobUserVarAttr(x_user, "callgroup");
		if (!mobuserattr) {
			allMobUsersInGroup++;
			continue;
		}

		//替换组的信息,使用新的组替代旧的组信息
		mobuserattr = switch_string_replace(mobuserattr, strOldGroupName.c_str(), strNewGroupName.c_str());

		//修改此用户的属性值,x_user的内容会被修改
		setMobUserVarAttr(x_user, "callgroup", mobuserattr);

		//重新创建修改后的用户信息文件
		xmlfilecontext = switch_xml_toxml(x_user, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(user_default_xmlfile, xmlfilecontext, err);

		//考虑释放switch_string_replace()函数分配了新的内存空间
		//注意:此方法导致系统出现致命错误???????????????????????????
		switch_safe_free(mobuserattr);
		switch_safe_free(user_default_xmlfile);//立即释放

		//释放xml结构
		switch_xml_free(x_user);

		if (!bRes) {
			allMobUsersInGroup++;
			continue;
		}

		//下一个用户
		allMobUsersInGroup++;
	}

	////////////////////////////////////////////////////////////////////////////
	////修改conf/dialplan/default/02_group_dial.xml的信息
	//diagroup_xmlfile = getDirectoryDialGroupXMLFile();
	//x_extension = cari_common_parseXmlFromFile(diagroup_xmlfile);
	//if (!x_extension) {
	//	strReturnDesc = getValueOfDefinedVar("MO_MOD_GROUP_FAILED_4");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
	//	goto end_flag;
	//}

	////查找对应的extension的结构,根据具体的属性来查找
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

	////将内容转换成xml的结构
	//x_diagroup = switch_xml_parse_str(diagroupcontext, strlen(diagroupcontext));
	///************************************************************************/
	////先进行增加
	//if (!switch_xml_insert(x_diagroup, x_extension, /*iGroupPos*/INSERT_INTOXML_POS)){//组的位置
	//	strReturnDesc = getValueOfDefinedVar("XML_INSERT_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());
	//	goto end_flag;
	//}

	////加入新的节点成功后,在删除原来的节点
	//switch_xml_remove(x_delgroup);
	///************************************************************************/

	////合并成完成的degault xml结构,重新写文件
	//diagroupcontext = switch_xml_toxml(x_extension, SWITCH_FALSE);
	//if (!diagroupcontext) {
	//	strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
	//	pDesc = switch_mprintf("%s",strReturnDesc.c_str());
	//	goto end_flag;
	//}

	////直接覆盖以前的文件 :conf/dialplan/default/02_group_dial.xml
	//bRes = cari_common_creatXMLFile(diagroup_xmlfile, diagroupcontext, err);
	//switch_safe_free(diagroupcontext);//创建完文件后释放此指针
	//if (!bRes) {
	//	//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
	//	//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

	//	goto end_flag;
	//}

	//重新加载对应的内存
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
	}

	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), diagroup_xmlfile);
		goto end_flag;
	}

/*---------*/
success_flag:

	//修改组成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("MOD_MOBGROUP_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strOldGroupName.c_str());

/*-----*/
end_flag:

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//xml结构的内存释放
	switch_xml_free(x_allgroups);

	switch_safe_free(pDesc);
	switch_safe_free(default_xmlfile);
	switch_safe_free(diagroup_xmlfile);
	return iFunRes;
}

/*查询mob组,如果输入组名,则查询此组下所有用户,否则,查询所有组的信息
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
	}; //每个组最多CARICCP_MAX_MOBUSER_IN_GROUP_NUM个mob用户

	string tableHeader[4] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("GROUP_NAME"),
		getValueOfDefinedVar("GROUP_DESC"),

		//区分两种不同情况的表头
		getValueOfDefinedVar("USER_ID"),
	};
	int iRecordCout = 0;//记录数量

	char **allMobInfo = mobInfo;
	char *chTmp = NULL, *strTableName;
	char *pDesc = NULL;
	string strAll = "";

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取组名和组的描述信息
	string strGroupName;

	//"groupName"
	strParamName = "groupName";//组名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);

	if (CARICCP_STRING_LENGTH_64 < strValue.length()){//组名超长
		//组名字超长,返回错误信息
	    strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strGroupName = strValue;

	//如果输入了组名,则查询此组下所有的用户信息
	//如果没有输入组名,则显示所有的组
	if (!switch_strlen_zero(strGroupName.c_str())) {
		//判断此组是否存在,不存在的情况
		if (!isExistedGroup(strGroupName.c_str())) {
			strReturnDesc = getValueOfDefinedVar("MO_GROUP_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());
			goto end_flag;
		}
		//////////////////////////////////////////////////////////////////////////
		//情况1:查询此组下所有的"用户"信息
		getMobUsersOfGroupFromMem(strGroupName.c_str(), allMobInfo);
		if (!*allMobInfo) {
			strReturnDesc = getValueOfDefinedVar("PRIVATE_NO_USER_IN_GROUP");
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}

		//表头信息设置
		strTableName = switch_mprintf("%s%s%s", 
			tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
			tableHeader[3].c_str());

		iFunRes = SWITCH_STATUS_SUCCESS;
		iCmdReturnCode = SWITCH_STATUS_SUCCESS;
		strReturnDesc = getValueOfDefinedVar("PRIVATE_USER_IN_GROUP");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());

		//设置表头字段
		myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
		switch_safe_free(strTableName);

		//遍历组中的每个用户
		while (*allMobInfo) {
			chTmp = *allMobInfo;//获得对应的用户id号
			char *name = switch_mprintf("%s%s", chTmp, CARICCP_ENTER_KEY);//每行记录之间使用CARICCP_ENTER_KEY区分

			//mod by xxl 2010-5-27 :增加序号的显示
			iRecordCout++;//序号递增
			strRecord = "";
			strSeq = intToString(iRecordCout);
			strRecord.append(strSeq);
			strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
			strRecord.append(name);

			//存放记录
			inner_RespFrame->m_vectTableValue.push_back(/*name*/strRecord);
			//mod by xxl 2010-5-27 end

			switch_safe_free(name);

			//下一个用户
			allMobInfo++;
		}
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//情况2:查询所有"组"的信息
	getAllGroupFromMem(strAll);
	if (0 == strAll.length()) {
		strReturnDesc = getValueOfDefinedVar("PRIVATE_NO_GROUP");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	//*allMobInfo = (char*)strAll.c_str();

	//表头信息设置
	strTableName = switch_mprintf("%s%s%s%s%s", 
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG, /*TABLE_HEADER_GROUP_ID, CARICCP_SPECIAL_SPLIT_FLAG,*/ 
		tableHeader[1].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[2].c_str());

	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("MOBGROUP_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

    splitString(strAll,CARICCP_ENTER_KEY,inner_RespFrame->m_vectTableValue);

	////遍历每个组
	//while (*allMobInfo) {
	//	chTmp = *allMobInfo;//获得对应的组信息
	//	char *name = switch_mprintf("%s%s", chTmp, CARICCP_ENTER_KEY);//每行记录之间使用CARICCP_ENTER_KEY区分

	//	//存放记录
	//	inner_RespFrame->m_vectTableValue.push_back(name);
	//	switch_safe_free(name);

	//	//下一个用户
	//	allMobInfo++;
	//}

	//设置表头字段
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);


/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));
	switch_safe_free(pDesc);

	return iFunRes;
}

/*设置用户组,即把某些用户增加到此组中.多个用户之间使用,隔开
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

	int userNum = 0,validUserNum = 0,existedUserNum = 0;//输入参数的用户号有效的数量和已经在组中存在的用户数量
	char *users[CARICCP_MAX_MOBUSER_IN_GROUP_NUM] = {
		0
	}; //最多CARICCP_MAX_MOBUSER_IN_GROUP_NUM个用户
	char *default_xmlfile = NULL, *groupInfo = NULL, *user_default_xmlfile = NULL;
	char *xmlfilecontext = NULL;
	char *pDesc = NULL;
	int i = 0,iMaxLen	;
	bool bAddOrDel = true;//增加还是删除用户的标识,默认为增加用户到组中

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	string strGroupName, strAddOrDelFlag, strAllUserName;

	//"groupName"
	strParamName = "groupName";//组名
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//组名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}

	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//组名字超长,返回错误信息
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
		bAddOrDel = false; //此时是要从组中删除用户
	}
	strAddOrDelFlag = strValue;

	//"userid"
	strParamName = "userid";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//组名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}

	if (CARICCP_STRING_LENGTH_512 < strValue.length()){//组名超长
		//组名字超长,返回错误信息
	    strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_512);
		goto end_flag;
	}
	strAllUserName = strValue;

	//判断此组是否存在,根据内存中结构进行判断
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

	//根据输入的用户号,分解每个用户
	userNum = switch_separate_string((char*) strAllUserName.c_str(), CARICCP_CHAR_COMMA_MARK, users, (sizeof(users) / sizeof(users[0])));

	//依次对每个用户进行处理组的信息
	for (i = 0; i < userNum; i++) {
		string newStr;
		char *new_group_xmlfile = NULL,*pFile;
		//查看此组中是否已经存在此用户???
		x_userInGroup = NULL;
		x_userInGroup = getMobUserXMLInfoInGroup(users[i], x_group, x_userInGroup);
		//如果是增加用户到组中
		if (bAddOrDel) {
			if (x_userInGroup) {
				validUserNum++;		//有效的用户数量
				existedUserNum++;	//已经存在的用户数量

				continue; //此组中已经存在此用户
			}
		} 
		else {
			//从组中删除用户
			if (!x_userInGroup) {
				//如果输入的用户号无效,不存在,直接进行判断下一个用户
				continue;
			}

			validUserNum++;		//有效的用户数量
			existedUserNum++;	//已经存在的用户数量
		}
		////再查看此用户是否有效
		//bRes = isExistedMobUser(users[i]);
		//if (!bRes)
		//{
		//	continue;
		//}
		pFile = getMobUserDefaultXMLPath();
		if (pFile) {
			user_default_xmlfile = switch_mprintf("%s%s.xml", pFile, users[i]);
			switch_safe_free(pFile);//此处释放
		}
		if (!user_default_xmlfile) {
			continue;
		}

		//需要释放此结构
        x_user = NULL;
		x_user = cari_common_parseXmlFromFile(user_default_xmlfile);

		//用户号无效,则继续处理,不返回错误!!!
		if (!x_user) {
			goto for_end;
		}
		//有效的用户数量 :如果是增加用户到组中
		if (bAddOrDel) {
			validUserNum++;
		} 
		else {
			existedUserNum++;	//已经存在的用户数量
		}

		new_group_xmlfile = getMobUserGroupXMLFile((char*) strGroupName.c_str(), users[i]);
		if (!new_group_xmlfile) {
			goto for_end;
		}
		/************************************************************************/
		//如果是增加用户到组中,增减文档到组的目录下
		if (bAddOrDel) {
			char *group_xmlfilecontext = switch_mprintf(SIMPLE_MOB_USER_XML_CONTEXT, users[i]);
			bRes = cari_common_creatXMLFile(new_group_xmlfile, group_xmlfilecontext, err);
			switch_safe_free(group_xmlfilecontext);
		} 
		else {//如果是从组中删除用户,则需要从组的目录下删除此用户的相关文件
			bRes = cari_common_delFile(new_group_xmlfile, err);
		}
		switch_safe_free(new_group_xmlfile);
		/************************************************************************/

		if (!bRes) {
			goto for_end;
		}

		//同时还需要修改default目录下用户的xml文件的callgroup的属性信息
		//修改default目录下对应的用户号的xml文件的组的属性
		groupInfo = NULL;
		groupInfo = getMobUserVarAttr(x_user, "callgroup");
		if (!groupInfo) {
			goto for_end;
		}
		//增加组操作
		if (bAddOrDel){
			//增加组的信息,考虑如果以前没有加入组,此组是第一个
			if (switch_strlen_zero(groupInfo)) {
				groupInfo = switch_mprintf("%s", strGroupName.c_str());
			} 
			else {
				//不再判断字串中是否已经含有???
			  	groupInfo = switch_mprintf("%s%s%s", groupInfo, ",", strGroupName.c_str());
			}
		} 
		else {
			string strNewG = getNewStr(groupInfo,strGroupName.c_str());
			//test
			//groupInfo =  (char *)strNewG.c_str();//如此赋值,由于strNewG是局部变量,导致生存期外,groupInfo值出现异常,xml文件出错!!!
			//printf("测试1: %s\n",groupInfo);
			groupInfo = switch_mprintf("%s", strNewG.c_str());
		}
		////test 
		//if (!bAddOrDel) {
		//	printf("测试2: %s\n",groupInfo);
		//}

		//修改此用户的属性值,x_user的内容会被修改
		setMobUserVarAttr(x_user, "callgroup", groupInfo);

		//重新创建修改后的用户信息文件,下面
		xmlfilecontext = switch_xml_toxml(x_user, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(user_default_xmlfile, xmlfilecontext, err);
		
		//先立即释放
		switch_safe_free(groupInfo);
		if (!bRes) {
			//不处理
		}

for_end:
		//释放xml结构
		switch_xml_free(x_user);
		
		//最后统一释放
		switch_safe_free(user_default_xmlfile);

	}//遍历所有的用户,设置完成

	//用户号都是无效的
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

	//增加用户到组中
	if (bAddOrDel) {
		//如果输入的用户号都已经在组中存在了,就没有必要再进行重新加载root xml的结构,提供效率
		if (validUserNum == existedUserNum) {
			goto no_reload;
		}
	} 
	else{//从组中删除用户
		//如果输入的用户号都是无效的,此时不需要操作
		if (0 == existedUserNum) {
			goto no_reload;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//重新加载对应的内存,此步骤是否为必要???,因为检查组是否存在的时候使用,也可以
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		//直接调用会产生阻塞,因为没有直接释放!!!
		//switch_xml_open_root(1,err);
	}
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), user_default_xmlfile);
		goto end_flag;
	}

/*------*/
no_reload:

	//增加用户的命令执行成功,需要通知所有注册的客户端
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("SET_MOBGROUP_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strGroupName.c_str());

/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(pDesc);
	return iFunRes;
}

/*设置号码的呼叫转移,呼叫前转/忙时,此时的呼叫转移到备用号码上
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

	//先进行参数的校验
	string strParamName, strValue, strChinaName;
	//获取用户号和用户的密码:必选参数
	string strTranscallType,strFirstUserName, strSecondUserName,strSecondUserType,strGateway;
	string strTimeslot1,strTimeslot2,strTimeslot3,strValidUser,strTransferUser;
	string strTmp;
	vector<string> vecTransferID;
	int ipos = 0;

	//"transcalltype"
	strParamName = "transcalltype";//转呼类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strTranscallType = strValue;

	//"userfirstname"
	strParamName = "userfirstname";//用户号
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户号超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//用户号不是整型数字,返回错误信息
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strFirstUserName = strValue;

	//"transcallusertype"
	strParamName = "transcallusertype";//转呼号用户类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strSecondUserType = strValue;

	//网关的设置
	if (isEqualStr("out",strSecondUserType)){
		//"gateway"
		strParamName = "gateway";//网关
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		if (0 == strValue.size()) {
			//为空,返回错误信息
			strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
			goto end_flag;
		}
		strGateway = strValue;

		//网关是否存在
		CModule_Route moduleRoute;
		if (!moduleRoute.isExistedGateWay("internal", strGateway.c_str())) {
			strReturnDesc = getValueOfDefinedVar("GATEWAY_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strGateway.c_str());
			goto end_flag;
		}
	}

	//"usersecname"
	strParamName = "usersecname";//转呼号
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户号超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strValue = filterSameStr(strValue,CARICCP_COMMA_MARK);//过滤相同的号
	if (0 == strValue.size()) {
		strReturnDesc = getValueOfDefinedVar("PARAM_VAL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	strSecondUserName = strValue;
	
	//add by xxl 2010-10-22 begin :如果是"夜呼"功能,则必须对时间段进行设置,
	//其中时间段1,时间段2和时间段3非必选.
	if (isEqualStr("night",strTranscallType)){
		//"timeslot1"
		strParamName = "timeslot1";//时间段1
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		strTimeslot1 = strValue;
		//检查时间格式是否正确
		if (0 != strTimeslot1.length()){
			if(!isValidTimeSlot(strTimeslot1)){
				//时间段格式无效
				strReturnDesc = getValueOfDefinedVar("PARAM_FORMAT_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
				goto end_flag;
			}
		}

		//"timeslot2"
		strParamName = "timeslot2";//时间段2
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		strTimeslot2 = strValue;
		//检查时间格式是否正确
		if (0 != strTimeslot2.length()){
			if(!isValidTimeSlot(strTimeslot2)){
				//时间段格式无效
				strReturnDesc = getValueOfDefinedVar("PARAM_FORMAT_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
				goto end_flag;
			}

			//封装时间格式
			if (0 != strTimeslot1.length()){
				strTimeslot1.append(CARICCP_COMMA_MARK);
			}
			strTimeslot1.append(strTimeslot2);
		}

		//"timeslot3"
		strParamName = "timeslot3";//时间段3
		strChinaName = getValueOfDefinedVar(strParamName);
		strValue = getValue(strParamName, reqFrame);
		trim(strValue);
		strTimeslot3 = strValue;
		//检查时间格式是否正确
		if (0 != strTimeslot3.length()){
			if(!isValidTimeSlot(strTimeslot3)){
				//时间段格式无效
				strReturnDesc = getValueOfDefinedVar("PARAM_FORMAT_ERROR");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
				goto end_flag;
			}

			//封装时间格式
			if (0 != strTimeslot1.length()){
				strTimeslot1.append(CARICCP_COMMA_MARK);
			}
			strTimeslot1.append(strTimeslot3);
		}
	}
	//add by xxl 2010-10-22 end

	//判断"主号"是否存在
	bRes = isExistedMobUser(strFirstUserName.c_str());
	if (!bRes) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("FIR_NUM_NOT_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());
		goto end_flag;
	}

	//此结构需要释放
	x_first_user = getMobDefaultXMLInfo(strFirstUserName.c_str());
	if (!x_first_user) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("MOBUSER_DEFAULT_XMLFILE_NOEXIST");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//以前处理逻辑 :"忙时"转呼
	if (isEqualStr("busy",strTranscallType)){
		//检查一下主号是否为其他号码的被转号
		account = getMobUserVarAttr(x_first_user, "accountcode");
		if (account
			&&(!switch_strlen_zero(account))) {
				//accountcode和id不相等,说明此主号为转换号,不要再进行设置,否则可能导致循环 
				if (strcasecmp(account, strFirstUserName.c_str())) {
					strReturnDesc = getValueOfDefinedVar("FIR_IS_TRANSFER");
					pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str(),account);
					goto end_flag;
				}
		}
		//mod by xxl 2010-10-26 begin :不做太复杂的处理,和"夜呼"处理保持一致.
		////检查一下主号目前是否已经设置了转呼号
		//transerid = getMobUserVarAttr(x_first_user, "fail_over");
		//if (transerid
		//	&&(!switch_strlen_zero(transerid))) {
		//		strTmp = transerid;

		//		////和当前的设置一致,不需更改视为成功
		//		//if (!strcasecmp(transerid, strSecondUserName.c_str())) {
		//		//	goto succuss_flag;
		//		//}
		//		//如果格式为:voicemail:1001,则继续处理,视为正常情况
		//		ipos = strTmp.find("voicemail");
		//		if (0 > ipos){
		//			//出错提示:必须先解除以前的转呼(涉及到很多文件的修改!!!此处不再做太复杂的处理)
		//			strReturnDesc = getValueOfDefinedVar("FIR_HAS_TRANSFER");
		//			pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str(),transerid);
		//			goto end_flag;
		//		}
		//}
		//mod by xxl 2010-10-26 end
	}

	strReturnDesc = getValueOfDefinedVar("OP_FAILED");

	//mod by xxl 2010-06-02 begin :不支持"多级"转呼功能,但是支持"多个"转呼功能
	//mod by xxl 2010-10-22 begin :增加条件限制 :对"本局"的转呼号进行详细的判断处理
	//                             "出局"的用户在软交换平台上无法识别,直接添加即可.
	splitString(strSecondUserName,CARICCP_COMMA_MARK,vecTransferID);

	//本局转呼
	if (isEqualStr("local",strSecondUserType)){
		//"多个"转呼号处理
		for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
			string strID = *it;
			
			//用户号是否为整型数字
			if (!isNumber(strID)) {
				strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
				goto end_flag;
			}
			//主号和转呼号是否相同
			if (!strcasecmp(strFirstUserName.c_str(), strID.c_str())) {
				strReturnDesc = getValueOfDefinedVar("FIR_SEC_EQUAL_ERROR");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto end_flag;
			}
			//判断"转呼号"是否存在
			bRes = isExistedMobUser(strID.c_str());
			if (!bRes) {
				iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
				strReturnDesc = getValueOfDefinedVar("SEC_NUM_NOT_EXIST");
				pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
				goto end_flag;
			}

			//////////////////////////////////////////////////////////////////////////
			//"忙时"处理
			if (isEqualStr("busy",strTranscallType)){

				//mod by xxl 2010-10-26 begin :不做太复杂的处理,和"夜呼"处理保持一致.
				/*
				x_second_user = getMobDefaultXMLInfo(strID.c_str());
				if (!x_second_user){
					continue;
				}
				beforeMainID = getMobUserVarAttr(x_second_user, "accountcode");//此时必须先取得accountcode号
				if (beforeMainID &&
					(!strcasecmp(beforeMainID, strFirstUserName.c_str()))) {//保持不变
						switch_xml_free(x_second_user);
						continue;
				}
				//此转呼号是否已经设置了其他的转呼号???不支持多级!!!!!!!!!!!
				char *chAttr = getMobUserVarAttr(x_second_user, "fail_over");
				if (chAttr){
					strTmp = chAttr;

					//如果格式为:voicemail:1001,则继续处理,视为正常情况
					ipos = strTmp.find("voicemail");
					if (0 > ipos){
						switch_xml_free(x_second_user);

						//出错提示:必须先解除以前的转呼(涉及到很多文件的修改!!!此处不再做太复杂的处理)
						strReturnDesc = getValueOfDefinedVar("FIR_HAS_TRANSFER_2");
						pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str(),chAttr);
						goto end_flag;
					}
				}

				//转呼号添加/修改下面的属性
				//<variable name="accountcode" value="主号"/>
				//<variable name="max_calls" value="1"/>
				//<variable name="fail_over" value="voicemail:主号"/>
				chFailover = switch_mprintf("voicemail:%s", strFirstUserName.c_str());
				setMobUserVarAttr(x_second_user, "accountcode", strFirstUserName.c_str(), true); //accountcode属性必须同主号一致
				setMobUserVarAttr(x_second_user, "max_calls", "1", true);
				setMobUserVarAttr(x_second_user, "fail_over", chFailover, true);

				//重写"转呼号"的文件
				bRes = rewriteMobUserXMLFile(strID.c_str(),x_second_user,strReturnDesc);
				if (!bRes) {
					switch_xml_free(x_second_user);

					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto end_flag;
				}
				switch_safe_free(chFailover);

				//此处还需要注意:如果此转呼号码是以前其他主号使用(既:多个主号对应同一个转呼号),则需要更改前主号的"fail_over"属性????
				if (!beforeMainID
					||switch_strlen_zero(beforeMainID)
					|| isEqualStr("",beforeMainID)) {
						switch_xml_free(x_second_user);
						continue;
				}
				//如果以前没有被设置转呼,不再处理.
				if (!strcasecmp(beforeMainID, strID.c_str())) {
					switch_xml_free(x_second_user);
					continue;
				}

				//则需要更改前主号的"fail_over"属性,不允许多个主号对应同一个转呼号
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

				//重写转呼号之前所绑定的主号的xml文件
				bRes = rewriteMobUserXMLFile(beforeMainID,x_beformain_user,strReturnDesc);
				switch_safe_free(chFailover);//立即释放
				if (!bRes) {
					//释放xml结构
					switch_xml_free(x_beformain_user);
					switch_xml_free(x_second_user);

					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto end_flag;
				}

				//最后释放xml结构
				switch_xml_free(x_beformain_user);
				switch_xml_free(x_second_user);
				*/
				//mod by xxl 2010-10-26 end

			}//end 忙时处理
			//////////////////////////////////////////////////////////////////////////
			//"夜呼"处理
			else{
				strValidUser.append("internal/");
			}

			strValidUser.append(strID);
			strValidUser.append(CARICCP_COMMA_MARK);

		}//end for 遍历处理

		if (0!= strValidUser.length()){
			strValidUser = strValidUser.substr(0,strValidUser.length()-1);//去除最后的逗号符号
		}
		strSecondUserName = strValidUser;
	}
	// "出局"的用户转呼处理,不对号码进行有效判断,直接写入
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

	//主号添加/修改属性
	//"忙呼"
	if (isEqualStr("busy",strTranscallType)){
		setMobUserVarAttr(x_first_user, "vm_extension", strFirstUserName.c_str(), true);
		setMobUserVarAttr(x_first_user, "max_calls", "1", true);
		setMobUserVarAttr(x_first_user, "fail_over", strSecondUserName.c_str(), true);//此号码呼叫失败转移到second号码上
	}
	//"夜呼"
	else{
		setMobUserVarAttr(x_first_user, "direct_transfer", strSecondUserName.c_str(), true);
		setMobUserVarAttr(x_first_user, "direct_transfer_period", strTimeslot1.c_str(), true);
	}

	//重写主号文件
	bRes = rewriteMobUserXMLFile(strFirstUserName.c_str(),x_first_user,strReturnDesc);
	if (!bRes) {
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//重新加载root xml文件的结构
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), default_xmlfile);
			goto end_flag;
		}
	}

	//设置转移呼叫成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("SET_TRANSFERCALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());


/*-----*/
end_flag : 
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//释放xml结构
	switch_xml_free(x_first_user);
	//switch_xml_free(x_second_user);

	switch_safe_free(pDesc);
	switch_safe_free(chFailover);
	return iFunRes;
}

/*解除号码的呼叫转移设置
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

	//先进行参数的校验
	string strParamName, strValue, strChinaName;

	//获取用户号和用户的密码:必选参数
	string strTranscallType,strFirstUserName,strTransferID;
	string strTmp,strSegName;
	vector<string> vecTransferID;
	int ipos = 0;

	//"transcalltype"
	strParamName = "transcalltype";//转呼类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	//if (0 == strValue.size()) {
	//	//用户号为空,返回错误信息
	//	strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
	//	pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
	//	goto end_flag;
	//}
	strTranscallType = "all";//目前对所有绑定都解除

	//"userfirstname"
	strParamName = "userfirstname";//用户号
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//用户号为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//用户号超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_MAX_NAME_LENGTH);
		goto end_flag;
	}
	if (!isNumber(strValue)) {
		//用户号不是整型数字,返回错误信息
		strReturnDesc = getValueOfDefinedVar("MOBUSERNAME_INVALID");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
		goto end_flag;
	}
	strFirstUserName = strValue;

	//判断"主号"是否存在
	bRes = isExistedMobUser(strFirstUserName.c_str());
	if (!bRes) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("FIR_NUM_NOT_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());
		goto end_flag;
	}
	
	//此结构需要释放
	x_first_user = getMobDefaultXMLInfo(strFirstUserName.c_str());
	if (!x_first_user) {
		iCmdReturnCode = CARICCP_MOBUSER_EXIST_CODE;
		strReturnDesc = getValueOfDefinedVar("MOBUSER_DEFAULT_XMLFILE_NOEXIST");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//mod by xxl 2010-10-22 begin :针对不同的类型分别处理
	//先处理"忙时"转呼
	if (isEqualStr("busy",strTranscallType)
		|| isEqualStr("all",strTranscallType)){
			strSegName = "fail_over";

			//查看"主号"是否已经有绑定的号码
			chTransferID = getMobUserVarAttr(x_first_user, strSegName.c_str());
			if (!chTransferID) {
				strReturnDesc = getValueOfDefinedVar("NOT_BIND_SECNUM");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto night;//跳转到"夜呼"处理
			}
		
			//如果格式为:voicemail:1001,则视为没有绑定
			strTmp = chTransferID;
			ipos = strTmp.find("voicemail");
			if (0 <= ipos){
				strReturnDesc = getValueOfDefinedVar("NOT_BIND_SECNUM");
				pDesc = switch_mprintf("%s",strReturnDesc.c_str());
				goto night;//跳转到"夜呼'处理
			}

			//依次针对转呼号的文件进行修改
			splitString(chTransferID,CARICCP_COMMA_MARK,vecTransferID);
			for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
				string strID = *it;
				
				//判断绑定的号是否有效
				bRes = isExistedMobUser(strID.c_str());
				if (!bRes) {
					continue;
				}

				//1 先解除"转呼号"对应的绑定关系,修改转呼号的xml文件
				x_second_user = getMobDefaultXMLInfo(strID.c_str());//和上面的判断有些重复了!!!
				if (!x_second_user) {
					//转呼号的xml文件不存在,直接当成功处理
					continue;
				}

				//2 修改"转呼号"的xml文件属性
				chFailover = switch_mprintf("voicemail:%s", strID.c_str());
				setMobUserVarAttr(x_second_user, "accountcode", strID.c_str(), false); //accountcode属性必须同主号一致
				setMobUserVarAttr(x_second_user, "fail_over", chFailover, false);

				//3 重写"转呼号"的xml文件
				bRes = rewriteMobUserXMLFile(strID.c_str(),x_second_user,strReturnDesc);
				switch_safe_free(chFailover);//立即释放
				if (!bRes){
					pDesc = switch_mprintf("%s",strReturnDesc.c_str());
					goto night;//跳转到"夜呼'处理
				}
				
				//释放xml结构
				switch_xml_free(x_second_user);

			}//end for

			//3 最后解除"主号"对应的绑定号,修改主号的xml文件.如果下面转呼号的逻辑有误,则不要影响主号的修改操作
			chFailover = switch_mprintf("voicemail:%s", strFirstUserName.c_str());
			setMobUserVarAttr(x_first_user, "fail_over", chFailover, false);
						
			bBusy = true;
	}

night:

	//再对"夜呼"类型进行处理,简单处理 :直接对主号的有关字段进行清空
	if (isEqualStr("night",strTranscallType)
		|| isEqualStr("all",strTranscallType)){
			strSegName = "direct_transfer";

			//查看"主号"是否已经有绑定的号码
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
			//直接将两个属性段删除掉,不再保留.
			//动态更改内存的结构,将的direct节点删除,此时x_first_user的内容也会动态发生变化
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


	//重写"主号"的xml文件
	bRes = rewriteMobUserXMLFile(strFirstUserName.c_str(),x_first_user,strReturnDesc);
	if (!bRes){
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	switch_safe_free(chFailover);

	//////////////////////////////////////////////////////////////////////////
	//重新加载root xml文件的结构
	if (bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), default_xmlfile);
			goto end_flag;
		}
	}

	//设置转移呼叫成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("UNBIND_TRANSFERCALL_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strFirstUserName.c_str());


/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//释放xml结构
	switch_xml_free(x_first_user);

	switch_safe_free(pDesc);
	switch_safe_free(chFailover);
	return iFunRes;
}

/*查询当前呼叫转移的信息
*/
int CModule_User::lstTransferCallInfo(inner_CmdReq_Frame*& reqFrame, 
									  inner_ResultResponse_Frame*& inner_RespFrame)
{
    int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	char *strTableName = "";//查询的表头
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
		getValueOfDefinedVar("transcalltype"),      //转呼类型
		getValueOfDefinedVar("userfirstname"),      //主号
		getValueOfDefinedVar("usersecname"),        //转呼号
		getValueOfDefinedVar("USER_TRANS_USERTYPE"),//转呼号用户类型
		getValueOfDefinedVar("TIMESLOT")            //时间段
	};
	int iRecordCout = 0;//记录数量

	//////////////////////////////////////////////////////////////////////////
	//查询default组对应的所有用户的信息
	defaultGroupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, defaultGroupXMLinfo, strReturnDesc);
	if (defaultGroupXMLinfo) {
		//遍历所有的用户,查找users节点
		if ((x_users = switch_xml_child(defaultGroupXMLinfo, "users"))) {
			//遍历每个用户
			x_user = switch_xml_child(x_users, "user");
			for (; x_user; x_user = x_user->next) {
				//分别查询"忙时"转呼和"夜呼"两种类型
				
				//////////////////////////////////////////////////////////////////////////
				//1 查找"忙转"记录
				//方法1 :可以根据id和accountcode进行判断是否绑定的呼叫转移,目前采用此种方式进行处理
				userid = switch_xml_attr(x_user, "id");
				accountcode = getMobUserVarAttr(x_user,"accountcode",true);
				if (switch_strlen_zero(userid)) {
					continue;
				}
				if (switch_strlen_zero(accountcode)) {
					continue;
				}
				////如果id和accountcode不相同,则肯定是"忙呼"转移绑定
				////这种处理方式比较简单.
				//if (strcasecmp(userid,accountcode)){
				//	strTranscallType = getValueOfDefinedVar("USER_BUSY_TRANSFER");

				//	//主号和转呼号
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
				//		CARICCP_ENTER_KEY);//每行记录之间使用CARICCP_ENTER_KEY区分

				//	//mod by xxl 2010-5-27 :增加序号的显示
				//	iRecordCout++;//序号递增
				//	strRecord = "";
				//	strValue = intToString(iRecordCout);
				//	strRecord.append(strValue);
				//	strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);

				//	strRecord.append(result);

				//	//存放记录
				//	inner_RespFrame->m_vectTableValue.push_back(/*result*/strRecord);
				//	//mod by xxl 2010-5-27 end

				//	//释放内存
				//	switch_safe_free(result);
				//}

				//方法2 :可以根据id和fail_over的属性进行判断

				//////////////////////////////////////////////////////////////////////////
				//1 先查找忙时转呼
				busycode = getMobUserVarAttr(x_user,"fail_over",true);
				if (switch_strlen_zero(busycode)) {
					goto night;//跳转到"夜转"查看处
				}
				strTmp = busycode;
				//如果格式为:voicemail:1001,则无忙时转呼的情况
				ipos = strTmp.find("voicemail");
				if (0 <= ipos){
					goto night;//跳转到"夜转"查看处
				}

				strTimeSeg="";
				vecTransferInfo.clear();
				getTransferInfo(accountcode,busycode,0,vecTransferInfo);
				for (vector<string>::iterator it = vecTransferInfo.begin(); vecTransferInfo.end() != it; it++){
					string strInfo = *it;

					//主号和转呼号
					result = switch_mprintf("%s%s%s%s", 
						strInfo.c_str(),
						CARICCP_SPECIAL_SPLIT_FLAG,
						strTimeSeg.c_str(),
						CARICCP_ENTER_KEY);//每行记录之间使用CARICCP_ENTER_KEY区分


					iRecordCout++;//序号递增
					strRecord = "";
					strValue = intToString(iRecordCout);
					strRecord.append(strValue);
					strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
					strRecord.append(result);

					//存放记录
					inner_RespFrame->m_vectTableValue.push_back(/*result*/strRecord);

					//释放内存
					switch_safe_free(result);
				}

night:
				//////////////////////////////////////////////////////////////////////////
				//2 再查找"夜转"记录
				nightcode = getMobUserVarAttr(x_user,"direct_transfer",true);
				if (switch_strlen_zero(nightcode)) {
					continue;
				}
				
				//时间段
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

					//主号和转呼号
					result = switch_mprintf("%s%s%s%s", 
						strInfo.c_str(),
						CARICCP_SPECIAL_SPLIT_FLAG,
						strTimeSeg.c_str(),
						CARICCP_ENTER_KEY);//每行记录之间使用CARICCP_ENTER_KEY区分


					iRecordCout++;//序号递增
					strRecord = "";
					strValue = intToString(iRecordCout);
					strRecord.append(strValue);
					strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
					strRecord.append(result);

					//存放记录
					inner_RespFrame->m_vectTableValue.push_back(/*result*/strRecord);

					//释放内存
					switch_safe_free(result);
				}

			}//遍历users
		}
	}
	else{
		if (0 != strReturnDesc.length()){
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

	//没有记录存在的情况下
	if (0 == inner_RespFrame->m_vectTableValue.size()){
		strReturnDesc = getValueOfDefinedVar("FIR_SEC_BIND_NULL");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s", //表头
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

	//设置表头字段
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
/* 增加调度员                                                           */
/************************************************************************/
int CModule_User::addDispatcher(inner_CmdReq_Frame*& reqFrame, 
								inner_ResultResponse_Frame*& inner_RespFrame,
								bool bReLoadRoot)
{

	simpleParamStruct paramStruct;
	paramStruct.strParaName="dispatopt";
	paramStruct.strParaValue="add_dispat";//添加标识

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setDispatcher(reqFrame,inner_RespFrame,true);
}

/************************************************************************/
/* 删除调度员                                                           */
/************************************************************************/
int CModule_User::delDispatcher(inner_CmdReq_Frame*& reqFrame, 
								inner_ResultResponse_Frame*& inner_RespFrame,
								bool bReLoadRoot)
{

	simpleParamStruct paramStruct;
	paramStruct.strParaName="dispatopt";
	paramStruct.strParaValue="del_dispat";//删除标识

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setDispatcher(reqFrame,inner_RespFrame,true);

}

/*设置调度员,增加或删除
*/
int CModule_User::setDispatcher(inner_CmdReq_Frame*& reqFrame, 
								inner_ResultResponse_Frame*& inner_RespFrame,
								bool bReLoadRoot)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "";
	switch_xml_t x_dispatchers = NULL,x_dispatcher = NULL/*,x_user*/;
	int userNum = 0,validUserNum = 0,existedDiapatNum = 0;//输入参数的用户号有效的数量和已经是调度员
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
	//int iDispatcherPriority = USERPRIORITY_SPECIAL; //调度员的权限
	//string strDispatcherPriority="";

	//先进行参数的校验
	string strParamName,strValue/*,strUser*/,strChinaName;

	//必选参数
	string strDispatchopt,strDispatcherID;

	//"dispatopt"
	strParamName = "dispatopt";//增加还是删除调度
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("add_dispat",strValue)){
		bAddOrDel = true;
	}
	else{
		bAddOrDel = false;//其他情况下都标识为"删除"
	}
	strDispatchopt= strValue;

	//"dispatcherID"
	strParamName = "dispatcherID";//调度员号
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_64 < strValue.length()) {
		//名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strDispatcherID = strValue;

	//根据输入的用户号,分解每个用户
	userNum = switch_separate_string((char*) strDispatcherID.c_str(), CARICCP_CHAR_COMMA_MARK, users, (sizeof(users) / sizeof(users[0])));
	x_dispatchers = getAllDispatchersXMLInfo();//获得当前的结构
	if (!x_dispatchers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), CARI_CCP_DISPAT_XML_FILE);
		goto end_flag;
	}
	//依次对每个用户进行处理
	for (i = 0; i < userNum; i++) {
		//先查看此用户是否已经存在
		if (!isExistedMobUser(users[i])){
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST_1");
			pDesc = switch_mprintf(strReturnDesc.c_str(), users[i]);
			goto end_flag;
		}
		validUserNum++;

		//查看此用户是否已经是调度员
		x_dispatcher = getDispatcherXMLInfo(users[i],x_dispatchers);
		if (x_dispatcher){
			existedDiapatNum++;	//已经存在的用户数量
			if (!bAddOrDel){//如果是删除操作
				
				//删除此调度员
				switch_xml_remove(x_dispatcher);

				//mod by xxl 2010-6-24:此用户的权限也要同步修改,设置为一般权限:USERPRIORITY_OUT
				//vecPriUser.push_back(users[i]);
				//mod by xxl 2010-6-24 end
			}
			continue;
		}

		if (bAddOrDel){//如果是增加操作
			//设置用户为调度员
			setdispatcherVarAttr(x_dispatchers,users[i],true);

			//mod by xxl 2010-6-24:调度员的权限也要同步修改,设置为最高权限:USERPRIORITY_SPECIAL
			//vecPriUser.push_back(users[i]);
			//mod by xxl 2010-6-24 end
		}

	}//遍历所有的用户,设置完成

	if (bAddOrDel){//如果是增加操作
		//用户号都是无效的
		if (0 == validUserNum) {
			strReturnDesc = getValueOfDefinedVar("ADD_DISPAT_FAILED");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
		//如果输入的用户号都已经在组中存在了,直接返回成功
		if (validUserNum == existedDiapatNum) {
			goto no_reload;
		}
	}
	else{
		//如果输入的用户号都不是调度员
		if (0 == existedDiapatNum) {
			strReturnDesc = getValueOfDefinedVar("DEL_DISPAT_FAILED");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
	}
	//重新将结构写入到文件
	dispatFile = getDispatFile();
	if (dispatFile) {
		xmlfilecontext = switch_xml_toxml(x_dispatchers, SWITCH_FALSE);
		bRes = cari_common_creatXMLFile(dispatFile, xmlfilecontext, err);
		switch_safe_free(dispatFile);//立即释放
		if (!bRes) {
			strReturnDesc = *err;
			pDesc = switch_mprintf("%s",strReturnDesc.c_str());
			goto end_flag;
		}
	}

	//mod by xxl 2010-6-24:调度员的权限也要同步修改,修改用户的"配置文件",如果是增加,设置为最高权限,
	//如果是删除,设置为一般的权限
	//if (bAddOrDel){
	//	strDispatcherPriority = intToString(USERPRIORITY_SPECIAL);
	//}
	//else{
	//	strDispatcherPriority = intToString(USERPRIORITY_OUT);
	//}
	//for (vector<string>::iterator it = vecPriUser.begin();it != vecPriUser.end();it++){
	//	strUser = *it;

	//	//修改default目录下对应的用户号的xml文件的权限属性
	//	x_user = getMobDefaultXMLInfo(strUser.c_str());
	//	if (!x_user) {
	//		continue;
	//	}
	//	//设置对应的"priority"的属性,最高级别"USERPRIORITY_OUT",x_user的内容会被修改
	//	setMobUserVarAttr(x_user, "priority", strDispatcherPriority.c_str(), true);

	//	//重写文件
	//	bRes = rewriteMobUserXMLFile(strUser.c_str(),x_user,strReturnDesc);

	//	//释放
	//	switch_xml_free(x_user);
	//}
	//mod by xxl 2010-6-24 end


	//mod by xxl 2009-5-18: 调度设置问题
	//重新加载root xml文件.因为调度模块是从内存中直接读取
	if (bReLoadRoot) {
		//直接调用会产生阻塞,因为没有直接释放!!!
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR_2");
			pDesc = switch_mprintf(strReturnDesc.c_str());
			goto end_flag;
		}
	}
	//mod by xxl 2009-5-18 end

	//////////////////////////////////////////////////////////////////////////
	//由于和调度模块相互关联,此处产生事件
	if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
		if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
			switch_event_destroy(&event);
		}
	}

	/*------*/
no_reload:

	//命令执行成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
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

	//释放xml结构
	if (bAddOrDel){//如果是增加操作
		switch_xml_free(x_dispatchers);
	}
	switch_safe_free(pDesc);
	return iFunRes;

}
/*查询调度员
*/
int CModule_User::lstDispatcher(inner_CmdReq_Frame*& reqFrame,
								inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;
	string strReturnDesc = "",strname ;

	char *pDesc = NULL;
	char *strTableName = "";//查询的表头;
	string strAll = "";
	string tableHeader[2] = {
		getValueOfDefinedVar("SEQUENCE"),
		getValueOfDefinedVar("USER_ID"),
	};

	//获得所有的调度员字串信息
	getAllDispatchers(strAll);
	if (0 == strAll.length()) {
		strReturnDesc = getValueOfDefinedVar("DISPAT_RECORD_NULL");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}
	splitString(strAll,CARICCP_ENTER_KEY,inner_RespFrame->m_vectTableValue);

	//表头信息设置
	strTableName = switch_mprintf("%s%s%s", 
		tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		tableHeader[1].c_str());
	iFunRes = SWITCH_STATUS_SUCCESS;
	iCmdReturnCode = SWITCH_STATUS_SUCCESS;
	strReturnDesc = getValueOfDefinedVar("DISPAT_RECORD");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());

	//设置表头字段
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
/*增加录音用户                                                          */
/************************************************************************/
	 
int CModule_User::addRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot)
{
	simpleParamStruct paramStruct;
	paramStruct.strParaName="recordopt";
	paramStruct.strParaValue="add_record";//增加标识

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setRecord(reqFrame,inner_RespFrame);
}

/************************************************************************/
/*删除录音用户                                                          */
/************************************************************************/
int CModule_User::delRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot)
{
	simpleParamStruct paramStruct;
	paramStruct.strParaName="recordopt";
	paramStruct.strParaValue="del_record";//删除标识

	reqFrame->innerParam.m_vecParam.push_back(paramStruct);

	return setRecord(reqFrame,inner_RespFrame);
}

/*设置录音用户
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

	//先进行参数的校验
	string strParamName, strValue, strChinaName;//参数的中文显示名

	//必选参数
	string strRecordopt,strRecordID;

	//"recordopt"
	strParamName = "recordopt";//增加录音还是取消录音
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (isEqualStr("add_record",strValue)){
		strFlag = "yes";//增加录音
		bSetOrCancle = true;
	}
	else{
		strFlag = "no";//取消录音
		bSetOrCancle = false;
	}
	strRecordopt = strValue;

	//"recorduser"
	strParamName = "recorduser";//录音用户
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//名字为空,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_256 < strValue.length()) {
		//名字超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_64);
		goto end_flag;
	}
	strRecordID = strValue;

	//根据输入的用户号,分解每个用户
	userNum = switch_separate_string((char*) strRecordID.c_str(), CARICCP_CHAR_COMMA_MARK, users, (sizeof(users) / sizeof(users[0])));
	//依次对每个用户进行处理
	for (i = 0; i < userNum; i++) {
		char *recordallow = NULL;

		//此结构需要释放
		x_user = getMobDefaultXMLInfo(users[i]);
		//先查看此用户是否已经存在
		if (!x_user){
			strReturnDesc = getValueOfDefinedVar("MO_USER_NOEXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), users[i]);
			goto end_flag;
		}

		//查看此属性是否真正的需要修改
		recordallow = getMobUserVarAttr(x_user,"record_allow");
		if (!switch_strlen_zero(recordallow)
			&&isEqualStr(recordallow,strFlag)){

			continue;
		}
	
		//主号添加/修改下面的属性
		setMobUserVarAttr(x_user, "record_allow", strFlag.c_str(),true);//标识为yes或no

		//重写用户的xml文件
		bRes = rewriteMobUserXMLFile(users[i],x_user,strReturnDesc);
		if (!bRes) {
			continue;
		}

		ivalidnum++;//有效数字修改

		//xml结构释放
		switch_xml_free(x_user);

	}//遍历所有的用户,设置完成

	//////////////////////////////////////////////////////////////////////////
	//重新加载root xml文件的结构
	if (0 != ivalidnum 
		&&bReLoadRoot) {
		iFunRes = cari_common_reset_mainroot(err);
		if (SWITCH_STATUS_SUCCESS != iFunRes) {
			strReturnDesc = getValueOfDefinedVar("RESET_ROOT_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strRecordID.c_str());//以用户名替代文件名
			goto end_flag;
		}
	}

	//设置录音用户成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
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

/*查询录音用户
*/
int CModule_User::lstRecord(inner_CmdReq_Frame*& reqFrame,
							inner_ResultResponse_Frame*& inner_RespFrame)
 {
	 int iFunRes = CARICCP_SUCCESS_STATE_CODE;
	 int iCmdReturnCode = CARICCP_ERROR_STATE_CODE;

	 //结果集合
	 char *pDesc          = NULL,*strTableName=NULL;
	 const char *userid = NULL;
	 switch_xml_t groupXMLinfo = NULL,x_users = NULL, x_user = NULL;
	 //char *strResult = "";	//如果如此定义会出现问题,内存没有释放掉.使用memset()
	 string strResult = "",strReturnDesc,strSeq;
	 string tableHeader[2] = {
		 getValueOfDefinedVar("SEQUENCE"),
		 getValueOfDefinedVar("USER_ID")
	 };
	 int iRecordCout = 0;//记录数量

	 //////////////////////////////////////////////////////////////////////////
	 //查询default组对应的所有用户的信息
	 groupXMLinfo = getGroupXMLInfoFromMem(DEFAULT_GROUP_NAME, groupXMLinfo, strReturnDesc);
	 if (groupXMLinfo) {
		 if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {
			 //遍历每个用户
			 x_user = switch_xml_child(x_users, "user");
			 for (; x_user; x_user = x_user->next) {
				 char *recordallow = NULL;
				 userid = switch_xml_attr(x_user, "id");
				 recordallow = getMobUserVarAttr(x_user,"record_allow",true);
				 if (!switch_strlen_zero(userid)
					 &&NULL != recordallow 
					 &&isEqualStr(recordallow,"yes")){

						 //mod by xxl 2010-5-27 :增加序号的显示
						 iRecordCout++;//序号递增
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

	 //返回信息给客户端,重新设置下返回信息
	 //表头信息设置
	 strTableName = switch_mprintf("%s%s%s", 
		 tableHeader[0].c_str(), CARICCP_SPECIAL_SPLIT_FLAG,
		 tableHeader[1].c_str());
	 iFunRes = CARICCP_SUCCESS_STATE_CODE;
	 iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	 strReturnDesc = getValueOfDefinedVar("RECORD_RECORD");
	 pDesc = switch_mprintf(strReturnDesc.c_str());

	 //设置表头字段
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
/*								其他操作 								*/
/************************************************************************/

/*根据用户的ID号判断是否(注册)有效
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
	//		strUserID.c_str());//必须使用c_str(),否则出错
	//
	//
	//	//如果和数据库的连接句柄profile->master_odbc失败,是否考虑重连???
	//	if (SWITCH_ODBC_SUCCESS != switch_odbc_handle_exec(odbc_handle, sql, &stmt)) 
	//	{
	//		errorMsg = switch_odbc_handle_get_error(odbc_handle, stmt);
	//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, switch_str_nil(errorMsg));
	//
	//		//释放内存
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
	//	//释放内存,由调用者负责释放
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

/*查看此用户是否已经存在
*@param userid :用户的名(号)
*/
bool CModule_User::isExistedMobUser(const char *userid)
{
	switch_status_t iFunRes = SWITCH_STATUS_SUCCESS;

	//如何查找用户的信息???在哪里查找?存放在哪里的,如何查找?
	const char *domain_name, *group_name, *group_id,*mobuserid;
	switch_xml_t xml = NULL, group = NULL;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users,x_user;
	switch_event_t *params = NULL;
	bool bExsited = false;

	//用户号(名)判断,非必要
	if (switch_strlen_zero(userid)) {
		goto end;
	}
	//根据profile的属性获得对应的域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//注意 :下面的这个方法不是很严格,最好根据default组来进行查找用户是否存在
	//使用下面的方法,上述方法会造成"增/删"用户的阻塞??!!!why???
	//iFunRes = switch_xml_locate_user("id", userid, domain_name, NULL, &xml, &domain, &user, &group, params);

	//先查找domain,此处的调用会分配内存,必须要释放
	iFunRes = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != iFunRes) {
		goto end;
	}
	//根据xml文件,分级查找对应组信息domain-->groups-->group->users->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		//遍历每个组
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");
			group_id = switch_xml_attr(x_group, "id");
			//default组
			if (NULL != group_name 
				&& isEqualStr(group_name, DEFAULT_GROUP_NAME)){
				
				//查找users节点
				if ((x_users = switch_xml_child(x_group, "users"))) {
					//遍历每个用户
					x_user = switch_xml_child(x_users, "user");
					for (; x_user; x_user = x_user->next) {
						mobuserid = switch_xml_attr(x_user, "id");
						if (switch_strlen_zero(userid)) {
							continue;
						}
						//存在此用户
						if (!strcasecmp(userid, mobuserid)) {
							bExsited = true;
							goto end;
						}
					}//遍历users
				}
			}
		}
	}
	
end:

	//读取获得root内存结构,RWLOCK锁被锁住,因此需要释放
	//此处实际上是释放了锁RWLOCK,而不是真正了释放了root的xml结构
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain()函数,因此必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	//返回值为bool类型
	return bExsited;
}

/*查看此用户是否已经注册
*/
bool CModule_User::isRegisteredMobUser(const char* userID,
								  switch_core_db_t *db)
{
	bool ret = false;

	//目前使用数据形式进行查询
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

	//根据查询的记录查看是否存在此"注册"用户
	string strRes = getValueOfVec(0,resultVec);
	int iNum = stringToInt(strRes.c_str());
	if (0 != iNum){
		ret = true;
	}

	return ret;
}

/*查看组是否存在,主要从root xml内存中进行查找
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
		//特殊的默认的组不需要考虑
		if (!strncasecmp(DEFAULT_GROUP_NAME, mobgroupname, iMaxLen)) {
			goto end;
		}
	}
	
	//域名
	domain_name = getDomainIP();

	//先查找domain,此处的调用会分配内存,必须要释放
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//根据xml文件,分级查找对应组信息domain-->groups-->group
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		//遍历每个组
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");
			group_id = switch_xml_attr(x_group, "id");

			if (NULL != group_name 
				&& (!switch_strlen_zero(mobgroupname))) {
				if (isEqualStr(group_name, mobgroupname)){
				//if (!strcasecmp(group_name, mobgroupname)) {//组名相同,则存在
					bExsited = true;
					break;
				}
			}
			//如果使用默认值,则不考虑组号的问题
			if (-1 == groupid) {
				continue;
			}
			if (NULL != group_id) {
				mobgroupid = stringToInt(group_id);        //组号相同,则存在
				if (mobgroupid == groupid) {
					bExsited = true;
					break;
				}
			}//end 查找相同的组
		}
	}

end:
	//读取获得root内存结构,RWLOCK锁被锁住,因此需要释放
	//此处实际上是释放了锁RWLOCK,而不是真正了释放了root的xml结构
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return bExsited;
}

/*获得组号
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
	//特殊的默认的组不需要考虑
	if (!strncasecmp(DEFAULT_GROUP_NAME, mobgroupname, iMaxLen)) {//isEqualStr()
		goto end;
	}

	//域名
	domain_name = getDomainIP();

	//先查找domain,必须要释放分配的内存
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}
	//根据xml文件,分级查找对应组信息domain-->groups-->group
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		//遍历每个组
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");
			group_id = switch_xml_attr(x_group, "id");
			if (NULL != group_name) {
				if (isEqualStr(group_name, mobgroupname)){
					//if (!strcasecmp(group_name, mobgroupname)) {//组名相同,则存在
					strGID = group_id;
					break;
				}
			}
		}
	}

end:
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return strGID;

}

/*字符串中去除delStr串剩下的字串
*/
string CModule_User::getNewStr(const char* oldStr,const char* delStr)
{
	if (NULL == oldStr){
		return "";
	}

	vector<string> vecG;
	vecG.clear();
	splitString(oldStr,",",vecG);

	//查找是否存在此组
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

/*获得用户的组的信息,默认的组不需要考虑
*/
void CModule_User::getMobGroupOfUserFromMem(const char *mobuserid, 
											char **allMobGroup)//输出参数 :用户组的信息
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, *userid;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users = NULL,x_user = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(mobuserid)) {
		goto end;
	}

	//域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//先查找domain
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//根据xml文件,分级查找对应的用户domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			//default不需要考虑
			iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, group_name);

			//特殊的默认default的组不需要考虑
			if (!strncasecmp(DEFAULT_GROUP_NAME, group_name, iMaxLen)) {
				continue;
			}

			//查找users节点
			if ((x_users = switch_xml_child(x_group, "users"))) {
				//遍历每个用户
				x_user = switch_xml_child(x_users, "user");
				for (; x_user; x_user = x_user->next) {
					userid = switch_xml_attr(x_user, "id");
					if (switch_strlen_zero(userid)) {
						continue;
					}

					//存在此用户
					if (!strcasecmp(userid, mobuserid)) {
						//组的信息保存下来
						//*allMobGroup = (char *)switch_xml_attr(x_group,"name");

						//临时变量是否会影响值???
						*allMobGroup = (char*) group_name;//因为此值是从内存中取得,所以有效

						//位置后移动一位
						allMobGroup ++;

						break;
					}
				}//遍历users
			}
		}//遍历组groups
	}

	end:
	if (root) {
		switch_xml_free(root);//必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return;
}

/*获得某个组的所用用户的信息,从root xml的结构中进行查找
*/
void CModule_User::getMobUsersOfGroupFromMem(const char *mobgroupname, 
											 char **allMobUsersInGroup)//输出参数 :某个组的所有用户信息
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, *userid;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users = NULL,x_user = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(mobgroupname)) {
		goto end;
	}

	//域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//先查找domain,因为分配了内存,必须要释放
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//根据xml文件,分级查找对应的用户domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			iMaxLen = CARICCP_MAX_STR_LEN(mobgroupname, group_name);

			//查找到对应的组,其他组不处理
			if (strncasecmp(mobgroupname, group_name, iMaxLen)) {
				continue;
			}

			//查找users节点
			if ((x_users = switch_xml_child(x_group, "users"))) {
				//遍历每个用户
				x_user = switch_xml_child(x_users, "user");
				for (; x_user; x_user = x_user->next) {
					userid = switch_xml_attr(x_user, "id");//实际上从内存中获得
					if (switch_strlen_zero(userid)) {
						continue;
					}

					//保存下来用户的id号
					*allMobUsersInGroup = (char*) userid;

					//位置后移动一位
					allMobUsersInGroup ++;
				}//遍历users
			}
		}//遍历组groups
	}


end:
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return;
}

/*获得所有组的信息,备注:default组不考虑
*/
void CModule_User::getAllGroupFromMem(string &allGroups,//输出参数
									  bool bshowSeq)//返回的记录是否要带序号
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, /**group_id,*/ *group_desc;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;

	int iMaxLen = 0;
	int groupNum = 0;
	int iRecordCout = 0;//记录数量
	string strSeq,strRecord;

	//域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//先查找domain,因为分配了内存,必须要释放
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//根据xml文件,分级查找对应的用户domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="g1" desc ="">
			group_name = switch_xml_attr(x_group, "name");
			//group_id = switch_xml_attr(x_group, "id");
			group_desc = switch_xml_attr(x_group, "desc");

			iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, group_name);

			//查找到对应的组,default组除外
			if (!strncasecmp(DEFAULT_GROUP_NAME, group_name, iMaxLen)) {
				continue;
			}

			//非NULL校验
			if (group_name) {
				/* *allGroups = (char *)group_name; */

				char *gInfo = NULL;
				//封装组的信息
				if (group_desc) {
					gInfo/**allGroups*/ = switch_mprintf("%s%s%s", group_name, CARICCP_SPECIAL_SPLIT_FLAG, /*group_id, CARICCP_SPECIAL_SPLIT_FLAG,*/ group_desc);
				} else {
				  	gInfo = switch_mprintf("%s%s%s", group_name, CARICCP_SPECIAL_SPLIT_FLAG, /*group_id, CARICCP_SPECIAL_SPLIT_FLAG,*/ "");
				}

				//mod by xxl 2010-5-27 :增加序号的显示
				if (bshowSeq){
					iRecordCout++;//序号递增
					strRecord = "";
					strSeq = intToString(iRecordCout);
					allGroups.append(strSeq);
					allGroups.append(CARICCP_SPECIAL_SPLIT_FLAG);
				}
				//mod by xxl 2010-5-27 end

				allGroups.append(gInfo);
				allGroups.append(CARICCP_ENTER_KEY);
				switch_safe_free(gInfo);//及时释放内存

				//组的数量
				groupNum++;
			}
		}//遍历组groups
	}

end:
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return;
}

/*获得当前域内组的数量.备注:default组的数量要考虑
*/
int CModule_User::getGroupNumFromMem()
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL;

	int iMaxLen = 0;
	int groupNum = 0;

	//域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//先查找domain,因为分配了内存,必须要释放
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//根据xml文件,分级查找对应的用户domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="g1" desc ="">
			group_name = switch_xml_attr(x_group, "name");

			////查找到对应的组,default组除外
			//iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME,group_name);
			//if (!strncasecmp(DEFAULT_GROUP_NAME,group_name,iMaxLen))
			//{
			//	continue;
			//}

			//非NULL校验
			if (group_name) {
				//组的数量
				groupNum++;
			}
		}//遍历组groups
	}

	end:
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return groupNum;
}

/*获得组的信息,从内存中获得
*/
switch_xml_t CModule_User::getGroupXMLInfoFromMem(const char *groupName, 
												  switch_xml_t groupXMLinfo, //输出参数 :组的xml信息
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

	//域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		err = getValueOfDefinedVar("INVALID_DOMOMAIN");
		goto end;
	}

	//先查找domain,因为分配了内存,必须要释放
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		err = getValueOfDefinedVar("DOMOMAIN_NO_EXIST");//域不存在
		goto end;
	}

	//根据xml文件,分级查找对应的用户domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			//只需要考虑default组的情况,包含了所有的用户信息
			iMaxLen = CARICCP_MAX_STR_LEN(groupName, group_name);

			if (!strncasecmp(groupName, group_name, iMaxLen)) {
				groupXMLinfo = x_group;

				break;
			}

			continue;
		}//遍历组groups
	}

end:
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return groupXMLinfo;
}

/*获得用户的xml信息
*/
switch_xml_t CModule_User::getMobUserXMLInfoFromMem(const char *mobuserid, 
													switch_xml_t userXMLinfo) //输出参数 :用户的xml信息
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *params = NULL;

	const char *domain_name, *group_name, *userid;
	switch_xml_t root = NULL,x_domain = NULL,x_groups = NULL,x_group = NULL,x_users = NULL,x_user = NULL;

	int iMaxLen = 0;

	if (switch_strlen_zero(mobuserid)) {
		goto end;
	}

	//域名
	domain_name = getDomainIP();
	if (switch_strlen_zero(domain_name)) {
		goto end;
	}

	//先查找domain,因为分配了内存,必须要释放
	status = switch_xml_locate_domain(domain_name, params, &root, &x_domain);
	if (SWITCH_STATUS_SUCCESS != status) {
		goto end;
	}

	//根据xml文件,分级查找对应的用户domain-->groups-->group-->users--->user
	if ((x_groups = switch_xml_child(x_domain, "groups"))) {
		x_group = switch_xml_child(x_groups, "group");
		for (; x_group; x_group = x_group->next) {
			//查找到对应的组,格式 :<group name="default">
			group_name = switch_xml_attr(x_group, "name");

			//只需要考虑default组的情况,包含了所有的用户信息
			iMaxLen = CARICCP_MAX_STR_LEN(DEFAULT_GROUP_NAME, group_name);

			//特殊的默认default的组不需要考虑
			if (!strncasecmp(DEFAULT_GROUP_NAME, group_name, iMaxLen)) {
				//查找users节点
				if ((x_users = switch_xml_child(x_group, "users"))) {
					//遍历每个用户
					x_user = switch_xml_child(x_users, "user");
					for (; x_user; x_user = x_user->next) {
						userid = switch_xml_attr(x_user, "id");

						//存在此用户
						if (!strcasecmp(userid, mobuserid)) {
							userXMLinfo = x_user;

							break;
						}
					}//遍历users
				}

				break;
			}

			continue;
		}//遍历组groups
	}

end:
	if (root) {
		switch_xml_free(root);//因为调用了switch_xml_locate_domain函数,必须要释放(释放线程锁???),否则重加载的时候会阻塞
		root = NULL;
	}

	return userXMLinfo;
}

/*从某个组中获得某个用户的信息
*/
switch_xml_t CModule_User::getMobUserXMLInfoInGroup(const char *mobuserid, //用户号
													switch_xml_t groupXMLinfo, //组的xml信息
													switch_xml_t userXMLinfo)  //输出参数 :用户的xml信息
{
	switch_xml_t x_users = NULL, x_user = NULL;
	const char *userid;

	//查找users节点
	if ((x_users = switch_xml_child(groupXMLinfo, "users"))) {
		//遍历每个用户
		x_user = switch_xml_child(x_users, "user");
		for (; x_user; x_user = x_user->next) {
			userid = switch_xml_attr(x_user, "id");
			if (switch_strlen_zero(userid)) {
				continue;
			}
			//存在此用户
			if (!strcasecmp(userid, mobuserid)) {
				userXMLinfo = x_user;

				break;
			}
		}//遍历users
	}

	return userXMLinfo;
}

/*获得default目录下用户默认的xml结构,从xml文件中直接获得
 *返回的xml结构必须释放,由调用的函数负责
*/
switch_xml_t CModule_User::getMobDefaultXMLInfo(const char *mobuserid)
{
	switch_xml_t x_user = NULL;
	char *user_default_xmlfile = NULL,*file = NULL;

	//修改default目录下对应的用户号的xml文件的组的属性
	//创建(O_CREAT)新的MOB用户文件
	file = getMobUserDefaultXMLPath();
	if (file) {
		user_default_xmlfile = switch_mprintf("%s%s%s", file, mobuserid, CARICCP_XML_FILE_TYPE);
		switch_safe_free(file);
	}
	if (!user_default_xmlfile) {
		goto end;
	}

	//此结构必须要释放,由调用函数负责
	x_user = cari_common_parseXmlFromFile(user_default_xmlfile);
	switch_safe_free(user_default_xmlfile);

end : 
	return x_user;
}

/*从文件中获得所用调度员的xml信息
 *返回的xml结构要释放.
*/
switch_xml_t CModule_User::getAllDispatchersXMLInfo()
{
	switch_xml_t x_dispatusers = NULL;
	char *file = getDispatFile();
	if (file) {
		//此结构必须要释放,由调用函数负责
		x_dispatusers = cari_common_parseXmlFromFile(file);
		switch_safe_free(file);//释放
	}
	return x_dispatusers;
}

/*获得一个调度人员的xml结构
*/
switch_xml_t CModule_User::getDispatcherXMLInfo(const char* dispatcherid,
												switch_xml_t x_all)
{
	switch_xml_t x_dispatusers = NULL, x_dispatuser = NULL;
	const char *userid = NULL;

	//遍历每个调度人员
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
		//存在此调度人员
		if (!strcasecmp(userid, dispatcherid)) {
			goto end;
		}
	}//遍历users

end:
	return x_dispatuser;
}

/*获得所用的调度员信息
*/
void CModule_User::getAllDispatchers(string &allDispatchers,
									 bool bshowSeq)//是否要显示序号
{
	switch_xml_t x_all = NULL, x_dispatusers = NULL, x_dispatuser = NULL;
	const char *userid = NULL;
	string strSeq;
	int iRecordCout = 0;

	//此结构要释放
	x_all = getAllDispatchersXMLInfo();
	if (!x_all){
		return;
	}
	x_dispatusers = switch_xml_child(x_all, "dispatchers");
	if (!x_dispatusers){
		return;
	}

	//遍历每个调度人员
	x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
	for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
		userid = switch_xml_attr(x_dispatuser, "id");
		if (switch_strlen_zero(userid)) {
			continue;
		}

		//mod by xxl 2010-5-27 :增加序号的显示
		if (bshowSeq){
			iRecordCout++;//序号递增
			strSeq = intToString(iRecordCout);
			allDispatchers.append(strSeq);
			allDispatchers.append(CARICCP_SPECIAL_SPLIT_FLAG);
		}
		//mod by xxl 2010-5-27 end

		//存在此调度人员
		allDispatchers.append(userid);
		allDispatchers.append(CARICCP_ENTER_KEY);

	}//遍历users

	//释放xml结构
	switch_xml_free(x_all);
}

/*是否存在调度员
*/
bool CModule_User::isExistedDispatcher(const char* dispatcherid)
{
	switch_xml_t x_dispatuser = NULL,x_dispatusers = NULL,x_all = NULL;
	const char *userid = NULL;
	bool blexist = false;

	//此结构要释放
	x_all = getAllDispatchersXMLInfo();
	if (!x_all) {
		goto end;
	}

	//查找是否存在此调度人员
	if ((x_dispatusers = switch_xml_child(x_all, "dispatchers"))) {
		//遍历每个调度人员
		x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
		for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
			userid = switch_xml_attr(x_dispatuser, "id");
			if (switch_strlen_zero(userid)) {
				continue;
			}
			//存在此调度人员
			if (!strcasecmp(userid, dispatcherid)) {
				blexist = true;
				goto end;
			}
		}//遍历users
	}

end : 
	//释放xml结构
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

	//查找是否存在此调度人员
	if ((x_dispatusers = switch_xml_child(x_all, "dispatchers"))) {
		//遍历每个调度人员
		x_dispatuser = switch_xml_child(x_dispatusers, "dispatcher");
		for (; x_dispatuser; x_dispatuser = x_dispatuser->next) {
			userid = switch_xml_attr(x_dispatuser, "id");
			if (switch_strlen_zero(userid)) {
				continue;
			}
			//存在此调度人员
			if (!strcasecmp(userid, dispatcherid)) {
				blexist = true;
				goto end;
			}
		}//遍历users
	}

end : 
	return blexist;

}

/*获得用户的详细信息,已经是封装好的报文信息
*/
void CModule_User::getSingleMobInfo(switch_xml_t userXMLinfo,  string &strOut) //输出参数 :用户的信息
{
	switch_xml_t x_variables, x_variable, x_params, x_param;
	//用户xml文件的详细信息
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

	//封装的信息应该和配置文件的相一致
	//userid  domain  groups  allows context accountcode
	char *usrInfo = "", *var, *val;
	char *infoarray[4] = {//其他4个字段
		"","","",""
	}; //只需要查看当前用户的3个属性即可
	char **tmpInfo = infoarray; //赋予首地址,此种方法不可靠,如果缺少某个字段,将导致计算错误!!!
	int i = 0,ilength = sizeof(infoarray) / sizeof(infoarray[0]);

	/*权限的描述信息
	*/
	string chPriDesc[USERPRIORITY_ONLY_DISPATCH + 1] = {
		"",                                                    //无效描述
		getValueOfDefinedVar("MOBUSER_PRIORITY_INTERNATIONAL"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_DOMESTIC"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_OUT"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_LOCAL"),
		getValueOfDefinedVar("MOBUSER_PRIORITY_ONLY_DISPATCH")
	};

	if (userXMLinfo) {
		//首先需要用户号
		usrInfo = (char*) switch_xml_attr_soft(userXMLinfo, "id");//根据id获得对应的用户号

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
					//mod by xxl 2010-6-24 :不再使用toll_allow字段
					//需要特殊转换一下:
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
					//tmpInfo ++;//是否会造成数组越界???

					infoarray[3] = val;
					continue;
				}
			}
		}

		//封装完整的信息
		//tmpInfo = infoarray;//重新赋初始位置
		//下面的方法不可靠,会出现0xcccccccc(tmpInfo)内存访问问题
		//if (tmpInfo)
		//{
		//	while(*tmpInfo)
		//	{
		//		usrInfo = switch_mprintf("%s%s%s",usrInfo,CARICCP_SPECIAL_SPLIT_FLAG,*tmpInfo);

		//		tmpInfo++;
		//	}
		//}

		//封装查询的用户信息
		strOut.append(usrInfo);
		for (; i < ilength; i++) {
			strOut.append(CARICCP_SPECIAL_SPLIT_FLAG);
			if (NULL != infoarray[i]){
				strOut.append(infoarray[i]);
			}
		}

		//输出参数
		//*info = usrInfo;
	}
}

/*获得转呼信息
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

	//解析简洁的格式
	vecTransferID.clear();
	splitString(transferinfo,CARICCP_SEMICOLON_MARK,vecTransferID);//以";"为分割符???

	//"多种"转呼号处理,目前只考虑一种情况
	for (vector<string>::iterator it = vecTransferID.begin(); vecTransferID.end() != it; it++){
		string strID = *it;
		vec.clear();
		strRes="";

		splitString(strID,"/",vec);

		strType    = getValueOfVec(0,vec);//转呼号的区域类型

		//如果是"出局类型","忙转"和"夜转"统一处理
		if (isEqualStr("gateway",strType)){//
			strGateway       = getValueOfVec(1,vec);//目前默认是多转呼号属于同一个网关
			strID            = getValueOfVec(2,vec);
			strTransUserType = getValueOfDefinedVar("ROUTER_OUT");
			strTransUserType.append("(");
			strTransUserType.append(getValueOfDefinedVar("ROUTER_GATEWAY"));
			strTransUserType.append(":");
			strTransUserType.append(strGateway);
			strTransUserType.append(")");
		}
		//如果是"本局类型","忙转"和"夜转"则根据idIndex区分处理
		else{
			strGateway       = "";
			strID            = getValueOfVec(idIndex,vec);//索引
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

/*从xml文件中获得mob user的某项属性,
*从default的文件目录中用户的xml文件variables子节点中获得用户的某些属性值
*/
char * CModule_User::getMobUserVarAttr(switch_xml_t userXMLinfo, 
									   const char *attName,
									   bool bContainedUserNode) //是否还包含user节,false:需要进一步从user节点提取xml结构
									                            //                 true:直接使用
{
	switch_xml_t x_user, x_variables, x_var;
	char *attVal = NULL;

	//从user子节点开始
	if (!bContainedUserNode){//如果从xml文件中直接查找获得,则需要此步骤
		x_user = switch_xml_child(userXMLinfo, "user");//提取user的xml结构
	}
	else{//此时userXMLinfo就是user子节点xml结构
		x_user = userXMLinfo;
	}

	if (!x_user){
		return NULL;	
	}

	//主要是从"variables"节点获得属性
	/*if (x_user = switch_xml_child(userXMLinfo, "user")) {*/
		if ((x_variables = switch_xml_child(x_user, "variables"))) {
			for (x_var = switch_xml_child(x_variables, "variable"); x_var; x_var = x_var->next) {
				const char *var = switch_xml_attr(x_var, "name");
				const char *val = switch_xml_attr(x_var, "value");

				//如果查找到相同的属性
				if (!strcasecmp(attName, var) && val) {
					attVal = (char*) val;
					return attVal;
				}
			}
		}
	/*}*/

	return NULL;
}

/*根据xml文件进行设置或修改mob user的params属性,主要是针对default/目录下的默认配置文件进行修改
*如果属性不存在,则可添加
*/
bool CModule_User::setMobUserParamAttr(switch_xml_t userXMLinfo, //xml node
									   const char *attName, //属性名
									   const char *attVal, //属性值
									   bool bAddattr)  	//如果属性不存在是否需要增加,默认为不增加
{
	bool isExistAttr = false;
	switch_xml_t x_user, x_params, x_param, x_newChild;
	int iPos = 0;
	char *newVariableContext = "";

	//从user子节点开始
	if (x_user = switch_xml_child(userXMLinfo, "user")) {
		if ((x_params = switch_xml_child(x_user, "params"))) {
			for (x_param = switch_xml_child(x_params, "param"); x_param; x_param = x_param->next) {
				iPos++;
				const char *var = switch_xml_attr(x_param, "name");
				const char *val = switch_xml_attr(x_param, "value");

				//如果查找到相同的属性
				if (!strcasecmp(attName, var) && val) {
					//替换以前的属性值
					//设置名-值对,如: <variable name="attName" value="g2" /> 
					switch_xml_set_attr(x_param, "value", attVal);
					isExistAttr = true;
					break;
				}
			}

			//如果属性子节点不存在,并运行新增加,则如下处理
			if (!isExistAttr && bAddattr) {
				newVariableContext = switch_mprintf(MOB_USER_XML_PARAM, attName, attVal);
				
				//需要释放
				x_newChild = switch_xml_parse_str(newVariableContext, strlen(newVariableContext));
				if (!x_newChild) {
					return false;
				}
				if (!switch_xml_insert(x_newChild, x_params, INSERT_INTOXML_POS)){//放置到最后
					return false;
				}
				
				//此处的内存是和xml结果绑定的,不能释放,否则会造成x_newChild结构异常
				//应该考虑释放xml的结构,由调用函数负责
				//switch_safe_free(newVariableContext);
			}
		}
	}

	return true;
}

/*设置或修改mob user的variables属性,主要是针对default/目录下的默认配置文件进行修改
*如果属性不存在,则可添加
*/
bool CModule_User::setMobUserVarAttr(switch_xml_t userXMLinfo, //xml node 
									 const char *attName,      //属性名
									 const char *attVal,       //属性值 
									 bool bAddattr)	           //如果属性不存在是否需要增加,默认为不增加false
{
	bool isExistAttr = false;
	switch_xml_t x_user, x_variables, x_var, x_newChild;
	int iPos = 0;
	char *newVariableContext = "";

	//从user子节点开始
	if (x_user = switch_xml_child(userXMLinfo, "user")) {
		if ((x_variables = switch_xml_child(x_user, "variables"))) {
			for (x_var = switch_xml_child(x_variables, "variable"); x_var; x_var = x_var->next) {
				iPos++;
				const char *var = switch_xml_attr(x_var, "name");
				const char *val = switch_xml_attr(x_var, "value");

				//如果查找到相同的属性
				if (!strcasecmp(attName, var) && val) {
					//替换以前的属性值
					//设置名-值对,如: <variable name="attName" value="g2" /> 
					switch_xml_set_attr(x_var, "value", attVal);

					isExistAttr = true;
					break;
				}
			}

			//如果属性子节点不存在,并运行新增加,则如下处理
			if (!isExistAttr && bAddattr) {
				newVariableContext = switch_mprintf(MOB_USER_XML_VARATTR, attName, attVal);

				//需要释放
				x_newChild = switch_xml_parse_str(newVariableContext, strlen(newVariableContext));
				if (!x_newChild) {
					return false;
				}

				if (!switch_xml_insert(x_newChild, x_variables, INSERT_INTOXML_POS)){//放置到最后
					return false;
				}
				//此处的内存是和xml结果绑定的,不能释放,否则会造成x_newChild结构异常
				//应该考虑释放xml的结构,由调用函数负责
				//switch_safe_free(newVariableContext);
			}
		}
	}

	return true;
}

/*设置调度员属性
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
			
			//需要释放
			x_newChild = switch_xml_parse_str(newVariableContext, strlen(newVariableContext));
			if (!x_newChild) {
				return false;
			}
			//添加到此结构中
			if (!switch_xml_insert(x_newChild, x_dispatchers, INSERT_INTOXML_POS)){//放置到最后
				return false;
			}
			//此处的内存是和xml结果绑定的,不能释放,否则会造成x_newChild结构异常
			//switch_safe_free(newVariableContext);
		}
	}
	//	else{
	//		//从dispatchers子节点开始
	//		if (x_dispatchers = switch_xml_child(dispathcersXml, "dispatchers")) {
	//			for (x_dispatcher = switch_xml_child(x_dispatchers, "dispatcher"); x_dispatcher; x_dispatcher = x_dispatcher->next) {
	//				const char *varid = switch_xml_attr(x_dispatcher, "id");

	//				//如果查找到相同的属性
	//				if (!strcasecmp(attVal, varid)) {
	//					//删除掉属性值
	//					switch_xml_set_attr(x_dispatcher, "value", attVal);
	//					isExistAttr = true;
	//					break;
	//				}
	//			}

	//	}
	//}

	return true;
}

/*重写mob user的xml文件内容
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
		err = getValueOfDefinedVar("XMLFILE_TO_STR_ERROR");//文件转换有误
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
	
	err = "operation sucess.";//操作成功
	return bRes;
}
