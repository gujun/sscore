#ifndef CARINETNOTIFY_C_H
#define CARINETNOTIFY_C_H

#include <switch.h>

//���������֪ͨ
int cari_net_ne_reboot(int notifyCode,switch_event_t *event);

//mob user������״̬�ı��֪ͨ/����,ɾ���û���֪ͨ��֪ͨ���͵���Ϣ����
int cari_net_mobuserstatus_change(int notifyCode,switch_event_t *event);

//call����״̬֪ͨ
int cari_net_callstatus_change(int notifyCode,switch_event_t *event);

//��Ԫ�ı��֪ͨ/����,ɾ����֪ͨ��֪ͨ���͵���Ϣ����
int cari_net_ne_statechange(int notifyCode,switch_event_t *event);

#endif
