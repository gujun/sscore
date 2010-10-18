/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2010, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
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
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * mod_cdr_csv.c -- Asterisk Compatible CDR Module
 *
 */
#include <sys/stat.h>
#include <switch.h>


#define CDR_EVENT "event::custom::cdr"

typedef enum {
	CDR_LEG_A = (1 << 0),
	CDR_LEG_B = (1 << 1)
} cdr_leg_t;

struct cdr_fd {
	//int fd;
	switch_file_t *fd;
	char *path;
	int64_t bytes;
	switch_mutex_t *mutex;
};
typedef struct cdr_fd cdr_fd_t;

const char *default_template =
	"\"${caller_id_name}\",\"${caller_id_number}\",\"${destination_number}\",\"${context}\",\"${start_stamp}\","
	"\"${answer_stamp}\",\"${end_stamp}\",\"${duration}\",\"${billsec}\",\"${hangup_cause}\",\"${uuid}\",\"${bleg_uuid}\", \"${accountcode}\"\n";

static struct {
	switch_memory_pool_t *pool;
	switch_hash_t *fd_hash;
	switch_hash_t *template_hash;
	char *log_dir;
	char *default_template;
	int masterfileonly;
	int shutdown;
	int rotate;
	int debug;
	cdr_leg_t legs;
} globals;

SWITCH_MODULE_LOAD_FUNCTION(mod_cdr_csv_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cdr_csv_shutdown);
SWITCH_MODULE_DEFINITION(mod_cdr_csv, mod_cdr_csv_load, mod_cdr_csv_shutdown, NULL);
/*
static off_t fd_size(int fd)
{
	struct stat s = { 0 };
	fstat(fd, &s);
	return s.st_size;
}
*/
static void do_close(cdr_fd_t *fd){
	if(fd == NULL || fd->fd == NULL){
		return;
	}
	
	if(switch_file_close(fd->fd) == SWITCH_STATUS_SUCCESS){
		fd->fd = NULL;
	}
}

static switch_status_t do_open(cdr_fd_t *fd){
	unsigned int flags = 0;

	if(fd == NULL || fd->path == NULL){
		return SWITCH_STATUS_FALSE;
	}

	do_close(fd);
	fd->fd = NULL;

	flags |= SWITCH_FOPEN_CREATE;
	flags |= SWITCH_FOPEN_READ;
	flags |= SWITCH_FOPEN_WRITE;
	flags |= SWITCH_FOPEN_APPEND;

	if (switch_file_open(&fd->fd, fd->path, flags, SWITCH_FPROT_UREAD | SWITCH_FPROT_UWRITE|SWITCH_FPROT_WREAD|SWITCH_FPROT_GREAD, globals.pool) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error opening %s\n",fd->path);
		fd->fd = NULL;
		return SWITCH_STATUS_FALSE;
	}
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t do_reopen(cdr_fd_t *fd)
{
	int x = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;

	if(fd == NULL || fd->path == NULL){
		return SWITCH_STATUS_FALSE;
	}
	//if (fd->fd > -1) {
	do_close(fd);
	fd->fd = NULL;

	for (x = 0; x < 10; x++) {
		//if ((fd->fd = open(fd->path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) > -1) {
		if(do_open(fd) == SWITCH_STATUS_SUCCESS && fd->fd != NULL){
			fd->bytes = switch_file_get_size(fd->fd);//fd_size(fd->fd);
			status = SWITCH_STATUS_SUCCESS;
			break;
		}
		switch_yield(100000);
	}
	return status;

}

static switch_status_t do_rotate(cdr_fd_t *fd)
{
	switch_time_exp_t tm;
	char date[80] = "";
	switch_size_t retsize;
	char *p;
	size_t len;

	switch_status_t status = SWITCH_STATUS_FALSE;
	if(fd == NULL){
		return SWITCH_STATUS_FALSE;
	}
	//close(fd->fd);
	//fd->fd = -1;
	
	do_close(fd);
	fd->fd = NULL;


	if (globals.rotate) {
		switch_time_exp_lt(&tm, switch_micro_time_now());
		switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d-%H-%M-%S", &tm);

		len = strlen(fd->path) + strlen(date) + 2;
		p = switch_mprintf("%s.%s", fd->path, date);
		assert(p);
		switch_file_rename(fd->path, p, globals.pool);
		free(p);
	}

	status = do_reopen(fd);

	//if (fd->fd < 0) {
	if(fd->fd == NULL){
		switch_event_t *event;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Error opening %s\n", fd->path);
		if (switch_event_create(&event, SWITCH_EVENT_TRAP) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Critical-Error", "Error opening cdr file %s\n", fd->path);
			switch_event_fire(&event);
		}
		status = SWITCH_STATUS_FALSE;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "%s CDR logfile %s\n", globals.rotate ? "Rotated" : "Re-opened", fd->path);
	}

	return status;
}

static void write_cdr(const char *path, const char *log_line)
{
	cdr_fd_t *fd = NULL;
	//unsigned int bytes_in, bytes_out;
	switch_size_t bytes_in , bytes_out ;
	//int loops = 0;

	if (!(fd = switch_core_hash_find(globals.fd_hash, path))) {
		fd = switch_core_alloc(globals.pool, sizeof(*fd));
		switch_assert(fd);
		memset(fd, 0, sizeof(*fd));
		//fd->fd = -1;
		switch_mutex_init(&fd->mutex, SWITCH_MUTEX_NESTED, globals.pool);
		fd->path = switch_core_strdup(globals.pool, path);
		switch_core_hash_insert(globals.fd_hash, path, fd);
	}

	switch_mutex_lock(fd->mutex);
	bytes_out = (unsigned) strlen(log_line);
	bytes_in = bytes_out;

	if (fd->fd == NULL) {
		do_reopen(fd);
		if (fd->fd == NULL) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error opening %s\n", path);
			goto end;
		}
	}

	if (fd->bytes + bytes_out > UINT_MAX) {
		if(do_rotate(fd) != SWITCH_STATUS_SUCCESS ){
			goto end;
		}

	}

	/*	while ((bytes_in = write(fd->fd, log_line, bytes_out)) != bytes_out && ++loops < 10) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Write error to file %s %d/%d\n", path, (int) bytes_in, (int) bytes_out);
		do_rotate(fd);
		switch_yield(250000);
	}

	if (bytes_in > 0) {
		fd->bytes += bytes_in;
	}
*/
	if(switch_file_write(fd->fd,log_line,&bytes_in) != SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Write error to file %s %d/%d\n", path, (int) bytes_in, (int) bytes_out);
	}
	

	fd->bytes += bytes_in;

  end:

	switch_mutex_unlock(fd->mutex);
}
#define get_var(a) \
	a = switch_channel_get_variable(channel,#a);
#define event_add_header(a) \
	switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,#a,a);

static switch_status_t fire_cdr_event(switch_channel_t *channel){
	
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *event = NULL;

	const char *desc = NULL;
	const char *caller_id_name = NULL;
	const char *caller_id_number = NULL;

	const char *destination_number = NULL;
	const char *dialed_extension = NULL;

	const char *context = NULL;
	const char *context_extension = NULL;

	const char *start_epoch = NULL;
	const char *answer_epoch = NULL;
	const char *end_epoch = NULL;

	const char *start_stamp = NULL;
	const char *answer_stamp = NULL;
	const char *end_stamp = NULL;
	const char *duration = NULL;
	const char *billsec = NULL;
	
	const char *originate_disposition = NULL;
	const char *hangup_cause = NULL;
	const char *uuid = NULL;
	const char *bleg_uuid = NULL;

	const char *read_codec = NULL;
	const char *write_codec = NULL;

	if(!channel){
		return status;
	}
	get_var(desc);
	get_var(caller_id_name);
	get_var(caller_id_number);
	//caller_id_name = switch_channel_get_variable(channel,"caller_id_name");
	//caller_id_number = switch_channel_get_variable(channel,"caller_id_number");
	get_var(destination_number);
	get_var(dialed_extension);
	get_var(context);
	get_var(context_extension);
	get_var(start_epoch);
	get_var(answer_epoch);
	get_var(end_epoch);
	get_var(start_stamp);
	get_var(answer_stamp);
	get_var(end_stamp);
	get_var(duration);
	get_var(billsec);
	get_var(hangup_cause);
	get_var(originate_disposition);
	get_var(uuid);
	get_var(bleg_uuid);
	get_var(read_codec);
	get_var(write_codec);

	
    if(!caller_id_number || !destination_number ||
	   !start_epoch  || !start_stamp || !uuid){
		return status;
	}

	if(switch_event_create_subclass(&event,SWITCH_EVENT_CUSTOM,CDR_EVENT) == SWITCH_STATUS_SUCCESS){
		event_add_header(caller_id_number);
		if(desc){
			caller_id_name = desc;
		}
		//if(!caller_id_name){
		//caller_id_name = caller_id_number;
		//}
		if(caller_id_name){
			event_add_header(caller_id_name);
		}

		event_add_header(destination_number);
		if(dialed_extension && strstr(dialed_extension,"$1") == NULL){
			event_add_header(dialed_extension);//dialed_extension = destination_number;
		}
		
		if(context){
			event_add_header(context);
		}
		if(context_extension){
			event_add_header(context_extension);
		}
		event_add_header(start_epoch);
		if(switch_strlen_zero(end_epoch)){
			end_epoch = start_epoch;
		}
		event_add_header(end_epoch);

		if(switch_strlen_zero(answer_epoch)){
			answer_epoch = end_epoch;
		}
		event_add_header(answer_epoch);

		
		event_add_header(start_stamp);

		if(switch_strlen_zero(end_stamp)){
			end_stamp = start_stamp;
		}
		event_add_header(end_stamp);

		if(switch_strlen_zero(answer_stamp)){
			answer_stamp = end_stamp;
		}
		event_add_header(answer_stamp);
		
		if(switch_strlen_zero(duration)){
			duration = "0";
		}
		event_add_header(duration);
		if(switch_strlen_zero(billsec)){
			billsec = "0";
		}
		event_add_header(billsec);
		//if(originate_disposition){
		//event_add_header(originate_disposition);
		//}

		if(originate_disposition){

			if(hangup_cause){
				switch_call_cause_t originate = switch_channel_str2cause(originate_disposition);
				switch_call_cause_t hangup = switch_channel_str2cause(hangup_cause);
				if((hangup == SWITCH_CAUSE_NORMAL_CLEARING || hangup == SWITCH_CAUSE_NONE)
				   && (originate != SWITCH_CAUSE_NORMAL_CLEARING)
				   && (originate != SWITCH_CAUSE_NONE)){
					hangup_cause = originate_disposition;
				}
			}
			else{
				hangup_cause = originate_disposition;
			}
		}
		if(!hangup_cause){
			hangup_cause = "NONE";
		}

		if(hangup_cause){
			event_add_header(hangup_cause);
		}

		event_add_header(uuid);
		if(bleg_uuid){
			event_add_header(bleg_uuid);
		}
		if(read_codec){
			event_add_header(read_codec);
		}
		if(write_codec){
			event_add_header(write_codec);
		}
    
		status = switch_event_fire(&event);
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"fire cdr event for %s\n",caller_id_number); 
	}

	return status;
}

static switch_status_t my_on_reporting(switch_core_session_t *session)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	const char *log_dir = NULL, *accountcode = NULL, *a_template_str = NULL, *g_template_str = NULL;
	char *log_line, *path = NULL,*file = NULL;
	const char *start_stamp = NULL;

	char *lbuf = NULL;
	char *argv[4] = { 0 };
	int argc = 0;

	char *start_day=NULL;
	char *dargv[8] = {0};
	int dargc = 0;
	char *Y,*M,*D;

	if (globals.shutdown) {
		return SWITCH_STATUS_SUCCESS;
	}

	if (!((globals.legs & CDR_LEG_A) && (globals.legs & CDR_LEG_B))) {
		if ((globals.legs & CDR_LEG_A)) {
			if (switch_channel_get_originator_caller_profile(channel)) {
				return SWITCH_STATUS_SUCCESS;
			}
		} else {
			if (switch_channel_get_originatee_caller_profile(channel)) {
				return SWITCH_STATUS_SUCCESS;
			}
		}
	}

	get_var(start_stamp);
	if(!start_stamp){
		return SWITCH_STATUS_SUCCESS;
	}
	fire_cdr_event(channel);

	if(!(lbuf = strdup(start_stamp))){
		return SWITCH_STATUS_SUCCESS;
	}
	argc = switch_separate_string(lbuf,' ',argv,(sizeof(argv)/sizeof(argv[0])));
	if(argc < 1){
		goto done;
	}
	if(!(start_day = strdup(argv[0]))){
		goto done;
	}
	dargc = switch_separate_string(start_day,'-',dargv,(sizeof(dargv)/sizeof(dargv[0])));
	if(dargc < 3){
		goto done;
	}
	Y = dargv[0];
	M = dargv[1];
	D = dargv[2];

	if (!(log_dir = switch_channel_get_variable(channel, "cdr_csv_base"))) {
		log_dir = globals.log_dir;
	}

	if (switch_dir_make_recursive(log_dir, SWITCH_DEFAULT_DIR_PERMS, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating %s\n", log_dir);
		//return SWITCH_STATUS_FALSE;
		goto done;
	}

	/*if (globals.debug) {
		switch_event_t *event;
		if (switch_event_create_plain(&event, SWITCH_EVENT_CHANNEL_DATA) == SWITCH_STATUS_SUCCESS) {
			char *buf;
			switch_channel_event_set_data(channel, event);
			switch_event_serialize(event, &buf, SWITCH_FALSE);
			switch_assert(buf);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "CHANNEL_DATA:\n%s\n", buf);
			switch_event_destroy(&event);
			free(buf);
		}
	}*/

	g_template_str = (const char *) switch_core_hash_find(globals.template_hash, globals.default_template);

	if (!(accountcode = switch_channel_get_variable(channel, "ACCOUNTCODE"))) {
		accountcode = switch_channel_get_variable(channel,"dialed_extension");
	}
	if(accountcode){
		a_template_str = (const char *) switch_core_hash_find(globals.template_hash, accountcode);
	}
	if (!g_template_str) {
		g_template_str = "\"${accountcode}\",\"${caller_id_number}\",\"${destination_number}\",\"${context}\",\"${caller_id}\",\"${channel_name}\",\"${bridge_channel}\",\"${last_app}\",\"${last_arg}\",\"${start_stamp}\",\"${answer_stamp}\",\"${end_stamp}\",\"${duration}\",\"${billsec}\",\"${hangup_cause}\",\"${amaflags}\",\"${uuid}\",\"${userfield}\";";
	}

	if (!a_template_str) {
		a_template_str = g_template_str;
	}

	
	if ((accountcode) && (!globals.masterfileonly)) {

		log_line = switch_channel_expand_variables(channel, a_template_str);
		if (!log_line) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating cdr\n");
			//return SWITCH_STATUS_FALSE;
			goto done;
		}

		path = switch_mprintf("%s%s%s%s%s-%s",log_dir,SWITCH_PATH_SEPARATOR,accountcode,SWITCH_PATH_SEPARATOR,Y,M);
		assert(path);
		if (switch_dir_make_recursive(path, SWITCH_DEFAULT_DIR_PERMS| SWITCH_FPROT_WREAD | SWITCH_FPROT_WEXECUTE, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating %s\n", path);
			//return SWITCH_STATUS_FALSE;
			free(path);
			goto done;
		}
		
		//free(path);
		
		file = switch_mprintf("%s%s%s.csv", path, SWITCH_PATH_SEPARATOR, D);
		assert(file);
		write_cdr(file, log_line);
		free(file);
		free(path);

		if (log_line != a_template_str) {
			free(log_line);
		}
		
	}
	else{
		log_line = switch_channel_expand_variables(channel, g_template_str);

		if (!log_line) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating cdr\n");
			//return SWITCH_STATUS_FALSE;
			goto done;
		}

		path = switch_mprintf("%s%sMaster.csv", log_dir, SWITCH_PATH_SEPARATOR);
		assert(path);
		write_cdr(path, log_line);
		free(path);
		
		if (log_line != g_template_str) {
			free(log_line);
		}
	}

 done:
	switch_safe_free(start_day);
	switch_safe_free(lbuf);
	return status;

}


static void event_handler(switch_event_t *event)
{
	const char *sig = switch_event_get_header(event, "Trapped-Signal");
	switch_hash_index_t *hi;
	void *val;
	cdr_fd_t *fd;

	if (globals.shutdown) {
		return;
	}

	if (sig && !strcmp(sig, "HUP")) {
		for (hi = switch_hash_first(NULL, globals.fd_hash); hi; hi = switch_hash_next(hi)) {
			switch_hash_this(hi, NULL, NULL, &val);
			fd = (cdr_fd_t *) val;
			switch_mutex_lock(fd->mutex);
			do_rotate(fd);
			switch_mutex_unlock(fd->mutex);
		}
	}
}


static switch_state_handler_table_t state_handlers = {
	/*.on_init */ NULL,
	/*.on_routing */ NULL,
	/*.on_execute */ NULL,
	/*.on_hangup */ NULL,
	/*.on_exchange_media */ NULL,
	/*.on_soft_execute */ NULL,
	/*.on_consume_media */ NULL,
	/*.on_hibernate */ NULL,
	/*.on_reset */ NULL,
	/*.on_park */ NULL,
	/*.on_reporting */ my_on_reporting
};



static switch_status_t load_config(switch_memory_pool_t *pool)
{
	char *cf = "cdr_csv.conf";
	switch_xml_t cfg, xml, settings, param;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	memset(&globals, 0, sizeof(globals));
	switch_core_hash_init(&globals.fd_hash, pool);
	switch_core_hash_init(&globals.template_hash, pool);

	globals.pool = pool;

	switch_core_hash_insert(globals.template_hash, "default", default_template);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Adding default template.\n");
	globals.legs = CDR_LEG_A;

	if ((xml = switch_xml_open_cfg(cf, &cfg, NULL))) {

		if ((settings = switch_xml_child(cfg, "settings"))) {
			for (param = switch_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");
				if (!strcasecmp(var, "debug")) {
					globals.debug = switch_true(val);
				} else if (!strcasecmp(var, "legs")) {
					globals.legs = 0;

					if (strchr(val, 'a')) {
						globals.legs |= CDR_LEG_A;
					}

					if (strchr(val, 'b')) {
						globals.legs |= CDR_LEG_B;
					}
				} else if (!strcasecmp(var, "log-base")) {
					globals.log_dir = switch_core_sprintf(pool, "%s%scdr-csv", val, SWITCH_PATH_SEPARATOR);
				} else if (!strcasecmp(var, "rotate-on-hup")) {
					globals.rotate = switch_true(val);
				} else if (!strcasecmp(var, "default-template")) {
					globals.default_template = switch_core_strdup(pool, val);
				} else if (!strcasecmp(var, "master-file-only")) {
					globals.masterfileonly = switch_true(val);
				}
			}
		}

		if ((settings = switch_xml_child(cfg, "templates"))) {
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
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Adding template %s.\n", var);
				}
			}
		}
		switch_xml_free(xml);
	}


	if (zstr(globals.default_template)) {
		globals.default_template = switch_core_strdup(pool, "default");
	}

	if (!globals.log_dir) {
		globals.log_dir = switch_core_sprintf(pool, "%s%scdr-csv", SWITCH_GLOBAL_dirs.log_dir, SWITCH_PATH_SEPARATOR);
	}

	return status;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_cdr_csv_load)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	load_config(pool);

	if ((status = switch_dir_make_recursive(globals.log_dir, SWITCH_DEFAULT_DIR_PERMS, pool)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error creating %s\n", globals.log_dir);
		return status;
	}

	if ((status = switch_event_bind(modname, SWITCH_EVENT_TRAP, SWITCH_EVENT_SUBCLASS_ANY, event_handler, NULL)) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return status;
	}

	if(switch_event_reserve_subclass(CDR_EVENT) != SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Cound't register subclass %s!",CDR_EVENT);
		return status;
	}

	switch_core_add_state_handler(&state_handlers);
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);




	return status;
}


SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cdr_csv_shutdown)
{

	globals.shutdown = 1;

	switch_event_free_subclass(CDR_EVENT);

	switch_event_unbind_callback(event_handler);
	switch_core_remove_state_handler(&state_handlers);


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
