#include "CModule_NEManager.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"

#define MIN_NEID                    1
#define MAX_NEID                    1000

//不同类型的设备名称
#define  CCP                        "CCP"     //cari core platform
#define  SW                         "SW"      //switch
#define  AP                         "AP"      //access point
#define  GATEWAY                    "GATEWAY" //gateway


//网元简单的xml内容
#define NE_XML_SIMPLE_CONTEXT 		"<ne id=\"%s\" type = \"%s\" ip = \"%s\" desc = \"%s\"/>"
#define OPUSER_XML_SIMPLE_CONTEXT 	"<opuser name=\"%s\" pwd = \"%s\" priority = \"%s\"/>"
#define SUPER_USER                  "admin"


CModule_NEManager::CModule_NEManager()
	: CBaseModule()
{
	const char *errmsg[1]={ 0 };
	bool bRes = false;

	//ne的xml文件
	char *nefile = switch_mprintf("%s%s%s", 
								  SWITCH_GLOBAL_dirs.conf_dir, 
								  SWITCH_PATH_SEPARATOR, 
								  CARI_CCP_NE_XML_FILE
								  );
	//char *neContext = "<include></include>";
	//如果此文件不存在,则重新创建,并且只需要创建一次即可
	if (!cari_common_isExistedFile(nefile)){
		char *neContext = switch_mprintf("%s%s", 
			"<include>", 
			"</include>"
			);
		bRes = cari_common_creatXMLFile(nefile,neContext,errmsg);
		switch_safe_free(neContext);
	}
	if (!bRes){
		//打印日志???
	}

	//操作用户的xml文件
	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);
	//如果此文件不存在,则重新创建,并且只需要创建一次即可
	if (!cari_common_isExistedFile(opuserfile)){
		//创建初始登录的默认操作用户
		char *opuserContext = switch_mprintf("%s%s%s", 
			"<include>", 
			"<opuser name=\"admin\" pwd = \"123456\" priority = \"0\"/>", //用户操作的优先级从0开始,最大
			"</include>"
			);
		bRes = cari_common_creatXMLFile(opuserfile,opuserContext,errmsg);
		switch_safe_free(opuserContext);
	}
	if (!bRes){
		//打印日志???
	}

	//释放内存
	switch_safe_free(nefile);
	switch_safe_free(opuserfile);
}

CModule_NEManager::~CModule_NEManager()
{
}

/*根据接收到的帧,处理具体的命令,分发到具体的处理函数
*/
int CModule_NEManager::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
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
int CModule_NEManager::sendRespMsg(common_ResultResponse_Frame *&respFrame)
{
	int	iFunRes	= CARICCP_SUCCESS_STATE_CODE;
	return iFunRes;
}

int CModule_NEManager::cmdPro(const inner_CmdReq_Frame *&reqFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}

/*增加设备网元,需要通知其他注册的client
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

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知其他客户端
	switch_event_t *notifyInfoEvent      = NULL;
	int notifyCode                       = CARICCP_NOTIFY_ADD_EQUIP_NE;
	char *nefile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_NE_XML_FILE
		);
	switch_xml_t x_NEs=NULL,x_newNE=NULL;
	char *newNEContext=NULL,*nesContext=NULL;

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strID,strType,strIP,strDesc;
	string			strTmp;
	int				neid=0;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"neid"
	strParamName = "neid";//neid
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//neid号必须是有效数字
	if (!isNumber(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//范围限制
	neid = stringToInt(strValue.c_str());
	if (MIN_NEID>neid 
		|| MAX_NEID < neid){
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(),MIN_NEID,MAX_NEID);
		goto end_flag;
	}
	strID = strValue;

	//"NEType"
	strParamName = "netype";//网元类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue  = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//NEType类型是必选参数
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

	//ip地址
	strParamName = "ip";//ip
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//ip类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//地址必须有效 
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
	//判断此网元是否已经存在???
	if (isExistedNE(neid)){
		strReturnDesc = getValueOfDefinedVar("NE_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//开始增加网元////////////////////////////////////////////////////////////
	//将内容转换成xml的结构(注意:中文字符问题)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), nefile);
		goto end_flag;
	}

	//先构建xml的内存结构
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

	//将新增的网元的结构添加进来
	if (!switch_xml_insert(x_newNE, x_NEs, INSERT_INTOXML_POS)) {
		strReturnDesc = getValueOfDefinedVar("INNER_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//将全部网元的内存结构转换成字串
	nesContext = switch_xml_toxml(x_NEs, SWITCH_FALSE);
	if (!nesContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//重新创建网元的文件
	bRes = cari_common_creatXMLFile(nefile,nesContext,err);
	if (!bRes){
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("ADD_NE_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());

	/************************************************************************/
	/*                             发送通知                                 */
	/************************************************************************/
	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//此时还是按照成功处理
		goto end_flag;
	}
	//通知码
	notifyCode = CARICCP_NOTIFY_ADD_EQUIP_NE;
	//结果码和结果集
	pNotifyNode = switch_mprintf("%d", notifyCode);
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY,     "1");   		   //通知标识
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyNode);     //返回码=通知码
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "neid",	         strID.c_str());   //通知的结果数据
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "netype",           strType.c_str());
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "ip",	             strIP.c_str());
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "desc",	         strDesc.c_str());

	//发送通知----------------------
	cari_net_event_notify(notifyInfoEvent);

	//释放内存
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pNotifyNode);
	

/*-----*/
end_flag : 
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	//释放xml结构
	switch_xml_free(x_NEs);

	switch_safe_free(nefile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*删除网元,需要通知其他注册的client
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

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知其他客户端
	switch_event_t *notifyInfoEvent = NULL;
	int            notifyCode       = CARICCP_NOTIFY_DEL_EQUIP_NE;

	char           *nefile          = switch_mprintf("%s%s%s", 
													SWITCH_GLOBAL_dirs.conf_dir, 
													SWITCH_PATH_SEPARATOR, 
													CARI_CCP_NE_XML_FILE
									                );
	switch_xml_t   x_NEs,x_delNE;
	char           *newNEContext    =NULL,*nesContext=NULL;

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strID;
	string			strTmp;
	int             neid=0;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"neid"
	strParamName = "neid";//neid
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//id号必须是有效数字
	if (!isNumber(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	neid = stringToInt(strValue.c_str());
	strID = strValue;

	//网元是否存在
	if (!isExistedNE(neid)){
		strReturnDesc = getValueOfDefinedVar("NE_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//删除掉此网元的相关信息
	//将内容转换成xml的结构(注意:中文字符问题)
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
	//动态更改内存的结构
	switch_xml_remove(x_delNE);
	/************************************************************************/

	//重新写文件
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
		//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
		//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("DEL_NE_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());

	/************************************************************************/
	/*							  发送通知                                  */
	/************************************************************************/
	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//此时还是按照成功处理
		goto end_flag;
	}
	//通知码
	notifyCode = CARICCP_NOTIFY_DEL_EQUIP_NE;
	//结果码和结果集
	pNotifyCode = switch_mprintf("%d", notifyCode);
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY,     "1");   		 //通知标识
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode);   //返回码=通知码
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "neid",	         strID.c_str()); //通知的结果数据

	//发送通知----------------------
	cari_net_event_notify(notifyInfoEvent);

	//释放内存
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

/*修改网元操作,需要通知其他注册的client
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

	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知其他客户端
	switch_event_t *notifyInfoEvent = NULL;
	int            notifyCode       = CARICCP_NOTIFY_MOD_EQUIP_NE;

	char           *nefile          = switch_mprintf("%s%s%s",
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_NE_XML_FILE
		);

	switch_xml_t   x_NEs            = NULL,x_modNE= NULL;
	char           *newNEContext    = NULL,*nesContext=NULL;

	//涉及到参数
	string			strParamName,strValue,strVar,strChinaName;
	string			strID,strType,strIP,strDesc;
	string			strTmp;
	int				neid = 0;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"neid"
	strParamName = "neid";//neid
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//id号必须是有效数字
	if (!isNumber(strValue)){
		strReturnDesc = getValueOfDefinedVar("PARAM_NUMBER_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//范围限制
	neid = stringToInt(strValue.c_str());
	if (MIN_NEID>neid 
		|| MAX_NEID < neid){
		strReturnDesc = getValueOfDefinedVar("PARAM_RANGE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(),MIN_NEID,MAX_NEID);
		goto end_flag;
	}
	strID = strValue;

	//"NEType"
	strParamName = "netype";//网元类型
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//NEType类型是必选参数
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

	//ip地址
	strParamName = "ip";//ip
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//ip类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	//地址必须有效 
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
	//判断此网元是否已经存在???
	if (!isExistedNE(neid)){
		strReturnDesc = getValueOfDefinedVar("NE_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strID.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//开始修改网元的属性
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

	//设置名-值对,如: <ne id="1" type = "AP" ip = "127.0.0.1" desc = "ap1"/>
	//根据"网元类型"或"网元ip"或"描述信息"判断是否和以前的相同,另:id号不能进行修改
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


	//如果属性真正的进行了修改,则需要修改配置文件,如果修改的值和以前保存的值相同,此处直接返回成功信息即可
	//也不需要再进行广播通知
	if (bMod){
		//重新写文件
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
			//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
			//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

			goto end_flag;
		}
	}
	
	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("MOD_NE_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(),strID.c_str());

	/************************************************************************/
	/*							发送通知                                    */
	/************************************************************************/
	//涉及到通知类型的处理 :命令属于通知类型,需要广播通知所有注册的client
	if (bMod){
		//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
		if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
			//此时还是按照成功处理
			goto end_flag;
		}
		//通知码
		notifyCode = CARICCP_NOTIFY_MOD_EQUIP_NE;
		//结果码和结果集
		pNotifyCode = switch_mprintf("%d", notifyCode);
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY,     "1");   		  //通知标识
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, pNotifyCode);    //返回码=通知码
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "neid",             strID.c_str());  //通知的结果数据
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "netype",           strType.c_str());
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "ip",	             strIP.c_str());
		switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, "desc",	         strDesc.c_str());

		//发送通知----------------------
		cari_net_event_notify(notifyInfoEvent);

		//释放内存
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

/*查询设备NE的信息,主要查询对应的id号,类型,已经ip地址
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
	int iRecordCout = 0;//记录数量

	char *nefile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_NE_XML_FILE
		);

	switch_xml_t x_NEs=NULL,x_ne=NULL;
	char *newNEContext=NULL,*nesContext=NULL,*strTableName=NULL;

	//将网元的xml内容转换成xml的结构(注意:中文字符问题)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//遍历查找所有的网元
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

			//mod by xxl 2010-5-27 :增加序号的显示
			iRecordCout++;//序号递增
			strRecord = "";
			strSeq = intToString(iRecordCout);
			strRecord.append(strSeq);
			strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
			strRecord.append(singleRecord);

			//存放记录
			inner_RespFrame->m_vectTableValue.push_back(/*singleRecord*/strRecord);
			//mod by xxl 2010-5-27 end

			switch_safe_free(singleRecord);
		}
	}

	//当前记录为空
	if (0 ==inner_RespFrame->m_vectTableValue.size()){
		strReturnDesc = getValueOfDefinedVar("NO_NE_RECORD");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("NE_RECORD");

	//设置表头字段
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

/*增加操作用户
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

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strUserName,strPwd;
	string			strTmp;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"opusername"
	strParamName = "opusername";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//用户超长,返回错误信息
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
		//超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strPwd = strValue;

	//////////////////////////////////////////////////////////////////////////
	//判断此"操作用户"是否已经存在???
	if (isExistedOpUser(strUserName)){
		strReturnDesc = getValueOfDefinedVar("OPUSER_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());
		goto end_flag;
	}

	//////////////////////////////////////////////////////////////////////////
	//开始增加操作用户

	//将内容转换成xml的结构(注意:中文字符问题)
	x_opusers = cari_common_parseXmlFromFile(opuserfile);
	if (!x_opusers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), opuserfile);
		goto end_flag;
	}

	//先构建xml的内存结构
	newOpuserContext = OPUSER_XML_SIMPLE_CONTEXT;
	newOpuserContext = switch_mprintf(newOpuserContext,strUserName.c_str(), strPwd.c_str(),"0");//默认为超级用户
	x_newOpuser = switch_xml_parse_str(newOpuserContext, strlen(newOpuserContext));
	if (!x_newOpuser) {
		strReturnDesc = getValueOfDefinedVar("STR_TO_XML_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//将新增的操作用户的结构添加进来
	if (!switch_xml_insert(x_newOpuser, x_opusers, INSERT_INTOXML_POS)) {
		strReturnDesc = getValueOfDefinedVar("INNER_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//将全部操作用户的内存结构转换成字串
	opusersContext = switch_xml_toxml(x_opusers, SWITCH_FALSE);
	if (!opusersContext) {
		strReturnDesc = getValueOfDefinedVar("XML_TO_STR_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//重新创建网元的文件
	bRes = cari_common_creatXMLFile(opuserfile,opusersContext,err);
	if (!bRes){
		strReturnDesc = *err;
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("ADD_OPUSER_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());


	/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_opusers);//释放

	switch_safe_free(opuserfile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*删除操作用户
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

	//涉及到参数
	string			strParamName, strValue, strChinaName;
	string			strUserName;
	string			strTmp;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"opusername"
	strParamName = "opusername";//opusername
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strUserName = strValue;

	//判断此"操作用户"是否已经存在???
	if (!isExistedOpUser(strUserName)){
		strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());
		goto end_flag;
	}

	//特殊判断一下,如果登录用户相同,则不允许
	if (isEqualStr(strUserName,reqFrame->header.strLoginUserName)){
		strReturnDesc = getValueOfDefinedVar("DEL_OPUSER_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str());
		goto end_flag;
	}

	//如果为固定用户admin,则一定不能删除
	if (isEqualStr(strUserName,SUPER_USER)){
		strReturnDesc = getValueOfDefinedVar("DEL_ADMIN_OPUSER_FAILED");
		pDesc = switch_mprintf(strReturnDesc.c_str(),SUPER_USER);
		goto end_flag;
	}

	//删除掉此操作的相关信息
	//将内容转换成xml的结构(注意:中文字符问题)
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
	//动态更改内存的结构
	switch_xml_remove(x_deOpuser);
	/************************************************************************/

	//重新写文件
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
		//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
		//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("DEL_OPUSER_SUCCESS");
	pDesc = switch_mprintf(strReturnDesc.c_str(), strUserName.c_str());


	/*-----*/
end_flag :

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_opusers);//释放

	switch_safe_free(opuserfile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*修改操作用户
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

	//涉及到参数
	string			strParamName,strValue,strVar,strChinaName;
	string			strOldName,strNewName,strPwd;
	string			strTmp;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"oldopusername"
	strParamName = "oldopusername";//oldopusername
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//超长,返回错误信息
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
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//超长,返回错误信息
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
		//id类型是必选参数
		strReturnDesc = getValueOfDefinedVar("PARAM_NULL_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str());
		goto end_flag;
	}
	if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
		//超长,返回错误信息
		strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
		goto end_flag;
	}
	strPwd = strValue;


	//检查oldname是否存在
	if (!isExistedOpUser(strOldName)){
		strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
		pDesc = switch_mprintf(strReturnDesc.c_str(), strOldName.c_str());
		goto end_flag;
	}

	//oldname和newname不相同
	if (!isEqualStr(strOldName,strNewName)){
		if (isEqualStr(strOldName,SUPER_USER)){//admin用户,不能修改为其他用户
			strReturnDesc = getValueOfDefinedVar("MOD_SUPERUSER_FAILED_1");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strOldName.c_str());
			goto end_flag;
		}

		if (isEqualStr(strNewName,SUPER_USER)){//修改的新用户为admin,不允许
			strReturnDesc = getValueOfDefinedVar("MOD_SUPERUSER_FAILED_2");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strNewName.c_str(),strOldName.c_str());
			goto end_flag;
		}

		//检查newname是否存在
		if (isExistedOpUser(strNewName)){
			strReturnDesc = getValueOfDefinedVar("OPUSER_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strNewName.c_str());
			goto end_flag;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//开始修改操作用户的属性
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

	//设置名-值对,如: <opuser name=\"%s\" pwd = \"%s\" priority = \"%s\"/>
	switch_xml_set_attr(x_modOpuser, "name", strNewName.c_str());
	switch_xml_set_attr(x_modOpuser, "pwd", strPwd.c_str());

	//重新写文件
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
		//是否考虑恢复以前的模板结构!!!!!!!!!!!!!!!
		//如果出现错误,不一定会是写内容的时候出现,此处不再考虑回退问题.

		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;

	//描述信息再重新封装一下
	strReturnDesc = getValueOfDefinedVar("MOD_OPUSER_SUCCESS");
	pDesc = switch_mprintf("%s",strReturnDesc.c_str());


	/*-----*/
end_flag : 

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, pDesc, sizeof(inner_RespFrame->header.strResuleDesc));

	switch_xml_free(x_opusers);//释放

	switch_safe_free(opuserfile);
	switch_safe_free(pDesc);
	return iFunRes;
}

/*查询操作用户
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
	int iRecordCout = 0;//记录数量

	char *opuserfile = switch_mprintf("%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		CARI_CCP_OPUSER_XML_FILE
		);

	switch_xml_t x_opusers=NULL,x_opuser=NULL;
	char *newOpuserContext=NULL,*opusersContext=NULL,*strTableName=NULL;

	//////////////////////////////////////////////////////////////////////////
	//对参数进行解析
	//"opusername"
	strParamName = "opusername";
	strChinaName = getValueOfDefinedVar(strParamName);
	strValue = getValue(strParamName, reqFrame);
	trim(strValue);
	if (0 == strValue.size()) {
		bAllOrOne = true;
	}
	else{//查询有个具体的操作用户信息
		if (CARICCP_STRING_LENGTH_32 < strValue.length()) {
			//用户超长,返回错误信息
			strReturnDesc = getValueOfDefinedVar("PARAM_MAXLENGTH_ERROR");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strChinaName.c_str(), CARICCP_STRING_LENGTH_32);
			goto end_flag;
		}

		//判断此"操作用户"是否存在
		if (!isExistedOpUser(strValue)){
			strReturnDesc = getValueOfDefinedVar("OPUSER_NO_EXIST");
			pDesc = switch_mprintf(strReturnDesc.c_str(), strValue.c_str());
			goto end_flag;
		}

		bAllOrOne = false;
	}
	strUserName = strValue;


	//将网元的xml内容转换成xml的结构(注意:中文字符问题)
	x_opusers = cari_common_parseXmlFromFile(opuserfile);
	if (!x_opusers){
		strReturnDesc = getValueOfDefinedVar("PARSE_XML_FILE_ERROR");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//遍历查找所有的操作用户
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

			//mod by xxl 2010-5-27 :增加序号的显示
			iRecordCout++;//序号递增
			strRecord = "";
			strSeq = intToString(iRecordCout);
			strRecord.append(strSeq);
			strRecord.append(CARICCP_SPECIAL_SPLIT_FLAG);
			strRecord.append(singleRecord);

			//存放记录
			inner_RespFrame->m_vectTableValue.push_back(/*singleRecord*/strRecord);
			//mod by xxl 2010-5-27 end

			switch_safe_free(singleRecord);
		}
	}

	//当前记录为空
	if (0 ==inner_RespFrame->m_vectTableValue.size()){
		strReturnDesc = getValueOfDefinedVar("RECORD_NULL");
		pDesc = switch_mprintf("%s",strReturnDesc.c_str());
		goto end_flag;
	}

	//成功
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("OPUSER_RECORD");
	pDesc = switch_mprintf(strReturnDesc.c_str());

	//设置表头字段
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

	//释放xml结构
	switch_xml_free(x_opusers);

	switch_safe_free(pDesc);
	switch_safe_free(opuserfile);
	return iFunRes;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

/*根据id号进行判断,是否存在此网元
*/
bool CModule_NEManager::isExistedNE(int id)
{
	//临时方案:目前根据文件读取
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
		////重新创建此文件
		//cari_common_creatXMLFile(nefile,neContext,errmsg);

		goto end;
	}

	//将内容转换成xml的结构(注意:中文字符问题)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//遍历查找
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

/*网元是否存在,根据ip地址进行判断
*/
bool CModule_NEManager::isExistedNE(string ip)
{
	//临时方案:目前根据文件读取
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
		////重新创建此文件
		//cari_common_creatXMLFile(nefile,neContext,errmsg);

		goto end;
	}

	//将内容转换成xml的结构(注意:中文字符问题)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//遍历查找
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


/*是否存在此操作用户
*/
bool CModule_NEManager::isExistedOpUser(string username)
{
	//临时方案:目前根据文件读取
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

	//将内容转换成xml的结构(注意:中文字符问题)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//遍历查找
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
