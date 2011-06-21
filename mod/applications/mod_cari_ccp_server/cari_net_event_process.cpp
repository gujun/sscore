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

#else //linuxϵͳʹ��
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#endif


/*******************************************************/
/*������ģ�鴦����ϵĽ��,����,��cari net server������
*/
int cari_net_event_recvResult(switch_event_t *event)
{
	if (NULL == event) {
		return CARICCP_ERROR_STATE_CODE;
	}

	//new�µĵ���Ӧ֡
	inner_ResultResponse_Frame	*inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//��ʼ��"�ڲ���Ӧ֡"�ṹ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����ֵ
	initInnerResponseFrame(inner_respFrame, event);


	//�¼�����
	//switch (event->event_id) 
	//{
	//case CARINET_EVENT_RETURNRESULT:

	//	break;

	//default:
	//	return CARICCP_ERROR_STATE_CODE;
	//}

	//����ǲ�ѯ��ķ��ؽ����
	const char	*result	= switch_event_get_header(event, CARICCP_RESULTVALUE);
	if (NULL != result) {
		//�鿴�м�����¼����
		splitString(result, CARICCP_SPECIAL_SPLIT_FLAG, inner_respFrame->m_vectTableValue);
	}

	//��������ظ����ʹ������client
	CMsgProcess::getInstance()->commonSendRespMsg(inner_respFrame);

	//����"��Ӧ֡"
	CARI_CCP_VOS_DEL(inner_respFrame);

	return CARICCP_SUCCESS_STATE_CODE;
}

/*������ģ���֪ͨ�¼�,֪ͨcari net server������
*�����֪ͨ���͸��ݶ�Ӧ��"֪ͨ��",�¼����Ӧ���ǲ�ͬ���¼�����,�䷵�صĲ���Ҳ��ͬ
*/
int cari_net_event_notify(switch_event_t *event)
{
	int iEventID		= -1;
	int iNotifyCode		= -1;
	int iFunRes			= CARICCP_ERROR_STATE_CODE;
	char *strReturnCode = NULL;

	//֪ͨ���͵��¼���Ҫ���ظ�����ע��Ŀͻ��˽��д���
	if (NULL == event) {
		return CARICCP_ERROR_STATE_CODE;
	}

	//1 �¼�����
	iEventID = event->event_id;
	switch (iEventID) 
	{
	case /*CARICCP_EVENT_STATE_NOTIFY*/SWITCH_EVENT_CUSTOM:  //cari net server���¼�֪ͨ
		break;

	default:
		break;
		//goto end;//�����δ�����,ֻ��Ҫ����"֪ͨ��"����.
	}

	//2 ֪ͨ��(�������㹻)
	strReturnCode = switch_event_get_header(event, CARICCP_RETURNCODE);	 //֪ͨ��,��:������
	iNotifyCode	= stringToInt(strReturnCode);

	//��Ч��֪ͨ��
	if (CARICCP_NOTIFY_CODE_BEGIN > iNotifyCode || CARICCP_NOTIFY_CODE_END < iNotifyCode) {
		goto end;
	}

	//ϸ�ֽ����
	switch (iNotifyCode) {
	//������֪ͨ
	case CARICCP_NOTIFY_REBOOT:
		iFunRes = cari_net_ne_reboot(iNotifyCode,event);
		break;

	case CARICCP_NOTIFY_ADD_MOBUSER:
	case CARICCP_NOTIFY_DEL_MOBUSER:
	case CARICCP_NOTIFY_MOD_MOBUSER:
	case CARICCP_NOTIFY_MOBUSER_STATE_CHANGED:
		//����ĵ��ú���
		iFunRes = cari_net_mobuserstatus_change(iNotifyCode,event);
		break;

	case CARICCP_NOTIFY_MOBUSER_REALTIME_CALL:
		iFunRes = cari_net_callstatus_change(iNotifyCode,event);
		break;

	//�漰����Ԫ�Ĵ���
	case CARICCP_NOTIFY_ADD_EQUIP_NE: 
	case CARICCP_NOTIFY_DEL_EQUIP_NE:
	case CARICCP_NOTIFY_MOD_EQUIP_NE:
		{
			//����ĵ��ú���
			iFunRes = cari_net_ne_statechange(iNotifyCode,event);
			break;
		}

	default:
		break;	
	}

end:
	return iFunRes;
}
