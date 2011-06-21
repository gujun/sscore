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

  /*获得对应的数据库连接的句柄
  */
  switch_odbc_handle_t* cari_net_getOdbcHandle();

  //sql语句的执行,不需要返回结果集
  switch_status_t cari_execute_sql(char* sql, switch_event_t* outMsg);

  ////sql语句的执行,不需要返回结果集
  //switch_status_t cari_execute_sql2(char *sql);


#ifdef __cplusplus
}
#endif

#endif /* TESTC_H */

