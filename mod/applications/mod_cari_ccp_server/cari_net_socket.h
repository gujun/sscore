#include <switch.h>

#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"
#include "CGarbageRecover.h"



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>


#if defined(_WIN32) || defined(WIN32)

#else //linuxϵͳʹ��
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#endif


/*
*socket�ľ��,��������
*/
class CSocketHandle
{
public:
  CSocketHandle()
  {
  };


public:
  //SOCKET  sock_client;
  //sockaddr_in addr_client;

  //su_socket_t      sock_client;
  //struct sockaddr  addr_client;

  //switch_socket_t    sock_client;
  //switch_sockaddr_t  addr_client;


  int sock_client; //�ͻ��˵�socket��

#if defined(_WIN32) || defined(WIN32)
  struct sockaddr_in addr_client; //�ͻ��˵ĵ�ַ��Ϣ
#else
  struct sockaddr addr_client;
  //struct in_addr   addr_client;  
#endif
  //switch_socket_t *everySocket;  //ÿ���ͻ��˵�socket�ṹ

protected:
private:
};



void* proClientSocket(void* socketHandle);

void* waitForMutilClients(void* par);

//���溯����ʵ����cpp�ļ���,Ϊ���ṩ��c����,ʹ��extern "C" ����
extern "C"
{
  //net server����
  int cari_net_server_start();

  //�ر�socket
  void shutdownSocketAndGarbage();
}
