/*���ڵ�ʵ��������ͨ��new����,���䵽"��ջ"�е�,���
*���Ӧ���������������ͷŴ˶���,ר�Ŵ�����������ͷ�
*��ʵ��.
*��ϵͳ�˳���ʱ����ü���
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
	  CModuleManager::getInstance()->clear();				//���"ģ��ʵ��"���ڴ���Դ
	  CMsgProcess::getInstance()->clear();					//��
	  CMsgProcess::getInstance()->clearAllCmdFromQueue();   //�����������е���Դ

	  //��ʵ������ϵͳ�˳�ʱ�Զ��ͷ�
	  //CARI_CCP_VOS_DEL(CMsgProcess::getInstance());
	  //CARI_CCP_VOS_DEL(CModuleManager::getInstance());
	  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n---------------- 3 CGarbageRecover() end\n");
  }
}; 


#endif 
