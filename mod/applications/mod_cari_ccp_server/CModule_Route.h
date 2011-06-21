#ifndef CMODULE_ROUTE_H
#define CMODULE_ROUTE_H

#include "CBaseModule.h"


/*路由管理模块,包括网关设置等信息处理
*/
class CModule_Route :public CBaseModule
{
public:
	 CModule_Route();
	 ~CModule_Route();

public://method
	 int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//接收命令
	 int sendRespMsg(common_ResultResponse_Frame*& reqFrame);           //发送结果

	 int cmdPro(const inner_CmdReq_Frame*& reqFrame); //命令处理

public:
	 //查看网关是否存在
	 bool isExistedGateWay(const char* profileType, const char* gatewayname);

protected:

  //method,其中:FromMem 是从内存中获得;FromFile 是从文件中获得
private:
	 int addGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int mobGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstGateWay(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //路由设置
	 int setRouter(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int delRouter(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame, bool bReLoadRoot = true);
	 int lstRouter(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //测试短信息功能
	 int sendMessage(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //获得所用的网关信息
	 void getAllGatewaysFromMem(char** allGateways);

	 //创建网关的配置文件
	 bool creatGateWayXMLFile(const char* profileType, const char* gatewayname, const char* filecontext, const char** err);

	 //删除网关文件
	 bool delGateWayXMLFile(const char* profileType, const char* gatewayname, const char** err);

	 //设置gateway xml的属性
	 bool setGatewayAttribute(switch_xml_t userXMLinfo, const char* attName, const char* attVal, bool bAddattr = false);

	 //获得某项属性
	 char* getGatewayParamAttribute(switch_xml_t gatewayXMLinfo, const char* attName);

	 //获得某个目录下dateway的xml结构
	 switch_xml_t getGatewayXMLInfo(const char* profileType, const char* gatewayname);

	 //设置默认拨号路由的属性
	 bool setDialPlanAttr(switch_xml_t xcontext, const char* externName, const char* condiFieldName, const char* expressVal);

	 //设置out拨号路由
	 bool setOutDialPlanAttr(switch_xml_t xextension, const char* condiFieldVal, const char* expressVal);

	 //获得"本局"路由信息
	 void getLocalRouterInfo(switch_xml_t xDialContext, string &strOut);

	 //获得与"调度"有关的路由信息:只呼叫调度和拨入调度台
	 void getDispatchRouterInfo(switch_xml_t xDialContext, int type, string &strOut);

	 //获得"出局"的路由信息
	 void getOutRouterInfo(switch_xml_t xOutDialContext, string &strOut);

	 //删除出局路由
	 bool delOutRouter(bool bSpecial,string gateway,/*string &strErr*/char *&pDesc);

	 //修改出局路由
	 bool modOutRouter(string oldGateway,string newGateway,/*string &strErr*/char *&pDesc);
};


#endif
