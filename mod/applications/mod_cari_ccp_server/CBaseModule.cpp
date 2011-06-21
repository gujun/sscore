//#include "modules\CBaseModule.h"
#include "CBaseModule.h"
#include "CMsgProcess.h"
#include "cari_net_event_process.h"
#include "cari_net_socket.h"

//////////////////////////////////////////////////////////////////////////
/*重启服务(主机)的线程的主体函数
*/
void* thread_reboot(void *attr)
{
	string strRebootCmd = "";
	bool bServerOrHost = true;
	char *chReboot = "0";//默认为重启服务进程
	if (NULL != attr){
		chReboot = (char *)attr;
	}

	//重启服务器主机
	if (isEqualStr("1",chReboot)){
#if defined(_WIN32) || defined(WIN32)
		strRebootCmd = "shutdown  /r";
#else
		strRebootCmd = "reboot";//当前linux系统不支持shutdown命令(shutdown -rt 10)
#endif
	}
	else{//重启服务进程
#if defined(_WIN32) || defined(WIN32)
		//strRebootCmd = "shutdown  /r";
#else
		strRebootCmd = "/sbin/service softswitch restart";//自己配置的重启服务进程命令
#endif
	}

	//负责关闭所有的socket连接
	shutdownSocketAndGarbage();

//重启服务进程或主机的操作
#if defined(_WIN32) || defined(WIN32)
	cari_common_excuteSysCmd(strRebootCmd.c_str());
#else
	CARI_SLEEP(3*1000);//等待一会
	for (int i=0;i<3;i++){
		cari_common_excuteSysCmd("sync");//加快数据更新,将内存中的数据写入到磁盘中,建议如此使用
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
	//释放内存
	switch_safe_free(m_localIP);
}

/*基类模块主要负责处理内部命令
*/
int CBaseModule::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	if (NULL == inner_reqFrame) {
		//帧为空
		return CARICCP_ERROR_STATE_CODE;
	}

	//请求帧的"源模块"号则是响应帧的"目的模块"号
	//请求帧的"目的模块"号则是响应帧的"源模块"号,可逆的
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;

	int iRes = CARICCP_SUCCESS_STATE_CODE;

	//判断命令码
	int iCmdCode = inner_reqFrame->body.iCmdCode;
	switch (iCmdCode) {
	case CARICCP_REBOOT://重启
		reboot(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_LOGIN://用户登录
		iRes = login(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_CLIENTID://查询client id号
		queryClientID(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_SHAKEHAND://握手
		shakehand(inner_reqFrame, inner_RespFrame);
		break;

	default:
		break;
	};

	//命令执行不成功
	if (CARICCP_SUCCESS_STATE_CODE != iRes) {
		inner_RespFrame->header.bNotify = false;
	} else {
		inner_RespFrame->header.bNotify = inner_reqFrame->body.bNotify;
	}

	//打印日志
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
/*具体的针对每条命令的处理函数                                          */
/************************************************************************/

/**
/*登录命令
*/
int CBaseModule::login(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)//输出参数,响应帧
{
	int iRes = CARICCP_ERROR_STATE_CODE;

	//如果此命令又涉及到其他子模块的处理,如何??????
	//当成内部命令---------------------------------

	string strParamName, strValue,strDec,strChinaName;
	char* chDec = NULL;

	//获取用户名和用户的密码
	string strUserName,strPwd;
	string strTmp;
	strParamName = "loginName";//用户名
	strChinaName = getValueOfDefinedVar(strParamName);
	strUserName = getValue(strParamName, inner_reqFrame);
	if (0 == strUserName.size()) {
		//用户名字为空,返回错误信息
		strDec = getValueOfDefinedVar("PARAM_NULL_ERROR");
		chDec = switch_mprintf(strDec.c_str(), strChinaName.c_str());
		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;

		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

		switch_safe_free(chDec);
		goto end;
	}

	strParamName = "loginPwd";//用户密码
	strChinaName = getValueOfDefinedVar(strParamName);
	strPwd = getValue(strParamName, inner_reqFrame);
	if (0 == strPwd.length()) {
		//用户密码,返回错误信息
		strDec = getValueOfDefinedVar("PARAM_NULL_ERROR");
		chDec = switch_mprintf(strDec.c_str(), strChinaName.c_str());//switch_mprintf(str_NULL_Error.c_str(),strName)出错，为什么????????

		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;
		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

		switch_safe_free(chDec);
		goto end;
	}

	//mod by xxl 2010-5-19 :增强对操作用户的处理问题
	//检查此用户是否存在
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

	////分配惟一的客户端号给用户
	//string strVal = "";
	//strVal = "ClientID = 2009";
	//inner_RespFrame->m_vectTableValue.push_back(strVal);

end:
	return iRes;
}

/*查询获得对应的client id号,如果是多个客户端同时重连,导致对容器有增有删,如果容器没有加锁,则会出现异常
*/
int CBaseModule::queryClientID(inner_CmdReq_Frame*& inner_reqFrame, 
							   inner_ResultResponse_Frame*& inner_RespFrame)
{
	//在建立客户端线程的时候,已经分配好了对应的id号,此时直接取出即可
	int iRes = CARICCP_SUCCESS_STATE_CODE;
	u_int iSocketID = inner_reqFrame->socketClient;
    u_short iClientID = CARICCP_MAX_SHORT;

	//先校验一下此id是否有效,必须根据请求帧
	u_short id = inner_reqFrame->header.sClientID;
	if (CARICCP_MAX_SHORT == id){//初次登录并开始查询
		iClientID = CModuleManager::getInstance()->assignClientID();//重新分配id号
	}
	else{//查看此用户号是否能继续使用
		bool bUsed = true;
		bUsed = CModuleManager::getInstance()->isUsedID(id);
		if (bUsed){//已经被其他的客户端使用
			iClientID = CModuleManager::getInstance()->assignClientID();//重新分配id号
		}
		else{
			iClientID = id;//继续使用以前分配的id号
			//同时在容器中需要清除掉此号码
			CModuleManager::getInstance()->removeClientID(iClientID);
		}
	}

	//开始分配此客户端对应的id号,另外,重登录的时候,可以校验以前使用的id是否还有效
	socket3element	element;
	CModuleManager::getInstance()->getSocket(iSocketID,element);

	element.sClientID = iClientID;//保存此客户端号

	//先删除此元素
	CModuleManager::getInstance()->eraseSocket(iSocketID);

	//然后再重新增加此元素
	CModuleManager::getInstance()->saveSocket(element);

	//关键信息的设置
	inner_RespFrame->header.sClientID = iClientID;
	return iRes;
}

/*重新启动服务器
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

	//涉及到通知类型的处理:命令属于通知类型,需要广播通知其他客户端
	switch_event_t *notifyInfoEvent = NULL;


	string strModel,strParamName,strChinaName;
	string strTmp;

	//model
	strParamName = "model";//model
	strChinaName = getValueOfDefinedVar(strParamName);
	strModel = getValue(strParamName, inner_reqFrame);
	if (0 == strModel.size()) {
		//模式为空,返回错误信息
		strDec = getValueOfDefinedVar("PARAM_NULL_ERROR");
		chDec = switch_mprintf(strDec.c_str(), strChinaName.c_str());
		inner_RespFrame->header.iResultCode = CARICCP_ERROR_STATE_CODE;

		myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

		switch_safe_free(chDec);
		goto end_flag;
	}
	if (isEqualStr("host",strModel)){
		chReboot = "1";
		//描述信息
		strDec = getValueOfDefinedVar("HOST_REBOOT");
	}
	else{
		chReboot = "0";
		strDec = getValueOfDefinedVar("SERVER_REBOOT");
	}

	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&notifyInfoEvent, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&notifyInfoEvent, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		//此时还是按照成功处理
		goto end_flag;
	}

	//通知码
	notifyCode = CARICCP_NOTIFY_REBOOT;

	//结果码和结果集
	pChNotifyNode = switch_mprintf("%d", notifyCode);
	strNotifyCode = pChNotifyNode;
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, "1");   					//通知标识
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RETURNCODE, strNotifyCode.c_str()); //返回码=通知码
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CLIENT_ID,          strClientID.c_str()); //客户端号
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_CMDCODE,    strCode.c_str());     //
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_CMDNAME,    strCmdName.c_str());
	switch_event_add_header_string(notifyInfoEvent, SWITCH_STACK_BOTTOM, CARICCP_RESULTDESC, strDec.c_str());	   //描述信息

	//发送通知----------------------
	cari_net_event_notify(notifyInfoEvent);

	//释放内存
	switch_event_destroy(&notifyInfoEvent);
	switch_safe_free(pChNotifyNode);

	//为了返回给客户端能显示特殊颜色,此处的返回码不为0,为CARICCP_NOTIFY_REBOOT
	inner_RespFrame->header.iResultCode = CARICCP_NOTIFY_REBOOT;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strDec.c_str(), strlen(strDec.c_str()));//返回的描述信息


end_flag:

	//然后再重新启动一个线程来处理重启,因为本命令返回的结果还没有广播给所有的client,服务器就开始重启了
	//可以限制重启时间间隔,保证结果能返回即可.
	pthread_t pth;
	int iRet= pthread_create(&pth,NULL,thread_reboot,(void *)chReboot);
	if(CARICCP_SUCCESS_STATE_CODE != iRet){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Creat reboot thred failed!");
	} 

//#if defined(_WIN32) || defined(WIN32)
//	cari_common_excuteSysCmd("shutdown  /r");
//#else
//	for (int i=0;i<3;i++){
//		cari_common_excuteSysCmd("sync");//加快数据更新,将内存中的数据写入到磁盘中,建议如此使用
//	}
//	cari_common_excuteSysCmd("reboot");//当前linux系统不支持shutdown命令(shutdown -rt 10)
//#endif

	return iRes;
}

/*握手
*/
int CBaseModule::shakehand(inner_CmdReq_Frame*& inner_reqFrame, 
						   inner_ResultResponse_Frame*& inner_RespFrame)
{
	int iRes = CARICCP_SUCCESS_STATE_CODE;
	inner_RespFrame->header.iResultCode = CARICCP_SUCCESS_STATE_CODE;

	return iRes;
}

/************************************************************************/
/* 提供通用的处理方法                                                   */
/************************************************************************/

/*获得对应的domain名,从内存root结构中查找下面的字段
*<list name="domains" default="deny">
*<node type="allow" domain="192.168.17.123"/>
*</list>
*
*或者使用internal profile的配置项
*<param name="force-register-domain" value="192.168.17.123"/>
*/
char * CBaseModule::getDomainIP()
{
	if (NULL != m_localIP){
		return m_localIP;
	}

	//	char *profile_name = CARICCP_SIP_PROFILES_INTERNAL; //根据此internal.xml文件的结构设置
	//	char *domain_name = NULL;//"192.168.17.123";
	//	switch_xml_t x_root, x_section, x_config, x_networklists, x_list, x_node;
	//	x_root = switch_xml_root();
	//	if (!x_root) {
	//		goto mid_flag;
	//	}
	//
	//	//方法1 :通过访问控制列表ACL获得,但是这种方法不可靠
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

	//方法2 :通过获得本机的IP地址,如果是双网卡呢???
	char guess_ip4[256] = "";
	switch_find_local_ip(guess_ip4, sizeof(guess_ip4),NULL,AF_INET);//这个函数因为版本的不同而不同
	//domain_name = guess_ip4;//临时变量,直接赋值无效
	m_localIP = switch_mprintf("%s", guess_ip4);

	//end_flag:
	return /*domain_name*/m_localIP;
}

/*获得用户的当前default目录结构
*/
char * CBaseModule::getMobUserDefaultXMLPath()
{
	//conf\directory\default,所有的用户都必须在此目录下存放完整的xml文件
	char *path = switch_mprintf("%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DEFAULT_DIRECTORY, 
		SWITCH_PATH_SEPARATOR);

	return path;
}

/*获得用户的default/目录下的默认配置xml文件
*/
char * CBaseModule::getMobUserDefaultXMLFile(const char *mobusername)
{
	char *file = getMobUserDefaultXMLPath();
	char *userFile = switch_mprintf("%s%s%s", 
		file, 
		mobusername, 
		CARICCP_XML_FILE_TYPE);

	switch_safe_free(file);
	return userFile;//何处进行释放
}

/*获得用户所在组xml文件
*/
char * CBaseModule::getMobUserGroupXMLFile(const char *groupname, const char *mobusername)
{
	if (NULL == groupname || NULL == mobusername || switch_strlen_zero(groupname) || switch_strlen_zero(mobusername)) {
		return NULL;
	}

	//conf\directory\groups\用户组名
	char *path = switch_mprintf("%s%s%s%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIR_DEFINE_GROUPS,  	//组的根目录groups  		  
		SWITCH_PATH_SEPARATOR, 
		groupname, 
		SWITCH_PATH_SEPARATOR, 
		mobusername, 
		CARICCP_XML_FILE_TYPE);

	return path;
}

/*获得组的目录
*/
char * CBaseModule::getMobGroupDir(const char *groupname)
{
	//conf\directory\groups\组名
	char *path = switch_mprintf("%s%s%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIR_DEFINE_GROUPS,  	//组的根目录groups  		  
		SWITCH_PATH_SEPARATOR, 
		groupname);

	return path;//何处释放
}

/*获得conf\directory\default.xml文件
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

/*获得dia group的xml文件
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
		DIAPLAN_GROUP_XMLFILE,  	  //固定的文件名 02_group_dial
		CARICCP_XML_FILE_TYPE);

	return defaultXmlFile;
}

/*获得conf\sip_profiles目录
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

/*获得conf\autoload_configs\dispat.conf.xml
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


/*login是否成功
*/
bool CBaseModule::isLoginSuc(string username,
							 string userpwd)
{
	//临时方案:目前根据文件读取
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

	//将内容转换成xml的结构(注意:中文字符问题)
	x_NEs = cari_common_parseXmlFromFile(nefile);
	if (!x_NEs){
		goto end;
	}

	//遍历查找
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
