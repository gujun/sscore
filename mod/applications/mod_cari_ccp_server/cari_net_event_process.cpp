#include "cari_net_event_process.h"
#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"
#include "CGarbageRecover.h"
#include "cari_net_notify.h"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>


#if defined(_WIN32) || defined(WIN32)

#else //linux系统使用
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#endif


/*******************************************************/
/*将其他模块处理完毕的结果,返回,由cari net server负责处理
*/
int cari_net_event_recvResult(switch_event_t *event)
{
	if (NULL == event) {
		return CARICCP_ERROR_STATE_CODE;
	}

	//new新的的响应帧
	inner_ResultResponse_Frame	*inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//初始化"内部响应帧"结构体
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化赋值
	initInnerResponseFrame(inner_respFrame, event);


	//事件类型
	//switch (event->event_id) 
	//{
	//case CARINET_EVENT_RETURNRESULT:

	//	break;

	//default:
	//	return CARICCP_ERROR_STATE_CODE;
	//}

	//如果是查询类的返回结果集
	const char	*result	= switch_event_get_header(event, CARICCP_RESULTVALUE);
	if (NULL != result) {
		//查看有几条记录存在
		splitString(result, CARICCP_SPECIAL_SPLIT_FLAG, inner_respFrame->m_vectTableValue);
	}

	//将结果返回给发送此命令的client
	CMsgProcess::getInstance()->commonSendRespMsg(inner_respFrame);

	//销毁"响应帧"
	CARI_CCP_VOS_DEL(inner_respFrame);

	return CARICCP_SUCCESS_STATE_CODE;
}

/*将其他模块的通知事件,通知cari net server负责处理
*具体的通知类型根据对应的"通知码",事件码对应的是不同的事件类型,其返回的参数也不同
*/
int cari_net_event_notify(switch_event_t *event)
{
	int iEventID		= -1;
	int iNotifyCode		= -1;
	int iFunRes			= CARICCP_ERROR_STATE_CODE;
	char *strReturnCode = NULL;

	//通知类型的事件需要返回给所有注册的客户端进行处理
	if (NULL == event) {
		return CARICCP_ERROR_STATE_CODE;
	}

	//1 事件类型
	iEventID = event->event_id;
	switch (iEventID) 
	{
	case /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM:  //cari net server的事件通知
		break;

	default:
		break;
		//goto end;//先屏蔽此条件,只需要依赖"通知码"即可.
	}

	//2 通知码(此条件足够)
	strReturnCode = switch_event_get_header(event, CARICCP_RETURNCODE);	 //通知码,即:返回码
	iNotifyCode	= stringToInt(strReturnCode);

	//无效的通知码
	if (CARICCP_NOTIFY_CODE_BEGIN > iNotifyCode || CARICCP_NOTIFY_CODE_END < iNotifyCode) {
		goto end;
	}

	//细分结果码
	switch (iNotifyCode) {
	//重启的通知
	case CARICCP_NOTIFY_REBOOT:
		iFunRes = cari_net_ne_reboot(iNotifyCode,event);
		break;

	case CARICCP_NOTIFY_ADD_MOBUSER:
	case CARICCP_NOTIFY_DEL_MOBUSER:
	case CARICCP_NOTIFY_MOD_MOBUSER:
	case CARICCP_NOTIFY_MOBUSER_STATE_CHANGED:
		//具体的调用函数
		iFunRes = cari_net_mobuserstatus_change(iNotifyCode,event);
		break;

	case CARICCP_NOTIFY_MOBUSER_REALTIME_CALL:
		iFunRes = cari_net_callstatus_change(iNotifyCode,event);
		break;

	//涉及到网元的处理
	case CARICCP_NOTIFY_ADD_EQUIP_NE: 
	case CARICCP_NOTIFY_DEL_EQUIP_NE:
	case CARICCP_NOTIFY_MOD_EQUIP_NE:
		{
			//具体的调用函数
			iFunRes = cari_net_ne_statechange(iNotifyCode,event);
			break;
		}

	default:
		break;	
	}

end:
	return iFunRes;
}
