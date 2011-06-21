#ifndef CARINETNOTIFY_C_H
#define CARINETNOTIFY_C_H

#include <switch.h>

//重启服务的通知
int cari_net_ne_reboot(int notifyCode,switch_event_t *event);

//mob user的在线状态的变更通知/增加,删除用户的通知等通知类型的消息处理
int cari_net_mobuserstatus_change(int notifyCode,switch_event_t *event);

//call呼叫状态通知
int cari_net_callstatus_change(int notifyCode,switch_event_t *event);

//网元的变更通知/增加,删除的通知等通知类型的消息处理
int cari_net_ne_statechange(int notifyCode,switch_event_t *event);

#endif
