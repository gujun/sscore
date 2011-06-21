#ifndef CMODULE_SYSINFO_H
#define CMODULE_SYSINFO_H

#include "CBaseModule.h"

#if defined(_WIN32) || defined(WIN32)
	#pragma comment(lib,"Ws2_32.lib")

//���������ռ�Ľṹ��
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
/*�豸ϵͳ������״��,����CPU��Ϣ,�ڴ�ʹ�����,���̿ռ����Ϣ
*/
class CModule_SysInfo :public CBaseModule
{
public:
	CModule_SysInfo();
	~CModule_SysInfo();

public://method
	void init();
	int receiveReqMsg(inner_CmdReq_Frame*& reqFrame, inner_ResultResponse_Frame*& inner_RespFrame);//��������
	int sendRespMsg(common_ResultResponse_Frame*& reqFrame);										//���ͽ��
	int cmdPro(const inner_CmdReq_Frame*& reqFrame);												//�����

protected:

  //method
private:
	//��Ϣ�Ĳ�ѯ,��"��ѯ"
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
//����,��Ҫ�漰�����±ȽϿ�Ķ�̬��Ϣ,���Բ���"�ϱ�"��ʽ����
string extractValidStr(string strSource);
void getUpTime(string &strRunTime,string &strUserNum,string &strLoadedAver);
void getCPURate(string &strColName,/*string &strColValue*/char *chColVal);

//����ķ�����Ҫ����windows������ʹ��
#if defined(_WIN32) || defined(WIN32)
	void QueryOS(string &strOSKneral, string &strOSVersion);
	void QCpu();
	void QueryRAM(int &allRam,int &availRam,int &usedRam);
	void QueryDriveSpace(vector<DriverStruct> &driverVec);
#endif

#endif
