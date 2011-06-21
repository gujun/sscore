/*由于单实例对象都是通过new创建,分配到"堆栈"中的,因此
*其对应的析构函数不能释放此对象,专门创建此类进行释放
*单实例.
*在系统退出的时候调用即可
*/

#include "CMsgProcess.h"
#include "CModuleManager.h"

#ifndef CGarbageRecover_H
#define CGarbageRecover_H

class CGarbageRecover
{
public: 
  CGarbageRecover()
  {
    /*cout<<"start CGarbageRecover:"<<endl;*/
  } 

  ~CGarbageRecover()
  {
	  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n---------------- 3 CGarbageRecover() begin\n");
	  CModuleManager::getInstance()->clear();				//清除"模块实例"的内存资源
	  CMsgProcess::getInstance()->clear();					//无
	  CMsgProcess::getInstance()->clearAllCmdFromQueue();   //清除命令队列中的资源

	  //单实例对象系统退出时自动释放
	  //CARI_CCP_VOS_DEL(CMsgProcess::getInstance());
	  //CARI_CCP_VOS_DEL(CModuleManager::getInstance());
	  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n---------------- 3 CGarbageRecover() end\n");
  }
}; 


#endif 
