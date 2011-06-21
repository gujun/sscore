#ifndef CMODULE_ROUTE_H
#define CMODULE_ROUTE_H

#include "CBaseModule.h"


/*·�ɹ���ģ��,�����������õ���Ϣ����
*/
class CModule_Route :public CBaseModule
{
public:
	 CModule_Route();
	 ~CModule_Route();

public://method
	 int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//��������
	 int sendRespMsg(common_ResultResponse_Frame*& reqFrame);           //���ͽ��

	 int cmdPro(const inner_CmdReq_Frame*& reqFrame); //�����

public:
	 //�鿴�����Ƿ����
	 bool isExistedGateWay(const char* profileType, const char* gatewayname);

protected:

  //method,����:FromMem �Ǵ��ڴ��л��;FromFile �Ǵ��ļ��л��
private:
	 int addGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int mobGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //·������
	 int setRouter(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delRouter(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstRouter(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //���Զ���Ϣ����
	 int sendMessage(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //������õ�������Ϣ
	 void getAllGatewaysFromMem(char** allGateways);

	 //�������ص������ļ�
	 bool creatGateWayXMLFile(const char* profileType, const char* gatewayname, const char* filecontext, const char** err);

	 //ɾ�������ļ�
	 bool delGateWayXMLFile(const char* profileType, const char* gatewayname, const char** err);

	 //����gateway xml������
	 bool setGatewayAttribute(switch_xml_t userXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);

	 //���ĳ������
	 char* getGatewayParamAttribute(switch_xml_t gatewayXMLinfo, const char* attName);

	 //���ĳ��Ŀ¼��dateway��xml�ṹ
	 switch_xml_t getGatewayXMLInfo(const char* profileType, const char* gatewayname);

	 //����Ĭ�ϲ���·�ɵ�����
	 bool setDialPlanAttr(switch_xml_t xcontext, const char* externName, const char* condiFieldName, const char* expressVal);

	 //����out����·��
	 bool setOutDialPlanAttr(switch_xml_t xextension, const char* condiFieldVal, const char* expressVal);

	 //���"����"·����Ϣ
	 void getLocalRouterInfo(switch_xml_t xDialContext, string &strOut);

	 //�����"����"�йص�·����Ϣ:ֻ���е��ȺͲ������̨
	 void getDispatchRouterInfo(switch_xml_t xDialContext, int type, string &strOut);

	 //���"����"��·����Ϣ
	 void getOutRouterInfo(switch_xml_t xOutDialContext, string &strOut);

	 //ɾ������·��
	 bool delOutRouter(bool bSpecial,string gateway,/*string &strErr*/char *&pDesc);

	 //�޸ĳ���·��
	 bool modOutRouter(string oldGateway,string newGateway,/*string &strErr*/char *&pDesc);
};


#endif
