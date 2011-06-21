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

#else //linux系统使用
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#endif


/*
*socket的句柄,保存下来
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


  int sock_client; //客户端的socket号

#if defined(_WIN32) || defined(WIN32)
  struct sockaddr_in addr_client; //客户端的地址信息
#else
  struct sockaddr addr_client;
  //struct in_addr   addr_client;  
#endif
  //switch_socket_t *everySocket;  //每个客户端的socket结构

protected:
private:
};



void* proClientSocket(void* socketHandle);

void* waitForMutilClients(void* par);

//下面函数的实现在cpp文件中,为了提供给c调用,使用extern "C" 修饰
extern "C"
{
  //net server启动
  int cari_net_server_start();

  //关闭socket
  void shutdownSocketAndGarbage();
}
