#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"


#ifndef C_BASEMODULE_H
#define C_BASEMODULE_H

//////////////////////////////////////////////////////////////////////////
#define INSERT_INTOXML_POS		1000    //switch_xml_insert()��������ڵ��λ��,���Ƿ��õ����

#define DEFAULT_GROUP_NAME		"default"   //Ĭ�ϵ�����,����Ҫ����
#define DIR_DEFINE_GROUPS		"groups"    //�Զ��������Ŀ¼

#define DIAPLAN_GROUP_XMLFILE	"02_group_dial" //diaplan group xml�ļ�������,conf\dialplan\default\Ŀ¼��
#define DEFAULT_DIRECTORY		"default"		//Ĭ�ϵ�Ŀ¼,conf\...\default\Ŀ¼��
#define USER_DIRECTORY			"directory"		//����û�xml�����ļ��ĸ�Ŀ¼,conf\directory
#define DIAPLAN_DIRECTORY		"dialplan"		//����û�xml�����ļ��ĸ�Ŀ¼,conf\dialplan

#define SIP_PROFILE_DIRECTORY	"sip_profiles"	//conf\sip_profiles


//////////////////////////////////////////////////////////////////////////
extern "C"
{
  //��ʼ��������Ϣ
  void cari_net_init();
}


/*����һ��BaseClass,���е���ģ��̳д���
*/
class CBaseModule
{
public:
  CBaseModule();
  ~CBaseModule();

  //�̳�ʹ�õĺ���.
  //method�����ṩ��صĽӿں���,�ܹ�����runtime��ִ��ʶ��
public:
	  virtual int receiveReqMsg(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame); 
	  virtual int sendRespMsg(common_ResultResponse_Frame*& respFrame);
	  virtual int cmdPro(const inner_CmdReq_Frame*& reqFrame);

  //ͨ�õĹ�������
public:

	  //���domain��
	  char* getDomainIP();

	  //���mob user��xml�ļ��Ĵ��Ŀ¼,�˴���Ŀ¼��ָĬ��Ŀ¼
	  //conf\directory\default,���е��û��������ڴ�Ŀ¼�´��������xml�ļ�
	  char* getMobUserDefaultXMLPath();

	  //����û���default/Ŀ¼�µ�Ĭ������xml�ļ�
	  char* getMobUserDefaultXMLFile(const char* mobusername);

	  //����û�������������ļ�·��
	  char* getMobUserGroupXMLFile(const char* groupname, const char* mobusername);

	  //������Ŀ¼
	  char* getMobGroupDir(const char* groupname);

	  //���default���ļ�·��
	  char* getDirectoryDefaultXMLFile();

	  //���dia group��xml�ļ�
	  char* getDirectoryDialGroupXMLFile();

	  //���conf\sip_profilesĿ¼
	  char* getSipProfileDir();

	  //��õ��ȵ��ļ�conf\autoload_configs\dispat.conf.xml
	  char* getDispatFile();

	  //У���¼�û�
	  bool isLoginSuc(string username,string userpwd);

protected:

private:
	int login(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//�������,��Ӧ֡
	int queryClientID(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int reboot(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int shakehand(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

private:
	char *m_localIP;
};

#endif
