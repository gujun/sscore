#ifndef CMODULE_NEMANAGER_H
#define CMODULE_NEMANAGER_H

#include "CBaseModule.h"

/*网元管理以及操作用户管理,包括设备单元的处理等
*/
class CModule_NEManager :public CBaseModule
{
public:
	 CModule_NEManager();
	 ~CModule_NEManager();

public://method
	 int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//接收命令
	 int sendRespMsg(common_ResultResponse_Frame*& reqFrame);										//发送结果
	 int cmdPro(const inner_CmdReq_Frame*& reqFrame);												//命令处理

public:

protected:

  //method
private:
	//add by xxl 1010-5-19 :增加对操作用户的设置
	int addOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int delOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int modOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int lstOPUser(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	//add by xxl 1010-5-19 end

	//初始静态信息的查询
	int addEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int delEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int modEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int lstEquipNE(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	//其他操作,对网元的操作临时的先存放到cari_ccp_ne.xml文件中
	bool isExistedNE(int id);//应该根据id号,一个设备可能有多个ip地址
	bool isExistedNE(string ip);

	//操作用户
	bool isExistedOpUser(string username);

	//设置网元的属性或增加属性
	//bool setNEAttribute(switch_xml_t neXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);
};


#endif
