#ifndef CARINETEVENTPROCESS_C_H
#define CARINETEVENTPROCESS_C_H

#include <switch.h>


//���溯����ʵ����cpp�ļ���,Ϊ���ṩ��c����,ʹ��extern "C" ����
extern "C"
{
  //һ��Ĵ�������֪ͨ���͵Ĵ���ֿ�

  //�����õĽ��
  int cari_net_event_recvResult(switch_event_t* event);

  //֪ͨ�¼�����
  int cari_net_event_notify(switch_event_t* event);

  //��ϸ���¼�����---------------------------------------
  //int cari_net_mobuserstatus_change(int notifyCode,switch_event_t *event);
}

#endif
