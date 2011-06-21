#ifndef CARINETMANAGER_C_H
#define CARINETMANAGER_C_H

#include <switch.h>
#include "switch_types.h"
#include "switch_odbc.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void cari_net_lock();
  void cari_net_unlock();

  /*��ö�Ӧ�����ݿ����ӵľ��
  */
  switch_odbc_handle_t* cari_net_getOdbcHandle();

  //sql����ִ��,����Ҫ���ؽ����
  switch_status_t cari_execute_sql(char* sql, switch_event_t* outMsg);

  ////sql����ִ��,����Ҫ���ؽ����
  //switch_status_t cari_execute_sql2(char *sql);


#ifdef __cplusplus
}
#endif

#endif /* TESTC_H */

