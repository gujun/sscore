/*CCPƽ̨�����,�����¼��Ĵ����������ڴ˴�����ע��
*���ǵ�ǰ��C��C++�Ĳ��,��ڶ��ڴ��ļ��ж���
*����Ĵ�����鿴cari_net_event_process.cpp�ļ�
*��ע :#include "cari_net_event_process.h"   ��������,�������"�ַ���"����
*/

//#if defined _WIN32
//#define EXPORT __declspec(dllexport)
//#else
//#define EXPORT
//#endif

#include "mod_cari_ccp_server.h"
#include "cari_net_stateInfo.h"

#define CARI_NET_PROFILE	   "default" //��Ӧ�����ļ��� <profile name="default">

//����C++�ļ�����ĺ���,���ּ���
extern int cari_net_server_start(); 							//��������ʼ����
extern int cari_net_event_notify(switch_event_t *event);	    //�������֪ͨ���͵��¼�
extern void cari_common_convertStrToEvent(const char *context, const char *split, switch_event_t *event);
extern void cari_net_init();
extern void setLoopFlag(switch_bool_t bf);
extern void shutdownSocketAndGarbage();
extern int shutdownFlag;

/*add by cari 2009-9
*cari�Զ�����¼�����ص���ں���,�������֪ͨ�¼�.�ɲ��� general_event_handler()��ʵ�ַ�ʽ
*��Ҫ�����漰��״̬�ı�Ĵ���,��:�û�������״̬
*/
static void cari_net_event_statechange(switch_event_t *event) //event�����Ƿ�Ҫ����???
{
	//����µ���Ӧ֡,���ظ�client.
	//����c��c++�ļ��ļ�����,�˴�ת��c++������ļ��н��д���
	cari_net_event_notify(event);
}

/*�������ݿ����Դ
*/
static switch_bool_t cari_net_destroy()
{
	//cari_net_profile_t *profile = NULL;
	//const char *profile_name  = CARI_NET_PROFILE;//��Ӧ�����ļ��� <profile name="default">

	////�Ȳ���profile�Ľṹ
	//profile = switch_core_hash_find(cari_net_globals.profile_hash, profile_name);
	//if (profile 
	//  && profile->odbc_dsn
	//  && profile->master_odbc)
	//{
	//  //�������ݿ�ľ��
	//  switch_odbc_handle_destroy(&profile->master_odbc);
	//}

	////hash��������
	//switch_core_hash_destroy(&cari_net_globals.profile_hash);

	//socket��Ӧ��client���߳��˳�
	setLoopFlag(SWITCH_FALSE);

	//�¼��ص����������
	switch_event_unbind_callback(cari_net_event_statechange);

	//�ͷŶ˿ں�
	shutdownSocketAndGarbage();

	return SWITCH_TRUE;
}

/*����API����:cari��֪ͨ����,����ģ���漰��ʵʱ״̬�ĸı�,��Ҫ��ʱ֪ͨcariƽ̨���з���
*(_In_opt_z_ const char *cmd, _In_opt_ switch_core_session_t *session, _In_ switch_stream_handle_t *stream)
*/
SWITCH_STANDARD_API(cari_net_proNotify)
{
	//�����ĵ�һ������
	const char *paramArg = cmd; 
	switch_event_t *param_event = NULL;

	if (NULL == paramArg) {
		return SWITCH_STATUS_FALSE;
	}

	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&param_event, CARICCP_EVENT_STATE_NOTIFY)) {
	if (SWITCH_STATUS_SUCCESS != switch_event_create_subclass(&param_event, SWITCH_EVENT_CUSTOM, CARI_CCP_EVENT_NAME)) {
		return SWITCH_STATUS_MEMERR;
	}

	//��ʱ���漰����ֵ
	//switch_event_t *result_event = NULL;
	//if (SWITCH_STATUS_SUCCESS != switch_event_create(&result_event, SWITCH_EVENT_LOG))
	//{
	//  //�����ͷŵ�һ��������event���ڴ�
	//  switch_event_destroy(&param_event);

	//  return SWITCH_STATUS_MEMERR;
	//}

	//������ν��ִ�װ����event���¼�����
	//��ʽ: ������=����ֵ,(��) ������2=����ֵ2
	cari_common_convertStrToEvent(paramArg, CARICCP_SPECIAL_SPLIT_FLAG, param_event);

	//������߼�����------------------
	cari_net_event_notify(param_event);

	//�����ͷ��ڴ�
	switch_event_destroy(&param_event);

	//�������
	stream->data = "";

	return SWITCH_STATUS_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
//�궨�� :����Ϊ�������ص�ģ��.�������������������dll(so)������һֻ��ʼ:mod_cari_ccp_server
//����Ч,��������Ŀ����-->"�̳е���Ŀ��"������w32\module_debug.vsprops
SWITCH_MODULE_LOAD_FUNCTION(mod_cari_ccp_server_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cari_ccp_server_shutdown);
SWITCH_MODULE_DEFINITION(mod_cari_ccp_server, mod_cari_ccp_server_load, mod_cari_ccp_server_shutdown, NULL); 


/*cari ƽ̨����ں���,ע����Ϣ�¼�����Ĺ��Ӻ���
*/
SWITCH_MODULE_LOAD_FUNCTION(mod_cari_ccp_server_load)
{
	//�Ƿ���Ҫ����ĳЩ�¼��Ļص�����,���д���ؼ���
	switch_api_interface_t *api_interface;//api�ӿ�
	//switch_status_t status;

	//���������ļ���Ϣ,�����������ݿ������,Ŀǰ���漰���˷���,����ȥ
	//if (SWITCH_STATUS_SUCCESS != (status = load_cari_net_config()))
	//{
	//  //return status;
	//}

	/* connect my internal structure to the blank pointer passed to me */   			   
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	//Ҫע�����溯���ĵ���˳��,������ĺ궨�����������Ч��ʹ��
	//��ʼ������,����һЩ��Ҫ�����ӵ�Ŀ¼,���ļ�֮��
	cari_net_init();

	//add by cari 2009-11 begin:����cari net���͵��¼�,�󶨻ص�����
	//��CARICCP_EVENT_STATE_NOTIFY�����¼�,����Ϊ:SWITCH_EVENT_CUSTOM
	if (switch_event_bind(modname, /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM, SWITCH_EVENT_SUBCLASS_ANY, cari_net_event_statechange, NULL) != SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind cari_net_event_statechange()!\n");
		return SWITCH_STATUS_GENERR;
	}
	//��CARICCP_EVENT_ALL�����¼�
	//if (switch_event_bind(modname, CARICCP_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, cari_net_event_handler, NULL) != SWITCH_STATUS_SUCCESS) 
	//{
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind cari_net_event_handler()!\n");
	//	return SWITCH_STATUS_GENERR;
	//}
	//add by cari 2009-11 end

	//�������api�����ӿڴ������,���ڵĴ��������ͬ������ʽ
	SWITCH_ADD_API(api_interface, CARI_NET_PRO_NOTIFY, "cari net notify", cari_net_proNotify, "cari net notify");

	//�ڲ������µ��߳� :����client����������
	cari_net_server_start();

	shutdownFlag = 0;
	return SWITCH_STATUS_SUCCESS;
}

/*��ģ���˳�,������Դ,�ҳ���ʱ�˺���������???
*/
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cari_ccp_server_shutdown)
{
	shutdownFlag = 1;//���ñ�ʶ��

	//������Դ
	cari_net_destroy();

	return SWITCH_STATUS_SUCCESS;
}
