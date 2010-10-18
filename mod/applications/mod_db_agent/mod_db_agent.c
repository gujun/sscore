/*
 * CARI mod in FreeSWITCH
 * Copyright(C)2009-2010,Ritchie Gu <ritchie.gu@gmail.com>
 *
 * Version:MPL 1.1
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is
 * Ritchie Gu <ritchie.gu@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * mod_db_agent.c -- db agent for cdr,recording,sms
 */
#include <switch.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_db_agent_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_db_agent_shutdown);
SWITCH_MODULE_DEFINITION(mod_db_agent,mod_db_agent_load,mod_db_agent_shutdown,NULL);



#define CDR_EVENT "event::custom::cdr"
#define RECORDING_EVENT "event::custom::recording"
#define SMS_EVENT "event::custom::sms"
#define SMS_REDO_EVENT "event::custom::sms::rodo"
#define REG_EVENT "sofia::register"
#define EVENT_DISPAT_KB "dispat_kb::info"
#define DISPAT_CHAT_PROTO "dispat"
//#define CALL_DISPATCHER_FAIL_EVENT "call::dispatcher::fail"

#define SMS_QUEUE_SIZE 5000
static const char *DB_AGENT_CONF_FILE="db_agent.conf";
static int sms_1 = 1;
static int sms_0 = 0;

const char *default_cdr_template = "\"${caller_id_name}\",\"${caller_id_number}\",\"${destination_number}\"\n";
const char *default_recording_template = "\"${call_name}\",\"${start_stamp}\",\"${end_stamp}\",\"${file_path}\",\"${file_len}\"\n";
const char *default_sms_template = "\"${proto}\",\"${to}\",\"${from}\",\"${subject}\",\"${body}\",\"${type}\",\"${hint}\"\n";

static struct {
  
	int running;

	char *odbc_dsn;
	char *odbc_user;
	char *odbc_pass;

	switch_odbc_handle_t *master_odbc;

	char *dbname;

	switch_hash_t *template_hash;

	char *cdr_template;
	char *recording_template;
	char *g_recording_template;
	char *sms_ok_template;
	char *sms_err_template;

	int sms_call;
	int check_disk_rec_number;
	int disk_used_high;
	int disk_used_low;

	switch_event_node_t *cdr_event_node;
  
	switch_event_node_t *recording_event_node;

	switch_event_node_t *sms_event_node;

	switch_event_node_t *reg_event_node;

	switch_event_node_t *event_dispat_kb_node;

	switch_event_node_t *xml_event_node;

	switch_queue_t *sms_queue;
	switch_hash_t *sms_id_hash;

	int32_t threads;
	switch_memory_pool_t *pool;
	switch_mutex_t *mutex;

	
}globals;

static char g_recordings_sql[] =
	"CREATE TABLE g_recordings (\n"
	"   call_name      VARCHAR(255),\n"
	"   start_epoch    INTEGER,\n"
	"   end_epoch     INTEGER,\n"
	"   start_stamp    TIMESTAMP,\n"
	"   end_stamp     TIMESTAMP,\n"
	"   file_path     VARCHAR(255),\n" 
	"   file_len   INTEGER,\n" 
	"   flags         INTEGER,\n" 
	"   read_flags    INTEGER\n" 
	");\n";

static char recordings_sql[] =
	"CREATE TABLE recordings (\n"
	"   caller_id_name VARCHAR(255),\n"
	"   caller_id_number VARCHAR(255),\n"
	"   uuid VARCHAR(255),\n"
	"   bound VARCHAR(255),\n"
	"   call_name     VARCHAR(255),\n"
	"   start_epoch   INTEGER,\n"
	"   end_epoch INTEGER,\n"
	"   start_stamp   TIMESTAMP,\n"
	"   end_stamp     TIMESTAMP,\n"
	"   file_path     VARCHAR(255),\n" 
	"   file_len   INTEGER,\n" 
	"   flags         INTEGER,\n" 
	"   read_flags    INTEGER\n" 
	");\n";
static char sms_ok_sql[] = 
	" CREATE TABLE sms_ok (\n"
	"    chat VARCHAR(255),\n"
	"    sms_to VARCHAR(255),\n"
	"    proto  VARCHAR(255),\n"
	"    sms_from VARCHAR(255),\n"
	"    subject VARCHAR(255),\n"
	"    body  VARCHAR(1023),\n"
	"    sms_type  VARCHAR(255),\n"
	"    hint  VARCHAR(255),\n"
	"    start_epoch INTEGER,\n"
	"    start_stamp  TIMESTAMP\n"
	");\n";

static char sms_err_sql[] = 
	" CREATE TABLE sms_err (\n"
	"    chat VARCHAR(255),\n"
	"    sms_to VARCHAR(255),\n"
	"    proto  VARCHAR(255),\n"
	"    sms_from VARCHAR(255),\n"
	"    subject VARCHAR(255),\n"
	"    body  VARCHAR(1023),\n"
	"    sms_type  VARCHAR(255),\n"
	"    hint  VARCHAR(255),\n"
	"    start_epoch INTEGER,\n"
	"    start_stamp TIMESTAMP\n"
	");\n";

static char cdr_sql[] = 
	" CREATE TABLE cdr (\n"
	"    caller_id_name VARCHAR(255),\n"
	"    caller_id_number VARCHAR(255),\n"
	"    destination_number  VARCHAR(255),\n"
	"    dialed_extension VARCHAR(255),\n"
	"    context VARCHAR(255),\n"
	"    context_extension  VARCHAR(255),\n"
	"    start_epoch  INTEGER,\n"
	"    answer_epoch   INTEGER,\n"
	"    end_epoch   INTEGER,\n"
	"    start_stamp  TIMESTAMP,\n"
	"    answer_stamp TIMESTAMP,\n"
	"    end_stamp   TIMESTAMP,\n"
	"    duration    INTEGER,\n"
	"    billsec    INTEGER,\n"
	"    hangup_cause  VARCHAR(255),\n"
	"    uuid  VARCHAR(255),\n"
	"    bleg_uuid VARCHAR(255),\n"
	"    read_codec VARCHAR(255),\n"
	"    write_codec VARCHAR(255),\n"
	"    flags INTEGER \n"
	");\n";

static char *index_list[] = {
	"create index g_recordings_idx1 on g_recordings(start_stamp)",
	"create index recordings_idx1 on recordings(start_stamp)",
	"create index recordings_idx2 on recordings(caller_id_number)",
	"create index sms_ok_idx1 on sms_ok(start_stamp)",
	"create index sms_ok_idx2 on sms_ok(sms_from)",
	"create index sms_err_idx1 on sms_err(start_stamp)",
	"create index sms_err_idx2 on sms_err(sms_to)",
	"create index cdr_idx1 on cdr(start_stamp)",
	"create index cdr_idx2 on cdr(caller_id_number)",
	NULL
};

static switch_status_t free_odbc(){
	if(switch_odbc_available() && globals.master_odbc){
		switch_odbc_handle_destroy(&globals.master_odbc);
	}

	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t init_sql(switch_mutex_t *mutex)
{

	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_core_db_t *db = NULL;
	int x = 0;
	if(mutex){
		switch_mutex_lock(mutex);
	}
	free_odbc();
	if (switch_odbc_available() && globals.odbc_dsn) {
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"init odbc sql\n");
		if (!(globals.master_odbc = switch_odbc_handle_new(globals.odbc_dsn, globals.odbc_user, globals.odbc_pass))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
			goto end;

		}
		if (switch_odbc_handle_connect(globals.master_odbc) != SWITCH_ODBC_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open ODBC Database!\n");
			goto end;
		}

		if (switch_odbc_handle_exec(globals.master_odbc, "select count(start_epoch) from g_recordings", NULL, NULL) != SWITCH_ODBC_SUCCESS) {
			switch_odbc_handle_exec(globals.master_odbc, "drop table g_recordings", NULL, NULL);
			switch_odbc_handle_exec(globals.master_odbc, g_recordings_sql, NULL, NULL);
		}

		if (switch_odbc_handle_exec(globals.master_odbc, "select count(start_epoch) from recordings", NULL, NULL) != SWITCH_ODBC_SUCCESS) {
			switch_odbc_handle_exec(globals.master_odbc, "drop table recordings", NULL, NULL);
			switch_odbc_handle_exec(globals.master_odbc, recordings_sql, NULL, NULL);
		}

		if (switch_odbc_handle_exec(globals.master_odbc, "select count(start_epoch) from sms_ok", NULL, NULL) != SWITCH_ODBC_SUCCESS) {
			switch_odbc_handle_exec(globals.master_odbc, "drop table sms_ok", NULL, NULL);
			switch_odbc_handle_exec(globals.master_odbc, sms_ok_sql, NULL, NULL);
		}

		if (switch_odbc_handle_exec(globals.master_odbc, "select count(start_epoch) from sms_err", NULL, NULL) != SWITCH_ODBC_SUCCESS) {
			switch_odbc_handle_exec(globals.master_odbc, "drop table sms_err", NULL, NULL);
			switch_odbc_handle_exec(globals.master_odbc, sms_err_sql, NULL, NULL);
		}

		if (switch_odbc_handle_exec(globals.master_odbc, "select count(start_epoch) from cdr", NULL, NULL) != SWITCH_ODBC_SUCCESS) {
			switch_odbc_handle_exec(globals.master_odbc, "drop table cdr", NULL, NULL);
			switch_odbc_handle_exec(globals.master_odbc, cdr_sql, NULL, NULL);
		}

		for (x = 0; index_list[x]; x++) {
			switch_odbc_handle_exec(globals.master_odbc, index_list[x], NULL, NULL);
		}
	} else if(globals.dbname){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"init core sql\n");		
		if ((db = switch_core_db_open_file(globals.dbname))) {
			char *errmsg;
			switch_core_db_test_reactive(db, "select count(start_epoch) from g_recordings", "drop table g_recordings", g_recordings_sql);
			switch_core_db_test_reactive(db, "select count(start_epoch) from recordings", "drop table recordings", recordings_sql);
			switch_core_db_test_reactive(db, "select count(start_epoch) from sms_ok", "drop table sms_ok", sms_ok_sql);
			switch_core_db_test_reactive(db, "select count(start_epoch) from sms_err", "drop table sms_err", sms_err_sql);
			switch_core_db_test_reactive(db, "select count(start_epoch) from cdr", "drop table cdr", cdr_sql);
									
			for (x = 0; index_list[x]; x++) {
				errmsg = NULL;
				switch_core_db_exec(db, index_list[x], NULL, NULL, &errmsg);
				switch_safe_free(errmsg);
			}

		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Cannot Open SQL Database!\n");
			goto end;
		}
		switch_core_db_close(db);
	}

	status = SWITCH_STATUS_SUCCESS;
 end:
	if(mutex){
		switch_mutex_unlock(mutex);
	}

	return status;
}

static switch_status_t load_conf()
{
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_xml_t cfg=NULL, xml=NULL, settings=NULL,param=NULL;
  switch_memory_pool_t *pool = globals.pool;

  globals.sms_call = 1;
  globals.check_disk_rec_number = 10;
  globals.disk_used_high = 85;
  globals.disk_used_low = 80;

  if(!(xml = switch_xml_open_cfg(DB_AGENT_CONF_FILE,&cfg,NULL))){
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open of %s failed\n",DB_AGENT_CONF_FILE);
    status = SWITCH_STATUS_FALSE;
  } else {
    if((settings = switch_xml_child(cfg,"settings"))){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"settings\n");
      for(param = switch_xml_child(settings,"param");param;param=param->next){
	char *var = (char *)switch_xml_attr_soft(param,"name");
	char *val = (char *)switch_xml_attr_soft(param,"value");

	if(!strcmp(var,"odbc-dsn")){
		   globals.odbc_dsn = switch_core_strdup(pool,val);
    }else if(!strcmp(var,"dbname")){
		globals.dbname = switch_core_strdup(pool,val);
	}else if(!strcmp(var,"cdr-template")){
	  globals.cdr_template = switch_core_strdup(pool,val);
	}else if(!strcmp(var,"g-recording-template")){
	  globals.g_recording_template = switch_core_strdup(pool,val);
	}else if(!strcmp(var,"recording-template")){
	  globals.recording_template = switch_core_strdup(pool,val);
	}else if(!strcmp(var,"sms-ok-template")){
	  globals.sms_ok_template = switch_core_strdup(pool,val);
	}else if(!strcmp(var,"sms-err-template")){
	  globals.sms_err_template = switch_core_strdup(pool,val);
	}else if(!strcmp(var,"sms-call")){
		if(switch_true(val)){
			globals.sms_call = 1;
		}else{
			globals.sms_call = 0;
		}
	}else if(!strcmp(var,"check-disk-rec-number")){
		globals.check_disk_rec_number = atoi(val);
	}else if(!strcmp(var,"disk-used-high")){
		globals.disk_used_high = atoi(val);
	}else if(!strcmp(var,"disk-used-low")){
		globals.disk_used_low = atoi(val);
	}

      }//for
    }//"settings"
    if(globals.disk_used_low > globals.disk_used_high){
		int tmp = globals.disk_used_low;
		globals.disk_used_low = globals.disk_used_high;
		globals.disk_used_high = tmp;
	}

    if ((settings = switch_xml_child(cfg, "templates"))) {
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"templates\n");
      for (param = switch_xml_child(settings, "template"); param; param = param->next) {
	char *var = (char *) switch_xml_attr(param, "name");
	if (var) {
	  char *tpl;
	  size_t len = strlen(param->txt) + 2;
	  if (end_of(param->txt) != '\n') {
	    tpl = switch_core_alloc(pool, len);
	    switch_snprintf(tpl, len, "%s\n", param->txt);
	  } else {
	    tpl = switch_core_strdup(pool, param->txt);
	  }

	  switch_core_hash_insert(globals.template_hash, var, tpl);
	  //switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Adding template %s.\n", var);
	}
      }
    }


    switch_xml_free(xml);

	if (!switch_strlen_zero(globals.odbc_dsn)) {
		if ((globals.odbc_user = strchr(globals.odbc_dsn, ':'))) {
			*(globals.odbc_user++) = '\0';
			if ((globals.odbc_pass = strchr(globals.odbc_user, ':'))) {
				*(globals.odbc_pass++) = '\0';
			}
		}
	}

	init_sql(globals.mutex);


  }

  
  return status;
}

static switch_status_t execute_sql(char *sql,switch_mutex_t *mutex)
{
	switch_core_db_t *db;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

	if (switch_odbc_available() && globals.odbc_dsn) {
		switch_odbc_statement_handle_t stmt;
		if (switch_odbc_handle_exec(globals.master_odbc, sql, &stmt, NULL) != SWITCH_ODBC_SUCCESS) {
			char *err_str;
			err_str = switch_odbc_handle_get_error(globals.master_odbc, stmt);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ERR: [%s]\n[%s]\n", sql, switch_str_nil(err_str));
			switch_safe_free(err_str);
			status = SWITCH_STATUS_FALSE;
		}
		switch_odbc_statement_handle_free(&stmt);
	} else if(globals.dbname){
		if (!(db = switch_core_db_open_file(globals.dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", globals.dbname);
			status = SWITCH_STATUS_FALSE;
			goto end;
		}
		status = switch_core_db_persistant_execute(db, sql, 1);
		switch_core_db_close(db);
	}

 end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}
	return status;
}

static switch_bool_t execute_sql_callback(char *sql,switch_core_db_callback_func_t callback, void *pdata, switch_mutex_t *mutex)
{
	switch_bool_t ret = SWITCH_FALSE;
	switch_core_db_t *db;
	char *errmsg = NULL;

	if (mutex) {
		switch_mutex_lock(mutex);
	}

	if (switch_odbc_available() && globals.odbc_dsn) {
		switch_odbc_handle_callback_exec(globals.master_odbc, sql, callback, pdata, NULL);
	} else if(globals.dbname){
		if (!(db = switch_core_db_open_file(globals.dbname))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error Opening DB %s\n", globals.dbname);
			goto end;
		}

		switch_core_db_exec(db, sql, callback, pdata, &errmsg);

		if (errmsg) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "SQL ERR: [%s] %s\n", sql, errmsg);
			free(errmsg);
		}

		if (db) {
			switch_core_db_close(db);
		}
	}

 end:
	if (mutex) {
		switch_mutex_unlock(mutex);
	}
	return ret;
}


void sms_queue_event(switch_event_t *event);

static int SMS_THREAD_RUNNING = 0;
static int SMS_THREAD_STARTED = 0;
//static int CHECK_DISK_THREAD_RUNNING = 0;
//static int CHECK_DISK_THREAD_STARTED = 0;

static void sms_queue_event_handler(switch_event_t *event){
	const char *name = switch_event_get_header(event,"chat");
	const char *to = switch_event_get_header(event,"sms_to");
	const char *proto = switch_event_get_header(event,"proto");
	const char *from = switch_event_get_header(event,"sms_from");
	const char *subject = switch_event_get_header(event,"subject");
	const char *body = switch_event_get_header(event,"body");
	const char *type = switch_event_get_header(event,"sms_type");
	const char *hint = switch_event_get_header(event,"hint");
	if( !to  || !proto || !body){
		return;
	}

	if(switch_strlen_zero(name)){
		name = "sip";
	}
	switch_core_chat_send(name,proto,from,to,subject,body,type,hint);
}

static int getUsePresent(char *stream){
	/*
ok
Filesystem             Size   Used  Avail Use% Mounted on
/dev/mapper/VolGroup00-LogVol00
                       271G    50G   207G  20% /
*/

	char *data = NULL,*dat=NULL;
	char *Filesystem = NULL;
	char *Size = NULL;
	char *Used = NULL;
	char *Avail = NULL;
	char *Use = NULL;
	char *Mounted_on = NULL;
	int ret = 0;
	if(stream == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"stream null\n");
		return -1;
	}
	data = strdup(stream);
	if(data == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strdup null\n");
		return -1;
	}
	dat = data;
	if( 
	   (Filesystem = strstr(data,"Filesystem")) == NULL ||
	   (Size = strstr(data,"Size")) == NULL ||
	   (Used = strstr(data,"Used"))== NULL ||
	   (Avail = strstr(data,"Avail")) == NULL ||
	   (Use = strstr(data,"Use")) == NULL ||
	   (Mounted_on = strstr(data,"Mounted on"))==NULL){
		
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s\n",data);
		switch_safe_free(data);
		return -1;
	}
	data = Mounted_on+sizeof("Mounted on");
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"%s\n",data);
#define getFromData(A) \
	while((*data) =='\n' || (*data)=='\r' || (*data) ==' '  || (*data)=='\t'){	\
			data++; \
	} \
    A= data;\
	while((*data)!=' ' && (*data)!='\t' && (*data)!='\r' && (*data)!='\n' && (*data) != '\0'){ \
		data++; \
	} \
	if((*data)=='\0'){ \
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"data not valid:%s\n",dat); \
		switch_safe_free(dat); \
		return -1; \
	} \
	*data = '\0'; \
	data++;

	getFromData(Filesystem);
	getFromData(Size);
	getFromData(Used);
	getFromData(Avail);
	getFromData(Use);

	
	while((*Use)>='0' && (*Use)<='9'){
		ret = ret * 10;
		ret += (*Use) - '0';
		Use++;
	}
	
	switch_safe_free(dat);
	return ret;
	
}
static void update_recording_flag(switch_event_t *event){
	char * update_sql_t = "update recordings set flags=1 where flags=0 and caller_id_number='${caller_id_number}' and uuid='${uuid}' and start_stamp='${start_stamp}' and file_path='${file_path}'";
	char * update_sql = switch_event_expand_headers(event,update_sql_t);

	if(update_sql != NULL){
		switch_mutex_lock(globals.mutex);
		execute_sql(update_sql,NULL);
		switch_mutex_unlock(globals.mutex);

		if(update_sql != update_sql_t){
			switch_safe_free(update_sql);
		}
	}
}

static int check_disk_callback(void *pArg,int argc,char **argv,char **columnNames)
{
	switch_event_t *event;
	const char *file_path = NULL;// (char *)pArg;
	int *ok = (int *)pArg;
	int i;
	if(argc >= 3){
		
		if(switch_event_create_subclass(&event,SWITCH_EVENT_CUSTOM,SMS_REDO_EVENT) == SWITCH_STATUS_SUCCESS){
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,"redo-type","rec_flag");
			for(i = 0; i< argc; i++){
				switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,columnNames[i],argv[i]);
			}
			file_path = switch_event_get_header(event,"file_path");
			if(file_path != NULL){
				if(unlink(file_path)==0){

					switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"delete:%s\n",file_path);
					//sms_queue_event(event);
					
				}else{
					switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"ERROR delete:%s\n",file_path);
					//switch_event_destroy(&event);
				}
				update_recording_flag(event);
			}
			
			switch_event_destroy(&event);
			
		}
	}
					if(ok != NULL){
						(*ok)++;
					}

	return 0;
}

static void check_disk(){
	switch_event_t *event;
	if(switch_event_create_subclass(&event,SWITCH_EVENT_CUSTOM,SMS_REDO_EVENT) == SWITCH_STATUS_SUCCESS){
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,"redo-type","check_disk");
	
			sms_queue_event(event);
	
	}

}
static void check_disk_event_handler(switch_event_t *event){
	//	check_disk();
	char cmdbuf[128];
	char * select_sql = "select * from recordings where flags=0 order by start_stamp limit 10";
	//char * update_sql = "";
	switch_stream_handle_t mystream = { 0 };
	int used = 0;
	int first = 1;
	int ok = 0;

	do{
		ok = 0;
		//mystream={0};
	SWITCH_STANDARD_STREAM(mystream);

	/*kick user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"df -H .");
	if((switch_api_execute("stream_system",cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		
		used = getUsePresent(mystream.data);

		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"disk used:%d,global:%d-%d\n",used,
						  globals.disk_used_low,globals.disk_used_high);		
	} 

	switch_safe_free(mystream.data);
	if(first){
		first = 0;
		if(used < globals.disk_used_high){
			return;
		}
	}
	if(used > globals.disk_used_low){
		switch_mutex_lock(globals.mutex);
		execute_sql_callback(select_sql,check_disk_callback,&ok,NULL);
		switch_mutex_unlock(globals.mutex);
	}
	
	if(ok ==0){
		return;
	}
	}while(used > globals.disk_used_low);


}

void *SWITCH_THREAD_FUNC sms_queue_thread_run(switch_thread_t *thread,void *obj)
{
	void *pop;
	int done = 0;
	int count = 0;
	switch_event_t *event = NULL;
	const char *redo_type = NULL;
	switch_mutex_lock(globals.mutex);
	if(!SMS_THREAD_RUNNING){
		SMS_THREAD_RUNNING++;
		globals.threads++;
	} else {
		done =1 ;
	}
	switch_mutex_unlock(globals.mutex);
	if(done){
		return NULL;
	}
	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_NOTICE,"sms queue thread start\n");

	while(globals.running){
		count = 0;
		if(switch_queue_trypop(globals.sms_queue,&pop) == SWITCH_STATUS_SUCCESS){
			if(!pop){
				break;
			}
			event= (switch_event_t *)pop;
			redo_type = switch_event_get_header(event,"redo-type");
			if(redo_type != NULL){
				if(!strcmp(redo_type,"sms")){
					sms_queue_event_handler(event);
				}
				else if(!strcmp(redo_type,"check_disk")){
					check_disk_event_handler(event);
				}
				else if(!strcmp(redo_type,"rec_flag")){
					update_recording_flag(event);
				}
			}
			switch_event_destroy(&event);
			count++;
		}
		if(!count){
			switch_yield(1000000);
		}
	}

	while(switch_queue_trypop(globals.sms_queue,&pop) == SWITCH_STATUS_SUCCESS){
		event = (switch_event_t *)pop;
		if(event){
			switch_event_destroy(&event);
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"sms queue thread ended\n");
	
	switch_mutex_lock(globals.mutex);
	globals.threads--;
	SMS_THREAD_RUNNING = SMS_THREAD_STARTED = 0;
	switch_mutex_unlock(globals.mutex);
	
	return NULL;
}
void sms_queue_thread_start(void)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;
	int done = 0;

	switch_mutex_lock(globals.mutex);
	if(!SMS_THREAD_STARTED){
		SMS_THREAD_STARTED++;
	} else {
		done = 1;
	}
	switch_mutex_unlock(globals.mutex);
	if(done){
		return;
	}
	
	switch_threadattr_create(&thd_attr,globals.pool);
	switch_threadattr_detach_set(thd_attr,1);
	switch_threadattr_stacksize_set(thd_attr,SWITCH_THREAD_STACKSIZE);
	switch_threadattr_priority_increase(thd_attr);
	switch_thread_create(&thread,thd_attr,sms_queue_thread_run,NULL,globals.pool);
}

void sms_queue_event(switch_event_t *event)
{
	//	switch_event_t * cloned_event;
	if(!event){
		return;
	}
	//switch_event_dup(&cloned_event,event);
	//switch_assert(cloned_event);
	switch_queue_push(globals.sms_queue,event);

	if(!SMS_THREAD_STARTED){
		sms_queue_thread_start();
	}
}

static char *dup_id_from_sms_to(const char *sms_to){
	char *start=NULL;
	char *end=NULL;
	char *k = NULL;
	
	char *ret = NULL;
	char *to = NULL;
	char *to_end = NULL;
	char c;

	if(!sms_to){
		return NULL;
	}
	to = strdup(sms_to);
	if(!to){
		return NULL;
	}
	to_end = to+strlen(to);

	k = strchr(to,'@');
	if(k){
		k--;
		while(k >= to && *k == ' '){
			k--;
		}
		end = k+1;

		while(k >= to && *k <='9' && *k >= '0'){
			k--;
		}
		start = k+1;
		
		if(start >= to && start != end){
			c = *end;
			*end= '\0';
			ret = strdup(start);
			*end = c;
			goto done;
		}
	}
    
	k = strrchr(to,'/');
	if(k){
		k++;
		while(k <= to_end && *k == ' ' ){
			k++;
		}
		start = k;
		
		while(k <= to_end && *k>='0' && *k <= '9'){
			k++;
		}
		end = k;
		
		if(start != end){
			if(end == to_end){
				ret = strdup(start);
			}
			else{
				c = *end;
				*end = '\0';
				ret = strdup(start);
				*end = c;
			}
			
		}
		
	}

 done:
	switch_safe_free(to);
	return ret;

}
static void sms_event_handler(switch_event_t *event){
	const char *template = NULL;
	char *sql = NULL;
	char * is_ok = NULL;
	int ok = 0;
	char *id = NULL;
	int *sms = NULL;

	if(!event){
		return;
	}

	is_ok = switch_event_get_header(event,"is_ok");
	if(!is_ok){
		return;
	}
	ok = atoi(is_ok);

	if(ok){
		if(!globals.sms_ok_template || !globals.template_hash ||
		   (template = (const char *)switch_core_hash_find(globals.template_hash,globals.sms_ok_template)) == NULL)
			{
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"template null\n");
			  return;
			}
	}
	else{
		if(!globals.sms_err_template || !globals.template_hash ||
		   (template = (const char *)switch_core_hash_find(globals.template_hash,globals.sms_err_template)) == NULL)
			{
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"template null\n");
			  return;
			}
		id = dup_id_from_sms_to(switch_event_get_header(event,"to"));
		if(id){
			if(*id != '\0'){
			switch_mutex_lock(globals.mutex);
			sms = switch_core_hash_find(globals.sms_id_hash,id);
			if(sms){
				switch_core_hash_insert(globals.sms_id_hash,id,&sms_1);
			}
			else{
				switch_core_hash_insert(globals.sms_id_hash,switch_core_strdup(globals.pool,id),&sms_1);
			}
			
			switch_mutex_unlock(globals.mutex);
			//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s sms_1\n",id);
			}
			switch_safe_free(id);
		}
	}

	sql = switch_event_expand_headers(event,template);

	if(sql){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,sql);
	  execute_sql(sql, globals.mutex);
	  if(sql != template){
		switch_safe_free(sql);
	  }

	}
	else{
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"sms event:sql NULL");
	}

}
/*
static void dispatcher_fail_cdr_event(switch_event_t *event){
	const char *hangup_cause = switch_event_get_header(event,"hangup_cause");
	const char *context_extension = switch_event_get_header(event,"context_extension");

		const char *caller_id_number = switch_event_get_header(event,"caller_id_number");
		const char *caller_id_name = switch_event_get_header(event,"caller_id_name");
		const char *uuid = switch_event_get_header(event,"uuid");
		const char *start_stamp = switch_event_get_header(event,"start_stamp");


		switch_event_t *sevent = NULL;


	if(hangup_cause && context_extension && caller_id_number && start_stamp && uuid && 
	   !strstr(hangup_cause,"SUCCESS") &&
	   !strcmp(context_extension,"dispatcher") ){
		if (switch_event_create_subclass(&sevent, SWITCH_EVENT_CUSTOM,CALL_DISPATCHER_FAIL_EVENT) == SWITCH_STATUS_SUCCESS) {
			
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"caller_id_number",caller_id_number);
			
			if(!switch_strlen_zero(caller_id_name)){
				switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"caller_id_name",caller_id_name);
			}
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"start_stamp",start_stamp);
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"hangup_cause",hangup_cause);
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"uuid",uuid);

			switch_event_fire(&sevent);
		}
	}
	
}
*/
static void cdr_to_sms(switch_event_t *event){
	const char *hangup_cause = switch_event_get_header(event,"hangup_cause");
	const char *context_extension = switch_event_get_header(event,"context_extension");
	const char *destination_number = switch_event_get_header(event,"destination_number");
		const char *caller_id_number = switch_event_get_header(event,"caller_id_number");
		const char *caller_id_name = switch_event_get_header(event,"caller_id_name");
		const char *dialed_extension = switch_event_get_header(event,"dialed_extension");
		const char *start_stamp = switch_event_get_header(event,"start_stamp");
		const char *start_epoch = switch_event_get_header(event,"start_epoch");

		switch_event_t *sevent = NULL;


	if(hangup_cause && context_extension && destination_number &&
	   strstr(hangup_cause,"NOT_REGISTERED") &&
	   !strcmp(context_extension,"local") ){
		if (switch_event_create(&sevent, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
			
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"is_ok","0");
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"chat","sip");
			if(switch_strlen_zero(dialed_extension)){
				dialed_extension = destination_number;
			}
			switch_event_add_header(sevent,SWITCH_STACK_BOTTOM,"to","internal/%s",dialed_extension);
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"proto","agent");
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"from","0");
			//switch_event_add_header(sevent,SWITCH_STACK_BOTTOM,"subject","");
			if(switch_strlen_zero(caller_id_name) || !strcmp(caller_id_name,caller_id_number)){
				switch_event_add_header(sevent,SWITCH_STACK_BOTTOM,"body","尊敬的客户您好！%s在%s给您来电，请及时回复。",caller_id_number,start_stamp);
			}
			else{
				switch_event_add_header(sevent,SWITCH_STACK_BOTTOM,"body","尊敬的客户您好！%s（%s）在%s给您来电，请及时回复。",caller_id_number,caller_id_name,start_stamp);
			}
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"type","text/plain");
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"start_epoch",start_epoch);
			switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"start_stamp",start_stamp);
			sms_event_handler(sevent);
			switch_event_destroy(&sevent);
		}
	}

}
static void cdr_event_handler(switch_event_t *event){
	const char *template = NULL;
	char *sql = NULL;
	if(!event){
		return;
	}
	if(!globals.cdr_template || !globals.template_hash ||
	   (template = (const char *)switch_core_hash_find(globals.template_hash,globals.cdr_template)) == NULL)
	{
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"template null\n");
		return;
	}
	
	sql = switch_event_expand_headers(event,template);

	if(sql){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,sql);
	  execute_sql(sql, globals.mutex);
	  if(globals.sms_call){
		  cdr_to_sms(event);
	  }
	  
	  //dispatcher_fail_cdr_event(event);
	  if(sql != template){
		  switch_safe_free(sql);
	  }

	}
	else{
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"cdr event:sql NULL");
	}

}

static void recording_event_handler(switch_event_t *event){
	const char *template = NULL;
	char *sql = NULL;
	char * is_auto = NULL;
	int g = 0;
	static int rec_n =0;
	if(!event){
		return;
	}

	is_auto = switch_event_get_header(event,"is_auto");
	if(!is_auto){
		return;
	}
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"is auto = %s\n",is_auto);
	g = atoi(is_auto);

	if(g){
		if(!globals.g_recording_template || !globals.template_hash ||
		   (template = (const char *)switch_core_hash_find(globals.template_hash,globals.g_recording_template)) == NULL)
			{
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"template null\n");
			  return;
			}
	}
	else{
		if(!globals.recording_template || !globals.template_hash ||
		   (template = (const char *)switch_core_hash_find(globals.template_hash,globals.recording_template)) == NULL)
			{
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"template null\n");
			  return;
			}
	
	}

	sql = switch_event_expand_headers(event,template);

	if(sql){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,sql);
	  execute_sql(sql, globals.mutex);
	  if(sql != template){
		switch_safe_free(sql);
	  }
	  rec_n++;
	  if(rec_n >= globals.check_disk_rec_number){
		  rec_n = 0;
		  check_disk();
	  }
	}
	else{
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"recording event:sql NULL");
	}


}

static int sms_queue_callback(void *pArg,int argc,char **argv,char **columnNames)
{
	switch_event_t *event;
	//char *id = (char *)pArg;
	int i;
	if(argc >= 8){
		if(switch_event_create_subclass(&event,SWITCH_EVENT_CUSTOM,SMS_REDO_EVENT) == SWITCH_STATUS_SUCCESS){
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,"redo-type","sms");
			for(i = 0; i< argc; i++){
				switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,columnNames[i],argv[i]);
			}
			sms_queue_event(event);
		}
	}

	return 0;
}
static void reg_event_handler(switch_event_t *event){
	/*if (switch_event_create_subclass(&s_event, SWITCH_EVENT_CUSTOM, MY_EVENT_REGISTER) == SWITCH_STATUS_SUCCESS) {
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "profile-name", profile->name);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "from-user", to_user);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "from-host", reg_host);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "presence-hosts", profile->presence_hosts ? profile->presence_hosts : reg_host);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "contact", contact_str);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "call-id", call_id);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "rpid", rpid);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "statusd", reg_desc);
	  switch_event_add_header(s_event, SWITCH_STACK_BOTTOM, "expires", "%ld", (long) exptime);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "to-user", from_user);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "to-host", from_host);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "network-ip", network_ip);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "network-port", network_port_c);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "username", username);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "realm", realm);
	  switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "user-agent", agent);
	  switch_event_fire(&s_event);
	  }*/
	//switch_status_t status = SWITCH_STATUS_SUCCESS;
	//char  reply[256] = "";
	//switch_size_t reply_len = sizeof(reply);
	char sql[512] = "";
	int *sms = NULL;
	const char *user = switch_event_get_header(event, "username");//sip_auth_user;
	//char *agent = switch_event_get_header(event, "user-agent");
	//char *ip = switch_event_get_header(event, "network-ip");
	//char *port = switch_event_get_header(event, "network-port");
	if(!user){
		return;
	}
	sms = switch_core_hash_find(globals.sms_id_hash,user);
	if(sms && (*sms == 0)){
		return;
	}
	switch_snprintf(sql,sizeof(sql),"select * from sms_err where sms_to like '%s@%%' or sms_to like '%%/%s'",user,user);
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s\n",sql);
	switch_mutex_lock(globals.mutex);
	execute_sql_callback(sql,sms_queue_callback,NULL,NULL);

	switch_snprintf(sql,sizeof(sql),"delete from sms_err where sms_to like '%s@%%' or sms_to like '%%/%s'",user,user);
	execute_sql(sql,NULL);
	if(sms){
		switch_core_hash_insert(globals.sms_id_hash,user,&sms_0);
	} else {
		switch_core_hash_insert(globals.sms_id_hash,switch_core_strdup(globals.pool,user),&sms_0);
	}
    
	switch_mutex_unlock(globals.mutex);
}
/*
static void dispat_kb_reg_event_handler(switch_event_t *event){
	
	char sql[512] = "";
		
	switch_snprintf(sql,sizeof(sql),"select * from sms_err where chat like '%%%s%%'",DISPAT_CHAT_PROTO);
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s\n",sql);
	switch_mutex_lock(globals.mutex);
	execute_sql_callback(sql,sms_queue_callback,NULL,NULL);

	switch_snprintf(sql,sizeof(sql),"delete from sms_err where chat like '%%%s%%'",DISPAT_CHAT_PROTO);
	execute_sql(sql,NULL);
	switch_mutex_unlock(globals.mutex);
}
*/
static void event_dispat_kb_handler(switch_event_t *event){
	const char *action = switch_event_get_header(event,"action");
	if(switch_strlen_zero(action)){
		return;
	}
	if(!strcmp(action,"message")){
		sms_event_handler(event);
	} 
	/*else if(!strcmp(action,"reg")){
		dispat_kb_reg_event_handler(event);
	}*/
}
static void xml_event_handler(switch_event_t *event){
	load_conf();
}
#define STREAM_SYSTEM "<command>"
SWITCH_STANDARD_API(stream_system)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
#ifdef WIN32
	stream->write_function(stream,"<!-- exec not implemented int windowns yet -->");
#else
	int fds[2],pid = 0,bytes = 0;
	char buf[256] = "";
	if (switch_strlen_zero(cmd)) {
        stream->write_function(stream, "-USAGE: %s\n", STREAM_SYSTEM);
        return SWITCH_STATUS_SUCCESS;
    } 
	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Executing command: %s\n", cmd);
	
	if(pipe(fds)){
		stream->write_function(stream,"err,pipe error\n");
		goto end;
	}
	pid = fork();
	if(pid < 0){
		close(fds[0]);
		close(fds[1]);
		stream->write_function(stream,"err,fork error\n");
		goto end;
	}
	if(pid){//parent
		close(fds[1]);
		stream->write_function(stream, "ok\n");
		while((bytes = read(fds[0],buf,sizeof(buf)-1)) > 0){
			buf[bytes]='\0';
			//printf("%s",buf);
			stream->write_function(stream,"%s",buf);
		}
		close(fds[0]);
		//*(char *)(((char *)(stream->data))+(stream->data_size)-1)='\0';
		//int ret = getUsePresent(buf);
		//stream->write_function(stream,"\n%d\n",ret);
	} else {//child
		close(fds[0]);
		dup2(fds[1],STDOUT_FILENO);
		if(switch_system(cmd,SWITCH_TRUE) < 0){
			printf("Failed to execute command:%s\n",cmd);
		}
		close(fds[1]);
		exit(0);
	}
    ///if (switch_system(cmd, SWITCH_TRUE) < 0) {
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Failed to execute command: %s\n", cmd);
    //}
    


#endif
 end:
    return status;
}
SWITCH_MODULE_LOAD_FUNCTION(mod_db_agent_load)
{
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_api_interface_t *commands_api_interface;

  if(switch_event_reserve_subclass(SMS_REDO_EVENT) != SWITCH_STATUS_SUCCESS){
	  switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Counldn't register subclass %s\n",SMS_REDO_EVENT);
	  return SWITCH_STATUS_TERM;
  }
  /*
  if(switch_event_reserve_subclass(CALL_DISPATCHER_FAIL_EVENT) != SWITCH_STATUS_SUCCESS){
	  switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Counldn't register subclass %s\n",CALL_DISPATCHER_FAIL_EVENT);
	  return SWITCH_STATUS_TERM;
  }*/
  memset(&globals, 0, sizeof(globals));
  
   switch_core_new_memory_pool(&globals.pool);
   switch_core_hash_init(&globals.template_hash,globals.pool);
   switch_core_hash_init(&globals.sms_id_hash,globals.pool);
   switch_mutex_init(&globals.mutex,SWITCH_MUTEX_NESTED,globals.pool);
   switch_queue_create(&globals.sms_queue,SMS_QUEUE_SIZE,globals.pool);
  
 
  load_conf();

  globals.running = 1;
 

  // globals.pool = pool;

  /* Subscribe to presence request events*/
  if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,CDR_EVENT,
				 cdr_event_handler,NULL,&globals.cdr_event_node) != SWITCH_STATUS_SUCCESS) {
    return SWITCH_STATUS_GENERR;
  }

  if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,RECORDING_EVENT,
				 recording_event_handler,NULL,&globals.recording_event_node) != SWITCH_STATUS_SUCCESS) {
    return SWITCH_STATUS_GENERR;
  }
  if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,SMS_EVENT,
				 sms_event_handler,NULL,&globals.sms_event_node) != SWITCH_STATUS_SUCCESS) {
    return SWITCH_STATUS_GENERR;
  }
  if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,REG_EVENT,
				 reg_event_handler,NULL,&globals.reg_event_node) != SWITCH_STATUS_SUCCESS) {
    return SWITCH_STATUS_GENERR;
  }
  if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,EVENT_DISPAT_KB,
				 event_dispat_kb_handler,NULL,&globals.event_dispat_kb_node) != SWITCH_STATUS_SUCCESS) {
    return SWITCH_STATUS_GENERR;
  }
  if (switch_event_bind_removable(modname, SWITCH_EVENT_RELOADXML, 
								  NULL, xml_event_handler, NULL, &globals.xml_event_node) != SWITCH_STATUS_SUCCESS) {

	  return SWITCH_STATUS_GENERR;
  }
  *module_interface = switch_loadable_module_create_module_interface(pool, modname);

  SWITCH_ADD_API(commands_api_interface,"stream_system","stream_system",stream_system,STREAM_SYSTEM);
  
  switch_console_set_complete("add stream_system");

  return status;
}


SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_db_agent_shutdown)
{

	switch_memory_pool_t *pool = globals.pool;
	switch_mutex_t *mutex = globals.mutex;
	int sanity = 0;
  
  globals.running = 0;
  
  
  switch_event_unbind(&globals.cdr_event_node);
  switch_event_unbind(&globals.recording_event_node);
  switch_event_unbind(&globals.sms_event_node);
  switch_event_unbind(&globals.event_dispat_kb_node);
  switch_event_unbind(&globals.xml_event_node);

  while(globals.threads){
	  switch_cond_next();
	  if(++sanity >= 60000){
		  break;
	  }
  }
  switch_event_free_subclass(SMS_REDO_EVENT);
  //switch_event_free_subclass(CALL_DISPATCHER_FAIL_EVENT);

  switch_mutex_lock(mutex);
  switch_core_hash_destroy(&globals.template_hash);
  switch_core_hash_destroy(&globals.sms_id_hash);
  free_odbc();
  switch_mutex_unlock(mutex);
  


  memset(&globals,0,sizeof(globals));
  
  switch_core_destroy_memory_pool(&pool);

  return SWITCH_STATUS_SUCCESS;
}



/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
