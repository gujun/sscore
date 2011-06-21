/*��Ϣ����Ҫ������,����ͬ����Ϣ���첽��Ϣ
 *�Լ�������Ӧ��Ϣ����Ӧ�Ŀͻ���
*/

#ifndef C_REQUEST_H
#define C_REQUEST_H

//#include <Windows.h>
#include <list>

#include "CModuleManager.h"
#include "cari_net_commonHeader.h"

//������
#if defined(_WIN32) || defined(WIN32)
	#include "pthread.h"
#else
	#include <pthread.h>
#endif


/*�������,��Ƴɵ�ʵ��
*/
class CMsgProcess
{
public://���캯������Ϊ˽�е�,������
  
	 ~CMsgProcess();

public://��������
  
	 static CMsgProcess*& getInstance(); //��õ�ʵ������

	 void clear(); 

	 //�����첽����������߳�
	 void creatAsynCmdThread();

	 //ͳһ����������
	 void proCmd(u_int socketID, common_CmdReq_Frame*& common_reqFrame);

	 //����ķַ�
	 void dispatchCmd(); 

	 //ͬ����������
	 int syncProCmd(inner_CmdReq_Frame*& inner_ReqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //�첽��������
	 //��ĳ�Ա��������
	 void asynProCmd_2(inner_CmdReq_Frame*& inner_reqFrame);

	 //ͬ�����첽�Ĺ�ͬ�ӿ�:������Ӧ��Ϣ,����,�ڲ���ϸ��Ϊ"����"��"�㲥"
	 void commonSendRespMsg(inner_ResultResponse_Frame*& inner_respFrame,int nSendType = COMMON_RES_TYPE);

	 //����������Ӧ
	 bool unicastRespMsg(socket3element& element, const char* respMs, int  bufferSize, char* cmdname="");
	 bool unicastRespMsg(socket3element& element, common_ResultResponse_Frame* respFrame, int  bufferSize, char* cmdname="");

	 //�㲥������Ӧ
	 /*bool broadcastRespMsg(socket3element &element,const char *respMs,int  bufferSize);*/
	 bool broadcastRespMsg(const char* respMs, int  bufferSize, char* cmdname="");

	 //֪ͨ���͵Ĵ���:�����ֵ���ִ�����(Ҳ�����Ƕ��м�¼����м�¼)
	 void notifyResult(inner_CmdReq_Frame*& reqFrame, Notify_Type_Code notifyCode, string notifyValue);

	 //֪ͨ���͵Ĵ���
	 void notifyResult(inner_ResultResponse_Frame*& inner_respFrame);

	 //�����ϱ����͵Ĵ���,��Ҫ���ĳ��ģ��Ľ���ʵʱ�ϱ�
	 void initiativeReport(inner_ResultResponse_Frame*& inner_respFrame);

	 //"����"������ʽת��
	 /*void covertStrToParamStruct(char* strParameter,vector<simpleParamStruct> &vecParam);*/

	 //"ͨ������֡"ת��Ϊ"�ڲ�����֡"
	 void covertReqFrame(common_CmdReq_Frame*& common_reqMsg, inner_CmdReq_Frame*& inner_reqFrame);

	 //����֡��ϸ������,��һ����������
	 void analyzeReqFrame(common_CmdReq_Frame*& common_reqMsg);

	 //����ת�� :"��Ӧ"������ʽת��
	 void covertRespParam(inner_ResultResponse_Frame*& inner_RespFrame, common_ResultResponse_Frame*& common_respFrame);

	 //֡ת�� :"�ڲ���Ӧ֡"ת��Ϊ"ͨ����Ӧ֡"
	 void covertRespFrame(inner_ResultResponse_Frame*& inner_respFrame, common_ResultResponse_Frame*& common_respMsg);


	 /*------------�漰���� :��Ҫ�����"����"�Ĵ���--------------*/
	 //����ڶ�ͻ��˵����,�����Ƚ϶�,������Ҫ���д������
	 void lock();
	 void unlock();

	 //�ж϶����е�Ԫ���Ƿ�����
	 bool isFullOfCmdQueue();

	 //�ж϶������Ƿ��ѿ�
	 bool isEmptyOfCmdQueue();

	 //����Ԫ�ص�������
	 int pushCmdToQueue(inner_CmdReq_Frame*& reqFrame); 

	 //�Ӷ�����ȡ��һ��Ԫ��
	 int popCmdFromQueue(inner_CmdReq_Frame*& reqFrame);

	 //��������е�Ԫ��
	 void clearAllCmdFromQueue();

public://����

protected:

private://˽�з���
 
	 CMsgProcess();//���캯��������˽�е�

private://˽������
	 static CMsgProcess* m_reqProInstance;//��ʵ��ģʽ
	 //LPCRITICAL_SECTION  m_lock;   //����ʶ
	 //HANDLE m_semaphore;     //�ź���

	 pthread_mutex_t mutex;    //pthread�Դ��Ļ�����
	 pthread_cond_t cond ;    //��������

	 //vector<common_CmdReq_Frame*> vecRequestFrame;
	 list<inner_CmdReq_Frame*> m_listRequestFrame;//�������֡������

	 u_int i_full_waiters;  //��ʶ:�����Ƿ�����
	 u_int i_empty_waiters; //��ʶ:�����Ƿ��ѿ�
	 int i_terminated;
	 pthread_t thread_asynPro;//�첽�����߳�
};



#endif
