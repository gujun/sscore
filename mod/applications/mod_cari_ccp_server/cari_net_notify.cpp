#include "cari_net_notify.h"
#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"

/*���ļ���ϸ��ʵ�ֵĲ�ͬ��֪ͨ���͵Ĵ���ʽ
*/

/*���������֪ͨ
*/
int cari_net_ne_reboot(int notifyCode,			//֪ͨ�� 
					   switch_event_t *event)	//�¼�
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*cmdCode		 = NULL;
	char						*cmdName		 = NULL;

	result = switch_event_get_header(event, CARICCP_RESULTDESC);
	cmdCode = switch_event_get_header(event, CARICCP_CMDCODE);
	cmdName = switch_event_get_header(event, CARICCP_CMDNAME);

	//new��Ӧ֡
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//��ʼ��"�ڲ���Ӧ֡"�ṹ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����ֵ,������"֪ͨ���͵ı�ʶ"��"֪ͨ��"
	initInnerResponseFrame(inner_respFrame, event);
	
	//��֪ͨ���͵��¼����͸����пͻ���
	CMsgProcess::getInstance()->notifyResult(inner_respFrame);

	//����"��Ӧ֡"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}

/*mob user������״̬�ı��֪ͨ/����,ɾ���û���֪ͨ��֪ͨ���͵���Ϣ����
*/
int cari_net_mobuserstatus_change(int notifyCode,			//֪ͨ��
								  switch_event_t *event)	//�¼�
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*mobUserId		 = NULL;
	char						*mobUserState	 = NULL;
	char						*mobUserInfo	 = NULL;

	mobUserId = switch_event_get_header(event, CARICCP_MOBUSERNAME);	//�û���
	mobUserState = switch_event_get_header(event, CARICCP_MOBUSERSTATE);//�û���״̬
	mobUserInfo = switch_event_get_header(event, CARICCP_MOBUSERINFO);	//�û��������ؼ���Ϣ

	//new��Ӧ֡
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//��ʼ��"�ڲ���Ӧ֡"�ṹ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����ֵ,������"֪ͨ���͵ı�ʶ"��"֪ͨ��"
	initInnerResponseFrame(inner_respFrame, event);

	//����"֪ͨ��"������ϸ������
	switch(notifyCode){
	case CARICCP_NOTIFY_MOBUSER_STATE_CHANGED:
		{
			if (NULL == mobUserId || NULL == mobUserState){
				break;
			}
			//��װ�����,������¼"��ֵ",ʹ��CARICCP_COLON_MARK(:)�ָ�
			if (NULL == mobUserInfo || 0 == strlen(mobUserInfo)) {
				result = switch_mprintf("%s %s %s", mobUserId, CARICCP_COLON_MARK, mobUserState);
			} else {
				result = switch_mprintf("%s %s %s %s %s", mobUserId, CARICCP_COLON_MARK, mobUserState, CARICCP_COLON_MARK, mobUserInfo);
			}

			break;
		}
	case CARICCP_NOTIFY_ADD_MOBUSER:
	case CARICCP_NOTIFY_DEL_MOBUSER:
	case CARICCP_NOTIFY_MOD_MOBUSER:
		{
			//�û�������Ϊ�����
			result = switch_mprintf("%s", mobUserId);
			break;
		}

	default:
		goto end;
	}

	if (NULL != result) {
		//�鿴�м�����¼����,��¼֮��ʹ��\r\n���б�ʶ,ֵ��ֵ֮��Ӧ��ʹ�������"��"�ֿ�
		splitString(result, CARICCP_SPECIAL_SPLIT_FLAG, inner_respFrame->m_vectTableValue);

		//��֪ͨ���͵��¼����͸����пͻ���
		CMsgProcess::getInstance()->notifyResult(inner_respFrame);

		//�ͷ��ڴ�
		switch_safe_free(result);
	}

/**/
end:
	//����"��Ӧ֡"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}

////�½�һ�ֽṹ,���������Ϣ,��δ��䷢�ͳ�ȥ???
//typedef struct CallInfo
//{
//	short type;//����
//	short status;
//	short caller;//����
//	char callee[500];//����,�����ж��,����:��������
//}CallInfo;

/*call����״̬֪ͨ
*/
int cari_net_callstatus_change(int notifyCode,
							   switch_event_t *event)
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*mobCaller		 = NULL;
	char						*mobCallee	     = NULL;
	char						*mobCallState	 = NULL;
	char						*mobCallType	 = NULL;

	mobCaller = switch_event_get_header(event, CARICCP_MOBCALLER);	     //���к�
	mobCallee = switch_event_get_header(event, CARICCP_MOBCALLEE);       //���к�
	mobCallState = switch_event_get_header(event, CARICCP_MOBCALLSTATE); //����״̬
	mobCallType = switch_event_get_header(event, CARICCP_MOBCALLTYPE);	 //��������

	if (isNullStr(mobCallType)){//Ĭ��Ϊp2p����
		mobCallType = "P2P";
	}
	if (isNullStr(mobCallState)){
		string str = intToString(/*CARICCP_MOB_USER_HANGUP*/5);//Ĭ��Ϊ:�һ�
		mobCallState = (char*)str.c_str();
	}

	//new��Ӧ֡
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//��ʼ��"�ڲ���Ӧ֡"�ṹ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����ֵ,������"֪ͨ���͵ı�ʶ"��"֪ͨ��"
	initInnerResponseFrame(inner_respFrame, event);

	//����"֪ͨ��"������ϸ������
	switch(notifyCode){
	case CARICCP_NOTIFY_MOBUSER_REALTIME_CALL:
		{
			if (NULL == mobCaller || NULL == mobCallee){
				break;
			}
			//��װ�����,�涨�ĸ�ʽΪ type:״̬:���к��먓���к���1,���к���2,type����дĬ��Ϊ�����
			result = switch_mprintf("%s%s%s%s%s%s%s", mobCallType,CARICCP_COLON_MARK,
				mobCallState,CARICCP_COLON_MARK,
				mobCaller, CARICCP_SPECIAL_SPLIT_FLAG, 
				mobCallee);

			break;
		}
	default:
		goto end;
	}

	if (NULL != result) {
		//��ż�¼
		inner_respFrame->m_vectTableValue.push_back(result);

		//��֪ͨ���͵��¼����͸����пͻ���
		CMsgProcess::getInstance()->notifyResult(inner_respFrame);
		//�ͷ��ڴ�
		switch_safe_free(result);
	}

end:

	//����"��Ӧ֡"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}

/*��Ԫ�ı��֪ͨ/����,ɾ����֪ͨ��֪ͨ���͵���Ϣ����
*/
int cari_net_ne_statechange(int notifyCode,			//֪ͨ��
							switch_event_t *event)	//�¼�
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*neId		     = NULL;
	char						*neType	         = NULL;
	char						*neIP	         = NULL;
	char						*neDesc	         = NULL;

	neId   = switch_event_get_header(event, "neid");	//��Ԫ��
	neType = switch_event_get_header(event, "netype");  //��Ԫ����
	neIP   = switch_event_get_header(event, "ip");	    //��Ԫip��ַ
	neDesc = switch_event_get_header(event, "desc");	//��Ԫ������Ϣ

	//new��Ӧ֡
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//��ʼ��"�ڲ���Ӧ֡"�ṹ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����ֵ,������"֪ͨ���͵ı�ʶ"��"֪ͨ��"
	initInnerResponseFrame(inner_respFrame, event);

	//����"֪ͨ��"������ϸ������
	switch(notifyCode){
	//����/�޸���Ԫ
	case CARICCP_NOTIFY_ADD_EQUIP_NE:
	case CARICCP_NOTIFY_MOD_EQUIP_NE:
		{
			//Id,����,ipΪ���صļ���
			result = switch_mprintf("%s%s%s%s%s%s%s%s", 
				neId, 
				CARICCP_SPECIAL_SPLIT_FLAG,
				neType,
				CARICCP_SPECIAL_SPLIT_FLAG,
				neIP,
				CARICCP_SPECIAL_SPLIT_FLAG,
				neDesc,
				CARICCP_ENTER_KEY
				);

			break;
		}
	//ɾ����Ԫ
	case CARICCP_NOTIFY_DEL_EQUIP_NE:
		{
		    result = neId;
			break;
		}

	default:
		goto end;
	}

	if (NULL != result) {
		inner_respFrame->m_vectTableValue.push_back(result);

		//��֪ͨ���͵��¼����͸����пͻ���
		CMsgProcess::getInstance()->notifyResult(inner_respFrame);
	}

end:

	//����"��Ӧ֡"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}
