#include "CModule_SysInfo.h"
#include "mod_cari_ccp_server.h"
#include "cari_net_event_process.h"


#define NCPUSTATES /*5*/ 8

static long cp_time[NCPUSTATES]; 
static long cp_old[NCPUSTATES]; 
static long cp_diff[NCPUSTATES]; 
int cpu_states[NCPUSTATES]; 

const char* cpu_colName[NCPUSTATES] = {0};

//************************* begin 只在windows环境下使用的方法*************************************//
#if defined(_WIN32) || defined(WIN32)

#include <windows.h>
#include <direct.h>
#include "cpu_info.h"

#define GHZ_UNIT (1000*1000*1000)
#define MHZ_UNIT (1000*1000)

typedef struct tagTHREADINFO {
	int nProcessor;
} THREADINFO, *PTHREADINFO;

HANDLE l_semaphore_sync;		  //同步/异步线程之间使用的信号量
static vector<string> cpu_modelVec,cpu_SpeedVec;//使用全局容器存放cpu的信息
CRITICAL_SECTION   m_cpuLock;	 //锁标识,涉及到上面的容器争用问题

CPUInfo usageA;
int nProcessors = 1;//CPU的数量

/*初始化一些属性(变量)
*/
void initAttibute()
{
	l_semaphore_sync = CreateSemaphore(NULL, 0, 1, NULL);

	//InitializeCriticalSection(m_cpuLock); //Initialize the critical section.
	::InitializeCriticalSection(&m_cpuLock);
}

/*删除一些属性(变量)
*/
void clearAttibute()
{
	//DeleteCriticalSection(m_cpuLock);    //Delete Critical section
	::DeleteCriticalSection(&m_cpuLock);
}

//针对每个CPU单独启动一个线程,线程回调的函数
DWORD WINAPI DisplayCPUInformation(LPVOID lpParameter)
{ 
	//开始"上锁"
	::EnterCriticalSection(&m_cpuLock);//Enter Critical section

	PTHREADINFO ThreadInfo =(PTHREADINFO) lpParameter;
	int id = ThreadInfo->nProcessor;//编号
	CPUInfo * CPU = new CPUInfo();
	char szText[128];
	// First put the processor it into the root of the tree.
	sprintf(szText, "Processor %d", ThreadInfo->nProcessor);

	if(!CPU->DoesCPUSupportCPUID()) {
		// Inform the user that we cannot get any information.
		sprintf(szText, "BAD_CPUID");

		// Cleanup memory allocations.
		delete CPU;

		// Terminate the thread.
		return 0;
	}

	string str;

	//add by xxl 2009-12-29:使用下面的方式获得
	CPUID cpu;
	string vid = cpu.GetVID();
	string brand = cpu.GetBrand();
	LONGLONG freq = cpu.GetFrequency();
	//if (freq > GHZ_UNIT)
	//{
	//	double freqGHz = (double)freq/GHZ_UNIT;
	//	printf("%.2fGHz\n",freqGHz); //保留小数点后两位输出
	//}
	//else
	//{
	//	cout<<freq/MHZ_UNIT<<"MHz"<<endl;
	//}

	// Add the processor name - either extended or classical.
	sprintf(szText, "Processor Name: %s", CPU->GetExtendedProcessorName());
	cpu_modelVec.push_back(brand);

	// Add the processor clock frequency.
	sprintf(szText, "Clock Frequency: %d MHz", CPU->GetProcessorClockFrequency());

	double freqGHz = (double)freq/MHZ_UNIT;
	sprintf(szText, "%.0fMHz", freqGHz);
	str =szText;
	cpu_SpeedVec.push_back(str);

	// Display the processor serial number.
	//if(CPU->DoesCPUSupportFeature(SERIALNUMBER_FEATURE)) {
	//sprintf(szText, "Serial Number: %s", CPU->GetProcessorSerialNumber());
	//}

	// Remove the CPUInfo object as we no longer need it.
	delete CPU;

	//最后一个线程的执行完毕,需要通知主线程继续执行
	if (id == nProcessors){
		ReleaseSemaphore(l_semaphore_sync, 1, NULL);//释放信号量
	}

	//"解锁"
	::LeaveCriticalSection(&m_cpuLock);//Leave Critical section

	return 0;
}

typedef long long           int64_t;  
typedef unsigned long long  uint64_t; 

//时间转换  
static uint64_t file_time_2_utc(const FILETIME* ftime)  
{  
	LARGE_INTEGER li;  

	assert(ftime);  
	li.LowPart = ftime->dwLowDateTime;  
	li.HighPart = ftime->dwHighDateTime;  
	return li.QuadPart;  
}  

//获得CPU的核数  
static int get_processor_number()  
{  
	SYSTEM_INFO info;  
	GetSystemInfo(&info);  
	return (int)info.dwNumberOfProcessors;  
}  

//本方法比较可靠
int get_cpu_usage()  
{  
	//cpu数量  
	static int processor_count_ = -1;

	//上一次的时间  
	static int64_t last_time_ = 0;  
	static int64_t last_system_time_ = 0;  

	FILETIME now;  
	FILETIME creation_time;  
	FILETIME exit_time;  
	FILETIME kernel_time;  
	FILETIME user_time;  
	int64_t system_time;  
	int64_t time;  
	int64_t system_time_delta;  
	int64_t time_delta;  

	int cpu = -1;
	if(processor_count_ == -1){  
		processor_count_ = get_processor_number();
	}  

	GetSystemTimeAsFileTime(&now);  

	if (!GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time,  
		&kernel_time, &user_time)){  
		// We don't assert here because in some cases (such as in the Task   

		//Manager)  
		// we may call this function on a process that has just exited but   

		//we have  
		// not yet received the notification.  
		return -1;  
	} 
	system_time = (file_time_2_utc(&kernel_time) + file_time_2_utc(&user_time))
		/  
		processor_count_;  
	time = file_time_2_utc(&now);  

	if ((last_system_time_ == 0) || (last_time_ == 0)){  
		// First call, just set the last values.  
		last_system_time_ = system_time;  
		last_time_ = time;  
		return -1;  
	}  

	system_time_delta = system_time - last_system_time_;  
	time_delta = time - last_time_;  

	assert(time_delta != 0);  

	if (time_delta == 0)  
		return -1;  

	// We add time_delta / 2 so the result is rounded.  
	cpu = (int)((system_time_delta * 100 + time_delta / 2) / time_delta);  
	last_system_time_ = system_time;  
	last_time_ = time;  
	return cpu;  
}

#include <stdio.h>   
#include <conio.h>   
#include <tchar.h>   
#include <pdh.h>   
#pragma comment ( lib , "Pdh.lib" )   
#define MAXPATH 80   

//特定进程的CPU占用率
int getProcCPUUsage()
{
	HQUERY hQuery;   
	HCOUNTER *pCounterHandle;   
	PDH_STATUS pdhStatus;   
	PDH_FMT_COUNTERVALUE fmtValue;   
	DWORD ctrType;
	CHAR szPathBuffer[MAXPATH] = {'\0'};
	int nRetCode = 0;
	 // Open the query object.   
	pdhStatus = PdhOpenQuery (0, 0, &hQuery);    
	pCounterHandle = (HCOUNTER *)GlobalAlloc(GPTR, sizeof(HCOUNTER));   

/*  
				23. \\Processor(_Total)\\% Processor Time CPU使用率  
				24. \\System\\Processes 当前系统进程数  
				25. \\System\\Threads 当前系统线程数  
				26. \\Memory\\Commit Limit 总共内存数K (包括虚拟内存)  
				27. \\Memory\\Committed Bytes 已用内存数K (包括虚拟内存)  
				28. \\TCP\\Connections Active 系统中已建立的 TCP连接个数  
				29. \\其它Object Items 可以利用PdhEnumObjects()和PdhEnumObjectItems()得到  
				30.  
				31. */  
				// strcat(szPathBuffer,"\\System\\Processes");    
				   
				 // pdhStatus = PdhAddCounter (hQuery, szPathBuffer, 0, pCounterHandle);   
				//得到QQ进程的CPU占用率,这句出错   
	pdhStatus = PdhAddCounter(hQuery,_T("\\Process(FreeSwitch)\\% Processor Time"),0,pCounterHandle);   
		
	// "Prime" counters that need two values to display a    
	// formatted value.   
	pdhStatus = PdhCollectQueryData (hQuery);    
	// Get the current value of this counter.   
	 pdhStatus = PdhGetFormattedCounterValue (*pCounterHandle, PDH_FMT_DOUBLE,   
	&ctrType,   
	&fmtValue);   
	
	 //fmtValue.doubleValue为所要的结果   
	
	 if (pdhStatus == ERROR_SUCCESS) {   
		//printf (TEXT(",\"%.20g\"\n"), fmtValue.doubleValue);   
		
	 }   
	 else {   
	
		 // Print the error value.   
		//printf (TEXT("error.\"-1\""));    
		}   
	 // Close the query.   
	 pdhStatus = PdhCloseQuery (hQuery);     
	
	 return nRetCode;   
}

#endif
//**************************** end windows环境*************************************//


//////////////////////////////////////////////////////////////////////////
static inline char * skip_token(const char *p) 
{ 
	while(isspace(*p)) 
		p++;
	while(*p && !isspace(*p)) 
		p++;

	return(char *)p;
} 

/*百分比
*/
long percentages(int cnt, 
				 int *out, 
				 register long *newData, 
				 register long *oldData, 
				 long *diffs) 
{ 
    register int i; 
    register long change; 
    register long total_change; 
    register long *dp; 
    long half_total; 

    /* initialization */ 
    total_change = 0; 
    dp = diffs; 

    /* calculate changes for each state and the overall change */ 
    for(i = 0; i < cnt; i++) { 
		if((change = *newData - *oldData) < 0) { 
			/* this only happens when the counter wraps */ 
			change =(int) 
			((unsigned long)*newData-(unsigned long)*oldData); 
		} 
		total_change +=(*dp++ = change); 
		*oldData++ = *newData++; 
    } 

    /* avoid divide by zero potential */ 
    if(total_change == 0){ 
		total_change = 1; 
    } 

    /* calculate percentages based on overall change, rounding up */ 
    half_total = total_change / 2l; 
    half_total = 0; 
    for(i = 0; i < cnt; i++) { 
		//printf("dd %ld %ld\n",(*diffs* 1000 + half_total),total_change); 
		*out++ =(int)((*diffs++ * 1000 + half_total) / total_change); 
    } 

    /* return the total in case the caller wants to use it */ 
    return(total_change); 
}


/*线程的主体函数
*主动上报一些变化频繁的信息,如:CPU的使用情况等
*/
void* thread_sysInfoInit(void *attr)
{
	int clientModuleId = 12;
	//string strSavedCpu = "NULL";
	string strCpuRate,strColName;
	char chColVal[100];

	//下面是间隔时间,查询动态的信息
	while(loopFlag){

		//根据间隔时间进行轮询一次,"毫秒"
		CARI_SLEEP(2000);//2秒

		if (0 == global_client_num){
			//WaitForSingleObject(m_sysInfoSemaphore, INFINITE);
			continue;
		}

		//1: CPU的占用率上报处理
		memset(chColVal,0,sizeof(chColVal));
		getCPURate(strColName,/*strCpuRate*/chColVal);//CPU的使用率

		////判断是否发生了改变,如果改变的话,则上报,否则不上报(这样处理有问题,摒弃!!!)
		//if (isEqualStr(strSavedCpu,strCpuRate)){
		//	continue;
		//}
		//strSavedCpu = strCpuRate;

		//new响应帧
		inner_ResultResponse_Frame *inner_respFrame = CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);
		initDownRespondFrame(inner_respFrame,clientModuleId,CARICCP_QUERY_CPU_RATE);
		myMemcpy(inner_respFrame->header.strCmdName,"cpu rate",sizeof(inner_respFrame->header.strCmdName));
		strCpuRate = chColVal;
		inner_respFrame->m_vectTableValue.push_back(strCpuRate);

		//将主动上报给对应的客户端
		CMsgProcess::getInstance()->commonSendRespMsg(inner_respFrame,DOWN_RES_TYPE);
		//printf("CPU格式 :%s\n",strCpuRate.c_str());

		//2: 用户的数量上报处理

		//3: 内存的使用情况上报

		//4: 磁盘控件的使用情况上报

		//销毁"响应帧"
		CARI_CCP_VOS_DEL(inner_respFrame);
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
CModule_SysInfo::CModule_SysInfo()
	: CBaseModule()
{
	init();
}

CModule_SysInfo::~CModule_SysInfo()
{
#if defined(_WIN32) || defined(WIN32)
	clearAttibute();
#endif
}

/*初始化,包括一些全局属性等设置
*/
void CModule_SysInfo::init()
{
#if defined(_WIN32) || defined(WIN32)
	initAttibute();
#endif

	pthread_t pth;
	int iRet= pthread_create(&pth,NULL,thread_sysInfoInit,NULL);
	if(CARICCP_SUCCESS_STATE_CODE != iRet){//可考虑多创建几次
	} 
}

/*根据接收到的帧,处理具体的命令,分发到具体的处理函数
*/
int CModule_SysInfo::receiveReqMsg(inner_CmdReq_Frame *&inner_reqFrame, inner_ResultResponse_Frame *&inner_RespFrame)
{
	if(NULL == inner_reqFrame) {
		//帧为空
		return CARICCP_ERROR_STATE_CODE;
	}

	//请求帧的"源模块"号则是响应帧的"目的模块"号
	//请求帧的"目的模块"号则是响应帧的"源模块"号,可逆的
	inner_RespFrame->sSourceModuleID = inner_reqFrame->sDestModuleID;
	inner_RespFrame->sDestModuleID = inner_reqFrame->sSourceModuleID;


	int				iFunRes		= CARICCP_SUCCESS_STATE_CODE;
	bool	bRes		= true; 

	//判断命令码来判断命令(命令码是惟一的),也可以根据命令名进行设置
	int				iCmdCode	= inner_reqFrame->body.iCmdCode;
	switch(iCmdCode) {
	case CARICCP_QUERY_SYSINFO:
		iFunRes = querySysInfo(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_UPTIME:
		iFunRes = queryUptimeInfo(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_CPU_INFO:
		iFunRes = queryCPUInfo(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_MEMORY_INFO:
		iFunRes = queryMemoryInfo(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_DISK_INFO:
		iFunRes = queryDiskInfo(inner_reqFrame, inner_RespFrame);
		break;

	case CARICCP_QUERY_CPU_RATE:
		iFunRes = queryCPURate(inner_reqFrame, inner_RespFrame);
		break;

	default:
		break;
	};


	//命令执行不成功
	if(CARICCP_SUCCESS_STATE_CODE != iFunRes) {
		inner_RespFrame->header.bNotify = false;
	} else {
		inner_RespFrame->header.bNotify = inner_reqFrame->body.bNotify;
	}

	/*--------------------------------------------------------------------------------------------*/
	//将处理的结果发送给对应的客户端
	//在命令的接收入口函数中统一处理

	//打印日志
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "proc cmd %s end.\n", inner_reqFrame->body.strCmdName);
	return iFunRes;
}

/*发送信息给对应的客户端
*/
int CModule_SysInfo::sendRespMsg(common_ResultResponse_Frame *&respFrame)
{
	int	iFunRes	= CARICCP_SUCCESS_STATE_CODE;
	return iFunRes;
}

int CModule_SysInfo::cmdPro(const inner_CmdReq_Frame *&reqFrame)
{
	return CARICCP_SUCCESS_STATE_CODE;
}

/*查询当前的运行环境,包括OS信息和版本等
*/
int CModule_SysInfo::querySysInfo(inner_CmdReq_Frame*& reqFrame, 	  
								  inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";

	string strTmp,strUpTime,strUserNum,strLoadedAver;
	vector<string> vec;
	char			*chTmp = NULL, *strTableName = NULL, *strRes=NULL;
	char hostname[256];
	char *domain_name = NULL,*kernel=NULL,*distroName=NULL/*,*kernelRelease=NULL,*kernelVersion=NULL,*hardwarePlatform=NULL,*kernel=NULL,*distroName=NULL*/;
	string kernelRelease,kernelVersion,hardwarePlatform;
	string strTColHostName,strTColHostIP,strTColHostKernal,strTColHostVer,strTColHostRuntime,strTColHostUsernum,strTColHostLoad;

	memset(hostname,0,sizeof(hostname));
	//使用下面的逻辑进行查询
	//主机名hostname
	gethostname(hostname,sizeof(hostname));

	//监听的ip地址,根据当前的域名获得
	domain_name = getDomainIP();

#if defined(_WIN32) || defined(WIN32)

	 QueryOS(hardwarePlatform, kernelVersion);
	 kernel = switch_mprintf("%s", hardwarePlatform.c_str());
	 distroName = switch_mprintf("%s",kernelVersion.c_str());

#else

	//kernel version信息
	cari_common_getSysCmdResult("uname -r",kernelRelease);   //格式 :2.6.18-128.el5xen
	cari_common_getSysCmdResult("uname -v",kernelVersion);   //格式 :#1 SMP Wed Jan 21 11:12:42 EST 2009
	cari_common_getSysCmdResult("uname -p",hardwarePlatform);//格式 :x86_64
	trim(kernelRelease);
	trim(kernelVersion);
	trim(hardwarePlatform);

	strTmp = kernelVersion;
	splitString(strTmp,CARICCP_SPACEBAR,vec);
	strTmp = getValueOfVec(1,vec);
	kernel = switch_mprintf("%s%s%s%s%s",
		kernelRelease.c_str(),
		"(",
		strTmp.c_str(),
		") ",
		hardwarePlatform.c_str());

	//Description: CentOS release 5.3(Final)
	cari_common_getSysCmdResult("lsb_release -d",strTmp);
	strTmp = getValueOfStr(CARICCP_COLON_MARK,1,strTmp);
	distroName = switch_mprintf("%s",strTmp.c_str());

#endif

	//系统的运行时间,当前用户数,以及平均加载时间峰值
	getUpTime(strUpTime,strUserNum,strLoadedAver);

	//设置返回的结果值
	strRes = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s%s%s", //内容
		hostname, CARICCP_SPECIAL_SPLIT_FLAG, 
		domain_name, CARICCP_SPECIAL_SPLIT_FLAG, 
		kernel,CARICCP_SPECIAL_SPLIT_FLAG,
		distroName,CARICCP_SPECIAL_SPLIT_FLAG, 
		strUpTime.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strUserNum.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strLoadedAver.c_str());

	inner_RespFrame->m_vectTableValue.push_back(strRes);

	//测试test 大数据量发送问题
	//for(int i=0;i<=320;i++){
	//	inner_RespFrame->m_vectTableValue.push_back(strRes);
	//}

	//返回结果信息
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	inner_RespFrame->iResultNum = inner_RespFrame->m_vectTableValue.size();
	strReturnDesc = getValueOfDefinedVar("SYS_INFO");

	//设置表头字段
	strTColHostName	=getValueOfDefinedVar("SYS_HOST_NAME");
	strTColHostIP		=getValueOfDefinedVar("SYS_HOST_IP");
	strTColHostKernal	=getValueOfDefinedVar("SYS_HOST_KERNAL");
	strTColHostVer		=getValueOfDefinedVar("SYS_HOST_VERSION");
	strTColHostRuntime =getValueOfDefinedVar("SYS_HOST_RUNTIME");
	strTColHostUsernum =getValueOfDefinedVar("SYS_USER_NUM");
	strTColHostLoad	=getValueOfDefinedVar("SYS_LOAD");
	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s%s%s", //表头
		strTColHostName.c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		strTColHostIP.c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		strTColHostKernal.c_str()	,CARICCP_SPECIAL_SPLIT_FLAG,
		strTColHostVer.c_str()		,CARICCP_SPECIAL_SPLIT_FLAG, 
		strTColHostRuntime.c_str()	,CARICCP_SPECIAL_SPLIT_FLAG,
		strTColHostUsernum.c_str()	,CARICCP_SPECIAL_SPLIT_FLAG,
		strTColHostLoad.c_str());
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	//释放内存
	switch_safe_free(kernel);
	switch_safe_free(distroName);
	switch_safe_free(strRes);
	switch_safe_free(strTableName);
	return iFunRes;
}

/*查询CPU的使用情况
*/
int CModule_SysInfo::queryCPUInfo(inner_CmdReq_Frame*& reqFrame, 
								  inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";
	const char		*err[1]									= {
		0
	};

	char			*chTmp									= NULL, *strTableName=NULL;
	string strColName="",strCpuRate="",strCpuRateValue;
	string strRes,strTmp,strBak,strTmpVal,strNew,strNew_1,strCpuModel,strCpuSpeed,strCpuCashSize,strCpuBogomips;
	string strTColCPUModel,strTColCPUCashSize,strTColCPUSpeed,strTColCPUSysBog;
	vector<string> vec,vecData,vecHeader,vecCpuModel,vecCpuSpeed,vecCpuCashSize,vecCpuBogomips;
	int iCpuCount=1;
	

#if defined(_WIN32) || defined(WIN32)
 
	//特殊处理
	cpu_modelVec.clear();
	cpu_SpeedVec.clear();
	int inum = 0;

rebegin:
    //有些CPU的信息不能识别,此处暂时不处理
	QCpu();

	//此时需要cpu的线程都执行完了,再继续进行执行
	WaitForSingleObject(l_semaphore_sync, 5 * 1000);//当前线程等待,减少信号量,单位:毫秒

	//由于上面的处理是启动了线程,造成时间上的延迟,因此此处要特殊处理一下
	for (;inum < 3; inum++){
		if (0 == cpu_modelVec.size()){
			inum++;
			//Sleep(inum*1000);
			goto rebegin;
		}
	}

	strRes = "";
	iCpuCount = cpu_modelVec.size();//容器的数量相同大小
	for(int i=0;i< iCpuCount;i++){
		strCpuModel = getValueOfVec(i,cpu_modelVec);
		strCpuSpeed = getValueOfVec(i,cpu_SpeedVec);
		trim(strCpuModel);
		trim(strCpuSpeed);
		char *pRes = switch_mprintf("%s%s%s",
			strCpuModel.c_str(),
			CARICCP_SPECIAL_SPLIT_FLAG,
			strCpuSpeed.c_str());

		inner_RespFrame->m_vectTableValue.push_back(pRes);
		switch_safe_free(pRes);
	}

	//设置表头字段
	strTColCPUModel	=getValueOfDefinedVar("CPU_MODEL");
	strTColCPUSpeed	=getValueOfDefinedVar("CPU_SPEED");
	strTableName = switch_mprintf("%s%s%s",
		strTColCPUModel.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColCPUSpeed.c_str());

#else//linux系统的使用
	int idex;

	//cpu type :格式如下:
	//model name      : Intel(R) Xeon(R) CPU           E5405  @ 2.00GHz
	//model name      : Intel(R) Xeon(R) CPU           E5405  @ 2.00GHz
	cari_common_getSysCmdResult("grep -i \"model name\" /proc/cpuinfo",strRes);
	//printf("CPU 类型\n%s",strRes.c_str());

	replace_all(strRes, CARICCP_NEWLINE_CR, CARICCP_NEWLINE_LF);
	splitString(strRes,CARICCP_NEWLINE_LF,vec);
	for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		strTmp = *it;
		trim(strTmp);
		idex = strTmp.find(CARICCP_COLON_MARK);
		if(0 <= idex){
			strNew = strTmp.substr(idex+1);
		}
		else{
			strNew = strTmp;
		}
		trim(strNew);

		strTmp = strNew;
		strBak = strTmp;
		stringToLower(strTmp);//先转换成小写
		idex = strTmp.find("cpu");//关键字符cpu
		if(0 <= idex){
			strNew = strBak.substr(0,idex+2);
			strNew_1 = strBak.substr(idex+3);
		}
		else{
			strNew = strBak;
			strNew_1 = "";
		}

		//重新封装信息
		trim(strNew);
		trim(strNew_1);
		strTmpVal = "";
		strTmpVal.append(strNew);
		strTmpVal.append("  ");
		strTmpVal.append(strNew_1);

		vecCpuModel.push_back(strTmpVal);//保持值到容器中
	}

	//cpu MHz,格式如下:
	//cpu MHz         : 1995.067
	//cpu MHz         : 1995.067
	vec.clear();
	cari_common_getSysCmdResult("grep -i \"cpu MHz\" /proc/cpuinfo",strRes);
	/*printf("CPU MHz\n%s",strRes.c_str());*/
	replace_all(strRes, CARICCP_NEWLINE_CR, CARICCP_NEWLINE_LF);
	splitString(strRes,CARICCP_NEWLINE_LF,vec);
	for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		strTmp = *it;
		trim(strTmp);
		idex = strTmp.find(CARICCP_COLON_MARK);//冒号
		if(0 <= idex){
			strNew = strTmp.substr(idex+1);
		}
		else{
			strNew = strTmp;
		}
		trim(strNew);
		
		vecCpuSpeed.push_back(strNew);//保持值到容器中
	}

	//cache size,格式如下:
	//cache size      : 6144 KB
	//cache size      : 6144 KB
	vec.clear();
	cari_common_getSysCmdResult("grep -i \"cache size\" /proc/cpuinfo",strRes);
	/*printf("cache size\n%s",strRes.c_str());*/
	replace_all(strRes, CARICCP_NEWLINE_CR, CARICCP_NEWLINE_LF);
	splitString(strRes,CARICCP_NEWLINE_LF,vec);
	for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		strTmp = *it;
		trim(strTmp);
			idex = strTmp.find(CARICCP_COLON_MARK);//冒号
		if(0 <= idex){
			strNew = strTmp.substr(idex+1);
		}
		else{
			strNew = strTmp;
		}
		trim(strNew);

		vecCpuCashSize.push_back(strNew);//保持值到容器中
	}

	//bogomips,格式如下:
	//bogomips        : 4990.45
	//bogomips        : 4990.45
	vec.clear();
	cari_common_getSysCmdResult("grep -i \"bogomips\" /proc/cpuinfo",strRes);
	/*printf("bogomips\n%s",strRes.c_str());*/
	replace_all(strRes, CARICCP_NEWLINE_CR, CARICCP_NEWLINE_LF);
	splitString(strRes,CARICCP_NEWLINE_LF,vec);
	for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		strTmp = *it;
		trim(strTmp);
		idex = strTmp.find(CARICCP_COLON_MARK);//冒号
		if(0 <= idex){
			strNew = strTmp.substr(idex+1);
		}
		else{
			strNew = strTmp;
		}
		trim(strNew);

		vecCpuBogomips.push_back(strNew);//保持值到容器中
	}

	//cpu rate :占用率信息
	//getCPURate(strColName,strCpuRateValue);

	//////////////////////////////////////////////////////////////////////////
	//记录的封装,针对每个cpu进行信息封装
	strRes = "";
	iCpuCount = vecCpuModel.size();//容器的数量相同大小
	for(int i=0;i< iCpuCount;i++){
		strCpuModel = getValueOfVec(i,vecCpuModel);
		strCpuSpeed = getValueOfVec(i,vecCpuSpeed);
		strCpuCashSize = getValueOfVec(i,vecCpuCashSize);
		strCpuBogomips = getValueOfVec(i,vecCpuBogomips);

		char *pRes = switch_mprintf("%s%s%s%s%s%s%s",
			strCpuModel.c_str(),
			CARICCP_SPECIAL_SPLIT_FLAG,
			strCpuSpeed.c_str(),
			CARICCP_SPECIAL_SPLIT_FLAG,
			strCpuCashSize.c_str(),
			CARICCP_SPECIAL_SPLIT_FLAG,
			strCpuBogomips.c_str());

		inner_RespFrame->m_vectTableValue.push_back(pRes);
		switch_safe_free(pRes);
	}

	//设置表头字段
	strTColCPUModel	=getValueOfDefinedVar("CPU_MODEL");
	strTColCPUSpeed	=getValueOfDefinedVar("CPU_SPEED");
	strTColCPUCashSize	=getValueOfDefinedVar("CPU_CASHSIZE");
	strTColCPUSysBog	=getValueOfDefinedVar("CPU_SYS_BOG");
	strTableName = switch_mprintf("%s%s%s%s%s%s%s",
		strTColCPUModel.c_str(),	CARICCP_SPECIAL_SPLIT_FLAG,
		strTColCPUSpeed.c_str(),	CARICCP_SPECIAL_SPLIT_FLAG,
		strTColCPUCashSize.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColCPUSysBog.c_str());

#endif

	//最后一条记录为CPU的使用率,由于记录的列数不同,又因为此处的信息为动态显示
	//因此,使用其他单独的命令进行查询即可
	//inner_RespFrame->m_vectTableValue.push_back(strCpuRateValue);

	//////////////////////////////////////////////////////////////////////////
	inner_RespFrame->iResultNum = inner_RespFrame->m_vectTableValue.size();
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("CPU_INFO");
	
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	return iFunRes;
}

/*查询内存使用状态的信息
*/
int CModule_SysInfo::queryMemoryInfo(inner_CmdReq_Frame*& reqFrame, 
									 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";

	char			*chTmp = NULL, *strTableName = NULL;
	string strTmp,strRes,strSize,strUsed,strUsage;
	vector<string> vec,vecMemData,vecSwapData,vecTotalData;
	int iMaxSize=0;

	string strTColMemType,strTColMemSize,strTColMemUsed,strTColMemAvail,strTColMemUse,
		strTColMemShare,strTColMemBuffer,strTColMemCash;

//windows系统的使用
#if defined(_WIN32) || defined(WIN32)

	string strAvail;
	int iallRam=0,iavailRam=0,iusedRam =0;
	QueryRAM(iallRam,iavailRam,iusedRam);

	strSize = formatStr("%d",iallRam);
	strUsed = formatStr("%d",(iallRam -iavailRam));
	strAvail = formatStr("%d",iavailRam);
	strUsage = formatStr("%d",iusedRam);

	strRes = formatStr("%s%s%s%s%s%s%s%s%s",
	"Mem",/*CARICCP_COLON_MARK*/CARICCP_SPECIAL_SPLIT_FLAG,//属性之间一般使用"冒号",但是"报文"和'sys view'都要使用此数据显示,因此替换为CARICCP_SPECIAL_SPLIT_FLAG
	strSize.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
	strUsed.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
	strAvail.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
	strUsage.c_str());

	inner_RespFrame->m_vectTableValue.push_back(strRes);

	//设置表头字段
	strTColMemType	=getValueOfDefinedVar("MEM_TYPE");
	strTColMemSize	=getValueOfDefinedVar("MEM_SIZE");
	strTColMemUsed	=getValueOfDefinedVar("MEM_USED");
	strTColMemAvail=getValueOfDefinedVar("MEM_AVAIL");
	strTColMemUse	=getValueOfDefinedVar("MEM_USE");
	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s",
		strTColMemType.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemSize.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemUsed.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemAvail.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemUse.c_str(),CARICCP_PERCENT_MARK);


//linux系统的使用
#else

	int i=0,index=0,isize=0;

	//[xxl@cariteledell ~]$ free -t -m
	//			  total       used       free     shared    buffers     cached
	//Mem:          7654       7075        578          0        447       5583
	//-/+ buffers/cache:       1044       6609
	//Swap:         9983          0       9983
	//Total:       17637       7075      10562
	cari_common_getSysCmdResult("free -t -m",strTmp);//此处是按照M为单位查询

	//需要重新封装发送给client端,所有的处理由后台负责,client只需要直接显示即可
	replace_all(strTmp, CARICCP_NEWLINE_CR, CARICCP_NEWLINE_LF);
	splitString(strTmp,CARICCP_NEWLINE_LF,vec);
	for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		//过滤处理一下
		strTmp = *it;
		trim(strTmp);
		stringToLower(strTmp);
		index = strTmp.find(CARICCP_COLON_MARK);//根据冒号":"过滤一下,去除第一行标题行
		if (0>index){
			continue;
		}
		//将":"修改掉.属性之间一般使用"冒号",但是"报文"和'sys view'都要使用此数据显示
		//因此最终结果的封装将使用CARICCP_SPECIAL_SPLIT_FLAG
		replace_all(strTmp, CARICCP_COLON_MARK, CARICCP_SPACEBAR);

        //mem字段
		index = strTmp.find("mem");
		if(0 <= index){
			splitString(strTmp,CARICCP_SPACEBAR,vecMemData);

			strSize = getValueOfVec(1,vecMemData);//总的大小
			strUsed = getValueOfVec(2,vecMemData);//已使用大小
			strUsage = convertToPercent(strUsed,strSize);//转换成新的百分比
			insertElmentToVec(4,strUsage,vecMemData);//插入新的元素到容器
			continue;
		}
		//swap字段
		index = strTmp.find("swap");
		if(0 <= index){
			splitString(strTmp,CARICCP_SPACEBAR,vecSwapData);

			strSize = getValueOfVec(1,vecSwapData);//总的大小
			strUsed = getValueOfVec(2,vecSwapData);//已使用大小
			strUsage = convertToPercent(strUsed,strSize);//转换成新的百分比
			insertElmentToVec(4,strUsage,vecSwapData);//插入新的元素到容器
			continue;
		}
		//total字段
		index = strTmp.find("total");
		if(0 <= index){
			splitString(strTmp,CARICCP_SPACEBAR,vecTotalData);

			strSize = getValueOfVec(1,vecTotalData);//总的大小
			strUsed = getValueOfVec(2,vecTotalData);//已使用大小
			strUsage = convertToPercent(strUsed,strSize);//转换成新的百分比
			insertElmentToVec(4,strUsage,vecTotalData);//插入新的元素到容器
			continue;
		}
	}

    //////////////////////////////////////////////////////////////////////////
	//为了报文显示正常,此处需要封装一下处理
	iMaxSize = CARICCP_MAX(vecMemData.size(),vecSwapData.size());
	iMaxSize = CARICCP_MAX(iMaxSize,vecTotalData.size());
	isize = vecMemData.size();
	for(i = isize;i < iMaxSize;i++){
		vecMemData.push_back("");
	}
	isize = vecSwapData.size();
	for(i = isize;i < iMaxSize;i++){
		vecSwapData.push_back("");
	}
	isize = vecTotalData.size();
	for(i = isize;i < iMaxSize;i++){
		vecTotalData.push_back("");
	}

	//将封装好的结果数据保存到帧中
	strRes =encapResultFromVec(CARICCP_SPECIAL_SPLIT_FLAG,vecMemData);
	inner_RespFrame->m_vectTableValue.push_back(strRes);
	strRes =encapResultFromVec(CARICCP_SPECIAL_SPLIT_FLAG,vecSwapData);
	inner_RespFrame->m_vectTableValue.push_back(strRes);
	strRes =encapResultFromVec(CARICCP_SPECIAL_SPLIT_FLAG,vecTotalData);
	inner_RespFrame->m_vectTableValue.push_back(strRes);

	//设置表头字段
	//strTableName = "type          total       used       free       shared     buffers   cached";
	strTColMemType	=getValueOfDefinedVar("MEM_TYPE");
	strTColMemSize	=getValueOfDefinedVar("MEM_SIZE");
	strTColMemUsed	=getValueOfDefinedVar("MEM_USED");
	strTColMemAvail=getValueOfDefinedVar("MEM_AVAIL");
	strTColMemUse	=getValueOfDefinedVar("MEM_USE");
	strTColMemShare=getValueOfDefinedVar("MEM_SHARED");
	strTColMemBuffer=getValueOfDefinedVar("MEM_BUFFER");
	strTColMemCash=getValueOfDefinedVar("MEM_CASHED");
	
	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		strTColMemType.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemSize.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemUsed.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemAvail.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemUse.c_str(),CARICCP_PERCENT_MARK,CARICCP_SPECIAL_SPLIT_FLAG,//%符号
		strTColMemShare.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemBuffer.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColMemCash.c_str());

#endif

	//////////////////////////////////////////////////////////////////////////
	inner_RespFrame->iResultNum = inner_RespFrame->m_vectTableValue.size();
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("MEM_INFO");

	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));


	switch_safe_free(strTableName);
	return iFunRes;
}

/*查询磁盘空间的信息
*/
int CModule_SysInfo::queryDiskInfo(inner_CmdReq_Frame*& reqFrame, 
								   inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";
	char			*chTmp = NULL, *strTableName = NULL;
	string strTmp,strRes,strHeader,strUsed,strUsage;
	vector<string> vec,vecData,vecHeader;
	int icout=0,icol=0;
	string strTColDiskFileType,strTColDiskSize,strTColDiskUsed,strTColDiskAvail,strTColDiskUse,strTColDiskMountPoint;

//windows系统的使用
#if defined(_WIN32) || defined(WIN32)
	vector<DriverStruct> driverVec;
	QueryDriveSpace(driverVec);
	DriverStruct diver;
	for(vector<DriverStruct>::const_iterator it=driverVec.begin();it!=driverVec.end();it++){
		diver = *it;
		strRes = formatStr("%s%s%d%s%d%s%d%s%s%s%s%s",
			diver.m_FileSystem,CARICCP_SPECIAL_SPLIT_FLAG,
			diver.m_TotalSize,CARICCP_SPECIAL_SPLIT_FLAG,
			diver.m_UsedSize,CARICCP_SPECIAL_SPLIT_FLAG,
			diver.m_FreeSize,CARICCP_SPECIAL_SPLIT_FLAG,
			diver.m_Usage,CARICCP_PERCENT_MARK,CARICCP_SPECIAL_SPLIT_FLAG,
			diver.m_DriverType);
			
		inner_RespFrame->m_vectTableValue.push_back(strRes);//将封装好的记录存放到容器中
	}

#else//linux系统的使用

	//	[xxl@cariteledell ~]$ df -m
	//	Filesystem           1M-blocks      Used Available Use% Mounted on
	//	/dev/mapper/VolGroup00-LogVol00
	//						 258179    137434    107419  57% /
	//	/dev/sda3                   99        20        74  22% /boot
	//	tmpfs                     3827         0      3827   0% /dev/shm
	//	none                      3827         1      3827   1% /var/lib/xenstored
	cari_common_getSysCmdResult("df -h",strTmp);//-m :M为单位,-h以G为单位

	//需要重新封装发送给client端,所有的处理由后台负责,client只需要直接显示即可
	replace_all(strTmp, CARICCP_NEWLINE_CR, CARICCP_NEWLINE_LF);
	splitString(strTmp,CARICCP_NEWLINE_LF,vec);
	for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		if(0==icout){
			splitString(strTmp,CARICCP_SPACEBAR,vecHeader);//以空格为分割符
			icout++;
			continue;//表头去除
		}
		//过滤处理一下
		strTmp = *it;
		splitString(strTmp,CARICCP_SPACEBAR,vecData);//以空格为分割符
	}
	//重新封装字串,根据表头的列数封装
	icol = 6/*vecHeader.size()-1*/;//固定数量为6,vecHeader的数量不准确
	icout=0;
	if(0 != icol){
		for(vector<string>::const_iterator it=vecData.begin();it!=vecData.end();it++){
			strTmp = *it;
			icout++;
			if(0 == icout%icol){
				strRes.append(strTmp);
				inner_RespFrame->m_vectTableValue.push_back(strRes);//将封装好的记录存放到容器中
				strRes="";
				continue;
			}

			strRes.append(strTmp);
			strRes.append(CARICCP_SPECIAL_SPLIT_FLAG);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	inner_RespFrame->iResultNum = inner_RespFrame->m_vectTableValue.size();
	iFunRes = CARICCP_SUCCESS_STATE_CODE;
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	strReturnDesc = getValueOfDefinedVar("DISK_INFO");

	//设置表头字段
	/*strHeader = encapResultFromVec(CARICCP_SPECIAL_SPLIT_FLAG,vecHeader);
	strTColName =(char*)strHeader.c_str();*/
	//设置表头字段
	strTColDiskFileType=getValueOfDefinedVar("DISK_FILESYS");
	strTColDiskSize=getValueOfDefinedVar("DISK_SIZE");
	strTColDiskUsed=getValueOfDefinedVar("DISK_USED");
	strTColDiskAvail=getValueOfDefinedVar("DISK_AVAIL");
	strTColDiskUse=getValueOfDefinedVar("DISK_USE");
	strTColDiskMountPoint=getValueOfDefinedVar("DISK_MOUNTPOINT");
	strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s%s%s%s",
		strTColDiskFileType.c_str(),CARICCP_SPECIAL_SPLIT_FLAG,
		strTColDiskSize.c_str()	,CARICCP_SPECIAL_SPLIT_FLAG,
		strTColDiskUsed.c_str()	,CARICCP_SPECIAL_SPLIT_FLAG,
		strTColDiskAvail.c_str()	,CARICCP_SPECIAL_SPLIT_FLAG,
		strTColDiskUse.c_str(),CARICCP_PERCENT_MARK,CARICCP_SPECIAL_SPLIT_FLAG,//%符号
		strTColDiskMountPoint.c_str());

	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	switch_safe_free(strTableName);
	return iFunRes;
}

/*查询uptime的相关信息,主要是涉及到"动态"信息的查询
*/
int CModule_SysInfo::queryUptimeInfo(inner_CmdReq_Frame*& reqFrame, 
									 inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_ERROR_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_ERROR_STATE_CODE;
	string			strReturnDesc							= "";

	string strUpTime,strUserNum,strLoadedAver;
	vector<string> vec;
	char			*chTmp = NULL, *strTableName = NULL;

	//系统的运行时间,当前用户数,以及平均加载时间峰值
	getUpTime(strUpTime,strUserNum,strLoadedAver);

	//设置返回的结果值
	inner_RespFrame->m_vectTableValue.push_back(strUpTime);
	inner_RespFrame->m_vectTableValue.push_back(strUserNum);
	inner_RespFrame->m_vectTableValue.push_back(strLoadedAver);

	inner_RespFrame->iResultNum = inner_RespFrame->m_vectTableValue.size();
	iCmdReturnCode = CARICCP_SUCCESS_STATE_CODE;
	
	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	return iFunRes;
}

/*查询CPU的占用率
*/
int CModule_SysInfo::queryCPURate(inner_CmdReq_Frame*& reqFrame, 
								  inner_ResultResponse_Frame*& inner_RespFrame)
{
	int				iFunRes									= CARICCP_SUCCESS_STATE_CODE;
	int				iCmdReturnCode							= CARICCP_SUCCESS_STATE_CODE;
	string			strReturnDesc							= "";
	string strCpuRate,strRes,strColName;
	vector<string> vec;
	char		   *strTableName = NULL;
	char chColVal[100];
	string strTColCPURate;

	//CPU的使用率
	memset(chColVal,0,sizeof(chColVal));
	getCPURate(strColName,/*strCpuRate*/chColVal);

	strCpuRate = chColVal;
	inner_RespFrame->m_vectTableValue.push_back(strCpuRate);

	//////////////////////////////////////////////////////////////////////////
	inner_RespFrame->iResultNum = inner_RespFrame->m_vectTableValue.size();
	strReturnDesc = getValueOfDefinedVar("CPU_RATE_INFO");

	//表头重新封装
	//strTableName =(char*)strColName.c_str();
	/*strTableName = switch_mprintf("%s%s%s%s%s%s%s%s%s",
		"Usage(%)",CARICCP_SPECIAL_SPLIT_FLAG,
		"system",CARICCP_SPECIAL_SPLIT_FLAG,
		"nice",CARICCP_SPECIAL_SPLIT_FLAG,
		"idle",CARICCP_SPECIAL_SPLIT_FLAG,
		"iowait");*/

	strTColCPURate= getValueOfDefinedVar("CPU_RATE");
	strTableName = switch_mprintf("%s",
		strTColCPURate.c_str());

	//设置表头字段
	myMemcpy(inner_RespFrame->strTableName, strTableName, sizeof(inner_RespFrame->strTableName));
	switch_safe_free(strTableName);

	inner_RespFrame->header.iResultCode = iCmdReturnCode;
	myMemcpy(inner_RespFrame->header.strResuleDesc, strReturnDesc.c_str(), sizeof(inner_RespFrame->header.strResuleDesc));

	return iFunRes;
}


/************************************************************************/
/*                    其他和系统性能有关的函数                          */
/************************************************************************/
/*获得系统运行的时间,用户数量等动态的信息
*/
void getUpTime(string &strRunTime,     //输出参数 :系统运行的时间	
			   string &strUserNum,     //输出参数 :用户数量				
			   string &strLoadedAver)  //输出参数 :加载的平均峰值
{
	string strTmp,/*strYear,strMonth,*/strDay,strHour,strMinute,strUpTime;
	string strTColDay,strTColHour,strTColMinute;
	vector<string> vec;
	char *chTmp = NULL;

	strTColDay	= getValueOfDefinedVar("DAY");
	strTColHour	= getValueOfDefinedVar("HOUR");
	strTColMinute = getValueOfDefinedVar("MINUTE");

#if defined(_WIN32) || defined(WIN32)

	string strSecond;
	int iClientNUm = 0;
	//获取系统运行时间   
	long t=GetTickCount();

	strHour = formatStr("%d",t/3600000);
	t%=3600000;
	strMinute = formatStr("%d",t/60000);
	t%=60000;
	strSecond = formatStr("%d",t/1000);

	strUpTime.append(strHour);
	strUpTime.append(strTColHour);//"小时"
	strUpTime.append(strMinute);
	strUpTime.append(strTColMinute);//"分钟"

	strRunTime = strUpTime;
	strUserNum = "1";//windows系统只能登陆1个用户
	iClientNUm = /*CModuleManager::getInstance()->getClientUserNum()*/global_client_num;
	strUserNum = formatStr("%d",iClientNUm);
	strLoadedAver = "non-genetic";//windows系统不涉及


#else//linux系统的使用

	int ipos=-1;
	//系统的运行时间,当前用户数,以及平均加载时间峰值
	//格式1: 16:11:50 up 12 days,  6:27,  5 users,  load average: 0.01, 0.01, 0.00
	//格式2: 14:47:57 up 22 days, 33 min,  4 users,  load average: 0.02, 0.08, 0.08
	//格式3: 13:16:14 up 1 day,  1:55,  3 users,  load average: 0.00, 0.02, 0.00
	//格式4: 15:26:10 up  3:03,  5 users,  load average: 1.10, 0.61, 0.24
	//格式5: 08:49:24 up 1 min,  1 user,  load average: 0.39, 0.19, 0.07
	cari_common_getSysCmdResult("uptime",strTmp);
	trim(strTmp);

	ipos = strTmp.find("load average:");//load average
	if (0<ipos){
		strLoadedAver = strTmp.substr(ipos+strlen("load average:"));
		strTmp = strTmp.substr(0,ipos);
		trim(strLoadedAver);
		trim(strTmp);
	}

	//优化处理一下:去除up字段,否则容易和后面的格式混淆
	ipos = strTmp.find("up");//up
	if (0<ipos){
		strTmp = strTmp.substr(ipos+strlen("up"));
		trim(strTmp);
	}

	splitString(strTmp,CARICCP_COMMA_MARK,vec);//以","为分割符

	//先从后向前分析
	int isize = vec.size();

	//用户数
	strTmp = getValueOfVec(isize-1,vec);
	strUserNum = getValueOfStr(CARICCP_SPACEBAR,0,strTmp);

	//16:11:50 up 12 days,  6:27,
	//14:47:57 up 22 days, 33 min,
	//13:16:14 up 1 day,  1:55,    //单数天
	//15:26:10 up  3:03,
	//08:49:24 up 1 min,
	//时间进行分析
	for (int i=0;i<=(isize-2);i++){
		strTmp = getValueOfVec(i,vec);
		/*printf("strTmp = %s\n",strTmp.c_str());*/

		ipos = strTmp.find("days");//多数天数
		if (0<ipos){
			strDay = getValueOfStr(CARICCP_SPACEBAR,0,strTmp);
			continue;
		}
		ipos = strTmp.find("day");//单数(1)天数
		if (0<ipos){
			strDay = getValueOfStr(CARICCP_SPACEBAR,0,strTmp);
			continue;
		}
		ipos = strTmp.find("min");//分钟
		if (0<ipos){
			strMinute = getValueOfStr(CARICCP_SPACEBAR,0,strTmp);
			continue;
		}
		//查找最后的格式
		ipos = strTmp.find_last_of(CARICCP_COLON_MARK);//小时:分钟
		if (0<ipos){
			vector<string> vecH;
			ipos = strTmp.find_last_of(CARICCP_SPACEBAR);
			if (0<ipos){	
				strTmp = strTmp.substr(ipos+1);
			}

			splitString(strTmp,CARICCP_COLON_MARK,vecH);
			strHour= getValueOfVec(0,vecH);
			strMinute = getValueOfVec(1,vecH);
			//优化处理,去除前面的0
			int iMinute = stringToInt(strMinute.c_str());
			strMinute = intToString(iMinute);
			continue;
		}
	}

	//if (0 == strDay.length()){
	//	strDay = "0";
	//}
	//if (0 == strHour.length()){
	//	strHour = "0";
	//}
	//if (0 == strMinute.length()){
	//	strMinute = "0";
	//}
	strUpTime = "";
	if(0 != strDay.length()){
		 strUpTime.append(strDay);
		 strUpTime.append(strTColDay);//"天"
	}
	if(0 != strHour.length()){
		strUpTime.append(strHour);
		strUpTime.append(strTColHour);//"小时"
	}
	if(0 != strMinute.length()){
		strUpTime.append(strMinute);
		strUpTime.append(strTColMinute);//"分钟"
	}
	strRunTime = strUpTime;


	//strTmp = getValueOfVec(0,vec);
	//strDay = getValueOfStr(CARICCP_SPACEBAR,2,strTmp);//天数

	//strTmp = getValueOfVec(1,vec);
	//strHour = getValueOfStr(CARICCP_COLON_MARK,0,strTmp);//小时
	//strMinute = getValueOfStr(CARICCP_COLON_MARK,1,strTmp);//分钟

	////如果不含"天"
	//ipos = strDay.find("days");
	//if (0 > ipos){

	//}

	////如果不含有"小时"
	//ipos = strHour.find("min");
	//if (0 == strMinute.length()
	//	&&(0 <ipos)){
	//		strMinute = strHour.substr(0,ipos);
	//		strHour = "0";
	//		trim(strMinute);
	//}

	//strUpTime = "";
	//if(0 != strDay.length()){
	//	 strUpTime.append(strDay);
	//	 strUpTime.append(strTColDay);
	//}
	//if(0 != strHour.length()){
	//	strUpTime.append(strHour);
	//	strUpTime.append(strTColHour);
	//}
	//if(0 != strMinute.length()){
	//	strUpTime.append(strMinute);
	//	strUpTime.append(strTColMinute);
	//}
	//strRunTime = strUpTime;

	////用户数
	//strTmp = getValueOfVec(2,vec);
	//strUserNum = getValueOfStr(CARICCP_SPACEBAR,0,strTmp);

	////load average
	//strTmp = getValueOfVec(3,vec);
	//strLoadedAver = getValueOfStr(CARICCP_COLON_MARK,1,strTmp);

#endif
}

/*提取有效字串,从数字开始
*/
string extractValidStr(string strSource)
{
	char ch;
	string strnew;
	int ilen = strSource.length();
	int i = 0;
	for (i=0; i<ilen; i++){
		ch =  strSource.at(i);
		if ('0'<= ch
			&& '9' >= ch){
			break;
		}
	}
	strnew = strSource.substr(i,ilen - i);
	return strnew;
}

/*获得CPU的占用率
*/
void getCPURate(string &strColName,  //输出参数,列名
				/*string &strColValue*/char *chColVal) //输出参数,列值
{
	string strRes,strTmp,strNew,strType,strVal;
	vector<string> vec,vecRate,vecData;
	strColName = "Cpu(s) usage:";

#if defined(_WIN32) || defined(WIN32)
	int iUsage = get_cpu_usage();
	if (-1 == iUsage){
		iUsage = 0;//特殊处理一下
	}
	
	//int iUsage = usageA.GetCpuUsage();
	//int iUsage = getProcCPUUsage();
	string str = formatStr("%d%s",iUsage,CARICCP_PERCENT_MARK);//使用%封装
	myMemcpy(chColVal,str.c_str(),strlen(str.c_str()));

#else//linux系统的使用

	string strColValue;
	char *arry_cpuRateValue[NCPUSTATES] = { 0 };
	char **cpuRateVal = arry_cpuRateValue;
	int i,idex,icout,imax; 
	for(i = 0; i < NCPUSTATES; i++){ 
		cpu_states[i] = 0; 
		cp_diff[i] = 0; 
	} 

	//方案1:通过系统命令进行查询
	//格式: Cpu(s):  0.1%us,  0.1%sy,  0.0%ni, 99.7%id,  0.0%wa,  0.0%hi,  0.0%si,  0.1%st
	//此命令执行后的结果显示有异常符号,如Cpu(s): (B[m[39;49m(B[m  0.0%(...
	cari_common_getSysCmdResult("top -n 1|grep Cpu",strRes);
	
	idex = strRes.find(CARICCP_COLON_MARK);//冒号":"为分割符
	if(0 <= idex){
		strNew = strRes.substr(idex+1);
	}
	else{
		strNew = strRes;
	}

	splitString(strNew,CARICCP_COMMA_FLAG,vec);//以逗号","为分隔符
	icout=0;
	imax = vec.size();

	//以前的逻辑,详细列出各项的使用率
	//for(vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
	//	strTmp = *it;
	//	trim(strTmp);

	//	//增加过滤条件,后面的数据不需要,只提取前面的部分
	//	if(5 <= icout){
	//		break;
	//	}
	//	//注意:由于linux的内核,导致top命令的执行获得CPU的信息中含有异常字符,此处稍微屏蔽修改一下.没的其他办法???
	//	idex = strTmp.find(CARICCP_SPACEBAR);
	//	if (0 < idex){
	//		strTmp = strTmp.substr(idex+1);
	//		trim(strTmp);
	//	}

	//	idex = strTmp.find(CARICCP_PERCENT_MARK);//%为分割符
	//	if(0 <= idex){
	//		strVal = strTmp.substr(0,idex+1);//包含%符号
	//		strType= strTmp.substr(idex+1);
	//	}
	//	else{
	//		strVal ="";
	//		strType = strTmp;
	//	}
	//	strColName.append(strType);
	//	strColValue.append(strVal);

	//	if ((imax - 1)>icout){
	//		strColValue.append(CARICCP_COMMA_MARK);
	//	}
	//	
	//	icout++;
	//}

	//新的逻辑,只需要计算使用率
	float f;
	char	buffer[10];
	strVal = getValueOfVec(3,vec);//获得idle(空闲)的cpu使用率
	idex = strVal.find(CARICCP_PERCENT_MARK);//%为分割符
	if(0 <= idex){
		strVal = strVal.substr(0,idex);
	}
//printf("1 %s\n",strVal.c_str());
    //注意:由于linux的内核,导致top命令的执行获得CPU的信息中含有异常字符,此处稍微屏蔽修改一下.
	idex = strVal.find(CARICCP_SPACEBAR);
	if(0 <= idex){
		strVal = strVal.substr(idex+1);
	}
	trim(strVal);
//printf("2 %s\n",strVal.c_str());
	if(EOF == sscanf(strVal.c_str(), "%f", &f)){
		printf("Error\n");
		//错误
		f = 100;
	}
//printf("3 %f\n",f);
	f = 100 - f;//使用率
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer,"%.1f%s",
		f,
		CARICCP_PERCENT_MARK);//封装,小数点后一位

	myMemcpy(chColVal,buffer,sizeof(buffer));
//printf("4 %s\n",chColVal);
	//printf("%\n");

	////方案2:直接查找/proc/stat文件进行获得相关的信息
	////循环计算时间差
	//int fd,len;
	//char *p;
	//char cpuinfo[128];
	////char buffer[4096+1]; 
	//char buffer[1024]; 
	//for(i=0;i<2;i++){
	//	memset(buffer, 0 ,sizeof(buffer));

	//	fd = open("/proc/stat", O_RDONLY);
	//	if (0 > fd){
	//		const char *reason = strerror(errno);
	//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldnt open /proc/stat. (%s)\n", reason);
	//		continue;
	//	}
	//	len = read(fd, buffer, sizeof(buffer)-1);

	//	close(fd);
	//	buffer[len] = '\0'; 

	//	p = skip_token(buffer); /* "cpu" */ 
	//	cp_time[0] = strtoul(p, &p, 0); 
	//	cp_time[1] = strtoul(p, &p, 0); 
	//	cp_time[2] = strtoul(p, &p, 0); 
	//	cp_time[3] = strtoul(p, &p, 0); 
	//	cp_time[4] = strtoul(p, &p, 0); 
	//	percentages(NCPUSTATES, cpu_states, cp_time, cp_old, cp_diff); 
	//	//printf("cpu  user:%4.1f nice:%4.1f sys:%4.1f idle:%4.1f iowait:%4.1f\n",
	//	//	cpu_states[0]/10.0,
	//	//	cpu_states[1]/10.0,
	//	//	cpu_states[2]/10.0,
	//	//	cpu_states[3]/10.0,
	//	//	cpu_states[4]/10.0); 

	//	memset(cpuinfo, 0 ,sizeof(cpuinfo));
	//	//封装,需要free内存
	//	/*strColValue = switch_mprintf(" user:%4.1f%s  nice:%4.1f%s  system:%4.1f%s  idle:%4.1f%s  iowait:%4.1f%s",
	//		cpu_states[0]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[1]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[2]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[3]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[4]/10.0,
	//		CARICCP_PERCENT_MARK);*/
	//	switch_snprintf(cpuinfo, sizeof(cpuinfo), 
	//		" user:%4.1f%s  nice:%4.1f%s  system:%4.1f%s  idle:%4.1f%s  iowait:%4.1f%s",
	//		cpu_states[0]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[1]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[2]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[3]/10.0,
	//		CARICCP_PERCENT_MARK,
	//		cpu_states[4]/10.0,
	//		CARICCP_PERCENT_MARK);

	//		//strColValue = cpuinfo;
	//		myMemcpy(chColVal,cpuinfo,sizeof(cpuinfo));

	//	//适当休眠一下
	//	CARI_SLEEP(CARICCP_MAX_CPU_WAIT_TIME);
	//}

#endif
}


#if defined(_WIN32) || defined(WIN32)
#include "tchar.h"

/************************************************************************/
/*          下面的方法主要是在windows下使用                             */
/************************************************************************/

/*获得OS的版本信息和核心信息
*/
void QueryOS(string &strOSKneral, //OS核心信息
			 string &strOSVersion)//OS版本信息
{
	OSVERSIONINFOEX osvi;
	BOOL bIsWindows64Bit;
	BOOL bOsVersionInfoEx;
	/*char * szOperatingSystem = new char [256];*/
	char szOperatingSystem[256];
	memset(szOperatingSystem,0,sizeof(szOperatingSystem));

	// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
	// If that fails, try using the OSVERSIONINFO structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi))) {
		// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO *) &osvi)) 
			return;
	}

	strOSKneral = "32-Bit";

	switch(osvi.dwPlatformId){
		case VER_PLATFORM_WIN32_NT:
			// Test for the product.
			if(osvi.dwMajorVersion <= 4) 
				strcpy(szOperatingSystem, "Microsoft WindowsNT  ");
			if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) 
				strcpy(szOperatingSystem, "Microsoft Windows2000 ");
			if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) 
				strcpy(szOperatingSystem, "Microsoft WindowsXP ");

			// Test for product type.
			if(bOsVersionInfoEx) {
				if(osvi.wProductType == VER_NT_WORKSTATION) {
					if(osvi.wSuiteMask & VER_SUITE_PERSONAL) 
						strcat(szOperatingSystem, "Personal");
					else 
						strcat(szOperatingSystem, "Professional");
				} else if(osvi.wProductType == VER_NT_SERVER) {
					// Check for .NET Server instead of Windows XP.
					if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) 
						strcpy(szOperatingSystem, "Microsoft Windows.NET");

					// Continue with the type detection.
					if(osvi.wSuiteMask & VER_SUITE_DATACENTER) 
						strcat(szOperatingSystem, "DataCenter Server");
					else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE) 
						strcat(szOperatingSystem, "Advanced Server");
					else 
						strcat(szOperatingSystem, "Server");
				}
			} 
			else {
				HKEY hKey;
				char szProductType[80];
				DWORD dwBufLen;

				// Query the registry to retrieve information.
				RegOpenKeyEx(HKEY_LOCAL_MACHINE, LPCWSTR("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_QUERY_VALUE, &hKey);
				RegQueryValueEx(hKey, LPCWSTR("ProductType"), NULL, NULL,(LPBYTE) szProductType, &dwBufLen);
				RegCloseKey(hKey);
				//if(lstrcmpi(LPCWSTR("WINNT"), szProductType) == 0) 
				if(isEqualStr("WINNT",szProductType))
					strcat(szOperatingSystem, "Professional");

				/*if(lstrcmpi(LPCWSTR("LANMANNT"), szProductType) == 0){*/
				if(isEqualStr("LANMANNT", szProductType)){
					// Decide between Windows 2000 Advanced Server and Windows .NET Enterprise Server.
					if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) 
						strcat(szOperatingSystem, "Standard Server");
					else 
						strcat(szOperatingSystem, "Server ");
					
					/*if(lstrcmpi(LPCWSTR("SERVERNT"), szProductType) == 0){*/
					if(isEqualStr("SERVERNT", szProductType)){
						// Decide between Windows 2000 Advanced Server and Windows .NET Enterprise Server.
						if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) 
							strcat(szOperatingSystem, "Enterprise Server ");
						else 
							strcat(szOperatingSystem, "Advanced Server ");
					}
				}
			}

			// Display version, service pack(if any), and build number.
			if(osvi.dwMajorVersion <= 4) {
				// NB: NT 4.0 and earlier.
				sprintf(szOperatingSystem, "%sversion %d.%d %s(Build %d)",
					szOperatingSystem,
					osvi.dwMajorVersion,
					osvi.dwMinorVersion,
					osvi.szCSDVersion,
					osvi.dwBuildNumber & 0xFFFF);
			} 
			else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
				// Windows XP and .NET server.
				typedef BOOL(CALLBACK* LPFNPROC)(HANDLE, BOOL *);
				HINSTANCE hKernelDLL; 
				LPFNPROC DLLProc;

				// Load the Kernel32 DLL.
				hKernelDLL = LoadLibrary(LPCWSTR("kernel32"));
				if(hKernelDLL != NULL)  { 
					// Only XP and .NET Server support IsWOW64Process so... Load dynamically!
					DLLProc =(LPFNPROC) GetProcAddress(hKernelDLL, "IsWow64Process"); 

					// If the function address is valid, call the function.
					if(DLLProc != NULL)(DLLProc)(GetCurrentProcess(), &bIsWindows64Bit);
					else bIsWindows64Bit = false;

					// Free the DLL module.
					FreeLibrary(hKernelDLL); 
				} 

				// IsWow64Process();
				if(bIsWindows64Bit) 
					//strcat(szOperatingSystem, "64-Bit");
					strOSKneral = "64-Bit";
				else 
					//strcat(szOperatingSystem, "32-Bit");
					strOSKneral = "32-Bit";
			} 
			else { 
				// Windows 2000 and everything else.
				sprintf(szOperatingSystem, "%s%s(Build %d)", szOperatingSystem, osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
			}

			break;

		case VER_PLATFORM_WIN32_WINDOWS:
			// Test for the product.
			if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
				strcpy(szOperatingSystem, "Microsoft Windows95 ");
				if(osvi.szCSDVersion[1] == 'C') 
					strcat(szOperatingSystem, "OSR 2.5");
				else if(osvi.szCSDVersion[1] == 'B') 
					strcat(szOperatingSystem, "OSR 2");
			} 

			if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
				strcpy(szOperatingSystem, "Microsoft Windows98 ");
				
				if(osvi.szCSDVersion[1] == 'A' ) 
					strcat(szOperatingSystem, "SE");
			} 

			if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
				strcpy(szOperatingSystem, "Microsoft WindowsMe");
			} 
			break;

		case VER_PLATFORM_WIN32s:

			strcpy(szOperatingSystem, "Microsoft Win32s");
			break;

		default:
			strcpy(szOperatingSystem, "Unknown Windows");
			break;
	}

	//版本信息
	strOSVersion = szOperatingSystem;
	if(0 == strOSVersion.length()){
		strOSVersion = "Microsoft Windows";
	}
}

/*查询CPU的信息
*/
void /*CModule_SysInfo::*/QCpu()
{
	// Define the local variables.
	HANDLE * hThread;
	DWORD * dwThreadID;
	SYSTEM_INFO SysInfo;
	PTHREADINFO * ThreadInfo;
	/*int nProcessors = 0;*/

	// Get the number of processors in the system.
	ZeroMemory(&SysInfo, sizeof(SYSTEM_INFO));
	GetSystemInfo(&SysInfo);

	// Number of physical processors in a non-Intel system
	// or in a 32-bit Intel system with Hyper-Threading technology disabled
	nProcessors = SysInfo.dwNumberOfProcessors;

	// For each processor; spawn a CPU thread to access details.
	hThread = new HANDLE [nProcessors];
	dwThreadID = new DWORD [nProcessors];
	ThreadInfo = new PTHREADINFO [nProcessors];

	// Check to see if the memory allocation happenned.
	if((hThread == NULL) ||(dwThreadID == NULL) ||(ThreadInfo == NULL)) {
		char * szMessage = new char [128];
		sprintf(szMessage, "Cannot allocate memory for threads and CPU information structures!");
		delete szMessage;
		return;
	}

	for(int nCounter = 0; nCounter < nProcessors; nCounter ++) {

		ThreadInfo[nCounter] = new THREADINFO;
		ThreadInfo[nCounter]->nProcessor = nCounter + 1;

		hThread[nCounter] = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) DisplayCPUInformation,(LPVOID) ThreadInfo[nCounter], CREATE_SUSPENDED, &dwThreadID[nCounter]);
		if(hThread[nCounter] == NULL) {
			char * szMessage = new char [128];
			sprintf(szMessage, "Cannot create processor interrogation thread for processor %d!", nCounter + 1);
			delete szMessage;
		}

		// Set the threads affinity to the correct processor.
		if(SetThreadAffinityMask(hThread[nCounter], nCounter + 1) == 0) {
			char * szMessage = new char [128];
			sprintf(szMessage, "Cannot set processor affinity for processor %d thread!", nCounter + 1);
			delete szMessage;
		}

		ResumeThread(hThread[nCounter]);
	}

	// Clean up memory allocations.
	delete [] ThreadInfo;
	delete [] dwThreadID;
	delete [] hThread;
}

/*查询RAM信息
*/
void /*CModule_SysInfo::*/QueryRAM(int &allRam,
							   int &availRam,
							   int &usedRam)//百分比例
{

	MEMORYSTATUS memoryStatus;
	ZeroMemory(&memoryStatus,sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof(MEMORYSTATUS);

	::GlobalMemoryStatus (&memoryStatus);
	allRam = (int)memoryStatus.dwTotalPhys/1024/1024;
	availRam = (int)memoryStatus.dwAvailPhys/1024/1024;
	usedRam = (int)memoryStatus.dwMemoryLoad;//百分比例
}

//先预先定义驱动器的编号
std::wstring HardDiskLetters[MAX_OF_HARD_DISKS] = 
{
	_T("c:"),	
	_T("d:"),	
	_T("e:"),	
	_T("f:"),	
	_T("g:"),	
	_T("h:"),
	_T("i:"),	
	_T("j:"),	
	_T("k:"),	
	_T("l:"),	
	_T("m:"),	
	_T("n:"),
	_T("o:"),	
	_T("p:"),	
	_T("q:"),	
	_T("r:"),	
	_T("s:"),	
	_T("t:"),
	_T("u:"),	
	_T("v:"),	
	_T("w:"),	
	_T("x:"),	
	_T("y:"),	
	_T("z:")
};

/*查询驱动磁盘的空间等信息
*/
void /*CModule_SysInfo::*/QueryDriveSpace(vector<DriverStruct> &driverVec)
{
	ULARGE_INTEGER AvailableToCaller, Disk, Free;
	for (int iCounter=0;iCounter<MAX_OF_HARD_DISKS;iCounter++){
		std::wstring  strDriverType;
		strDriverType = HardDiskLetters[iCounter];
		strDriverType.append(_T("\\"));

		int iType = GetDriveType(strDriverType.c_str());
		if (DRIVE_FIXED == iType){
			DriverStruct diver;
			memset(diver.m_DriverType,0,sizeof(diver.m_DriverType));
			memset(diver.m_FileSystem,0,sizeof(diver.m_FileSystem));
			memset(diver.m_Usage,0,sizeof(diver.m_Usage));
			diver.m_TotalSize = 0;
			diver.m_UsedSize = 0;
			diver.m_FreeSize = 0;

			if (GetDiskFreeSpaceEx(strDriverType.c_str(),
				&AvailableToCaller,
				&Disk, 
				&Free)){
				
				//使用临时变量进行转黄
				char chT[16]; 
				wsprintfA(chT, "%S",strDriverType.c_str());
				myMemcpy(diver.m_DriverType,chT,sizeof(diver.m_DriverType));
				diver.m_TotalSize = Disk.QuadPart/1024/1024;

				ULONGLONG Used=Disk.QuadPart-Free.QuadPart;
				diver.m_UsedSize = Used/1024/1024;
				if (0 != diver.m_TotalSize){
					int iUsage = (int)diver.m_UsedSize*1.0*100/diver.m_TotalSize;
					string strUsage = formatStr("%d",iUsage);
					myMemcpy(diver.m_Usage,strUsage.c_str(),sizeof(diver.m_Usage));
                }

				diver.m_FreeSize = Free.QuadPart/1024/1024;
			}

			//PARTITION_IFS,那么分区是NTFS分区,如果返回PARTITION_FAT32或者   
			//PARTITION_FAT32_XINT13,那么是FAT32分区.
			BYTE   bRet;   
			DWORD   dwIOCode;   
			DWORD   dwReturn;   
			HANDLE   hDevice;   
			PARTITION_INFORMATION   ParInfo;   
			memset(&ParInfo,   0,   sizeof(ParInfo));  

			//char   chFS='c';
			//wsprintf(buf,   "\\\\.\\%c:",   chFS);
			//必须使用下面的格式进行判断
			strDriverType = _T("");
			strDriverType.append(_T("\\\\.\\"));
			strDriverType.append(HardDiskLetters[iCounter]);

			dwIOCode   =   IOCTL_DISK_GET_PARTITION_INFO;   
			hDevice   =   CreateFile(strDriverType.c_str(),
				GENERIC_READ,
				FILE_SHARE_WRITE,
				0,
				OPEN_EXISTING,
				0,
				NULL);

			if  (hDevice!=INVALID_HANDLE_VALUE 
				&& DeviceIoControl(hDevice,dwIOCode,NULL,0,&ParInfo,sizeof(ParInfo),&dwReturn,NULL)){
				bRet   =   ParInfo.PartitionType;
			}
			else{
				bRet   =   PARTITION_ENTRY_UNUSED;
			}

			CloseHandle(hDevice);  

			string strPartitionType="FAT32";//默认使用此格式类型
			if (PARTITION_IFS == bRet){
				strPartitionType = "NTFS";
			}
			else if (PARTITION_FAT32 == bRet || PARTITION_FAT32_XINT13 == bRet){
				strPartitionType = "FAT32";
			}

			myMemcpy(diver.m_FileSystem,strPartitionType.c_str(),sizeof(diver.m_FileSystem));

			//存放到容器中
			driverVec.push_back(diver);
		}
	}
}

#endif
