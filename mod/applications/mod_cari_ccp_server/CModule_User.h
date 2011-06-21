#ifndef CMODULE_USER_H
#define CMODULE_USER_H

#include "CBaseModule.h"

//#define CARI_ODBC  //��ʶ�Ƿ�������ݿ�ķ�ʽ

/*�û�����ģ��,�����û���Ϣ,�û������Ϣ�����
 */

class CModule_User :public CBaseModule
{
public:	
	 CModule_User();
	 ~CModule_User();

public://method
	 int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//��������
	 int sendRespMsg(common_ResultResponse_Frame*& reqFrame);           //���ͽ��
	 int cmdPro(const inner_CmdReq_Frame*& reqFrame); //�����

public:

protected:

  //method,����:FromMem �Ǵ��ڴ��л��;FromFile �Ǵ��ļ��л��
private:

	 //�������� :���������Ӧ�Ĵ���(���)����
	 int addMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int mobMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //�ڲ����� :���ģ���ʼ�򿪵�ʱ��,���в�ѯ��ǰ�û�����Ϣ
	 int queryMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //�����Ĵ���
	 int addMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int delMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int modMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int lstMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	 int setMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);

	 //ת������
	 int setTransferCall(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int unbindTransferCall(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int lstTransferCallInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //����Ա������
	 int addDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int delDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int setDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int lstDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //¼���û�������
	 int addRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int setRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 /************************************************************************/
	 /*                                                                      */
	 /************************************************************************/
	 //�Ƿ񱾵�ע�����Ч�û�
	 bool isLocalMobUser(string strUserName);

	 //�鿴�û��Ƿ����
	 bool isExistedMobUser(const char* userid);

	 //�鿴�û��Ƿ��Ѿ�ע��
	 bool isRegisteredMobUser(const char* userID,switch_core_db_t *db);

	 //�鿴���Ƿ����
	 bool isExistedGroup(const char* mobgroupname = "", int groupid = -1);

	 //������
	 string getGroupID(const char* mobgroupname);

	 //ȥ���������ִ�,��úܶ��ִ�
	 string getNewStr(const char* oldStr,const char* delStr);

	 //����û��������Ϣ
	 void getMobGroupOfUserFromMem(const char* mobuserid, char** group);

	 //���ĳ����������û���Ϣ
	 void getMobUsersOfGroupFromMem(const char* mobgroupname, char** allMobUsers);

	 //��õ�ǰ���µ����������Ϣ
	 void getAllGroupFromMem(string &allGroups,bool bshowSeq = true);

	 //����������
	 int getGroupNumFromMem();

	 //��������Ϣ
	 switch_xml_t getGroupXMLInfoFromMem(const char* groupname, switch_xml_t groupXMLinfo, string &err);

	 //����û�����Ϣ,��root xml���ڴ�ṹ�л��
	 switch_xml_t getMobUserXMLInfoFromMem(const char* mobuserid, switch_xml_t userXMLinfo);

	 //��ĳ�����л��ĳ���û�����Ϣ
	 switch_xml_t getMobUserXMLInfoInGroup(const char* mobuserid, switch_xml_t groupXMLinfo, switch_xml_t userXMLinfo);

	 //���defaultĿ¼���û�Ĭ�ϵ�xml�ṹ
	 switch_xml_t getMobDefaultXMLInfo(const char* mobuserid);

	 //���ļ��л�����õ���Ա��xml��Ϣ
	 switch_xml_t getAllDispatchersXMLInfo();

     //���һ������Ա��xml��Ϣ
	 switch_xml_t getDispatcherXMLInfo(const char* dispatcherid,switch_xml_t x_all);

	 //������еĵ���Ա��Ϣ
	 void getAllDispatchers(string &allDispatchers,bool bshowSeq = true);

	 //�Ƿ���ڵ���(Ա)conf\autoload_configsĿ¼��
	 bool isExistedDispatcher(const char* dispatcherid);
	 bool isExistedDispatcher(const char* dispatcherid,switch_xml_t x_dispatchers);

	 void getSingleMobInfo(switch_xml_t userXMLinfo, string &strOut);

	 //���ת����Ϣ
	 void getTransferInfo(const char* firstid,const char* transferinfo,int type,vector<string> &vecrecord);

	 //���mob user��ĳ������
	 char* getMobUserVarAttr(switch_xml_t userXMLinfo, const char* attName, bool bContainedUserNode = false);

	 //����mob user������,��Ҫ���default/Ŀ¼�µ�Ĭ�������ļ������޸�
	 bool setMobUserParamAttr(switch_xml_t userXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);
	 bool setMobUserVarAttr(switch_xml_t userXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);

	 //���õ���Ա�ļ�������
	 bool setdispatcherVarAttr(switch_xml_t dispathcersXml, const char* attVal, bool bAddattr = false);

	 //��дmob user��xml�ļ�����
	 bool rewriteMobUserXMLFile(const char* mobuserid, switch_xml_t x_mobuser, string &err);
};


#endif

