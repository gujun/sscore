#ifndef CMODULE_SYSINFO_H
#define CMODULE_SYSINFO_H

#include "CBaseModule.h"

#if defined(_WIN32) || defined(WIN32)
	#pragma comment(lib,"Ws2_32.lib")

//定义驱动空间的结构体
typedef struct  
{
	char m_FileSystem[256];
	char m_DriverType[16];
	int  m_TotalSize;
	int  m_UsedSize;
	int  m_FreeSize;
	char m_Usage[10];
}DriverStruct;

#define MAX_OF_HARD_DISKS	24
//static char HardDiskLetters[MAX_OF_HARD_DISKS][4]={
//	"c:\\",	"d:\\",	"e:\\",	"f:\\",	"g:\\",	"h:\\",
//	"i:\\",	"j:\\",	"k:\\",	"l:\\",	"m:\\",	"n:\\",
//	"o:\\",	"p:\\",	"q:\\",	"r:\\",	"s:\\",	"t:\\",
//	"u:\\",	"v:\\",	"w:\\",	"x:\\",	"y:\\",	"z:\\"
//};

#endif

//////////////////////////////////////////////////////////////////////////
/*设备系统的运行状况,包括CPU信息,内存使用情况,磁盘空间等信息
*/
class CModule_SysInfo :public CBaseModule
{
public:
	CModule_SysInfo();
	~CModule_SysInfo();

public://method
	void init();
	int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//接收命令
	int sendRespMsg(common_ResultResponse_Frame*& reqFrame);										//发送结果
	int cmdPro(const inner_CmdReq_Frame*& reqFrame);												//命令处理

protected:

  //method
private:
	//信息的查询,或"轮询"
	int querySysInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int queryCPUInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int queryMemoryInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int queryDiskInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	int queryUptimeInfo(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	int queryCPURate(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);
	
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
//其他,主要涉及到更新比较快的动态信息,可以采用"上报"方式进行
string extractValidStr(string strSource);
void getUpTime(string &strRunTime,string &strUserNum,string &strLoadedAver);
void getCPURate(string &strColName,/*string &strColValue*/char *chColVal);

//下面的方法主要是在windows环境下使用
#if defined(_WIN32) || defined(WIN32)
	void QueryOS(string &strOSKneral, string &strOSVersion);
	void QCpu();
	void QueryRAM(int &allRam,int &availRam,int &usedRam);
	void QueryDriveSpace(vector<DriverStruct> &driverVec);
#endif

#endif
