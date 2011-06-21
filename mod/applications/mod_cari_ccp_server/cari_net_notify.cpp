#include "cari_net_notify.h"
#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"

/*本文件详细的实现的不同的通知类型的处理方式
*/

/*重启服务的通知
*/
int cari_net_ne_reboot(int notifyCode,			//通知码 
					   switch_event_t *event)	//事件
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*cmdCode		 = NULL;
	char						*cmdName		 = NULL;

	result = switch_event_get_header(event, CARICCP_RESULTDESC);
	cmdCode = switch_event_get_header(event, CARICCP_CMDCODE);
	cmdName = switch_event_get_header(event, CARICCP_CMDNAME);

	//new响应帧
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//初始化"内部响应帧"结构体
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化赋值,设置了"通知类型的标识"和"通知码"
	initInnerResponseFrame(inner_respFrame, event);
	
	//将通知类型的事件发送给所有客户端
	CMsgProcess::getInstance()->notifyResult(inner_respFrame);

	//销毁"响应帧"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}

/*mob user的在线状态的变更通知/增加,删除用户的通知等通知类型的消息处理
*/
int cari_net_mobuserstatus_change(int notifyCode,			//通知码
								  switch_event_t *event)	//事件
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*mobUserId		 = NULL;
	char						*mobUserState	 = NULL;
	char						*mobUserInfo	 = NULL;

	mobUserId = switch_event_get_header(event, CARICCP_MOBUSERNAME);	//用户号
	mobUserState = switch_event_get_header(event, CARICCP_MOBUSERSTATE);//用户的状态
	mobUserInfo = switch_event_get_header(event, CARICCP_MOBUSERINFO);	//用户的其他关键信息

	//new响应帧
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//初始化"内部响应帧"结构体
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化赋值,设置了"通知类型的标识"和"通知码"
	initInnerResponseFrame(inner_respFrame, event);

	//根据"通知码"进行详细的区分
	switch(notifyCode){
	case CARICCP_NOTIFY_MOBUSER_STATE_CHANGED:
		{
			if (NULL == mobUserId || NULL == mobUserState){
				break;
			}
			//组装结果集,单个记录"多值",使用CARICCP_COLON_MARK(:)分割
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
			//用户名设置为结果集
			result = switch_mprintf("%s", mobUserId);
			break;
		}

	default:
		goto end;
	}

	if (NULL != result) {
		//查看有几条记录存在,记录之间使用\r\n换行标识,值和值之间应该使用特殊符"〒"分开
		splitString(result, CARICCP_SPECIAL_SPLIT_FLAG, inner_respFrame->m_vectTableValue);

		//将通知类型的事件发送给所有客户端
		CMsgProcess::getInstance()->notifyResult(inner_respFrame);

		//释放内存
		switch_safe_free(result);
	}

/**/
end:
	//销毁"响应帧"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}

////新建一种结构,代表呼叫信息,如何传输发送出去???
//typedef struct CallInfo
//{
//	short type;//类型
//	short status;
//	short caller;//主叫
//	char callee[500];//被叫,可以有多个,比如:会议类型
//}CallInfo;

/*call呼叫状态通知
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

	mobCaller = switch_event_get_header(event, CARICCP_MOBCALLER);	     //主叫号
	mobCallee = switch_event_get_header(event, CARICCP_MOBCALLEE);       //被叫号
	mobCallState = switch_event_get_header(event, CARICCP_MOBCALLSTATE); //呼叫状态
	mobCallType = switch_event_get_header(event, CARICCP_MOBCALLTYPE);	 //呼叫类型

	if (isNullStr(mobCallType)){//默认为p2p类型
		mobCallType = "P2P";
	}
	if (isNullStr(mobCallState)){
		string str = intToString(/*CARICCP_MOB_USER_HANGUP*/5);//默认为:挂机
		mobCallState = (char*)str.c_str();
	}

	//new响应帧
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//初始化"内部响应帧"结构体
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化赋值,设置了"通知类型的标识"和"通知码"
	initInnerResponseFrame(inner_respFrame, event);

	//根据"通知码"进行详细的区分
	switch(notifyCode){
	case CARICCP_NOTIFY_MOBUSER_REALTIME_CALL:
		{
			if (NULL == mobCaller || NULL == mobCallee){
				break;
			}
			//组装结果集,规定的格式为 type:状态:主叫号码〒被叫号码1,被叫号码2,type不填写默认为点呼叫
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
		//存放记录
		inner_respFrame->m_vectTableValue.push_back(result);

		//将通知类型的事件发送给所有客户端
		CMsgProcess::getInstance()->notifyResult(inner_respFrame);
		//释放内存
		switch_safe_free(result);
	}

end:

	//销毁"响应帧"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}

/*网元的变更通知/增加,删除的通知等通知类型的消息处理
*/
int cari_net_ne_statechange(int notifyCode,			//通知码
							switch_event_t *event)	//事件
{
	inner_ResultResponse_Frame	*inner_respFrame = NULL;
	char						*result			 = NULL;
	char						*neId		     = NULL;
	char						*neType	         = NULL;
	char						*neIP	         = NULL;
	char						*neDesc	         = NULL;

	neId   = switch_event_get_header(event, "neid");	//网元号
	neType = switch_event_get_header(event, "netype");  //网元类型
	neIP   = switch_event_get_header(event, "ip");	    //网元ip地址
	neDesc = switch_event_get_header(event, "desc");	//网元描述信息

	//new响应帧
	inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

	//初始化"内部响应帧"结构体
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//初始化赋值,设置了"通知类型的标识"和"通知码"
	initInnerResponseFrame(inner_respFrame, event);

	//根据"通知码"进行详细的区分
	switch(notifyCode){
	//增加/修改网元
	case CARICCP_NOTIFY_ADD_EQUIP_NE:
	case CARICCP_NOTIFY_MOD_EQUIP_NE:
		{
			//Id,类型,ip为返回的集合
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
	//删除网元
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

		//将通知类型的事件发送给所有客户端
		CMsgProcess::getInstance()->notifyResult(inner_respFrame);
	}

end:

	//销毁"响应帧"
	CARI_CCP_VOS_DEL(inner_respFrame);
	return CARICCP_SUCCESS_STATE_CODE;
}
