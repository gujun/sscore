/*CCP平台的入口,所有事件的处理函数都是在此处进行注册
*考虑到前期C和C++的差别,入口都在此文件中定义
*具体的处理则查看cari_net_event_process.cpp文件
*备注 :#include "cari_net_event_process.h"   不能引用,否则出现"字符串"错误
*/

//#if defined _WIN32
//#define EXPORT __declspec(dllexport)
//#else
//#define EXPORT
//#endif

#include "mod_cari_ccp_server.h"
#include "cari_net_stateInfo.h"

#define CARI_NET_PROFILE	   "default" //对应配置文件的 <profile name="default">

//引用C++文件定义的函数,保持兼容
extern int cari_net_server_start(); 							//服务器初始启动
extern int cari_net_event_notify(switch_event_t *event);	    //负责接收通知类型的事件
extern void cari_common_convertStrToEvent(const char *context, const char *split, switch_event_t *event);
extern void cari_net_init();
extern void setLoopFlag(switch_bool_t bf);
extern void shutdownSocketAndGarbage();
extern int shutdownFlag;

/*add by cari 2009-9
*cari自定义的事件处理回调入口函数,负责接收通知事件.可参照 general_event_handler()的实现方式
*主要负责涉及到状态改变的处理,如:用户的在线状态
*/
static void cari_net_event_statechange(switch_event_t *event) //event对象是否要销毁???
{
	//组成新的响应帧,返回给client.
	//保持c和c++文件的兼容性,此处转到c++定义的文件中进行处理
	cari_net_event_notify(event);
}

/*销毁数据库等资源
*/
static switch_bool_t cari_net_destroy()
{
	//cari_net_profile_t *profile = NULL;
	//const char *profile_name  = CARI_NET_PROFILE;//对应配置文件的 <profile name="default">

	////先查找profile的结构
	//profile = switch_core_hash_find(cari_net_globals.profile_hash, profile_name);
	//if (profile 
	//  && profile->odbc_dsn
	//  && profile->master_odbc)
	//{
	//  //销毁数据库的句柄
	//  switch_odbc_handle_destroy(&profile->master_odbc);
	//}

	////hash容器销毁
	//switch_core_hash_destroy(&cari_net_globals.profile_hash);

	//socket对应的client的线程退出
	setLoopFlag(SWITCH_FALSE);

	//事件回调函数解除绑定
	switch_event_unbind_callback(cari_net_event_statechange);

	//释放端口号
	shutdownSocketAndGarbage();

	return SWITCH_TRUE;
}

/*增加API函数:cari的通知处理,其他模块涉及到实时状态的改变,需要及时通知cari平台进行反馈
*(_In_opt_z_ const char *cmd, _In_opt_ switch_core_session_t *session, _In_ switch_stream_handle_t *stream)
*/
SWITCH_STANDARD_API(cari_net_proNotify)
{
	//函数的第一个参数
	const char *paramArg = cmd; 
	switch_event_t *param_event = NULL;

	if (NULL == paramArg) {
		return SWITCH_STATUS_FALSE;
	}

	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&param_event, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&param_event, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		return SWITCH_STATUS_MEMERR;
	}

	//暂时不涉及返回值
	//switch_event_t *result_event = NULL;
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&result_event, SWITCH_EVENT_LOG))
	//{
	//  //销毁释放第一个创建的event的内存
	//  switch_event_destroy(&param_event);

	//  return SWITCH_STATUS_MEMERR;
	//}

	//考虑如何将字串装换成event的事件对象
	//格式: 参数名=参数值,(或〒) 参数名2=参数值2
	cari_common_convertStrToEvent(paramArg, CARICCP_SPECIAL_SPLIT_FLAG, param_event);

	//具体的逻辑处理------------------
	cari_net_event_notify(param_event);

	//销毁释放内存
	switch_event_destroy(&param_event);

	//输出参数
	stream->data = "";

	return SWITCH_STATUS_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
//宏定义 :设置为启动加载的模块.启动函数的命名必须和dll(so)的名字一只开始:mod_cari_ccp_server
//宏有效,必须在项目属性-->"继承的项目表"中设置w32\module_debug.vsprops
SWITCH_MODULE_LOAD_FUNCTION(mod_cari_ccp_server_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cari_ccp_server_shutdown);
SWITCH_MODULE_DEFINITION(mod_cari_ccp_server, mod_cari_ccp_server_load, mod_cari_ccp_server_shutdown, NULL); 


/*cari 平台的入口函数,注册消息事件处理的钩子函数
*/
SWITCH_MODULE_LOAD_FUNCTION(mod_cari_ccp_server_load)
{
	//是否需要增加某些事件的回调函数,进行处理关键的
	switch_api_interface_t *api_interface;//api接口
	//switch_status_t status;

	//加载配置文件信息,包括建立数据库的连接,目前不涉及到此方面,跳过去
	//if (SWITCH_STATUS_SUCCESS != (status = load_cari_net_config()))
	//{
	//  //return status;
	//}

	/* connect my internal structure to the blank pointer passed to me */   			   
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	//要注意下面函数的调用顺序,否则定义的宏定义变量不能有效的使用
	//初始化环境,包括一些需要新增加的目录,或文件之类
	cari_net_init();

	//add by cari 2009-11 begin:新增cari net类型的事件,绑定回调函数
	//绑定CARICCP_EVENT_STATE_NOTIFY类型事件,更改为:SWITCH_EVENT_CUSTOM
	if (switch_event_bind(modname, /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM, SWITCH_EVENT_SUBCLASS_ANY, cari_net_event_statechange, NULL) != SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind cari_net_event_statechange()!\n");
		return SWITCH_STATUS_GENERR;
	}
	//绑定CARICCP_EVENT_ALL类型事件
	//if (switch_event_bind(modname, CARICCP_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, cari_net_event_handler, NULL) != SWITCH_STATUS_SUCCESS) 
	//{
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind cari_net_event_handler()!\n");
	//	return SWITCH_STATUS_GENERR;
	//}
	//add by cari 2009-11 end

	//将对外的api函数接口存放起来,后期的处理将变成了同步处理方式
	SWITCH_ADD_API(api_interface, CARI_NET_PRO_NOTIFY, "cari net notify", cari_net_proNotify, "cari net notify");

	//内部启动新的线程 :负责client的连接请求
	cari_net_server_start();

	shutdownFlag = 0;
	return SWITCH_STATUS_SUCCESS;
}

/*本模块退出,销毁资源,找出何时此函数被调用???
*/
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cari_ccp_server_shutdown)
{
	shutdownFlag = 1;//设置标识符

	//销毁资源
	cari_net_destroy();

	return SWITCH_STATUS_SUCCESS;
}
