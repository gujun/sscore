#ifndef CMODULE_USER_H
#define CMODULE_USER_H

#include "CBaseModule.h"

//#define CARI_ODBC  //标识是否采用数据库的方式

/*用户管理模块,包括用户信息,用户组的信息管理等
 */

class CModule_User :public CBaseModule
{
public:	
	 CModule_User();
	 ~CModule_User();

public://method
	 int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//接收命令
	 int sendRespMsg(common_ResultResponse_Frame*& reqFrame);           //发送结果
	 int cmdPro(const inner_CmdReq_Frame*& reqFrame); //命令处理

public:

protected:

  //method,其中:FromMem 是从内存中获得;FromFile 是从文件中获得
private:

	 //配置命令 :具体命令对应的处理(入口)函数
	 int addMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int mobMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //内部命令 :监控模块初始打开的时候,进行查询当前用户的信息
	 int queryMobUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //针对组的处理
	 int addMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int delMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int modMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int lstMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	 int setMobGroup(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);

	 //转呼设置
	 int setTransferCall(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int unbindTransferCall(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int lstTransferCallInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //调度员的设置
	 int addDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int delDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int setDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame,bool bReLoadRoot = true);
	 int lstDispatcher(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //录音用户的设置
	 int addRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int setRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstRecord(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 /************************************************************************/
	 /*                                                                      */
	 /************************************************************************/
	 //是否本地注册的有效用户
	 bool isLocalMobUser(string strUserName);

	 //查看用户是否存在
	 bool isExistedMobUser(const char* userid);

	 //查看用户是否已经注册
	 bool isRegisteredMobUser(const char* userID,switch_core_db_t *db);

	 //查看组是否存在
	 bool isExistedGroup(const char* mobgroupname = "", int groupid = -1);

	 //获得组号
	 string getGroupID(const char* mobgroupname);

	 //去除包含的字串,获得很多字串
	 string getNewStr(const char* oldStr,const char* delStr);

	 //获得用户的组的信息
	 void getMobGroupOfUserFromMem(const char* mobuserid, char** group);

	 //获得某个组的所有用户信息
	 void getMobUsersOfGroupFromMem(const char* mobgroupname, char** allMobUsers);

	 //获得当前域下的所有组的信息
	 void getAllGroupFromMem(string &allGroups,bool bshowSeq = true);

	 //获得组的数量
	 int getGroupNumFromMem();

	 //获得组的信息
	 switch_xml_t getGroupXMLInfoFromMem(const char* groupname, switch_xml_t groupXMLinfo, string &err);

	 //获得用户的信息,从root xml的内存结构中获得
	 switch_xml_t getMobUserXMLInfoFromMem(const char* mobuserid, switch_xml_t userXMLinfo);

	 //从某个组中获得某个用户的信息
	 switch_xml_t getMobUserXMLInfoInGroup(const char* mobuserid, switch_xml_t groupXMLinfo, switch_xml_t userXMLinfo);

	 //获得default目录下用户默认的xml结构
	 switch_xml_t getMobDefaultXMLInfo(const char* mobuserid);

	 //从文件中获得所用调度员的xml信息
	 switch_xml_t getAllDispatchersXMLInfo();

     //获得一个调度员的xml信息
	 switch_xml_t getDispatcherXMLInfo(const char* dispatcherid,switch_xml_t x_all);

	 //获得所有的调度员信息
	 void getAllDispatchers(string &allDispatchers,bool bshowSeq = true);

	 //是否存在调度(员)conf\autoload_configs目录下
	 bool isExistedDispatcher(const char* dispatcherid);
	 bool isExistedDispatcher(const char* dispatcherid,switch_xml_t x_dispatchers);

	 void getSingleMobInfo(switch_xml_t userXMLinfo, string &strOut);

	 //获得转呼信息
	 void getTransferInfo(const char* firstid,const char* transferinfo,int type,vector<string> &vecrecord);

	 //获得mob user的某项属性
	 char* getMobUserVarAttr(switch_xml_t userXMLinfo, const char* attName, bool bContainedUserNode = false);

	 //设置mob user的属性,主要针对default/目录下的默认配置文件进行修改
	 bool setMobUserParamAttr(switch_xml_t userXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);
	 bool setMobUserVarAttr(switch_xml_t userXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);

	 //设置调度员文件的属性
	 bool setdispatcherVarAttr(switch_xml_t dispathcersXml, const char* attVal, bool bAddattr = false);

	 //重写mob user的xml文件内容
	 bool rewriteMobUserXMLFile(const char* mobuserid, switch_xml_t x_mobuser, string &err);
};


#endif

