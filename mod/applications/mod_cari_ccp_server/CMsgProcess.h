/*消息的主要处理类,负责同步消息和异步消息
 *以及发送响应消息给对应的客户端
*/

#ifndef C_REQUEST_H
#define C_REQUEST_H

//#include <Windows.h>
#include <list>

#include "CModuleManager.h"
#include "cari_net_commonHeader.h"

//其他库
#if defined(_WIN32) || defined(WIN32)
	#include "pthread.h"
#else
	#include <pthread.h>
#endif


/*命令处理类,设计成单实例
*/
class CMsgProcess
{
public://构造函数设置为私有的,见下面
  
	 ~CMsgProcess();

public://公共方法
  
	 static CMsgProcess*& getInstance(); //获得单实例对象

	 void clear(); 

	 //创建异步处理命令的线程
	 void creatAsynCmdThread();

	 //统一的命令处理入口
	 void proCmd(u_int socketID, common_CmdReq_Frame*& common_reqFrame);

	 //命令的分发
	 void dispatchCmd(); 

	 //同步处理命令
	 int syncProCmd(inner_CmdReq_Frame*& inner_ReqFrame, inner_ResultResponse_Frame*& inner_RespFrame);

	 //异步处理命令
	 //类的成员函数不能
	 void asynProCmd_2(inner_CmdReq_Frame*& inner_reqFrame);

	 //同步和异步的共同接口:返回响应信息,另外,内部又细分为"单播"和"广播"
	 void commonSendRespMsg(inner_ResultResponse_Frame*& inner_respFrame,int nSendType = COMMON_RES_TYPE);

	 //单播发送响应
	 bool unicastRespMsg(socket3element& element, const char* respMs, int  bufferSize, char* cmdname="");
	 bool unicastRespMsg(socket3element& element, common_ResultResponse_Frame* respFrame, int  bufferSize, char* cmdname="");

	 //广播发送响应
	 /*bool broadcastRespMsg(socket3element &element,const char *respMs,int  bufferSize);*/
	 bool broadcastRespMsg(const char* respMs, int  bufferSize, char* cmdname="");

	 //通知类型的处理:传入的值是字串类型(也可以是多列记录或多行记录)
	 void notifyResult(inner_CmdReq_Frame*& reqFrame, Notify_Type_Code notifyCode, string notifyValue);

	 //通知类型的处理
	 void notifyResult(inner_ResultResponse_Frame*& inner_respFrame);

	 //主动上报类型的处理,主要针对某个模块的进行实时上报
	 void initiativeReport(inner_ResultResponse_Frame*& inner_respFrame);

	 //"请求"参数格式转换
	 /*void covertStrToParamStruct(char* strParameter,vector<simpleParamStruct> &vecParam);*/

	 //"通用请求帧"转换为"内部请求帧"
	 void covertReqFrame(common_CmdReq_Frame*& common_reqMsg, inner_CmdReq_Frame*& inner_reqFrame);

	 //请求帧的细化解析,进一步解析参数
	 void analyzeReqFrame(common_CmdReq_Frame*& common_reqMsg);

	 //参数转换 :"响应"参数格式转换
	 void covertRespParam(inner_ResultResponse_Frame*& inner_RespFrame, common_ResultResponse_Frame*& common_respFrame);

	 //帧转换 :"内部响应帧"转换为"通用响应帧"
	 void covertRespFrame(inner_ResultResponse_Frame*& inner_respFrame, common_ResultResponse_Frame*& common_respMsg);


	 /*------------涉及到锁 :主要是针对"命令"的处理--------------*/
	 //针对于多客户端的情况,命令处理比较多,所以需要队列存放起来
	 void lock();
	 void unlock();

	 //判断队列中的元素是否已满
	 bool isFullOfCmdQueue();

	 //判断队列中是否已空
	 bool isEmptyOfCmdQueue();

	 //增加元素到队列中
	 int pushCmdToQueue(inner_CmdReq_Frame*& reqFrame); 

	 //从队列中取出一个元素
	 int popCmdFromQueue(inner_CmdReq_Frame*& reqFrame);

	 //清除队列中的元素
	 void clearAllCmdFromQueue();

public://属性

protected:

private://私有方法
 
	 CMsgProcess();//构造函数必须是私有的

private://私有属性
	 static CMsgProcess* m_reqProInstance;//单实例模式
	 //LPCRITICAL_SECTION  m_lock;   //锁标识
	 //HANDLE m_semaphore;     //信号量

	 pthread_mutex_t mutex;    //pthread自带的互斥锁
	 pthread_cond_t cond ;    //条件变量

	 //vector<common_CmdReq_Frame*> vecRequestFrame;
	 list<inner_CmdReq_Frame*> m_listRequestFrame;//存放请求帧的容器

	 u_int i_full_waiters;  //标识:队列是否已满
	 u_int i_empty_waiters; //标识:队列是否已空
	 int i_terminated;
	 pthread_t thread_asynPro;//异步处理线程
};



#endif
