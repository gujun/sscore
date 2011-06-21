#ifndef CMODULE_NEMANAGER_H
#define CMODULE_NEMANAGER_H

#include "CBaseModule.h"

/*��Ԫ�����Լ������û�����,�����豸��Ԫ�Ĵ����
*/
class CModule_NEManager :public CBaseModule
{
public:
	 CModule_NEManager();
	 ~CModule_NEManager();

public://method
	 int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//��������
	 int sendRespMsg(common_ResultResponse_Frame*& reqFrame);										//���ͽ��
	 int cmdPro(const inner_CmdReq_Frame*& reqFrame);												//�����

public:

protected:

  //method
private:
	//add by xxl 1010-5-19 :���ӶԲ����û�������
	int addOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int delOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int modOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int lstOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	//add by xxl 1010-5-19 end

	//��ʼ��̬��Ϣ�Ĳ�ѯ
	int addEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int delEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int modEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int lstEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	//��������,����Ԫ�Ĳ�����ʱ���ȴ�ŵ�cari_ccp_ne.xml�ļ���
	bool isExistedNE(int id);//Ӧ�ø���id��,һ���豸�����ж��ip��ַ
	bool isExistedNE(string ip);

	//�����û�
	bool isExistedOpUser(string username);

	//������Ԫ�����Ի���������
	//bool setNEAttribute(switch_xml_t neXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);
};


#endif
