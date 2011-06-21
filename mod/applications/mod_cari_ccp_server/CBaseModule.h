#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"


#ifndef C_BASEMODULE_H
#define C_BASEMODULE_H

//////////////////////////////////////////////////////////////////////////
#define INSERT_INTOXML_POS		1000    //switch_xml_insert()函数插入节点的位置,考虑放置到最后

#define DEFAULT_GROUP_NAME		"default"   //默认的组名,不需要考虑
#define DIR_DEFINE_GROUPS		"groups"    //自定义组的总目录

#define DIAPLAN_GROUP_XMLFILE	"02_group_dial" //diaplan group xml文件的名称,conf\dialplan\default\目录下
#define DEFAULT_DIRECTORY		"default"		//默认的目录,conf\...\default\目录下
#define USER_DIRECTORY			"directory"		//存放用户xml配置文件的根目录,conf\directory
#define DIAPLAN_DIRECTORY		"dialplan"		//存放用户xml配置文件的根目录,conf\dialplan

#define SIP_PROFILE_DIRECTORY	"sip_profiles"	//conf\sip_profiles


//////////////////////////////////////////////////////////////////////////
extern "C"
{
  //初始化环境信息
  void cari_net_init();
}


/*声明一个BaseClass,所有的子模块继承此类
*/
class CBaseModule
{
public:
  CBaseModule();
  ~CBaseModule();

  //继承使用的函数.
  //method基类提供相关的接口函数,能够进行runtime期执行识别
public:
	  virtual int receiveReqMsg(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame); 
	  virtual int sendRespMsg(common_ResultResponse_Frame*& respFrame);
	  virtual int cmdPro(const inner_CmdReq_Frame*& reqFrame);

  //通用的公共函数
public:

	  //获得domain名
	  char* getDomainIP();

	  //获得mob user的xml文件的存放目录,此处的目录是指默认目录
	  //conf\directory\default,所有的用户都必须在此目录下存放完整的xml文件
	  char* getMobUserDefaultXMLPath();

	  //获得用户的default/目录下的默认配置xml文件
	  char* getMobUserDefaultXMLFile(const char* mobusername);

	  //获得用户所属组的完整文件路径
	  char* getMobUserGroupXMLFile(const char* groupname, const char* mobusername);

	  //获得组的目录
	  char* getMobGroupDir(const char* groupname);

	  //获得default的文件路径
	  char* getDirectoryDefaultXMLFile();

	  //获得dia group的xml文件
	  char* getDirectoryDialGroupXMLFile();

	  //获得conf\sip_profiles目录
	  char* getSipProfileDir();

	  //获得调度的文件conf\autoload_configs\dispat.conf.xml
	  char* getDispatFile();

	  //校验登录用户
	  bool isLoginSuc(string username,string userpwd);

protected:

private:
	int login(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//输出参数,响应帧
	int queryClientID(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int reboot(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int shakehand(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

private:
	char *m_localIP;
};

#endif
