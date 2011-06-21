#ifndef C_MODULEMANAGER_H
#define C_MODULEMANAGER_H


#include "cari_net_commonHeader.h"
#include "CBaseModule.h"

// #ifdef 
// #endif
#include "CModule_User.h" 
#include "CModule_Route.h"
#include "CModule_SysInfo.h"
#include "CModule_NEManager.h"

#include "pthread.h"

/*----------------------------------------------------------------------------------------------------------------*/
//帧的清除原则 :谁调用,谁负责删除

class CModuleManager
{
public://构造函数设置为私有的,见下面
	 ~CModuleManager();

public://静态函数

	  //获得单实例对象
	  static CModuleManager*& getInstance();       

	  void clear();

public:
	  //获得模块号
	  int getModuleID(int cmdCode);         

	  //把客户端对应的socket保持起来
	  void saveSocket(socket3element& clientSocketElement);

	  //把客户端对应的socket删除掉
	  void eraseSocket(const u_int socketC);

	  //保存客户端连接的线程
	  void saveThrd(u_int socketC,pthread_t * tid);

	  //
	  void eraseThrd(const u_int socketC);

	  //kill all thrd
	   void killAllClientThrd();

	  //关闭所有客户端的socket
	  void closeAllClientSocket();

	  //socket是否存在
	  bool isExistSocket(const socket3element& clientSocketElement);

	  bool isExistThrd(u_int socketC);

	  //根据socket号获得对应的三元组
	  int getSocket(const u_int socketC, socket3element& element);

	  //获得子模块句柄
	  CBaseModule* getSubModule(int iModuleID);

	  //获得存放socket的容器
	  vector<socket3element>& getSocketVec();    

	  //此id号是否已经使用
	  bool isUsedID(u_short id);

	  //分配客户端号
	  u_short assignClientID();

	  //释放客户端号
	  void releaseClientID(u_short iClientID);
	  void releaseClientIDBySocket(u_int socketClient);

	  //移除分配的id号
	  void removeClientID(u_short iClientID);

	  //根据客户端的socket号获得客户端号
	  u_short getClientIDBySocket(u_int socketClient);

	  //获得当前连接的用户数量
	  int getClientUserNum();


private:
	CModuleManager();//构造函数必须是私有的

private:
	  static CModuleManager* m_moduleManagerInstance; //单实例模式

	  pthread_mutex_t mutex_container;  //pthread自带的互斥锁
	  pthread_mutex_t mutex_vecclient ; //pthread自带的互斥锁

	  //其他属性
	  CBaseModule* m_arrayModule[MAX_MODULE_ID];    //存放子模块,其下标值对应"子模块号"

	  /************************************************************************/
	  //由于容器没有加锁处理,如果多个客户端(线程)同时进行增/删操作,导致容器出现
	  //异常情况
	  /************************************************************************/
	  vector<socket3element> m_vecClientSocket;     //存放连接的客户端socket
	  vector<u_short> m_vecNoUsedClientID;          //存放未使用的client的id号
	  map<u_int,pthread_t *> m_mapThrd;     //客户端的线程存放

  //socket3element* socketelement;
};


#endif
