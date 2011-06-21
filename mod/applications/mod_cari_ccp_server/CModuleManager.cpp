
/*��ģ�������,����ģ��֮������Ľ��������ɴ�ģ�鸺��ת������
 *��ģ��֮�䲻ֱ���໥����
 */
//#pragma warning(disable:4786)
//#pragma warning(disable:4267)
//#pragma warning(disable:4996)

#include <sstream>
#include <iostream> 
#include <ostream>  
#include <fstream>
#include <string>

#include "CModuleManager.h"
#include "CMsgProcess.h"


////���ӿ�boost
//#include <boost/regex.hpp> //boost�������ռ�
//
//#include <boost/archive/basic_binary_iarchive.hpp>
//#include <boost/archive/basic_binary_oarchive.hpp>
//
//#include <boost/serialization/access.hpp>
//#include <boost/serialization/base_object.hpp>

// #include <portable_binary_oarchive.hpp>
// #include <portable_binary_iarchive.hpp>
// 
// #include <boost/serialization/base_object.hpp>

//using namespace boost;


//��̬��Ա�ĳ�ʼ��
CModuleManager	*CModuleManager::m_moduleManagerInstance	= CARI_CCP_VOS_NEW(CModuleManager);

// CBaseModule* arrayModule[MAX_MODULE_ID]; //�����ģ��,���±�ֵ��Ӧ"��ģ���"
// 
// vector<u_int> vecClientSocket;

/*
typedef struct common_CmdReq_Frame 
{
	unsigned short   sClientID;				  //�ͻ��˵�ID��			
	unsigned short   sSourceModuleID;   	  //Դģ���
	unsigned short   sDestModuleID; 		  //Ŀ��ģ���
	//string			 strLoginUserName;  	  //��¼�û���
	char			 strLoginUserName[CARICCP_MAX_NAME_LENGTH];
	//string			 timeStamp; 			  //ʱ���
	char			 timeStamp[CARICCP_MAX_TIME_LENGTH];
	
	bool   		  bSynchro; 			  //�����Ƿ�Ϊͬ������
	
	unsigned int	 iCmdCode;				  //������
	//string			 strCmdName;			 //������
	char			 strCmdName[CARICCP_MAX_NAME_LENGTH];
	
	//������,�Ƿ��������Ϊmap���д��??���ͺ���ն���ν�������
	//������Ϊ"�ַ���"����,�ڲ����з�װ
	//string strParamRegion;
	char	 strParamRegion[CARICCP_MAX_LENGTH];		  
	//map<string,string> m_mapParam; //�������Ϸ��ʹ˽ṹ������
	
}common_CmdReq_Frame;

*/

/*���캯��
 */
CModuleManager::CModuleManager()
{
	//���븺���ʼ�����е���ģ��,��:arrayModule����
	memset(m_arrayModule, 0, sizeof(m_arrayModule));

	//��ʼ����
	pthread_mutex_init(&mutex_container, NULL);
	pthread_mutex_init(&mutex_vecclient, NULL);

	//����δʹ�õ�client id��
	for (u_short id=1;id<=CARICCP_MAX_CLIENT_NUM;id++){
		m_vecNoUsedClientID.push_back(id);
	}

	//Ӧ�ø��������ļ������������,����Ӧ�����ౣ������
	m_arrayModule[BASE_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CBaseModule);
	m_arrayModule[USER_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_User);
	m_arrayModule[ROUTE_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_Route);
	m_arrayModule[SYSINFO_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_SysInfo);
	m_arrayModule[NE_MANAGER_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_NEManager);
	//arrayModule[11] = 
}

/*��������
 */
CModuleManager::~CModuleManager()
{
	//��ģ��ʵ�����ͷŲμ�clear()����
}

/*��õ�ʵ������
 */
CModuleManager *& CModuleManager::getInstance()
{
	if (NULL == m_moduleManagerInstance) {
		m_moduleManagerInstance = CARI_CCP_VOS_NEW(CModuleManager);
	}
	return m_moduleManagerInstance;
}

/*�����Դ
 */
void CModuleManager::clear()
{
	//m_moduleManagerInstance = NULL;
	for (int i=0;i < MAX_MODULE_ID; i++){
		CARI_CCP_VOS_DEL(m_arrayModule[i]);
	}
}

/*�����������ö�Ӧ��ģ���,view��ģ��źͺ�̨��ģ��Ų�һ��
 *�����߼���ͬ,�˴������������ö�Ӧ��ģ���
 */
int CModuleManager::getModuleID(int cmdCode)
{
	/*˵���˴�Ҫ��Ƴ������ļ����õķ�ʽ,����
	*
	*  ģ���     ģ������
	*    1  		ģ��Ļ���,����CBseModule
	*    11 		���������ģ��
	*    12 		���ģ��
	*    13         ϵͳ��Ϣͳ��ģ��
	*    14         ��Ԫ����ģ��
	*/

	//������������ڲ�����,�ڹ��ܻ����������ڲ�����,
	//���ǻ������ھ������ģ����д���

	int	iModuleID	= BASE_MODULE_CLASSID; //�������ģ��֮���໥��Ӧ
	switch (cmdCode) {
	case CARICCP_QUERY_MOB_USER:
		iModuleID = USER_MODULE_CLASSID; 	//MOB�û���ģ��
		break;

	//ϵͳ��Ϣ������
	case CARICCP_QUERY_SYSINFO:
	case CARICCP_QUERY_UPTIME:
	case CARICCP_QUERY_CPU_INFO:
	case CARICCP_QUERY_MEMORY_INFO:
	case CARICCP_QUERY_DISK_INFO:
	case CARICCP_QUERY_CPU_RATE:
		iModuleID = SYSINFO_MODULE_CLASSID; 	//ϵͳ��Ϣͳ��ģ��
		break;

	//��Ԫ�������Լ������û���
	case CARICCP_ADD_OP_USER:
	case CARICCP_DEL_OP_USER:
	case CARICCP_MOD_OP_USER:
	case CARICCP_LST_OP_USER:

	case CARICCP_ADD_EQUIP_NE:
	case CARICCP_DEL_EQUIP_NE:
	case CARICCP_MOD_EQUIP_NE:
	case CARICCP_LST_EQUIP_NE:
		
		iModuleID = NE_MANAGER_MODULE_CLASSID; 	//��Ԫ����ģ��
		break;

	default:
		 {
			 //0~200
			if (0 <= cmdCode && 200 >= cmdCode) {
				iModuleID = BASE_MODULE_CLASSID;		//"�ڲ�����"�Ĵ���ģ��
			} 
			//200~500
			else if (CARICCP_MOB_USER_BEGIN <= cmdCode 
				&& CARICCP_MOB_USER_END >= cmdCode) {
				iModuleID = USER_MODULE_CLASSID; 		//"MOB�û�"��ģ��
			}
			//504~
			else if (CARICCP_SET_ROUTER <= cmdCode){
				iModuleID = ROUTE_MODULE_CLASSID;		//"·��"����ģ��
			}
		 }
		
		 break;
	}

	return iModuleID;
}

/*��ô����͵�֡��ʵ�ʴ�С
*/
int getBufferSize(common_ResultResponse_Frame *&common_respFrame)
{
	//����ʵ�ʵ����ݽ��м���

	return sizeof(common_ResultResponse_Frame);
}

/*���жϴ�socket�Ƿ��Ѿ�����
 */
bool CModuleManager::isExistSocket(const socket3element &clientSocketElement)
{
	socket3element	socketTmp;	
	for (vector< socket3element>::iterator iter = m_vecClientSocket.begin(); iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (clientSocketElement.socketClient == socketTmp.socketClient) {
			return true;
		}
	}

	return false;
}

/*��Ե�thrdʵ���Ƿ��Ѿ�����
*/
bool CModuleManager::isExistThrd(u_int socketC)
{
	u_int id = 0;
	for (map<u_int,pthread_t *>::const_iterator map_it=m_mapThrd.begin();
		map_it !=m_mapThrd.end(); map_it++){
		id = map_it->first;
		if (id == socketC){
			return true;
		}
	}
	return false;
}

/*�����socket
 */
void CModuleManager::saveSocket(socket3element &clientSocketElement)
{
	if (isExistSocket(clientSocketElement)) {
		return;
	}

	//�ȼ���
	pthread_mutex_lock(&mutex_container);

	m_vecClientSocket.push_back(clientSocketElement);

	//�ͷ���
	pthread_mutex_unlock(&mutex_container);
}

/*ɾ����socket
 */
void CModuleManager::eraseSocket(const u_int socketC)
{
	//ϵͳshutdown��ʱ��������쳣,������ܴ���
	int oldtype;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	//�ȼ���
	pthread_mutex_lock(&mutex_container);

	socket3element						socketTmp;
	vector< socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketC == socketTmp.socketClient) {
			break;
		}
	}

	if (m_vecClientSocket.end() != iter) {
		m_vecClientSocket.erase(iter);
	}

	//�ͷ���
	pthread_mutex_unlock(&mutex_container);
	pthread_setcanceltype(oldtype, NULL);
}

/*����ͻ������ӵ��߳�ʵ��
*/
void CModuleManager::saveThrd(u_int socketC,
							  pthread_t *tid)
{
	if (isExistThrd(socketC)) {
		return;
	}

	//�ȼ���
	pthread_mutex_lock(&mutex_container);
	m_mapThrd.insert(map<u_int,pthread_t *>::value_type(socketC,tid));
	//�ͷ���
	pthread_mutex_unlock(&mutex_container);
}

/*ɾ���߳�ʵ��
*/
void CModuleManager::eraseThrd(const u_int socketC)
{
	if (isExistThrd(socketC)) {
		return;
	}

	//�ȼ���
	pthread_mutex_lock(&mutex_container);
	u_int id = 0;
	for (map<u_int,pthread_t *>::iterator map_it=m_mapThrd.begin();
		map_it !=m_mapThrd.end(); map_it++){
			id = map_it->first;
			if (id == socketC){
				m_mapThrd.erase(map_it);
				break;
			}
	}

	//�ͷ���
	pthread_mutex_unlock(&mutex_container);
}

/*kill all thrd
*/
void CModuleManager::killAllClientThrd()
{
	//�ȼ���
	pthread_mutex_lock(&mutex_container);

	u_int id = 0;
	pthread_t *thrd;
	for (map<u_int,pthread_t *>::iterator map_it=m_mapThrd.begin();
		map_it !=m_mapThrd.end(); map_it++){
			id = map_it->first;
			thrd = map_it->second;
			if (thrd){
				//ǿ��kill
				pthread_kill(*thrd,0);
			}
	}
	//ȫ�����
	m_mapThrd.clear();

	//�ͷ���
	pthread_mutex_unlock(&mutex_container);
}

/*�ر����пͻ��˵�socket
*/
void CModuleManager::closeAllClientSocket()
{
	//�ȼ���
	pthread_mutex_lock(&mutex_container);

	socket3element						socketTmp;
	int									sock_client;
	vector< socket3element>::iterator	iter	= m_vecClientSocket.begin();
	while (iter != m_vecClientSocket.end()){
		socketTmp = *iter;
		sock_client = socketTmp.socketClient;
		if (0 > sock_client) {
			continue;
		}

		//�رյ�ǰ�ͻ��˵�socket
		CARI_CLOSE_SOCKET(sock_client);
		iter   =   m_vecClientSocket.erase(iter); 
	}

// ����ķ���,��ϵͳshutdown��ʱ��,������쳣Expression: ("this->_Mycont != NULL", 0) (debugģʽ)
//  ��Ҫ������ ++iter����
//	for (; iter != m_vecClientSocket.end(); ++iter) {
//		socketTmp = *iter;
//		sock_client = socketTmp.socketClient;
//		if (0 > sock_client) {
//			continue;
//		}
//
//		//�رյ�ǰ�ͻ��˵�socket
//	    CARI_CLOSE_SOCKET(sock_client);
//	}
	m_vecClientSocket.clear();

	//�ͷ���
	pthread_mutex_unlock(&mutex_container);
}

/*����socket�ŷ��ض�Ӧ����Ԫ��Ԫ��
 */
int CModuleManager::getSocket(const u_int socketC, socket3element &element)
{
	socket3element						socketTmp;
	vector< socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketC == socketTmp.socketClient) {
			//return &socketTmp;	//���ؾֲ�����,������
			element.addr = socketTmp.addr;
			element.sClientID = socketTmp.sClientID;
			element.socketClient = socketTmp.socketClient;

			return CARICCP_SUCCESS_STATE_CODE;
		}
	}

	return CARICCP_ERROR_STATE_CODE;
}

/*����ģ���ȡ����Ӧ����ģ��ľ��
*/
CBaseModule * CModuleManager::getSubModule(int iModuleID)
{
	if (0 > iModuleID 
		|| MAX_MODULE_ID <= iModuleID){
		return NULL;
	}

	return m_arrayModule[iModuleID]; //ȡ����ŵ���ģ��
}

/*
*/
vector< socket3element> & CModuleManager::getSocketVec()
{
	return m_vecClientSocket;
}

/*��id���Ƿ��Ѿ�ʹ��,��:�Ѿ������ĳ���ͻ���ʹ��
*/
bool CModuleManager::isUsedID(u_short id)
{
	bool bExisted = false;
	socket3element	socketTmp;
	vector<socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (id == socketTmp.sClientID) {
			bExisted = true;
			break ;
		}
	}

	return bExisted;
}

/*����ͻ��˺�
*/
u_short CModuleManager::assignClientID()
{
	//�ȼ���
	pthread_mutex_lock(&mutex_vecclient);

	u_short iAssignedID = CARICCP_MAX_SHORT;

	//�������ķ���Ž��д���
    for (int id=1;id <= CARICCP_MAX_CLIENT_NUM;id++){

		//��id�Ƿ�δ����
        u_short iTmp = CARICCP_MAX_SHORT;
		for (vector<u_short>::iterator it = m_vecNoUsedClientID.begin();it != m_vecNoUsedClientID.end(); it++){
			iTmp = *it;
			if (id ==iTmp){
				iAssignedID = id;
				//��������ɾ����
				m_vecNoUsedClientID.erase(it);
				goto end;
			}
		}
    }

end:
	//�ͷ���
	pthread_mutex_unlock(&mutex_vecclient);
	return iAssignedID;
}

/*�ͷſͻ��˺�
*/
void CModuleManager::releaseClientID(u_short iClientID)
{
	/*����Ҫע�����,����̴߳���PTHREAD_CANCEL_ASYNCHRONOUS״̬,����ξ��п��ܳ���,
	*��ΪCANCEL�¼��п�����pthread_cleanup_push()��pthread_mutex_lock()֮�䷢��,������
	*pthread_mutex_unlock()��pthread_cleanup_pop()֮�䷢��,�Ӷ�����������unlockһ����
	*û�м�����mutex����,��ɴ������,��ʹ����������ʱ��,��Ӧ����ʱ���ó�PTHREAD_CANCEL_DEFERREDģʽ��
	*Ϊ��,POSIX��Linuxʵ���л��ṩ��һ�Բ���֤����ֲ��
	*�ο� :http://hi.baidu.com/linzhangkun/blog/item/da250d3ec226e2f3838b137f.html
	*/
	//�ȼ���,�������Ч???shutdown()������ʱ��˴���ʱ������쳣����
	//ʹ������ķ�����ܴ���һ��
	int oldtype;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
	pthread_mutex_lock(&mutex_vecclient);

	if (CARICCP_MAX_SHORT != iClientID 
		&& CARICCP_MAX_CLIENT_NUM >= iClientID){
		m_vecNoUsedClientID.push_back(iClientID);
	}

	//�ͷ���
	pthread_mutex_unlock(&mutex_vecclient);
	pthread_setcanceltype(oldtype, NULL);
}

/*���ǵ�Ԫ�ص��ڴ�������ܷ����仯,���Ǿ�������ȴ����,����socket�Ž����ͷŶ�Ӧ��client��
*/
void CModuleManager::releaseClientIDBySocket(u_int socketClient)
{
	u_short iClientId = CARICCP_MAX_SHORT;
	socket3element	socketTmp;
	vector<socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketClient == socketTmp.socketClient) {
			iClientId = socketTmp.sClientID;//ȡ����Ӧ��client id��
			break ;
		}
	}

	//�ͷ�id��
	releaseClientID(iClientId);
 }

/*�Ƴ�ID��
*/
void CModuleManager::removeClientID(u_short iClientID)
{
	//�ȼ���
	pthread_mutex_lock(&mutex_vecclient);

	u_short iTmp = CARICCP_MAX_SHORT;
	for (vector<u_short>::iterator it = m_vecNoUsedClientID.begin();it != m_vecNoUsedClientID.end(); it++){
		iTmp = *it;
		if (iClientID ==iTmp){
			m_vecNoUsedClientID.erase(it);
			break;
		}
	}

	//�ͷ���
	pthread_mutex_unlock(&mutex_vecclient);
}

/*��ÿͻ��˺�
*/
u_short CModuleManager::getClientIDBySocket(u_int socketClient)
{
	u_short iAssignedID = CARICCP_MAX_SHORT;

	socket3element	socketTmp;
	vector< socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketClient == socketTmp.socketClient) {
			iAssignedID = socketTmp.sClientID;//ȡ���Ѿ�����ŵ�id��
			break ;
		}
	}
	return iAssignedID;
}

/*��õ�ǰ��¼���û�������
*/
int CModuleManager::getClientUserNum()
{
	return m_vecClientSocket.size();
}
