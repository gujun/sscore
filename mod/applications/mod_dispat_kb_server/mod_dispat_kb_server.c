/*
 * CARI mod in FreeSWITCH
 * Copyright(C)2005-2009,Ritchie Gu <ritchie.gu@gmail.com>
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
 * mod_dispat_kb_server.c -- keyboard server Application Module
 */
#include <switch.h>
#define CMD_BUFLEN 1024*1000

SWITCH_MODULE_LOAD_FUNCTION(mod_dispat_kb_server_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_dispat_kb_server_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_dispat_kb_server_runtime);
SWITCH_MODULE_DEFINITION(mod_dispat_kb_server,mod_dispat_kb_server_load,mod_dispat_kb_server_shutdown,mod_dispat_kb_server_runtime);



typedef enum {
	LFLAG_AUTHED = (1 << 0),
	LFLAG_RUNNING = (1 << 1),
	LFLAG_EVENTS = (1 << 2),
	LFLAG_LOG = (1 << 3),
	LFLAG_FULL = (1 << 4),
	LFLAG_MYEVENTS = (1 << 5),
	LFLAG_SESSION = (1 << 6),
	LFLAG_ASYNC = (1 << 7),
	LFLAG_STATEFUL = (1 << 8),
	LFLAG_OUTBOUND = (1 << 9),
	LFLAG_LINGER = (1 << 10),
	LFLAG_HANDLE_DISCO = (1 << 11),
	LFLAG_CONNECTED = (1 << 12),
	LFLAG_RESUME = (1 << 13),
} event_flag_t;

struct listener{
	switch_socket_t *sock; 
	switch_queue_t *event_queue;
	switch_queue_t *log_queue;
	switch_memory_pool_t *pool;
	switch_mutex_t *flag_mutex;
	uint32_t flags;
	char *ebuf;
	uint8_t event_list[SWITCH_EVENT_ALL+1];
	switch_hash_t *event_hash;
	switch_thread_rwlock_t *rwlock;
	//switch_core_session *session;
	int lost_events;
	int lost_logs;
	time_t last_flush;
	uint32_t timeout;
	uint32_t id;
	switch_sockaddr_t *sa;
	char remote_ip[50];
	switch_port_t remote_port;

	char * dispatcher;
	struct listener * next;
};
typedef struct listener listener_t;
static int flag_1 = 1;
static struct{
	switch_mutex_t *listener_mutex;
	//switch_event_node_t *node;
	switch_event_node_t *reg_event_node;
	switch_event_node_t *unreg_event_node;
	switch_event_node_t *reg_expire_event_node;
	switch_event_node_t *dispat_state_event_node;
	switch_event_node_t *dispat_kb_event_node;
	switch_event_node_t *conference_event_node;
	switch_event_node_t *xml_event_node;
	switch_event_node_t *channel_progress_media_event_node;
	switch_event_node_t *channel_progress_event_node;
	switch_event_node_t *channel_answer_event_node;
	switch_event_node_t *channel_hangup_event_node;
	switch_event_node_t *call_dispatcher_fail_event_node;

	switch_hash_t *call_dispat_hash;
	switch_mutex_t *call_dispat_mutex;
	//switch_memory_pool_t *call_dispat_pool;

	int debug;

}globals;
static struct{
	switch_socket_t *sock;
	switch_mutex_t *sock_mutex;
	listener_t *listeners;
	uint8_t ready;
}listen_list;

#define MAX_ACL 100

static struct{
	//switch_mutext_t  *mutex;
	char *ip;
	uint16_t port;
	char *password;
	int done;
	int threads;
	char *acl[MAX_ACL];
	uint32_t acl_count;
	uint32_t id;
}prefs;


#define EVENT_REGISTER "sofia::register"
#define EVENT_UNREGISTER "sofia::unregister"
#define EVENT_REGISTER_EXPIRE "sofia::expire"
#define EVENT_DISPAT  "dispat::info"
#define EVENT_CONFERENCE "conference::maintenance"
#define EVENT_CALL_DISPATCHER_FAIL "event::custom::cdr"
#define EVENT_DISPAT_KB "dispat_kb::info"

typedef  switch_status_t (*command_api_function_t)(listener_t *listener, switch_event_t **event);
typedef struct command_api{
	char *command;
	command_api_function_t command_api_function;
}command_api_t;

typedef struct event_command_api{
	switch_event_types_t event;
	char *command;
	command_api_function_t command_api_function;
}event_command_api_t;

SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_pref_ip, prefs.ip);
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_pref_pass, prefs.password);


static void config(void)
{
	char *cf = "dispat_kb_server.conf";
	switch_xml_t cfg, xml, settings, param;

	memset(&prefs, 0, sizeof(prefs));

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", cf);
	} else {
		if ((settings = switch_xml_child(cfg, "settings"))) {
			for (param = switch_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

				if (!strcmp(var, "listen-ip")) {
					set_pref_ip(val);
				} else if (!strcmp(var, "debug")) {
					globals.debug = atoi(val);
				} else if (!strcmp(var, "listen-port")) {
					prefs.port = (uint16_t) atoi(val);
				} else if (!strcmp(var, "password")) {
					set_pref_pass(val);
				} else if (!strcasecmp(var, "apply-inbound-acl")) {
					if (prefs.acl_count < MAX_ACL) {
						prefs.acl[prefs.acl_count++] = strdup(val);
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Max acl records of %d reached\n", MAX_ACL);
					}
				}
			}
		}
		switch_xml_free(xml);
	}

	if (switch_strlen_zero(prefs.ip)) {
		set_pref_ip("127.0.0.1");
	}

	if (switch_strlen_zero(prefs.password)) {
		set_pref_pass("cari");
	}

	if (!prefs.port) {
		prefs.port = 6608;
	}

	//return 0;
}
#define DISPAT_CHAT_INTERFACE "sip"
#define DISPAT_CHAT_PROTO "dispat"

static int my_event_handler(switch_event_t *event)
{
	switch_event_t *clone = NULL;
	listener_t *l, *lp;

	int count = 0;

	switch_assert(event != NULL);

	if (!listen_list.ready) {
		return 0;
	}

	lp = listen_list.listeners;

	switch_mutex_lock(globals.listener_mutex);

	while(lp) {
		int send = 0;
		
		l = lp;
		lp = lp->next;
		
		//check send or not;
		if(switch_test_flag(l,LFLAG_EVENTS)){
			send = 1;
		}

		
		if(send){
			if (switch_event_dup(&clone, event) == SWITCH_STATUS_SUCCESS) {
				if (switch_queue_trypush(l->event_queue, clone) == SWITCH_STATUS_SUCCESS) {
					count++;
					if (l->lost_events) {
						int le = l->lost_events;
						l->lost_events = 0;
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Lost %d events!\n", le);
						clone = NULL;
						if (switch_event_create(&clone, SWITCH_EVENT_TRAP) == SWITCH_STATUS_SUCCESS) {
							switch_event_add_header(clone, SWITCH_STACK_BOTTOM, "info", "lost %d events", le);
							switch_event_fire(&clone);
						}
					}
				} else {
					l->lost_events++;
					switch_event_destroy(&clone);
				}
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Memory Error!\n");
			}
		}
	}
	switch_mutex_unlock(globals.listener_mutex);

	return count;
}

static void event_handler(switch_event_t *event){
	my_event_handler(event);
}
static void cdr_event_handler(switch_event_t *event){
	const char *hangup_cause = switch_event_get_header(event,"hangup_cause");
    const char *context_extension = switch_event_get_header(event,"context_extension");

	const char *caller_id_number = switch_event_get_header(event,"caller_id_number");
	const char *uuid = switch_event_get_header(event,"uuid");
	const char *start_stamp = switch_event_get_header(event,"start_stamp");


    if(hangup_cause && context_extension && caller_id_number && start_stamp && uuid &&
       !strstr(hangup_cause,"SUCCESS") &&
       !strcmp(context_extension,"dispatcher") ){
		
		my_event_handler(event);

	}
}

static void channel_event_handler(switch_event_t *event){
	const char *bound = switch_event_get_header(event,"Call-Direction");
    const char *context_extension = switch_event_get_header(event,"variable_context_extension");


	const char *uuid = switch_event_get_header(event,"Unique-ID");

	switch_event_types_t event_type = event->event_id;

	int *flag = NULL;

	//if(uuid != NULL) {

	  // switch_mutex_lock(globals.call_dispat_mutex);
	  // flag= switch_core_hash_find(globals.call_dispat_hash,uuid);
	  // switch_mutex_unlock(globals.call_dispat_mutex);

	//}

	if(event_type == SWITCH_EVENT_CHANNEL_PROGRESS ||
	   event_type == SWITCH_EVENT_CHANNEL_PROGRESS_MEDIA){
		if(bound && context_extension &&
		  !strcmp(bound,"inbound") &&
		  !strcmp(context_extension,"dispatcher") ){
			//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel event:%s,%s\n",bound,context_extension);	
			my_event_handler(event);
			if(uuid != NULL){
				switch_mutex_lock(globals.call_dispat_mutex);
				switch_core_hash_insert(globals.call_dispat_hash,uuid,&flag_1);
				switch_mutex_unlock(globals.call_dispat_mutex);
			}

		}
		
	}
	else {
		if(uuid != NULL) {

		    switch_mutex_lock(globals.call_dispat_mutex);
		    flag= switch_core_hash_find(globals.call_dispat_hash,uuid);
		    switch_mutex_unlock(globals.call_dispat_mutex);

		}

		if((flag != NULL) || (bound && context_extension &&
		 !strcmp(bound,"inbound") &&
		 !strcmp(context_extension,"dispatcher")) ){
		    my_event_handler(event);
		    if(event_type == SWITCH_EVENT_CHANNEL_ANSWER && flag == NULL){
		      switch_mutex_lock(globals.call_dispat_mutex);
	          switch_core_hash_insert(globals.call_dispat_hash,uuid,&flag_1);
	          switch_mutex_unlock(globals.call_dispat_mutex);
	        }else if(event_type == SWITCH_EVENT_CHANNEL_HANGUP && flag != NULL){
				switch_mutex_lock(globals.call_dispat_mutex);
				switch_core_hash_delete(globals.call_dispat_hash,uuid);
				switch_mutex_unlock(globals.call_dispat_mutex);
			}
        }

	}

}

static switch_status_t chat_send(const char *proto, const char *from, const char *to, const char *subject,
								 const char *body, const char *type, const char *hint)
{
	switch_event_t *event=NULL;
	int count = 0;
	switch_time_t start_time;
	switch_time_exp_t start_time_exp;
	char start_epoch_str[80] = "";
	char start_time_str[80] = "";
	switch_size_t retsize;

	if(switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, EVENT_DISPAT_KB)
		== SWITCH_STATUS_SUCCESS) {
			
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "action", "message");
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "chat",DISPAT_CHAT_PROTO);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", proto);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "from", from);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "to", to);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "subject", subject);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "body", body);
			
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "type", type);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "hint", hint);
			//switch_event_add_body(event,"%s",body);
			//switch_event_fire(&event);
			count = my_event_handler(event);
			if(count > 0){
				//switch_event_destroy(&event);
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM,"is_ok","1");
				//
			} else {
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM,"is_ok","0");
				
			}
			start_time = switch_micro_time_now();
			switch_time_exp_lt(&start_time_exp,start_time);
			switch_strftime_nocheck(start_time_str,&retsize,sizeof(start_time_str),"%Y-%m-%d %H:%M:%S",&start_time_exp);
			start_time = (switch_time_t)(start_time/1000000);
			switch_snprintf(start_epoch_str,sizeof(start_epoch_str),"%ld",start_time);
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,"start_epoch",start_epoch_str);
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,"start_stamp",start_time_str);
			switch_event_fire(&event);	
		}
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t reg_event_handler(listener_t *listener,switch_event_t **event)
{
	/*		if (switch_event_create_subclass(&s_event, SWITCH_EVENT_CUSTOM, MY_EVENT_REGISTER) == SWITCH_STATUS_SUCCESS) {
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
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[512] = "";
	switch_size_t reply_len = sizeof(reply);
	
	char *user = switch_event_get_header(*event, "username");//sip_auth_user;
	char *agent = switch_event_get_header(*event, "user-agent");
	char *ip = switch_event_get_header(*event, "network-ip");
	char *port = switch_event_get_header(*event, "network-port");
	char *contact = switch_event_get_header(*event,"contact");

	switch_snprintf(reply, sizeof(reply), "content-type:trap/registratioin\n"
								   "user-id:%s\n"
								   "status:reg\n"
								   "agent:%s\n"
								   "ip:%s\n"
					"port:%s\ncontact:%s\n\n",user,agent,ip,port,contact);
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);
	
	
	return  status;
	
}
static switch_status_t unreg_event_handler(listener_t *listener, switch_event_t **event)
{
	/*if (switch_event_create_subclass(&s_event, SWITCH_EVENT_CUSTOM, MY_EVENT_UNREGISTER) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "profile-name", profile->name);
				switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "from-user", to_user);
				switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "from-host", reg_host);
				switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "contact", contact_str);
				switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "call-id", call_id);
				switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "rpid", rpid);
				switch_event_add_header(s_event, SWITCH_STACK_BOTTOM, "expires", "%ld", (long) exptime);
			}*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[256] = "";
	switch_size_t reply_len = sizeof(reply);
	
	char *user = switch_event_get_header(*event, "from-user");//sip_user;

	switch_snprintf(reply, sizeof(reply), "content-type:trap/registratioin\n"
								   "user-id:%s\n"
								   "status:unreg\n\n",user);
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);
	
	return  status;
}
static switch_status_t reg_expire_event_handler(listener_t *listener, switch_event_t **event)
{
	/*if (switch_event_create_subclass(&s_event, SWITCH_EVENT_CUSTOM, MY_EVENT_EXPIRE) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "profile-name", argv[10]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "call-id", argv[0]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "user", argv[1]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "host", argv[2]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "contact", argv[3]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "expires", argv[6]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "user-agent", argv[7]);
			switch_event_fire(&s_event);
		}*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[256] = "";
	switch_size_t reply_len = sizeof(reply);
	
	char *user = switch_event_get_header(*event, "user");//sip_user;

	switch_snprintf(reply, sizeof(reply), "content-type:trap/registratioin\n"
								   "user-id:%s\n"
								   "status:expire\n\n",user);
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);
	
	
	return  status;
}
static char * dup_user_id_from_channel_name(const char *channel_name){
	char * user_id = NULL;
	char *name_dup=NULL;
	char *p_start =NULL,*p_end=NULL;
	if(!switch_strlen_zero(channel_name) && (name_dup = strdup(channel_name)) != NULL){
		/*switch_snprintf(name, sizeof(name), "sofia/%s/%s", profile->name, channame);*/
		if((p_start = (char *)switch_stristr("sip:",name_dup))){
			p_start += 4;
		}else if((p_start = (char *)switch_stristr("sips:",name_dup))){
			p_start += 5;
		}else if((p_start = (char *)switch_stristr("/",name_dup))){
			p_start++;
			p_start = (char *)switch_stristr("/",p_start);
			p_start++;
		}

		if(p_start == NULL){
			p_end = (char *)switch_stristr("@",p_start);
			if(p_end){
				p_start = p_end;
				while(p_start >= name_dup && *p_start  != '/'){
					p_start--;
				}
				p_start++;
			}
			else{
				p_start=name_dup;
			}
		}
		
		if(p_start){
			if(p_end == NULL){
				p_end = (char *)switch_stristr("@",p_start);
			}
			if(p_end != NULL){
				*p_end = '\0';
			}
			user_id = strdup(p_start);
		}
		switch_safe_free(name_dup);
		
	}
	return user_id;
}
static switch_status_t  dispat_event_handler(listener_t *listener, switch_event_t **event)
{
	/*if(switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, DISPAT_EVENT)
		== SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "action", "change_dispatcher_state");
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "old-state", dispat_state_discription[oldState]);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "new-state", dispat_state_discription[newState]);
			switch_event_fire(&event);
		}*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[256] = "";
	switch_size_t reply_len = sizeof(reply);
	char * name = switch_event_get_header(*event, "dispatcher_name");
	char * action = switch_event_get_header(*event, "action");
	if(!strcasecmp("change_dispatcher_state",action)){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"change_dispatcher_state\n");
		
		char * state = switch_event_get_header(*event, "new-state");
		char * id = dup_user_id_from_channel_name(name);
		if(!switch_strlen_zero(id)){
			switch_snprintf(reply, sizeof(reply), "content-type:trap/dispatcher\n"
								   "id:%s\n"
								   "status:%s\n\n",id,state);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s\n",reply);
			switch_safe_free(id);
		}
		
		
	}
	else{
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"not change_dispatcher_state\n");
		
	}
	

	return  status;
}

/*#define NODE_NAME_EVENT_HEADER "Caller-Caller-ID-Number"*/
#define NODE_NAME_EVENT_HEADER "Channel-Name"

static switch_status_t  conference_event_handler(listener_t *listener, switch_event_t **event)
{
/*

switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Conference-Name", conference->name);
switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Conference-Size", "%u", conference->count);
switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Conference-Profile-Name", conference->profile_name);


switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-State", switch_channel_state_name(channel->state));
	switch_snprintf(state_num, sizeof(state_num), "%d", channel->state);
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-State-Number", state_num);
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel-Name", switch_channel_get_name(channel));
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Unique-ID", switch_core_session_get_uuid(channel->session));
	
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Call-Direction",
								   channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Presence-Call-Direction",
								   channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");


	if (switch_channel_test_flag(channel, CF_ANSWERED)) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "answered");
	} else if (switch_channel_test_flag(channel, CF_EARLY_MEDIA)) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "early");
	} else {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Answer-State", "ringing");
	}


switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Member-ID", "%u", member->id);
switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Member-Type", "%s", switch_test_flag(member, MFLAG_MOD) ? "moderator" : "member");
switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Member-Flag", "%s", flag);
	
*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[256] = "";
	switch_size_t reply_len = sizeof(reply);
	int called =0;
	
	char *member_id =  switch_event_get_header(*event,"Member-ID");
	char *user_name = switch_event_get_header(*event,NODE_NAME_EVENT_HEADER);
	
	char * id = dup_user_id_from_channel_name(user_name);
	char *action = switch_event_get_header(*event,"Action");
	char *bound =  switch_event_get_header(*event,"Call-Direction");
	char * call_name = switch_event_get_header(*event,"Conference-Name");
	char * flag = switch_event_get_header(*event,"Member-Flag");
	char *gateway_name = switch_event_get_header(*event,"Gateway-Name");

	if(switch_strlen_zero(user_name) || switch_strlen_zero(member_id)){
		return status;
	}

	if(!switch_strlen_zero(action)){
		if(!strcasecmp(action,"add-member")){

			switch_snprintf(reply, sizeof(reply), "content-type:trap/call\n"
								   "id:%s\n"
								   "action:add\n"
								   "call-name:%s\n"
								   "member:%s\n"
								   "type:%s\n"
							       "gateway-name:%s\n"
								   "bound:%s\n"
								   "flag:%s\n"
								   "\n",
								   id,
								   call_name,
								   member_id,
							(!switch_strlen_zero(gateway_name))?"gateway":"user",
							//(((strstr(user_name,"gateway"))==NULL?"user":"gateway")),
							(!switch_strlen_zero(gateway_name))?gateway_name:"null",
							((strstr(bound,"out")==NULL)?"in":"out"),
									flag);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			
			called++;
		}
		else if(!strcasecmp(action,"del-member")){
			switch_snprintf(reply, sizeof(reply), "content-type:trap/call\n"
								   "id:%s\n"
								   "action:del\n"
								   "call-name:%s\n"
								   "member:%s\n"
							       "type:%s\n"
							       "gateway-name:%s\n"
							       "bound:%s\n"
							       "flag:%s\n"
								   "\n",
								   id,
								   call_name,
							member_id,
							(!switch_strlen_zero(gateway_name))?"gateway":"user",
							(!switch_strlen_zero(gateway_name))?gateway_name:"null",
							(strstr(bound,"out")==NULL)?"in":"out",
							(flag!=NULL)?flag:"null");
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			called++;
		}
	}
	if(called == 0){
		switch_snprintf(reply, sizeof(reply), "content-type:trap/call\n"
								   "id:%s\n"
								   "action:flag\n"
								   "call-name:%s\n"
								   "member:%s\n"
								   "type:%s\n"
						           "gateway-name:%s\n"
								   "bound:%s\n"
								   "flag:%s\n"
								   "\n",
								   id,
								   call_name,
								   member_id,
						(!switch_strlen_zero(gateway_name))?"gateway":"user",
						//(((strstr(user_name,"gateway"))==NULL?"user":"gateway")),
						(!switch_strlen_zero(gateway_name))?gateway_name:"null",
						((strstr(bound,"out")==NULL)?"in":"out"),
						flag);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
	}
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s\n",reply);
		
	switch_safe_free(id);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t trap_all_users(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	
	switch_xml_t xml_root, x_domains;
	char *buf;
	switch_size_t buf_len;
	if (switch_xml_locate("directory", NULL, NULL, NULL, &xml_root, &x_domains, NULL, SWITCH_FALSE) == SWITCH_STATUS_SUCCESS) {
		buf = switch_xml_toxml(x_domains, SWITCH_FALSE);
		switch_assert(buf);
		buf_len = strlen(buf);
		switch_snprintf(reply, sizeof(reply), "content-type:trap/all-users\ncontent-length:%d\n\n",buf_len);
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
		switch_socket_send(listener->sock,buf, &buf_len);
		free(buf);
		switch_xml_free(xml_root);
		
	}
	/*else{
		//do nothing;
	}*/
	
	return status;
}
static switch_status_t trap_all_gateways(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	
	switch_xml_t xml_root=NULL, cfg=NULL;
	char *buf;
	switch_size_t buf_len;
	int err=0;
	if((xml_root = switch_xml_open_cfg("sofia.conf",&cfg,NULL)) != NULL){
		buf = switch_xml_toxml(cfg,SWITCH_FALSE);
		if(buf == NULL){
			err++;
		}else{
			buf_len = strlen(buf);
			switch_snprintf(reply, sizeof(reply), "content-type:trap/all-gateways\ncontent-length:%d\n\n",buf_len);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			switch_socket_send(listener->sock,buf, &buf_len);
			free(buf);
			err = 0;
		}
		switch_xml_free(xml_root);
	}
	else{
		err++;
	}

	if(err){
		//	switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-gateways\nreply-text:err\ndiscription:no gateways information.\n\n");
		//reply_len = strlen(reply);
		//switch_socket_send(listener->sock,reply, &reply_len);
		status = SWITCH_STATUS_FALSE;
	}
	
	return status;

}
static switch_status_t reloadxml_event_handler(listener_t *listener, switch_event_t **event)
{
	/*if (switch_event_create_subclass(&s_event, SWITCH_EVENT_CUSTOM, MY_EVENT_EXPIRE) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "profile-name", argv[10]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "call-id", argv[0]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "user", argv[1]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "host", argv[2]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "contact", argv[3]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "expires", argv[6]);
			switch_event_add_header_string(s_event, SWITCH_STACK_BOTTOM, "user-agent", argv[7]);
			switch_event_fire(&s_event);
		}*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = trap_all_users(listener,NULL);
	status = trap_all_gateways(listener,NULL);
	
	return  status;
}
static switch_status_t  dispat_kb_event_handler(listener_t *listener, switch_event_t **event)
{
	/*if(switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, EVENT_DISPAT_KB)
		== SWITCH_STATUS_SUCCESS) {
			
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "action", "message");
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "proto", proto);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "from", from);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "to", to);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "subject", subject);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "body", body);
			
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "type", type);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "hint", hint);
			//switch_event_add_body(event,"%s",body);
			switch_event_fire(&event);
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"send sms:%s-->%s:%s\n",from,to,body);
		}*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[128] = "";
	switch_size_t reply_len = sizeof(reply);
	
	char * action = switch_event_get_header(*event, "action");
	if(!strcasecmp("message",action)){

		char *from = switch_event_get_header(*event, "from");
	
		char *to = switch_event_get_header(*event, "to");
		char *subject = switch_event_get_header(*event, "subject");
		char *type = switch_event_get_header(*event,"type");
		//char *body =  switch_event_get_body(*event);
		char *body = switch_event_get_header(*event,"body");
		
		
		char *buf = switch_mprintf("<sms>"
								   "<from>%s</from>"
								   "<to>%s</to>"
								   "<subject>%s</subject>"
								   "<type>%s</type>"
								   "<body>%s</body>"
								   "</sms>",
								   from,
								   to,
								   subject,
								   type,
								   body
								   );
		switch_size_t buf_len = strlen(buf);
		switch_snprintf(reply, sizeof(reply), "content-type:trap/message\ncontent-length:%d\n\n",buf_len);
		
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
		switch_socket_send(listener->sock,buf,&buf_len);
		switch_safe_free(buf);
		
	}
	else{
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"not message\n");
		
	}
	

	return  status;
}
static switch_status_t  call_dispatcher_fail_event_handler(listener_t *listener, switch_event_t **event)
{
	/*
	  if (switch_event_create_subclass(&sevent, SWITCH_EVENT_CUSTOM,DISPATCHER_FAIL_CDR_EVENT) == SWITCH_STATUS_SUCCESS) {

            switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"caller_id_number",caller_id_number);

            if(!switch_strlen_zero(caller_id_name)){
                switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"caller_id_name",caller_id_name);
            }
            switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"start_stamp",start_stamp);
            switch_event_add_header_string(sevent,SWITCH_STACK_BOTTOM,"hangup_cause",hangup_cause);

            switch_event_fire(&sevent);
        }

		}*/
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[512] = "";
	switch_size_t reply_len = sizeof(reply);
	char * caller_id_number = switch_event_get_header(*event, "caller_id_number");
	char * caller_id_name = switch_event_get_header(*event, "caller_id_name");

	char * start_stamp = switch_event_get_header(*event, "start_stamp");
	char * hangup_cause = switch_event_get_header(*event,"hangup_cause");
	char * uuid = switch_event_get_header(*event,"uuid");

	if(caller_id_number && start_stamp && hangup_cause && uuid){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"change_dispatcher_state\n");
		
   		if(!switch_strlen_zero(caller_id_number)){
			switch_snprintf(reply, sizeof(reply), "content-type:trap/call-dispatcher-fail\n"
								   "caller_id_number:%s\n"
							"caller_id_name:%s\n"
							"start_stamp:%s\n"
							"hangup_cause:%s\n"
							"uuid:%s\n\n",caller_id_number,caller_id_name,start_stamp,hangup_cause,uuid);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			
		}
		
		
	}
	
	

	return  status;
}
static switch_status_t  channel_progress_event_handler(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[512] = "";
	switch_size_t reply_len = sizeof(reply);
	char * caller_id_number = switch_event_get_header(*event, "Caller-Caller-ID-Number");
	char * caller_id_name = switch_event_get_header(*event, "Caller-Caller-ID-Name");

	char * uuid = switch_event_get_header(*event,"Unique-ID");
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel progress event:%s,%s\n",caller_id_number,uuid);
	if(caller_id_number &&  uuid){
		
   		if(!switch_strlen_zero(caller_id_number)){
			switch_snprintf(reply, sizeof(reply), "content-type:trap/call-dispatcher-progress\n"
								   "caller_id_number:%s\n"
							"caller_id_name:%s\n"
							"uuid:%s\n\n",caller_id_number,caller_id_name,uuid);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			
		}
		
	}
	return  status;
}
static switch_status_t  channel_answer_event_handler(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[512] = "";
	switch_size_t reply_len = sizeof(reply);
	char * caller_id_number = switch_event_get_header(*event, "Caller-Caller-ID-Number");
	char * caller_id_name = switch_event_get_header(*event, "Caller-Caller-ID-Name");

	char * uuid = switch_event_get_header(*event,"Unique-ID");
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel answer event:%s,%s\n",caller_id_number,uuid);
	if(caller_id_number &&  uuid){
		
   		if(!switch_strlen_zero(caller_id_number)){
			switch_snprintf(reply, sizeof(reply), "content-type:trap/call-dispatcher-answer\n"
								   "caller_id_number:%s\n"
							"caller_id_name:%s\n"
							"uuid:%s\n\n",caller_id_number,caller_id_name,uuid);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			
		}
		
	}
	return  status;
}

static switch_status_t  channel_hangup_event_handler(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[512] = "";
	switch_size_t reply_len = sizeof(reply);
	char * caller_id_number = switch_event_get_header(*event, "Caller-Caller-ID-Number");
	char * caller_id_name = switch_event_get_header(*event, "Caller-Caller-ID-Name");

	char * uuid = switch_event_get_header(*event,"Unique-ID");
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel hangup event:%s,%s\n",caller_id_number,uuid);
	if(caller_id_number &&  uuid){
		
   		if(!switch_strlen_zero(caller_id_number)){
			switch_snprintf(reply, sizeof(reply), "content-type:trap/call-dispatcher-hangup\n"
								   "caller_id_number:%s\n"
							"caller_id_name:%s\n"
							"uuid:%s\n\n",caller_id_number,caller_id_name,uuid);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			
		}
		
	}
	return  status;
}


static event_command_api_t event_command_api_s[]={
	{SWITCH_EVENT_CUSTOM, EVENT_CONFERENCE,conference_event_handler},
	{SWITCH_EVENT_CUSTOM, EVENT_DISPAT,dispat_event_handler},
	{SWITCH_EVENT_CUSTOM, EVENT_REGISTER,reg_event_handler},
	{SWITCH_EVENT_CUSTOM, EVENT_UNREGISTER,unreg_event_handler},
	{SWITCH_EVENT_CUSTOM, EVENT_REGISTER_EXPIRE,reg_expire_event_handler},
	{SWITCH_EVENT_CUSTOM,EVENT_DISPAT_KB,dispat_kb_event_handler},
	{SWITCH_EVENT_CUSTOM,EVENT_CALL_DISPATCHER_FAIL,call_dispatcher_fail_event_handler},
	{SWITCH_EVENT_RELOADXML,NULL,reloadxml_event_handler},
	{SWITCH_EVENT_CHANNEL_PROGRESS,NULL,channel_progress_event_handler},
	{SWITCH_EVENT_CHANNEL_PROGRESS_MEDIA,NULL,channel_progress_event_handler},
	{SWITCH_EVENT_CHANNEL_ANSWER,NULL,channel_answer_event_handler},
	{SWITCH_EVENT_CHANNEL_HANGUP,NULL,channel_hangup_event_handler},
	{0,NULL,NULL}
};

static switch_status_t filterEvent2Trap(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_event_types_t event_type ;
	char *cmd;
	int called;
	
	if(event == NULL || *event == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"parse_command:event null\n");
		return SWITCH_STATUS_FALSE;
	}
	event_type = (*event)->event_id;
	cmd = (*event)->subclass_name;
	called = 0;

	if(event_command_api_s != NULL){
	
		for(int i = 0;event_command_api_s[i].command_api_function != NULL;i++){
			if(event_type != event_command_api_s[i].event){
				continue;
			}
			if(event_command_api_s[i].event != SWITCH_EVENT_CUSTOM){
				called++;
			}
			else if(!strcasecmp(event_command_api_s[i].command,cmd)){
				called++;
			}
			if(called > 0){
				command_api_function_t api_function = event_command_api_s[i].command_api_function;
				if(api_function != NULL){
					status = api_function(listener,event);
					
				}
				else{
					called = 0;
				}
				break;
			}
		}
	}
	if(called == 0){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"event handler is  null\n");
	}
	
	//caller will destroy the event;
	return  status;
}
/*static void *SWITCH_THREAD_FUNC api_exec(switch_thread_t *thread, void *obj)
{

	struct api_command_struct *acs = (struct api_command_struct *) obj;
	switch_stream_handle_t stream = { 0 };
	char *reply, *freply = NULL;
	switch_status_t status;

	switch_mutex_lock(globals.listener_mutex);
	prefs.threads++;
	switch_mutex_unlock(globals.listener_mutex);

	
	if (!acs) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Internal error.\n");
		goto cleanup;
	}

	if (!acs->listener || !switch_test_flag(acs->listener, LFLAG_RUNNING) ||
		!acs->listener->rwlock || switch_thread_rwlock_tryrdlock(acs->listener->rwlock) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error! cannot get read lock.\n");
		acs->ack = -1;
		goto done;
	}
	
	acs->ack = 1;
	
	SWITCH_STANDARD_STREAM(stream);
	
	if ((status = switch_api_execute(acs->api_cmd, acs->arg, NULL, &stream)) == SWITCH_STATUS_SUCCESS) {
		reply = stream.data;
	} else {
		freply = switch_mprintf("%s: Command not found!\n", acs->api_cmd);
		reply = freply;
	}

	if (!reply) {
		reply = "Command returned no output!";
	}

	if (acs->bg) {
		switch_event_t *event;

		if (switch_event_create(&event, SWITCH_EVENT_BACKGROUND_JOB) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Job-UUID", acs->uuid_str);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Job-Command", acs->api_cmd);
			if (acs->arg) {
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Job-Command-Arg", acs->arg);
			}
			switch_event_add_body(event, "%s", reply);
			switch_event_fire(&event);
		}
	} else {
		switch_size_t rlen, blen;
		char buf[1024] = "";

		if (!(rlen = strlen(reply))) {
			reply = "-ERR no reply\n";
			rlen = strlen(reply);
		}

		switch_snprintf(buf, sizeof(buf), "Content-Type: api/response\nContent-Length: %" SWITCH_SSIZE_T_FMT "\n\n", rlen);
		blen = strlen(buf);
		switch_socket_send(acs->listener->sock, buf, &blen);
		switch_socket_send(acs->listener->sock, reply, &rlen);
	}

	switch_safe_free(stream.data);
	switch_safe_free(freply);

	if (acs->listener->rwlock) {
		switch_thread_rwlock_unlock(acs->listener->rwlock);
	}

  done:

	if (acs->bg) {
		switch_memory_pool_t *pool = acs->pool;
		if (acs->ack == -1) {
			int sanity = 2000;
			while(acs->ack == -1) {
				switch_cond_next();
				if (--sanity <= 0) break;
			}
		}

		acs = NULL;
		switch_core_destroy_memory_pool(&pool);
		pool = NULL;
			
	}

 cleanup:
	switch_mutex_lock(globals.listener_mutex);
	prefs.threads--;
	switch_mutex_unlock(globals.listener_mutex);

	return NULL;

}
*/

static switch_status_t auth_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	char *dispatcher = switch_event_get_header(*event, "dispatcher");
	char *pass = switch_event_get_header(*event,"password");
	int authed = 0;

	
	switch_xml_t xml_root, x_domains,domain,group,user,params,param;
	
	switch_stream_handle_t stream = { 0 };
	char *apiReply = NULL;
	char * apiReplyDispatcher =NULL;
       switch_xml_t xmldynamic = NULL;
       switch_xml_t xmldispatcher =NULL;
       const char *dispatcherId = NULL;
       const char *dispatcherStatus = NULL;
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"auth_command:%s\n",pass);

	   char digest[SWITCH_MD5_DIGEST_STRING_SIZE]={0};

	if(dispatcher == NULL){
		goto done;
	}
	
	//auth
	if(dispatcher !=NULL &&
		switch_xml_locate("directory", NULL, NULL, NULL, &xml_root, &x_domains, NULL, SWITCH_FALSE) == SWITCH_STATUS_SUCCESS) {
		if(x_domains != NULL){
			domain = switch_xml_child(x_domains,"domain");
			while(domain != NULL){
				user = NULL;
				if(switch_xml_locate_user_in_domain(dispatcher,domain,&user,&group)== SWITCH_STATUS_SUCCESS){
					if(user != NULL){
						params = switch_xml_child(user,"params");
						if(params != NULL){
							param = switch_xml_child(params,"param");
							while(param != NULL){
								const char * var = switch_xml_attr_soft(param,"name");
								const char * val = switch_xml_attr_soft(param,"value");
								if(!strcasecmp("password", var)){
									if(pass != NULL && strlen(pass)>0){
																		
										if(val == NULL){
											val = "";
										}
										switch_md5_string(digest,(void *)val,strlen(val));
										if(!strcasecmp(digest, pass)){
											authed++;
										}
									}
									break;
								}
								param = switch_xml_next(param);
							}
							
						}
						else{
							authed++;
							break;
						}
					}
				}
				
				domain = switch_xml_next(domain);
			}
		}
		
		
		
		switch_xml_free(xml_root);
		
		
		
	}
	if(authed > 0){ //sheck if it is dispatchers
		authed = 0;
		SWITCH_STANDARD_STREAM(stream);


		if((status = switch_api_execute("dispat","id",NULL,&stream)) == SWITCH_STATUS_SUCCESS){
			apiReply = stream.data;

		}
		if(apiReply != NULL){
			apiReplyDispatcher= strstr(apiReply,"<dispatchers>\n");
		}

		if(apiReplyDispatcher!= NULL){


			xmldynamic= switch_xml_parse_str_dynamic(apiReplyDispatcher,SWITCH_TRUE);
			if(xmldynamic != NULL){
				xmldispatcher = switch_xml_child(xmldynamic,"dispatcher");
				while(xmldispatcher != NULL){
					dispatcherId =  switch_xml_attr_soft(xmldispatcher,"id");
					dispatcherStatus = switch_xml_attr_soft(xmldispatcher,"status");
					if(!strcasecmp(dispatcher, dispatcherId)){
						authed++;
						break;
					}
					xmldispatcher=switch_xml_next(xmldispatcher);
				}
				
			}
			
		}
		switch_safe_free(stream.data);
	}
done:	
	if(authed > 0){
		listener->dispatcher = switch_core_strdup(listener->pool,dispatcher);
		if(dispatcherStatus == NULL){
			dispatcherStatus = "none";
		}
		switch_snprintf(reply, sizeof(reply), "content-type:auth/reply\nreply-text:ok\nstatus:%s\n\n",dispatcherStatus);
		switch_set_flag_locked(listener,LFLAG_AUTHED);
	}
	else{
		switch_snprintf(reply, sizeof(reply), "content-type:auth/reply\nreply-text:err\ndiscription:pass invalid.\n\n");
		switch_clear_flag_locked(listener,LFLAG_RUNNING);
	}
	
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);

	if(xmldynamic != NULL){
		switch_xml_free(xmldynamic);
	}
	return status;
}
static switch_status_t query_all_users_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	
	switch_xml_t xml_root, x_domains;
	char *buf;
	switch_size_t buf_len;
	if (switch_xml_locate("directory", NULL, NULL, NULL, &xml_root, &x_domains, NULL, SWITCH_FALSE) == SWITCH_STATUS_SUCCESS) {
		buf = switch_xml_toxml(x_domains, SWITCH_FALSE);
		switch_assert(buf);
		buf_len = strlen(buf);
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-users\nreply-text:ok\ncontent-length:%d\n\n",buf_len);
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
		switch_socket_send(listener->sock,buf, &buf_len);
		free(buf);
		switch_xml_free(xml_root);
		
	}
	else{
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-users\nreply-text:err\ndiscription:no users information.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	
	return status;
}
static switch_status_t query_all_gateways_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	
	switch_xml_t xml_root=NULL, cfg=NULL;
	char *buf;
	switch_size_t buf_len;
	int err=0;
	if((xml_root = switch_xml_open_cfg("sofia.conf",&cfg,NULL)) != NULL){
		buf = switch_xml_toxml(cfg,SWITCH_FALSE);
		if(buf == NULL){
			err++;
		}else{
			buf_len = strlen(buf);
			switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-gateways\nreply-text:ok\ncontent-length:%d\n\n",buf_len);
			reply_len = strlen(reply);
			switch_socket_send(listener->sock,reply, &reply_len);
			switch_socket_send(listener->sock,buf, &buf_len);
			free(buf);
			err = 0;
		}
		switch_xml_free(xml_root);
	}
	else{
		err++;
	}

	if(err){
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-gateways\nreply-text:err\ndiscription:no gateways information.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	
	return status;
}
static switch_status_t query_all_reg_users_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_stream_handle_t stream = { 0 };
	switch_stream_handle_t streamRet = { 0 };
	
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	switch_size_t buf_len = 0;
	char *apiReply = NULL;
	char * reg =NULL;

	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"query_all_reg_users_command now.\n");
	
	SWITCH_STANDARD_STREAM(stream);
	
	if((status = switch_api_execute("sofia","status profile internal",NULL,&stream)) == SWITCH_STATUS_SUCCESS){
		apiReply = stream.data;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"api(sofia) not found!\n");
		status = SWITCH_STATUS_SUCCESS;
	}
	
	if(apiReply != NULL){
		reg = strstr(apiReply,"\nRegistrations:\n");
	}
	if(reg == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"reg informantion error.\n");
		
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-reg-users\nreply-text:err\ndiscription:api not found.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	else{
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",apiReply);
		char * call_id = strstr(reg,"Call-ID:");
		char * var = NULL;
		char *peak = NULL;
		SWITCH_STANDARD_STREAM(streamRet);
		streamRet.write_function(&streamRet,"<registrations>\n");
		streamRet.write_function(&streamRet,"<users>\n");
		while(call_id != NULL){
			reg = call_id + sizeof("Call-ID:");
			
			streamRet.write_function(&streamRet,"<user");
			
#define USER_PROP(A,B)	var = strstr(reg,A":"); \
			if(var != NULL){ \
				var = strchr(var,'\t'); \
				var++; \
				peak = strchr(var,'\n'); \
				if(peak != NULL){ \
					*peak = '\0'; \
				} \
				streamRet.write_function(&streamRet," "B"=\""); \
				while(*var !='\0'){if(*var !='"'){streamRet.write_function(&streamRet,"%c",*var);} var++;} \
				streamRet.write_function(&streamRet,"\"");	\
				if(peak != NULL){ \
				*peak = '\n'; \
				} \
			}
			USER_PROP("Contact","contact");
			USER_PROP("Agent","agent");
			USER_PROP("IP","ip");
			USER_PROP("Port","port");
			USER_PROP("Auth-User","id");
			
			streamRet.write_function(&streamRet," />\n");
			
			call_id = strstr(reg,"Call-ID:");
		}

		streamRet.write_function(&streamRet,"</users>\n");
		streamRet.write_function(&streamRet,"</registrations>\n");
		
		apiReply = streamRet.data;
		buf_len = strlen(apiReply);
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",apiReply);

		
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-reg-users\nreply-text:ok\ncontent-length:%d\n\n",buf_len);
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
		switch_socket_send(listener->sock,apiReply, &buf_len);
		
		switch_safe_free(streamRet.data);
	}
	switch_safe_free(stream.data);
	
	return status;
}
static switch_status_t query_all_current_calls_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_stream_handle_t stream = { 0 };
	switch_stream_handle_t streamRet = { 0 };
	
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	switch_size_t buf_len = 0;
	char *apiReply = NULL;
	char * call =NULL;
	int noCall = 0;

	char *argv[25] = { 0 };
	int argc;
	
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"query_all_current_calls_command now.\n");
	
	SWITCH_STANDARD_STREAM(stream);
	
	if((status = switch_api_execute("conference","list delim ;",NULL,&stream)) == SWITCH_STATUS_SUCCESS){
		apiReply = stream.data;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"api(conference) not found!\n");
		status = SWITCH_STATUS_SUCCESS;
	}
	
	if(apiReply != NULL){
		call = strstr(apiReply,"Conference");
	}
	if(call == NULL){
		call =  strstr(apiReply,"No active conferences");
		if(call != NULL){
			noCall++;
		}
	}
	if(call == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"calls informantion error.\n");
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s\n",apiReply);
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-current-calls\nreply-text:err\ndiscription:api not found.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	else{
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",apiReply);
		SWITCH_STANDARD_STREAM(streamRet);
		streamRet.write_function(&streamRet,"<calls>\n");
		if(noCall == 0){

		char t = 0;
		int members = 0;
		char *memberStart = NULL;
		char * nameStart = NULL;
		while(call != NULL){
			memberStart = NULL;
			members = 0;
			call  += sizeof("Conference");
			nameStart =  strstr(call,")\n");
			if(nameStart == NULL){
				call = strstr(call,"Conference");
				continue;
			}
			
			streamRet.write_function(&streamRet,"<call");

			//call name
			while(*call == ' '){
				call++;
			}
			nameStart = call;

			while(*call != '\n'){
				call++;
			}
			call--;
			if(*call == ')'){
				while(*call  != '('){
					call--;
				}
				memberStart =call+1;
				call--;
				while(*call == ' '){
					call--;
				}
				call++;
				
				if(call > nameStart){
					t = *call;
					*call = '\0';
					streamRet.write_function(&streamRet," name=\"%s\"",nameStart);
					*call = t;
				}
			}
			
			streamRet.write_function(&streamRet,">\n");

			if(memberStart !=NULL){
				//while call member
				while(*memberStart >='0' && *memberStart<='9'){
					members *=10;
					members += (int)(*memberStart - '0');
					memberStart++;
				}
				while(*memberStart != '\n') memberStart++;
				while(*memberStart == '\n') memberStart++;
				
				while(members > 0){
					call = memberStart;
					while(*call != '\n' && *call !='\0') call++;
					t = *call;
					*call = '\0';
					argc = switch_separate_string(memberStart,';',argv,(sizeof(argv)/sizeof(argv[0])));
					*call = t;
					/*1;sofia/internal/1000@192.168.1.7;gateway-name;inbound;1000;1000;hear|speak|talking|floor;0;0;300
					0       1                             2          3        4    5    6                       7 8  9*/
					if(argc >= 6){
						char * id = dup_user_id_from_channel_name(argv[1]);
						
						streamRet.write_function(&streamRet,"<part member=\"%s\"",argv[0]);

						//nameStart = strstr(argv[1],"gateway");
						if(!switch_strlen_zero(argv[2])){
							streamRet.write_function(&streamRet," type=\"gateway\"");
							streamRet.write_function(&streamRet," gateway-name=\"%s\"",argv[2]);
						}else{

							streamRet.write_function(&streamRet," type=\"user\"");
							streamRet.write_function(&streamRet," gateway-name=\"null\"");
						}
						
						nameStart = strstr(argv[3],"out");
						if(nameStart != NULL){
							streamRet.write_function(&streamRet," bound=\"out\"");
						}
						else{
							streamRet.write_function(&streamRet," bound=\"in\"");
						}
						
						streamRet.write_function(&streamRet," id=\"%s\" flag=\"%s\"/>\n",id,argv[6]);//argv[4] just caller
						switch_safe_free(id);
					}
					memberStart = call;
					while(*memberStart == '\n') memberStart++;
					members--;
				}
			}
			streamRet.write_function(&streamRet,"</call>\n");
			call = strstr(call,"Conference");

		}

		}
		streamRet.write_function(&streamRet,"</calls>\n");
		
		apiReply = streamRet.data;
		buf_len = strlen(apiReply);
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",apiReply);

		
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-current-calls\nreply-text:ok\ncontent-length:%d\n\n",buf_len);
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
		switch_socket_send(listener->sock,apiReply, &buf_len);
		
		switch_safe_free(streamRet.data);
	}
	switch_safe_free(stream.data);
	
	return status;
}
static switch_status_t query_all_dispatchers_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_stream_handle_t stream = { 0 };
	
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	switch_size_t buf_len = 0;
	char *apiReply = NULL;
	char * dispatchers =NULL;

	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"query_all_dispatcher_phones_command now.\n");
	
	SWITCH_STANDARD_STREAM(stream);
	
	if((status = switch_api_execute("dispat","id",NULL,&stream)) == SWITCH_STATUS_SUCCESS){
		apiReply = stream.data;
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"api(dispat) not found!\n");
		status = SWITCH_STATUS_SUCCESS;
	}
	
	if(apiReply != NULL){
		dispatchers = strstr(apiReply,"<dispatchers>\n");
	}
	if(dispatchers == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"dispatchers informantion error.\n");
		
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-dispatchers\nreply-text:err\ndiscription:api not found.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	else{
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",dispatchers);
		buf_len = strlen(dispatchers);
		
		switch_snprintf(reply, sizeof(reply), "content-type:reply/query-all-dispatchers\nreply-text:ok\ncontent-length:%d\n\n",buf_len);
		reply_len = strlen(reply);
		
		switch_socket_send(listener->sock,reply, &reply_len);
		switch_socket_send(listener->sock,dispatchers, &buf_len);
		
	}
	switch_safe_free(stream.data);
	return status;
}

static switch_status_t md5ToRcode(char result[20],char digest[SWITCH_MD5_DIGEST_STRING_SIZE])
{
	//char HexChar[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	//char result[SWITCH_MD5_DIGEST_STRING_SIZE+3]={0};
	//char d,r;
	int slash = 0;
	if(switch_strlen_zero(digest)){
		return SWITCH_STATUS_FALSE;
	}
	
	for(int i = 0;i<16;i++){
		if((i > 0) && ((i%4)==0) ){
			result[i+slash]='-';
			slash++;
		}
		//d = digest[i];
		//r = HexChar[(d & 0x0f)];
		result[i+slash]=digest[i];
	}
	
	result[19]='\0';

	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t m_register_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	char digest[SWITCH_MD5_DIGEST_STRING_SIZE] = { 0 };
	char result[20]={0};
	char *mcode = switch_event_get_header(*event,"mcode");
	char *rcode = switch_event_get_header(*event,"rcode");
	int reg= 0;
	char *lpbuf;

	if(switch_strlen_zero(mcode) || switch_strlen_zero(rcode)){
		goto err;
	}
	lpbuf = switch_mprintf("%s:%s", "dingdang",mcode);
	switch_md5_string(digest, (void *) lpbuf, strlen(lpbuf));
	switch_safe_free(lpbuf);
	if(md5ToRcode(result,digest) != SWITCH_STATUS_SUCCESS){
		goto err;
	}
	if (!strcmp(rcode, result)) {
		reg++;
	}
	if(reg == 0){
		goto err;
	}
	switch_snprintf(reply,  sizeof(reply), "content-type:reply/m-register\nreply-text:ok\n\n");
    reply_len = strlen(reply);
    switch_socket_send(listener->sock,reply, &reply_len);

	goto done;
err:
	switch_snprintf(reply,  sizeof(reply), "content-type:reply/m-register\nreply-text:err\ndiscription:register code error.\n\n");
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);
done:		

	return status;
}
static switch_status_t dispatch_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[128] = "";
	switch_size_t reply_len = sizeof(reply);
	
	switch_stream_handle_t stream = { 0 };
	char *apiReply = NULL;
	char *data = NULL;
	
	char *dispatchType = switch_event_get_header(*event, "dispatch-type");
	
	char *dispatcher = switch_event_get_header(*event, "dispatcher");
	char *destination = switch_event_get_header(*event,"destination");
	
	if(dispatchType == NULL || dispatcher == NULL || destination == NULL){
		status =  SWITCH_STATUS_FALSE;
		goto err;
	}
	
	SWITCH_STANDARD_STREAM(stream);
	data = switch_mprintf("%s %s %s",dispatcher,dispatchType,destination);
	if(data == NULL){
		status =  SWITCH_STATUS_FALSE;
		goto err;
	}
	if((status = switch_api_execute("dispat",data,NULL,&stream)) == SWITCH_STATUS_SUCCESS){
		apiReply = stream.data;
		if(apiReply != NULL && strstr(apiReply,"err") != NULL){
			status =  SWITCH_STATUS_FALSE;
			goto err;
		}
		
	} else {
		status =  SWITCH_STATUS_FALSE;
		goto err;
	}

	switch_snprintf(reply, sizeof(reply), "content-type:reply/dispatch\nreply-text:ok\n\n");
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);
	goto done;
		
err:
	switch_snprintf(reply,  sizeof(reply), "content-type:reply/dispatch\nreply-text:err\ndiscription:state false.\n\n");
	reply_len = strlen(reply);
	switch_socket_send(listener->sock,reply, &reply_len);
done:		
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",apiReply);
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\n%s\n",reply);
	switch_safe_free(data);
	switch_safe_free(stream.data);
	return status;
}

static switch_status_t cfg_command(listener_t *listener, switch_event_t **event){
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[128]="";
	switch_size_t reply_len = sizeof(reply);
	const char *err;
	switch_xml_t xml_root;
	if ((xml_root = switch_xml_open_root(1, &err))) {
		switch_xml_free(xml_root);
		switch_snprintf(reply, sizeof(reply), "content-type:reply/cfg\nreply-text:ok\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	else{
		switch_snprintf(reply, sizeof(reply), "content-type:reply/cfg\nreply-text:err\ndiscription:no  information.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}
	
	return status;
}

static switch_status_t message_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char reply[512]="";
	switch_size_t reply_len = sizeof(reply);

	char *from = switch_event_get_header(*event, "message-from");
	
	char *to = switch_event_get_header(*event, "message-to");
	char *subject = switch_event_get_header(*event, "message-subject");
	char *type = switch_event_get_header(*event,"message-type");
       char *body =  switch_event_get_body(*event);

	   //switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"\nfrom:%s\nto:%s\ntype:%s\nmsg:%s\n",from,to,type,body);
	
	char too[128]; 
	char *ptr = to;
	char *endptr = to+strlen(to);
	char split= ';';
	char *splitptr = to;
	int oks=0;
	int fails = 0;
	char *tp = "text/plain";
	char *ffrom = "system";
	char *subj = "";
	
	if(switch_strlen_zero(to) || switch_strlen_zero(body)){
		status =  SWITCH_STATUS_FALSE;
		goto err;
	}

	if(!switch_strlen_zero(type)){
		tp = type;
	}

	if(!switch_strlen_zero(from)){
		ffrom = from;
	}

	if(!switch_strlen_zero(subject)){
		subj = subject;
	}
		
	while(ptr<endptr){
		while(splitptr<endptr && *splitptr!=split){
			splitptr++;
		}
		if(splitptr<endptr){
			*splitptr='\0';
		}
		if(splitptr > ptr){
			switch_snprintf(too, sizeof(too), "internal/%s", ptr);
			if(switch_core_chat_send(DISPAT_CHAT_INTERFACE,DISPAT_CHAT_PROTO,ffrom,too,subj,body,tp,NULL) 
				== SWITCH_STATUS_SUCCESS){
				oks++;		
			}
			else{
				fails++;
			}
		}
		if(splitptr<endptr){
			*splitptr=split;
		}
		splitptr++;
		ptr=splitptr;
	}
	
err:	
	if(oks > 0){
		switch_snprintf(reply, sizeof(reply), "content-type:reply/message\nreply-text:ok\ndiscription:ok(%d),fail(%d).\n\n",oks,fails);
		reply_len = strlen(reply);
	}
	else{
		switch_snprintf(reply, sizeof(reply), "content-type:reply/message\nreply-text:err\ndiscription:ok(%d),fail(%d).\n\n",oks,fails);
		reply_len = strlen(reply);
		
	}
	switch_socket_send(listener->sock,reply, &reply_len);
	return status;
}
static command_api_t command_api_s[]={
	{"auth",auth_command},
	{"command/m-register",m_register_command},
	{"command/dispatch",dispatch_command},
	{"command/message",message_command},
	{"command/query-all-users",query_all_users_command},
	{"command/query-all-gateways",query_all_gateways_command},
	{"command/query-all-reg-users",query_all_reg_users_command},
	{"command/query-all-current-calls",query_all_current_calls_command},
	{"command/query-all-dispatchers",query_all_dispatchers_command},
	{"cfg",cfg_command},
	{NULL,NULL}
};


static switch_status_t parse_command(listener_t *listener, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char  reply[128] = "";
	switch_size_t reply_len = sizeof(reply);
	char *cmd;
	int called = 0;
	if(event == NULL || *event == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"parse_command:event null\n");
		return SWITCH_STATUS_FALSE;
	}
	cmd = switch_event_get_header(*event, "content-type");

	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"parse_command:%s\n",switch_event_get_header(*event, "Command"));
	if(switch_strlen_zero(cmd)){
		return SWITCH_STATUS_FALSE;
	}
	
	
	if(command_api_s != NULL){
	
		for(int i = 0;command_api_s[i].command != NULL;i++){
			if(!strcasecmp(command_api_s[i].command,cmd)){
				command_api_function_t api_function = command_api_s[i].command_api_function;
				if(api_function != NULL){
					status = api_function(listener,event);
					called++;
				}
				
				break;
			}
		}
	}
	if(called == 0){
		switch_snprintf(reply, sizeof(reply), "content-type:reply/error\nreply-text:err\ndiscription:invalid command.\n\n");
		reply_len = strlen(reply);
		switch_socket_send(listener->sock,reply, &reply_len);
	}

	//caller will destroy the event;
	
	return SWITCH_STATUS_SUCCESS;
}

static void strip_cr(char *s)
{
	char *p;
	if ((p = strchr(s, '\r')) || (p = strchr(s, '\n'))) {
		*p = '\0';
	}
}




static switch_status_t read_packet(listener_t *listener, switch_event_t **event, uint32_t timeout)
{
	switch_size_t mlen, bytes = 0;
	char mbuf[2048] = "";
	//char buf[1024] = "";
	//switch_size_t len;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	int count = 0;
	uint32_t elapsed = 0;
	time_t start = 0;
	void *pop;
	char *ptr;
	uint8_t crcount = 0;
	uint32_t max_len = sizeof(mbuf);
	
	int clen = 0;

	*event = NULL;

	if (prefs.done) {
		return SWITCH_STATUS_FALSE;
	}

	start = switch_epoch_time_now(NULL);
	ptr = mbuf;

	
	while (listener->sock && !prefs.done) {
		uint8_t do_sleep = 1;
		mlen = 1;

		status = switch_socket_recv(listener->sock, ptr, &mlen);
		
		if (prefs.done || (!SWITCH_STATUS_IS_BREAK(status) && status != SWITCH_STATUS_SUCCESS)) {
			return SWITCH_STATUS_FALSE;
		}

		if (mlen) {
			bytes += mlen;
			do_sleep = 0;

			if (*mbuf == '\r' || *mbuf == '\n') {	/* bah */
				ptr = mbuf;
				mbuf[0] = '\0';
				bytes = 0;
				continue;
			}

			if (*ptr == '\n') {
				crcount++;
			} else if (*ptr != '\r') {
				crcount = 0;
			}
			ptr++;

			if (bytes >= max_len) {
				crcount = 2;
			}

			if (crcount == 2) {
				char *next;
				char *cur = mbuf;
				bytes = 0;
				while (cur) {
					if ((next = strchr(cur, '\r')) || (next = strchr(cur, '\n'))) {
						while (*next == '\r' || *next == '\n') {
							next++;
						}
					}
					count++;
					if (count == 1) {
						switch_event_create(event, SWITCH_EVENT_COMMAND);
						switch_event_add_header_string(*event, SWITCH_STACK_BOTTOM, "Command", mbuf);
					} 
					if (cur) {
						char *var, *val;
						var = cur;
						strip_cr(var);
						if (!switch_strlen_zero(var)) {
							if ((val = strchr(var, ':'))) {
								*val++ = '\0';
								while (*val == ' ') {
									val++;
								}
							}
							if (var && val) {
								//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s:%s\n",var,val);
								switch_event_add_header_string(*event, SWITCH_STACK_BOTTOM, var, val);
								if (!strcasecmp(var, "content-length")) {
									clen = atoi(val);
									
									if (clen > 0) {
										char *body;
										char *p;
										
										switch_zmalloc(body, clen + 1);

										p = body;
										while(clen > 0) {
											mlen = clen;
											
											status = switch_socket_recv(listener->sock, p, &mlen);

											if (prefs.done || (!SWITCH_STATUS_IS_BREAK(status) && status != SWITCH_STATUS_SUCCESS)) {
												return SWITCH_STATUS_FALSE;
											}
											
											/*
											if (channel && !switch_channel_ready(channel)) {
												status = SWITCH_STATUS_FALSE;
												break;
											}
											*/

											clen -= (int) mlen;
											p += mlen;
										}
										
										switch_event_add_body(*event, "%s", body);
										free(body);
									}
								}
							}
						}
					}
					
					cur = next;
				}
				break;
			}
		}
		
		if (timeout) {
			elapsed = (uint32_t) (switch_epoch_time_now(NULL) - start);
			if (elapsed >= timeout) {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"read packet,timeout\n");
								
				//switch_clear_flag_locked(listener, LFLAG_RUNNING);
				//return SWITCH_STATUS_FALSE;
			}
		}

		if (!*mbuf) {//没命令，再处理event
			
			
			if (switch_test_flag(listener, LFLAG_EVENTS)) {
				while (switch_queue_trypop(listener->event_queue, &pop) == SWITCH_STATUS_SUCCESS) {
					switch_event_t *pevent = (switch_event_t *) pop;

					if(pevent){
						filterEvent2Trap(listener,&pevent);
						switch_event_destroy(&pevent);
					}
				}
			}
		}

	
		if (do_sleep) {
			switch_cond_next();
		}
	}

	return status;

}
static void add_listener(listener_t *listener)
{
	/* add me to the listeners so I get events */
	switch_mutex_lock(globals.listener_mutex);
	listener->next = listen_list.listeners;
	listen_list.listeners = listener;
	switch_mutex_unlock(globals.listener_mutex);
}
static void remove_listener(listener_t *listener)
{
	listener_t *l, *last = NULL;

	switch_mutex_lock(globals.listener_mutex);
	for (l = listen_list.listeners; l; l = l->next) {
		if (l == listener) {
			if (last) {
				last->next = l->next;
			} else {
				listen_list.listeners = l->next;
			}
		}
		last = l;
	}
	switch_mutex_unlock(globals.listener_mutex);
}

static void flush_listener(listener_t *listener, switch_bool_t flush_log, switch_bool_t flush_events)
{
	void *pop;

	if (listener->log_queue) {
		while (switch_queue_trypop(listener->log_queue, &pop) == SWITCH_STATUS_SUCCESS) {
			switch_log_node_t *dnode = (switch_log_node_t *) pop;
			if (dnode) {
				free(dnode->data);
				free(dnode);
			}
		}
	}

	if (listener->event_queue) {
		while (switch_queue_trypop(listener->event_queue, &pop) == SWITCH_STATUS_SUCCESS) {
			switch_event_t *pevent = (switch_event_t *) pop;
			if (!pop) continue;
			switch_event_destroy(&pevent);
		}
	}
}
static void kill_all_listeners(void)
{
	listener_t *l;

	switch_mutex_lock(globals.listener_mutex);
	for (l = listen_list.listeners; l; l = l->next) {
		switch_clear_flag(l, LFLAG_RUNNING);
		if (l->sock) {
			switch_socket_shutdown(l->sock, SWITCH_SHUTDOWN_READWRITE);
			switch_socket_close(l->sock);
		}
	}
	switch_mutex_unlock(globals.listener_mutex);
}
static void close_socket(switch_socket_t **sock)
{
	switch_mutex_lock(listen_list.sock_mutex);
	if (*sock) {
		switch_socket_shutdown(*sock, SWITCH_SHUTDOWN_READWRITE);
		switch_socket_close(*sock);
		*sock = NULL;
	}
	switch_mutex_unlock(listen_list.sock_mutex);
}
static void *SWITCH_THREAD_FUNC listener_run(switch_thread_t *thread, void *obj)
{
	listener_t *listener = (listener_t *) obj;
	char buf[1024];
	switch_size_t len;
	switch_status_t status;
	switch_event_t *event;
	//char reply[512] = "";
	//switch_core_session_t *session = NULL;
	//switch_channel_t *channel = NULL;
	switch_event_t *revent = NULL;
	//const char *var;
	switch_event_t *fevent = NULL;

	switch_mutex_lock(globals.listener_mutex);
	prefs.threads++;
	switch_mutex_unlock(globals.listener_mutex);
	
	switch_assert(listener != NULL);
	
	switch_socket_opt_set(listener->sock, SWITCH_SO_TCP_NODELAY, TRUE);
	switch_socket_opt_set(listener->sock, SWITCH_SO_NONBLOCK, TRUE);
	
	if (prefs.acl_count && listener->sa && !switch_strlen_zero(listener->remote_ip)) {
		//acl
	}
	
	if (globals.debug > 0) {
		if (switch_strlen_zero(listener->remote_ip)) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connection Open\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connection Open from %s:%d\n", listener->remote_ip, listener->remote_port);
		}
	}
	
	
	switch_set_flag_locked(listener, LFLAG_RUNNING);
	
	//auth
	
	switch_snprintf(buf, sizeof(buf), "Content-Type: auth/request\nconnection-id:%d\n\n",listener->id);

	len = strlen(buf);
	switch_socket_send(listener->sock, buf, &len);

	while (!switch_test_flag(listener, LFLAG_AUTHED)) {
			status = read_packet(listener, &event, 25);
			if (status != SWITCH_STATUS_SUCCESS) {
				goto done;
			}
			if (!event) {
				continue;
			}

			if (parse_command(listener, &event) != SWITCH_STATUS_SUCCESS) {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"listener_run goto done \n");
	
				switch_clear_flag_locked(listener, LFLAG_RUNNING);
				goto done;
			}
			
			/*if (*reply != '\0') {
				if (*reply == '~') {
					switch_snprintf(buf, sizeof(buf), "Content-Type: command/reply\n%s", reply + 1);
				} else {
					switch_snprintf(buf, sizeof(buf), "Content-Type: command/reply\nReply-Text: %s\n\n", reply);
				}
				len = strlen(buf);
				switch_socket_send(listener->sock, buf, &len);
			}*/
			break;
	}
	if(!switch_test_flag(listener, LFLAG_AUTHED)){
		goto done;
	}
	
	// auth ok
	switch_set_flag_locked(listener, LFLAG_EVENTS);
	add_listener(listener);
	
	if(switch_event_create_subclass(&fevent, SWITCH_EVENT_CUSTOM, EVENT_DISPAT_KB)
	   == SWITCH_STATUS_SUCCESS) {

		switch_event_add_header_string(fevent, SWITCH_STACK_BOTTOM, "action", "reg");
		switch_event_fire(&fevent);
	}
	//read packet and parse
	while (!prefs.done && switch_test_flag(listener, LFLAG_RUNNING) && listen_list.ready) {
		len = sizeof(buf);
		memset(buf, 0, len);
		status = read_packet(listener, &revent, 0);

		if (status != SWITCH_STATUS_SUCCESS) {
			break;
		}

		if (!revent) {
			continue;
		}

		if (parse_command(listener, &revent) != SWITCH_STATUS_SUCCESS) {
			switch_clear_flag_locked(listener, LFLAG_RUNNING);
			break;
		}

		if (revent) {
			switch_event_destroy(&revent);
		}

		/*if (*reply != '\0') {
			if (*reply == '~') {
				switch_snprintf(buf, sizeof(buf), "Content-Type: command/reply\n%s", reply + 1);
			} else {
				switch_snprintf(buf, sizeof(buf), "Content-Type: command/reply\nReply-Text: %s\n\n", reply);
			}
			len = strlen(buf);
			switch_socket_send(listener->sock, buf, &len);
		}*/

	}
	
	
	remove_listener(listener);
	if (revent) {
		switch_event_destroy(&revent);
	}
	
	
done:
	
	if (event) {
		switch_event_destroy(&event);
	}
	
	if (globals.debug > 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Session complete, waiting for children\n");
	}
	
	switch_thread_rwlock_wrlock(listener->rwlock);
	flush_listener(listener, SWITCH_TRUE, SWITCH_TRUE);
	
	if (listener->sock) {
		char disco_buf[512] = "";
		const char message[] = "Disconnected, goodbye.\n";
		int mlen = strlen(message);
		
		/*if (listener->session) {
			switch_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: disconnect-notice\n"
							"Content-Length: %d\n\n", 
							switch_core_session_get_uuid(listener->session), mlen);
		} else {
			switch_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: disconnect-notice\nContent-Length: %d\n\n", mlen);
		}*/
		switch_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: disconnect-notice\nContent-Length: %d\n\n", mlen);
		
		len = strlen(disco_buf);
		switch_socket_send(listener->sock, disco_buf, &len);
		len = mlen;
		switch_socket_send(listener->sock, message, &len);
		close_socket(&listener->sock);
	}

	switch_thread_rwlock_unlock(listener->rwlock);
	if (globals.debug > 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Connection Closed\n");
	}

	switch_core_hash_destroy(&listener->event_hash);

	if (listener->pool) {
		switch_memory_pool_t *pool = listener->pool;
		switch_core_destroy_memory_pool(&pool);
	}
	
	switch_mutex_lock(globals.listener_mutex);
	prefs.threads--;
	switch_mutex_unlock(globals.listener_mutex);

	return NULL;
}
/* Create a thread for the socket and launch it */
static void launch_listener_thread(listener_t *listener)
{
	switch_thread_t *thread;
	switch_threadattr_t *thd_attr = NULL;

	switch_threadattr_create(&thd_attr, listener->pool);
	switch_threadattr_detach_set(thd_attr, 1);
	switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
	switch_thread_create(&thread, thd_attr, listener_run, listener, listener->pool);
}



SWITCH_MODULE_LOAD_FUNCTION(mod_dispat_kb_server_load)
{
	//switch_application_interface_t *app_interface;
	//switch_api_interface_t *api_interface;
	switch_chat_interface_t *chat_interface;
	
	memset(&globals,0,sizeof(globals));
	
	switch_mutex_init(&globals.listener_mutex,SWITCH_MUTEX_NESTED,pool);
	
	memset(&listen_list,0,sizeof(listen_list));
	switch_mutex_init(&listen_list.sock_mutex,SWITCH_MUTEX_NESTED,pool);

	
	switch_core_hash_init(&globals.call_dispat_hash, pool);
   
    switch_mutex_init(&globals.call_dispat_mutex, SWITCH_MUTEX_NESTED,pool);


	/* create /register cumstom event message type */
	if(switch_event_reserve_subclass(EVENT_DISPAT_KB) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!",EVENT_DISPAT_KB);
		return SWITCH_STATUS_TERM;
	}
	
	/*if (switch_event_bind_removable(modname, SWITCH_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, event_handler, NULL, &globals.node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}*/
	if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_REGISTER, event_handler, NULL, &globals.reg_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_UNREGISTER, event_handler, NULL, &globals.unreg_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_REGISTER_EXPIRE, event_handler, NULL, &globals.reg_expire_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_DISPAT, event_handler, NULL, &globals.dispat_state_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	//conference_event_node
	if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_CONFERENCE, event_handler, NULL, &globals.conference_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	if (switch_event_bind_removable(modname, SWITCH_EVENT_RELOADXML, NULL, event_handler, NULL, &globals.xml_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_CALL_DISPATCHER_FAIL, cdr_event_handler, NULL, &globals.call_dispatcher_fail_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	

	//CALLEE RING
	if(switch_event_bind_removable(modname, SWITCH_EVENT_CHANNEL_PROGRESS, NULL, channel_event_handler, NULL, &globals.channel_progress_event_node)!= SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	//CALLEE RING OR CALLER PREANSWER,SEE switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Call-Direction",
	//channel->direction == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound" : "inbound");
	//switch_snprintf(buf, sizeof(buf), "variable_%s", vvar);
	//switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, buf, vval);
	//            char *gateway = (char *)switch_channel_get_variable(channel,"sip_gateway_name");
	//if(!switch_strlen_zero(gateway)){
	//	switch_event_add_header(event,SWITCH_STACK_BOTTOM,"Gateway-Name","%s",gateway);
	//}

	if(switch_event_bind_removable(modname, SWITCH_EVENT_CHANNEL_PROGRESS_MEDIA, NULL, channel_event_handler, NULL, &globals.channel_progress_media_event_node)!= SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	if(switch_event_bind_removable(modname, SWITCH_EVENT_CHANNEL_ANSWER, NULL, channel_event_handler, NULL, &globals.channel_answer_event_node)!= SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}

	if(switch_event_bind_removable(modname, SWITCH_EVENT_CHANNEL_HANGUP, NULL, channel_event_handler, NULL, &globals.channel_hangup_event_node)!= SWITCH_STATUS_SUCCESS){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	/*if (switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM, EVENT_DISPAT_KB, event_handler, NULL, &globals.dispat_kb_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}*/
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_CHAT(chat_interface, DISPAT_CHAT_PROTO, chat_send);

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_dispat_kb_server_runtime)
{
	switch_memory_pool_t *pool = NULL, *listener_pool = NULL;
	switch_status_t rv;
	switch_sockaddr_t *sa;
	switch_socket_t *inbound_socket = NULL;
	listener_t *listener;
	uint32_t x = 0;
	
	
	if (switch_core_new_memory_pool(&pool) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "OH OH no pool\n");
		return SWITCH_STATUS_TERM;
	}

	config();
	
	while(!prefs.done) {
		rv = switch_sockaddr_info_get(&sa, prefs.ip, SWITCH_INET, prefs.port, 0, pool);
		if (rv)
			goto fail;
		rv = switch_socket_create(&listen_list.sock, switch_sockaddr_get_family(sa), SOCK_STREAM, SWITCH_PROTO_TCP, pool);
		if (rv)
			goto sock_fail;
		rv = switch_socket_opt_set(listen_list.sock, SWITCH_SO_REUSEADDR, 1);
		if (rv)
			goto sock_fail;
		rv = switch_socket_bind(listen_list.sock, sa);
		if (rv)
			goto sock_fail;
		rv = switch_socket_listen(listen_list.sock, 5);
		if (rv)
			goto sock_fail;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Socket up listening on %s:%u\n", prefs.ip, prefs.port);
		
		break;
	 sock_fail:
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error! Could not listen on %s:%u\n", prefs.ip, prefs.port);
		switch_yield(100000);
	}
	
	listen_list.ready = 1;
	
	while(!prefs.done){
		if (switch_core_new_memory_pool(&listener_pool) != SWITCH_STATUS_SUCCESS) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "OH OH no pool\n");
			goto fail;
		}
		
		if ((rv = switch_socket_accept(&inbound_socket, listen_list.sock, listener_pool))) {
			if (prefs.done) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Shutting Down\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Socket Error\n");
			}
			break;
		}
		
		if (!(listener = switch_core_alloc(listener_pool, sizeof(*listener)))) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Memory Error\n");
			break;
		}
		
		switch_thread_rwlock_create(&listener->rwlock, listener_pool);
		switch_queue_create(&listener->event_queue, SWITCH_CORE_QUEUE_LEN, listener_pool);
		switch_queue_create(&listener->log_queue, SWITCH_CORE_QUEUE_LEN, listener_pool);
		
		listener->sock = inbound_socket;
		listener->pool = listener_pool;
		listener_pool = NULL;
		
		switch_set_flag(listener, LFLAG_FULL);
		switch_mutex_init(&listener->flag_mutex, SWITCH_MUTEX_NESTED, listener->pool);
		
		switch_core_hash_init(&listener->event_hash, listener->pool);
		switch_socket_addr_get(&listener->sa, SWITCH_TRUE, listener->sock);
		switch_get_addr(listener->remote_ip, sizeof(listener->remote_ip), listener->sa);
		listener->remote_port = switch_sockaddr_get_port(listener->sa);
		launch_listener_thread(listener);
	}
	
	close_socket(&listen_list.sock);
	
	if (pool) {
		switch_core_destroy_memory_pool(&pool);
	}

	if (listener_pool) {
		switch_core_destroy_memory_pool(&listener_pool);
	}


	for (x = 0; x < prefs.acl_count; x++) {
		switch_safe_free(prefs.acl[x]);
	}
	
fail:
	return SWITCH_STATUS_TERM;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_dispat_kb_server_shutdown)
{
	int sanity = 0;

	prefs.done = 1;

	kill_all_listeners();
	//switch_log_unbind_logger(socket_logger);

	close_socket(&listen_list.sock);

	while (prefs.threads) {
		switch_yield(100000);
		kill_all_listeners();
		if (++sanity >= 200) {
			break;
		}
	}

	switch_event_unbind(&globals.reg_event_node);
	switch_event_unbind(&globals.unreg_event_node);
	switch_event_unbind(&globals.reg_expire_event_node);
	switch_event_unbind(&globals.dispat_state_event_node);
	switch_event_unbind(&globals.call_dispatcher_fail_event_node);
	switch_event_unbind(&globals.channel_hangup_event_node);
	switch_event_unbind(&globals.channel_answer_event_node);
	switch_event_unbind(&globals.channel_progress_media_event_node);
	switch_event_unbind(&globals.channel_progress_event_node);
	//switch_event_unbind(&globals.dispat_kb_event_node);
	//conference_event_node
	switch_event_unbind(&globals.conference_event_node);
	switch_event_unbind(&globals.xml_event_node);
	//switch_event_unbind(&globals.node);

	switch_safe_free(prefs.ip);
	switch_safe_free(prefs.password);	

	switch_mutex_lock(globals.call_dispat_mutex);
	switch_core_hash_destroy(&globals.call_dispat_hash);
	switch_mutex_unlock(globals.call_dispat_mutex);

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
