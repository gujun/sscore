
/*子模块管理类,所有模块之间命令的交互都是由此模块负责转发处理
 *子模块之间不直接相互调用
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


////附加库boost
//#include <boost/regex.hpp> //boost的命名空间
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


//静态成员的初始化
CModuleManager	*CModuleManager::m_moduleManagerInstance	= CARI_CCP_VOS_NEW(CModuleManager);

// CBaseModule* arrayModule[MAX_MODULE_ID]; //存放子模块,其下标值对应"子模块号"
// 
// vector<u_int> vecClientSocket;

/*
typedef struct common_CmdReq_Frame 
{
	unsigned short   sClientID;				  //客户端的ID号			
	unsigned short   sSourceModuleID;   	  //源模块号
	unsigned short   sDestModuleID; 		  //目的模块号
	//string			 strLoginUserName;  	  //登录用户名
	char			 strLoginUserName[CARICCP_MAX_NAME_LENGTH];
	//string			 timeStamp; 			  //时间戳
	char			 timeStamp[CARICCP_MAX_TIME_LENGTH];
	
	bool   		  bSynchro; 			  //命令是否为同步命令
	
	unsigned int	 iCmdCode;				  //命令码
	//string			 strCmdName;			 //命令名
	char			 strCmdName[CARICCP_MAX_NAME_LENGTH];
	
	//参数区,是否可以设置为map进行存放??发送后接收端如何解析？？
	//先设置为"字符串"类型,内部进行封装
	//string strParamRegion;
	char	 strParamRegion[CARICCP_MAX_LENGTH];		  
	//map<string,string> m_mapParam; //在网络上发送此结构有问题
	
}common_CmdReq_Frame;

*/

/*构造函数
 */
CModuleManager::CModuleManager()
{
	//必须负责初始化所有的子模块,既:arrayModule数组
	memset(m_arrayModule, 0, sizeof(m_arrayModule));

	//初始化锁
	pthread_mutex_init(&mutex_container, NULL);
	pthread_mutex_init(&mutex_vecclient, NULL);

	//分配未使用的client id号
	for (u_short id=1;id<=CARICCP_MAX_CLIENT_NUM;id++){
		m_vecNoUsedClientID.push_back(id);
	}

	//应该根据配置文件进行灵活适配,将对应的子类保持起来
	m_arrayModule[BASE_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CBaseModule);
	m_arrayModule[USER_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_User);
	m_arrayModule[ROUTE_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_Route);
	m_arrayModule[SYSINFO_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_SysInfo);
	m_arrayModule[NE_MANAGER_MODULE_CLASSID] = CARI_CCP_VOS_NEW(CModule_NEManager);
	//arrayModule[11] = 
}

/*析构函数
 */
CModuleManager::~CModuleManager()
{
	//子模块实例的释放参见clear()函数
}

/*获得单实例对象
 */
CModuleManager *& CModuleManager::getInstance()
{
	if (NULL == m_moduleManagerInstance) {
		m_moduleManagerInstance = CARI_CCP_VOS_NEW(CModuleManager);
	}
	return m_moduleManagerInstance;
}

/*清除资源
 */
void CModuleManager::clear()
{
	//m_moduleManagerInstance = NULL;
	for (int i=0;i < MAX_MODULE_ID; i++){
		CARI_CCP_VOS_DEL(m_arrayModule[i]);
	}
}

/*根据命令码获得对应的模块号,view的模块号和后台的模块号不一致
 *处理逻辑不同,此处根据命令码获得对应的模块号
 */
int CModuleManager::getModuleID(int cmdCode)
{
	/*说明此处要设计成配置文件配置的方式,灵活处理
	*
	*  模块号     模块名称
	*    1  		模块的基类,代表CBseModule
	*    11 		命令管理类模块
	*    12 		监控模块
	*    13         系统信息统计模块
	*    14         网元管理模块
	*/

	//先区分特殊的内部命令,在功能划分上属性内部命令,
	//但是还是属于具体的子模块进行处理

	int	iModuleID	= BASE_MODULE_CLASSID; //命令码和模块之间相互对应
	switch (cmdCode) {
	case CARICCP_QUERY_MOB_USER:
		iModuleID = USER_MODULE_CLASSID; 	//MOB用户的模块
		break;

	//系统信息处理类
	case CARICCP_QUERY_SYSINFO:
	case CARICCP_QUERY_UPTIME:
	case CARICCP_QUERY_CPU_INFO:
	case CARICCP_QUERY_MEMORY_INFO:
	case CARICCP_QUERY_DISK_INFO:
	case CARICCP_QUERY_CPU_RATE:
		iModuleID = SYSINFO_MODULE_CLASSID; 	//系统信息统计模块
		break;

	//网元管理类以及操作用户类
	case CARICCP_ADD_OP_USER:
	case CARICCP_DEL_OP_USER:
	case CARICCP_MOD_OP_USER:
	case CARICCP_LST_OP_USER:

	case CARICCP_ADD_EQUIP_NE:
	case CARICCP_DEL_EQUIP_NE:
	case CARICCP_MOD_EQUIP_NE:
	case CARICCP_LST_EQUIP_NE:
		
		iModuleID = NE_MANAGER_MODULE_CLASSID; 	//网元管理模块
		break;

	default:
		 {
			 //0~200
			if (0 <= cmdCode && 200 >= cmdCode) {
				iModuleID = BASE_MODULE_CLASSID;		//"内部命令"的处理模块
			} 
			//200~500
			else if (CARICCP_MOB_USER_BEGIN <= cmdCode 
				&& CARICCP_MOB_USER_END >= cmdCode) {
				iModuleID = USER_MODULE_CLASSID; 		//"MOB用户"的模块
			}
			//504~
			else if (CARICCP_SET_ROUTER <= cmdCode){
				iModuleID = ROUTE_MODULE_CLASSID;		//"路由"设置模块
			}
		 }
		
		 break;
	}

	return iModuleID;
}

/*获得待发送的帧的实际大小
*/
int getBufferSize(common_ResultResponse_Frame *&common_respFrame)
{
	//根据实际的数据进行计算

	return sizeof(common_ResultResponse_Frame);
}

/*先判断此socket是否已经存在
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

/*针对的thrd实例是否已经存在
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

/*保存此socket
 */
void CModuleManager::saveSocket(socket3element &clientSocketElement)
{
	if (isExistSocket(clientSocketElement)) {
		return;
	}

	//先加锁
	pthread_mutex_lock(&mutex_container);

	m_vecClientSocket.push_back(clientSocketElement);

	//释放锁
	pthread_mutex_unlock(&mutex_container);
}

/*删除此socket
 */
void CModuleManager::eraseSocket(const u_int socketC)
{
	//系统shutdown的时候会引发异常,次数规避处理
	int oldtype;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

	//先加锁
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

	//释放锁
	pthread_mutex_unlock(&mutex_container);
	pthread_setcanceltype(oldtype, NULL);
}

/*保存客户端连接的线程实例
*/
void CModuleManager::saveThrd(u_int socketC,
							  pthread_t *tid)
{
	if (isExistThrd(socketC)) {
		return;
	}

	//先加锁
	pthread_mutex_lock(&mutex_container);
	m_mapThrd.insert(map<u_int,pthread_t *>::value_type(socketC,tid));
	//释放锁
	pthread_mutex_unlock(&mutex_container);
}

/*删除线程实例
*/
void CModuleManager::eraseThrd(const u_int socketC)
{
	if (isExistThrd(socketC)) {
		return;
	}

	//先加锁
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

	//释放锁
	pthread_mutex_unlock(&mutex_container);
}

/*kill all thrd
*/
void CModuleManager::killAllClientThrd()
{
	//先加锁
	pthread_mutex_lock(&mutex_container);

	u_int id = 0;
	pthread_t *thrd;
	for (map<u_int,pthread_t *>::iterator map_it=m_mapThrd.begin();
		map_it !=m_mapThrd.end(); map_it++){
			id = map_it->first;
			thrd = map_it->second;
			if (thrd){
				//强制kill
				pthread_kill(*thrd,0);
			}
	}
	//全部清除
	m_mapThrd.clear();

	//释放锁
	pthread_mutex_unlock(&mutex_container);
}

/*关闭所有客户端的socket
*/
void CModuleManager::closeAllClientSocket()
{
	//先加锁
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

		//关闭当前客户端的socket
		CARI_CLOSE_SOCKET(sock_client);
		iter   =   m_vecClientSocket.erase(iter); 
	}

// 下面的方法,在系统shutdown的时候,会出现异常Expression: ("this->_Mycont != NULL", 0) (debug模式)
//  主要是由于 ++iter引起
//	for (; iter != m_vecClientSocket.end(); ++iter) {
//		socketTmp = *iter;
//		sock_client = socketTmp.socketClient;
//		if (0 > sock_client) {
//			continue;
//		}
//
//		//关闭当前客户端的socket
//	    CARI_CLOSE_SOCKET(sock_client);
//	}
	m_vecClientSocket.clear();

	//释放锁
	pthread_mutex_unlock(&mutex_container);
}

/*根据socket号返回对应的三元组元素
 */
int CModuleManager::getSocket(const u_int socketC, socket3element &element)
{
	socket3element						socketTmp;
	vector< socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketC == socketTmp.socketClient) {
			//return &socketTmp;	//返回局部变量,有问题
			element.addr = socketTmp.addr;
			element.sClientID = socketTmp.sClientID;
			element.socketClient = socketTmp.socketClient;

			return CARICCP_SUCCESS_STATE_CODE;
		}
	}

	return CARICCP_ERROR_STATE_CODE;
}

/*根据模块号取出对应的子模块的句柄
*/
CBaseModule * CModuleManager::getSubModule(int iModuleID)
{
	if (0 > iModuleID 
		|| MAX_MODULE_ID <= iModuleID){
		return NULL;
	}

	return m_arrayModule[iModuleID]; //取出存放的子模块
}

/*
*/
vector< socket3element> & CModuleManager::getSocketVec()
{
	return m_vecClientSocket;
}

/*此id号是否已经使用,即:已经分配给某个客户端使用
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

/*分配客户端号
*/
u_short CModuleManager::assignClientID()
{
	//先加锁
	pthread_mutex_lock(&mutex_vecclient);

	u_short iAssignedID = CARICCP_MAX_SHORT;

	//根据最大的分配号进行处理
    for (int id=1;id <= CARICCP_MAX_CLIENT_NUM;id++){

		//此id是否还未分配
        u_short iTmp = CARICCP_MAX_SHORT;
		for (vector<u_short>::iterator it = m_vecNoUsedClientID.begin();it != m_vecNoUsedClientID.end(); it++){
			iTmp = *it;
			if (id ==iTmp){
				iAssignedID = id;
				//从容器中删除掉
				m_vecNoUsedClientID.erase(it);
				goto end;
			}
		}
    }

end:
	//释放锁
	pthread_mutex_unlock(&mutex_vecclient);
	return iAssignedID;
}

/*释放客户端号
*/
void CModuleManager::releaseClientID(u_short iClientID)
{
	/*必须要注意的是,如果线程处于PTHREAD_CANCEL_ASYNCHRONOUS状态,代码段就有可能出错,
	*因为CANCEL事件有可能在pthread_cleanup_push()和pthread_mutex_lock()之间发生,或者在
	*pthread_mutex_unlock()和pthread_cleanup_pop()之间发生,从而导致清理函数unlock一个并
	*没有加锁的mutex变量,造成错误。因此,在使用清理函数的时候,都应该暂时设置成PTHREAD_CANCEL_DEFERRED模式。
	*为此,POSIX的Linux实现中还提供了一对不保证可移植的
	*参考 :http://hi.baidu.com/linzhangkun/blog/item/da250d3ec226e2f3838b137f.html
	*/
	//先加锁,如果锁无效???shutdown()操作的时候此处有时会出现异常现象
	//使用下面的方法规避处理一下
	int oldtype;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
	pthread_mutex_lock(&mutex_vecclient);

	if (CARICCP_MAX_SHORT != iClientID 
		&& CARICCP_MAX_CLIENT_NUM >= iClientID){
		m_vecNoUsedClientID.push_back(iClientID);
	}

	//释放锁
	pthread_mutex_unlock(&mutex_vecclient);
	pthread_setcanceltype(oldtype, NULL);
}

/*考虑到元素的内存变量可能发生变化,但是具体数据却不变,根据socket号进行释放对应的client号
*/
void CModuleManager::releaseClientIDBySocket(u_int socketClient)
{
	u_short iClientId = CARICCP_MAX_SHORT;
	socket3element	socketTmp;
	vector<socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketClient == socketTmp.socketClient) {
			iClientId = socketTmp.sClientID;//取出对应的client id号
			break ;
		}
	}

	//释放id号
	releaseClientID(iClientId);
 }

/*移除ID号
*/
void CModuleManager::removeClientID(u_short iClientID)
{
	//先加锁
	pthread_mutex_lock(&mutex_vecclient);

	u_short iTmp = CARICCP_MAX_SHORT;
	for (vector<u_short>::iterator it = m_vecNoUsedClientID.begin();it != m_vecNoUsedClientID.end(); it++){
		iTmp = *it;
		if (iClientID ==iTmp){
			m_vecNoUsedClientID.erase(it);
			break;
		}
	}

	//释放锁
	pthread_mutex_unlock(&mutex_vecclient);
}

/*获得客户端号
*/
u_short CModuleManager::getClientIDBySocket(u_int socketClient)
{
	u_short iAssignedID = CARICCP_MAX_SHORT;

	socket3element	socketTmp;
	vector< socket3element>::iterator	iter	= m_vecClientSocket.begin();
	for (; iter != m_vecClientSocket.end(); ++iter) {
		socketTmp = *iter;
		if (socketClient == socketTmp.socketClient) {
			iAssignedID = socketTmp.sClientID;//取出已经分配号的id号
			break ;
		}
	}
	return iAssignedID;
}

/*获得当前登录的用户的数量
*/
int CModuleManager::getClientUserNum()
{
	return m_vecClientSocket.size();
}
