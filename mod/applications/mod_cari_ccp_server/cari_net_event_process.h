#ifndef CARINETEVENTPROCESS_C_H
#define CARINETEVENTPROCESS_C_H

#include <switch.h>


//下面函数的实现在cpp文件中,为了提供给c调用,使用extern "C" 修饰
extern "C"
{
  //一般的处理结果和通知类型的处理分开

  //处理获得的结果
  int cari_net_event_recvResult(switch_event_t* event);

  //通知事件处理
  int cari_net_event_notify(switch_event_t* event);

  //详细的事件处理---------------------------------------
  //int cari_net_mobuserstatus_change(int notifyCode,switch_event_t *event);
}

#endif
