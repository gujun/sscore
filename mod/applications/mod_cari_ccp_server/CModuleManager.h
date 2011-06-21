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
//֡�����ԭ�� :˭����,˭����ɾ��

class CModuleManager
{
public://���캯������Ϊ˽�е�,������
	 ~CModuleManager();

public://��̬����

	  //��õ�ʵ������
	  static CModuleManager*& getInstance();       

	  void clear();

public:
	  //���ģ���
	  int getModuleID(int cmdCode);         

	  //�ѿͻ��˶�Ӧ��socket��������
	  void saveSocket(socket3element& clientSocketElement);

	  //�ѿͻ��˶�Ӧ��socketɾ����
	  void eraseSocket(const u_int socketC);

	  //����ͻ������ӵ��߳�
	  void saveThrd(u_int socketC,pthread_t * tid);

	  //
	  void eraseThrd(const u_int socketC);

	  //kill all thrd
	   void killAllClientThrd();

	  //�ر����пͻ��˵�socket
	  void closeAllClientSocket();

	  //socket�Ƿ����
	  bool isExistSocket(const socket3element& clientSocketElement);

	  bool isExistThrd(u_int socketC);

	  //����socket�Ż�ö�Ӧ����Ԫ��
	  int getSocket(const u_int socketC, socket3element& element);

	  //�����ģ����
	  CBaseModule* getSubModule(int iModuleID);

	  //��ô��socket������
	  vector<socket3element>& getSocketVec();    

	  //��id���Ƿ��Ѿ�ʹ��
	  bool isUsedID(u_short id);

	  //����ͻ��˺�
	  u_short assignClientID();

	  //�ͷſͻ��˺�
	  void releaseClientID(u_short iClientID);
	  void releaseClientIDBySocket(u_int socketClient);

	  //�Ƴ������id��
	  void removeClientID(u_short iClientID);

	  //���ݿͻ��˵�socket�Ż�ÿͻ��˺�
	  u_short getClientIDBySocket(u_int socketClient);

	  //��õ�ǰ���ӵ��û�����
	  int getClientUserNum();


private:
	CModuleManager();//���캯��������˽�е�

private:
	  static CModuleManager* m_moduleManagerInstance; //��ʵ��ģʽ

	  pthread_mutex_t mutex_container;  //pthread�Դ��Ļ�����
	  pthread_mutex_t mutex_vecclient ; //pthread�Դ��Ļ�����

	  //��������
	  CBaseModule* m_arrayModule[MAX_MODULE_ID];    //�����ģ��,���±�ֵ��Ӧ"��ģ���"

	  /************************************************************************/
	  //��������û�м�������,�������ͻ���(�߳�)ͬʱ������/ɾ����,������������
	  //�쳣���
	  /************************************************************************/
	  vector<socket3element> m_vecClientSocket;     //������ӵĿͻ���socket
	  vector<u_short> m_vecNoUsedClientID;          //���δʹ�õ�client��id��
	  map<u_int,pthread_t *> m_mapThrd;     //�ͻ��˵��̴߳��

  //socket3element* socketelement;
};


#endif
