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
 * mod_dispat.c -- Audio stream dispatching Application Module
 */
#include <switch.h>

#define GUJUN_CHANGE_ANSWER 1
#define GUJUN_CHANGE_DIRECT 1
#define GUJUN_CHANGE_HOLD 1
#define GUJUN_CHANGE_FORWARDING 1
#define GUJUN_CHANGE_GROUP 1


SWITCH_MODULE_LOAD_FUNCTION(mod_dispat_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_dispat_shutdown);
SWITCH_MODULE_DEFINITION(mod_dispat,mod_dispat_load,mod_dispat_shutdown,NULL);

#define DISPAT_CONF_FILE "dispat.conf"
#define DISPAT_CALL_CONF_FILE "dispat_call.conf"

#define CONF_EVENT_MAINT "conference::maintenance"
#define DISPAT_EVENT "dispat::info"
#define DISPAT_CFG_EVENT "custom::cfg::dispat"

#define PRIVATE_VARIABLE_OF_API  "_conference_api_after_adding_"
#define DISPAT_NAME_VARIABLE "_dispat_name_"

#define CONFERENCE_APP_COMMAND "conference"
#define SET_APP_COMMAND "set"
#define  CONFERENCE_SET_AUTO_OUTCALL  "conference_set_auto_outcall"
#define RECOVER_APP_COMMAND "recover"

#define CONFERENCE_API_COMMAND "conference"
#define DISPAT_API_COMMAND "dispat"

#define HOVER_ID_VARIABLE "_dispat_id_"

/*#define NODE_NAME_EVENT_HEADER "Caller-Caller-ID-Number"*/
#define NODE_NAME_EVENT_HEADER "Channel-Name"
#define NODE_NAME_VARIABLE "caller_id_number"

#define MAX_PHONE_NUM_LEN 24
#define MAX_DIAL_DURATION 5000

#define DISPAT_NODE_STATE_RESET_VAR  "diapat_node_state_reset"
#define DISPAT_NODE_FORWARDING_VAR  "diapat_node_forwarding"
#define DISPAT_NODE_FORWARDING_CURRENT_VAR  "diapat_node_forwarding_current"
#define VAR_DELIM  ';'
#define CLEAR_FLAG_NOT_READY_VAR "clear_flag_not_ready"

typedef void (*void_fn_t)(void);


typedef enum dispat_state {
	DISPAT_NONE =0,

	DISPAT_HOVER = 1,
		
	DISPAT_BRIDGE =2,//GROUP

	DISPAT_HEAR=3,//GROUP

	DISPAT_GRAB=4,//GROUP

	DISPAT_INSERT = 5,//GROUP
	
	DISPAT_GROUP=6,

	DISPAT_FORWARD=7,

	DISPAT_END = 8
}dispat_state_t;

static char * dispat_state_discription[] = {
	"none",
	"hover",
	"bridge",
	"hear",
	"grab",
	"insert",
	"group",
	"forward",
	"end"
};
typedef enum {
	DFLAG_RUNNING = (1<<0),
	DFLAG_DESTROY = (1<<2)
}dispat_flags_t;

/*typedef struct conference_info{
	
    char *uuid;
    
    switch_core_session_t *session;
    
    char *conference_name;
    
    char *conference_member_id;

	dispat_state_t state;

	switch_event_t *variables_event;

	switch_queue_t *private_event_queue;

}conference_info_t;
*/
typedef struct dispat_node {

	char *uuid;

	char *name;

	char *channel_name;

	switch_core_session_t *session;

	char *conference_name;

	char *conference_member_id;

	dispat_state_t state;

	switch_event_t *variables_event;

	switch_queue_t *private_event_queue;
#if GUJUN_CHANGE_HOLD
	switch_queue_t *hold_event_queue;
#endif

#if GUJUN_CHANGE_FORWARDING
	switch_queue_t *add_event_queue;
	switch_queue_t *del_event_queue;
#endif
	switch_mutex_t  *mutex;
	
	switch_thread_rwlock_t *rwlock;

	switch_memory_pool_t *pool;	

}dispat_node_t;


typedef struct user_node {
	
	char *name;
	
	switch_hash_t *dispat_hash;
	
	int32_t dispat_num;

	dispat_node_t *current;

	switch_mutex_t  *mutex;
	
	switch_thread_rwlock_t *rwlock;

	switch_memory_pool_t *pool;

} user_node_t;

static struct {

	int running;
	
	/*switch_hash_t *dispat_hash;
	
	switch_mutex_t *dispat_mutex;*/

	switch_hash_t *user_hash;
	
	switch_mutex_t *user_mutex;

	switch_memory_pool_t *pool;

	switch_event_node_t *conference_event_node;
	
	switch_event_node_t *node;

	switch_event_node_t *xml_event_node;
	
    switch_event_node_t *dispat_cfg_event_node;

	switch_event_node_t *channel_hangup_event_node;

	switch_hash_t * dispatchers_hash;
	
	switch_mutex_t *dispatchers_mutex;
	
}globals;

typedef  switch_status_t (*dispat_api_function_t)(switch_stream_handle_t *stream, dispat_node_t *node, char  **argv, int argc);

typedef struct dispat_api{

	char *api_command;

	dispat_api_function_t api_function;
	
	char *api_syntax;
	
}dispat_api_t;


typedef struct dispat_state_api_set{
	
	dispat_state_t state;

	dispat_api_t *api_s;

}dispat_state_api_set_t;

static struct {
	
	int can_dispatch_dispatcher;

	int endconf_after_inserting; /*need something like "conference endconf|unendconf memver_id", reserved now*/

	char *call_delim;

	int direct_transfer_enable;

	char *direct_transfer;

	char *direct_transfer_period;

}prefs;

typedef struct {

	int dtmfNum;

	switch_dtmf_t dtmf[MAX_PHONE_NUM_LEN+1];

}dtmf_handler_data_t;

static switch_status_t load_call_conf()
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_xml_t cfg=NULL, xml=NULL, settings=NULL,param=NULL;
	
	/*free prefs if nassecery*/
	switch_safe_free(prefs.call_delim);
	switch_safe_free(prefs.direct_transfer);
	switch_safe_free(prefs.direct_transfer_period);

	
	if(!(xml = switch_xml_open_cfg(DISPAT_CALL_CONF_FILE,&cfg,NULL))){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open of %s failed\n",DISPAT_CALL_CONF_FILE);
		status = SWITCH_STATUS_FALSE;
	} else {
		if(cfg && (settings = switch_xml_child(cfg,"settings"))){
			for(param = switch_xml_child(settings,"param");param;param=param->next){
				char *var = (char *)switch_xml_attr_soft(param,"name");
				char *val = (char *)switch_xml_attr_soft(param,"value");

				if(!strcmp(var,"call-delim")){
					if(!switch_strlen_zero(val)){
						prefs.call_delim = strdup(val);
					}
				} else if(!strcmp(var,"direct-transfer-enable")){
					if(switch_true(val)){
						prefs.direct_transfer_enable = 1;
					} else {
						prefs.direct_transfer_enable = 0;
					}
				} else if(!strcmp(var,"direct-transfer")){
					if(!switch_strlen_zero(val)){
						prefs.direct_transfer = strdup(val);
					}
				} else if(!strcmp(var,"direct-transfer-period")){
					if(!switch_strlen_zero(val)){
						prefs.direct_transfer_period = strdup(val);
					}
				}
			}
		}

		

		switch_xml_free(xml);
	}

	
	return status;

}
static switch_status_t load_conf()
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_xml_t cfg=NULL, xml=NULL, settings=NULL,param=NULL,dispatchers=NULL,dispatcher=NULL;

       switch_hash_index_t *hi=NULL;
	void *val=NULL;
	char * dispatcherID = NULL;
	
	/*free prefs if nassecery*/
	
	switch_mutex_lock(globals.dispatchers_mutex);
	 while((hi = switch_hash_first(NULL,globals.dispatchers_hash)) != NULL){
        switch_hash_this(hi,NULL,NULL,&val);
        dispatcherID = (char *)val;
        if(dispatcherID !=NULL){
            switch_core_hash_delete(globals.dispatchers_hash,dispatcherID);
	
            switch_safe_free(dispatcherID);
            dispatcherID  = NULL;
        }
    }
	switch_mutex_unlock(globals.dispatchers_mutex);
	
	if(!(xml = switch_xml_open_cfg(DISPAT_CONF_FILE,&cfg,NULL))){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open of %s failed\n",DISPAT_CONF_FILE);
		status = SWITCH_STATUS_FALSE;
	} else {
		if((settings = switch_xml_child(cfg,"settings"))){
			for(param = switch_xml_child(settings,"param");param;param=param->next){
				char *var = (char *)switch_xml_attr_soft(param,"name");
				char *val = (char *)switch_xml_attr_soft(param,"value");

				if(!strcmp(var,"can-dispatch-dispatcher")){
					if(switch_true(val)){ 
						prefs.can_dispatch_dispatcher = 1;
					} else {
						prefs.can_dispatch_dispatcher = 0;
					}
				}else if(!strcmp(var,"endconf_after_inserting")){
					if(switch_true(val)){ 
						prefs.endconf_after_inserting = 1;
					} else {
						prefs.endconf_after_inserting = 0;
					}
				}
			}
		}

		if((dispatchers = switch_xml_child(cfg,"dispatchers"))){
			switch_mutex_lock(globals.dispatchers_mutex);
			for(dispatcher = switch_xml_child(dispatchers,"dispatcher");dispatcher;dispatcher=dispatcher->next){
				char * id =  (char *)switch_xml_attr_soft(dispatcher,"id");
				if(id != NULL){
					dispatcherID = strdup(id);
					if(dispatcherID  != NULL){
						switch_core_hash_insert(globals.dispatchers_hash,dispatcherID,dispatcherID);
						
					}
					
				}
				
			}
			
			switch_mutex_unlock(globals.dispatchers_mutex);
		}
		

		switch_xml_free(xml);
	}

	
	return status;
}

static switch_status_t send_dispatcher_state_event(const char *name,const char *uuid,dispat_state_t oldState,dispat_state_t newState)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	switch_event_t *event=NULL;
	
	if(name == NULL || uuid == NULL){
		return SWITCH_STATUS_FALSE;
	}
	if(switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, DISPAT_EVENT) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "dispatcher_name", name);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM,"dispatcher_uuid",uuid);

		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "action", "change_dispatcher_state");
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "old-state", dispat_state_discription[oldState]);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "new-state", dispat_state_discription[newState]);
		switch_event_fire(&event);
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"send DISPAT_EVENT:%s\n",dispat_state_discription[newState]);
	}
	return status;
}
/*
static switch_status_t dispat_add_event_data(dispat_node_t *node, switch_event_t *event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "dispatcher_name", node->name);
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM,"dispatcher_uuid",node->uuid);
	return status;
}*/
static void dispat_set_state(dispat_node_t *node,dispat_state_t newState)
{
	dispat_state_t oldState = DISPAT_NONE;
	if(node == NULL){
		return;
	}
	oldState = node->state;
	node->state = newState;

	send_dispatcher_state_event(node->name,node->uuid,oldState,newState);
	
}



static switch_status_t check_dispatcher_by_id( char * id)
{
	switch_status_t  status= SWITCH_STATUS_FALSE;
	
	char *dispatcher = NULL;
	switch_hash_index_t *hi = NULL,*next = NULL;
	void *val = NULL;

	if(switch_strlen_zero(id)){
		return SWITCH_STATUS_FALSE;
	}
	
	switch_mutex_lock(globals.dispatchers_mutex);
	/*dispat_node*/
	if((hi = switch_hash_first(NULL,globals.dispatchers_hash)) != NULL){
		switch_hash_this(hi,NULL,NULL,&val);
		dispatcher = (char  *)val;
		
		if(dispatcher != NULL){
			if(!strcasecmp(dispatcher,id)){
					status = SWITCH_STATUS_SUCCESS;
					goto done;
			}
		}
		while((next = switch_hash_next(hi)) != NULL) {
			hi = next;
			switch_hash_this(hi,NULL,NULL,&val);
			dispatcher = (char  *)val;
			if(dispatcher != NULL){
				if(!strcasecmp(dispatcher,id)){
					status = SWITCH_STATUS_SUCCESS;
					goto done;
				}
			}
		}
	}
    
done:
	switch_mutex_unlock(globals.dispatchers_mutex);
	return status;
}


static user_node_t *user_node_new(const char *name)
{
	user_node_t *node = NULL;
	switch_memory_pool_t *pool = NULL;

	switch_assert(name != NULL);

	if(!globals.running) {
		return NULL;
	}
	switch_core_new_memory_pool(&pool);
	if(pool == NULL){
		return NULL;
	}
	node = switch_core_alloc(pool,sizeof(*node));
	if(node == NULL) {
		switch_core_destroy_memory_pool(&pool);
		return NULL;
	}
	
	node->name = switch_core_strdup(pool,name);
	
	switch_core_hash_init(&node->dispat_hash,pool);	
	
	node->dispat_num = 0;
	
	node->current = NULL;
	
	switch_mutex_init(&node->mutex,SWITCH_MUTEX_NESTED,pool);
	
	switch_thread_rwlock_create(&node->rwlock,pool);
	
	node->pool = pool;

	return node;
}

static dispat_node_t * dispat_node_new(const char *uuid,const char *channel_name,const char *name,switch_memory_pool_t *pool) 
{
	dispat_node_t *node=NULL;
		
	switch_assert(channel_name != NULL);
	switch_assert(name != NULL);
	switch_assert(uuid != NULL);
	switch_assert(pool != NULL);

	if(!globals.running) {
		return NULL;
	}
	
	node = switch_core_alloc(pool,sizeof(*node));
	if(node == NULL) {
		return NULL;
	}
	
	//node->name = name;
	node->channel_name = switch_core_strdup(pool,channel_name);

	node->name = switch_core_strdup(pool,name);
	
	node->uuid = switch_core_strdup(pool,uuid);
	
	node->session = NULL;

	node->state = DISPAT_NONE;

	
	switch_event_create(&node->variables_event, SWITCH_EVENT_CHANNEL_DATA);
	
	switch_queue_create(&node->private_event_queue,SWITCH_CORE_QUEUE_LEN,pool);
#if GUJUN_CHANGE_HOLD
	switch_queue_create(&node->hold_event_queue,SWITCH_CORE_QUEUE_LEN,pool);
#endif

#if GUJUN_CHANGE_FORWARDING
	switch_queue_create(&node->add_event_queue,SWITCH_CORE_QUEUE_LEN,pool);
	switch_queue_create(&node->del_event_queue,SWITCH_CORE_QUEUE_LEN,pool);
#endif
	switch_mutex_init(&node->mutex, SWITCH_MUTEX_NESTED, pool);
	
	switch_thread_rwlock_create(&node->rwlock,pool);
	
	node->pool = pool;
	
	return node;
}


static char * get_name_regu(const char *name)
{
	char * regularExp=NULL;
	regularExp = switch_mprintf("^(.+[/:]){0,1}%s(@.+){0,1}$",name);

	return regularExp;
}

static user_node_t * user_check_by_name(user_node_t * user_node,const char * user_name, const char *name_regu)
{
	user_node_t *user_node_p = NULL;
	int proceed = 0;
	switch_regex_t *re = NULL;
	int ovector[30];
	
	if(user_node != NULL && user_name != NULL && name_regu != NULL &&
	   strstr(user_node->name,user_name) != NULL){
			if ((proceed = switch_regex_perform(user_node->name, name_regu, &re, ovector, sizeof(ovector) / sizeof(ovector[0])))){
				user_node_p = user_node;
			}
			switch_regex_safe_free(re);
			
	}
	return user_node_p;
}

static user_node_t * user_get_by_name(const char *user_name, int *user_rdlock)
{
       char *name_regu = NULL;
	user_node_t *user_node = NULL;
	switch_hash_index_t *first=NULL,*next=NULL;
	void *val;

	if(user_name == NULL || strlen(user_name) == 0){
		return NULL;
	}
      if((name_regu=get_name_regu(user_name)) == NULL){
	    return NULL;
      }
	 switch_mutex_lock(globals.user_mutex);
     user_node = switch_core_hash_find(globals.user_hash,user_name);
     if(user_node == NULL){
		 if((first = switch_hash_first(NULL,globals.user_hash)) != NULL){
            switch_hash_this(first,NULL,NULL,&val);
            user_node =user_check_by_name( (user_node_t *)val,user_name,name_regu);
                while(user_node == NULL && (next = switch_hash_next(first)) != NULL){
                        first = next;
                        switch_hash_this(first,NULL,NULL,&val);
                        user_node = user_check_by_name((user_node_t *)val,user_name,name_regu);
                       
				}
                

		 }
     }

     if(user_node != NULL && user_rdlock != NULL){
        if(switch_thread_rwlock_rdlock(user_node->rwlock) == SWITCH_STATUS_SUCCESS){
            (*user_rdlock) ++;
        }
     }
    switch_mutex_unlock(globals.user_mutex);
	
		
      switch_safe_free(name_regu);
	return user_node;
}

static user_node_t * user_get_by_name_or_create(const char *user_name, int *user_rdlock)
{
	user_node_t *user_node = NULL;

	if(user_name == NULL || strlen(user_name) == 0){
		return NULL;
	}

	user_node = user_get_by_name(user_name,user_rdlock);
	
	if(user_node == NULL){
		user_node = user_node_new(user_name);
		if(user_node == NULL){
			return NULL;
		}
		switch_mutex_lock(globals.user_mutex);
		
		switch_core_hash_insert(globals.user_hash,user_node->name,user_node);
		if(user_rdlock != NULL){
			if(switch_thread_rwlock_rdlock(user_node->rwlock) == SWITCH_STATUS_SUCCESS){
				(*user_rdlock)++;
			}
		}
		switch_mutex_unlock(globals.user_mutex);


	}

	return user_node;

}
/*
static dispat_node_t * dispat_check_by_name(dispat_node_t * dispat_node,const char * dispat_name,const char *name_regu)
{
	dispat_node_t *dispat_node_p = NULL;
	int proceed = 0;
	switch_regex_t *re = NULL;
	int ovector[30];
	
	if(dispat_node != NULL && dispat_name != NULL && name_regu !=NULL &&
	   strstr(dispat_node->name,dispat_name) != NULL){
			if ((proceed = switch_regex_perform(dispat_node->name, name_regu, &re, ovector, sizeof(ovector) / sizeof(ovector[0])))){
				dispat_node_p = dispat_node;
			}
			switch_regex_safe_free(re);
			
	}
	return dispat_node_p;
}
*/
static dispat_node_t * dispat_get_by_uuid(user_node_t *user_node,const char *uuid, int *dispat_rdlock)
{
	dispat_node_t *dispat_node = NULL;

	if(user_node == NULL || user_node->dispat_hash == NULL ||
	   uuid == NULL || strlen(uuid) == 0){
		return NULL;
	}
	
	switch_mutex_lock(user_node->mutex);
	dispat_node = switch_core_hash_find(user_node->dispat_hash,uuid);
	if(dispat_node != NULL){
		if(dispat_rdlock != NULL){
			switch_thread_rwlock_rdlock(dispat_node->rwlock);
			(*dispat_rdlock) ++;
		}
	}
	switch_mutex_unlock(user_node->mutex);
	
	return dispat_node;
}
static dispat_node_t * dispat_get_by_uuid_or_create(user_node_t *user_node,const char *uuid, const char *channel_name,int *dispat_rdlock)
{
	dispat_node_t *dispat_node = NULL;
	if(user_node == NULL || user_node->dispat_hash == NULL ||
	   uuid == NULL || strlen(uuid) == 0){
		return NULL;
	}

	dispat_node = dispat_get_by_uuid(user_node,uuid,dispat_rdlock);

	if(dispat_node == NULL){
		dispat_node = dispat_node_new(uuid,channel_name,user_node->name,user_node->pool);
		if(dispat_node == NULL){
			return NULL;
		}
		switch_mutex_lock(user_node->mutex);
		
		switch_core_hash_insert(user_node->dispat_hash,dispat_node->uuid,dispat_node);
		user_node->dispat_num++;

		if(dispat_rdlock != NULL){
			if(switch_thread_rwlock_rdlock(dispat_node->rwlock) == SWITCH_STATUS_SUCCESS){
				(*dispat_rdlock)++;
			}
		}
		switch_mutex_unlock(user_node->mutex);
	}
	return dispat_node;

}
/*static dispat_node_t * dispat_get_by_uuid(const char *user_name, const char * uuid,int *dispat_rdlock)
{

	user_node_t *user_node = NULL;
	int user_rdlock = 0;
    dispat_node_t *dispat_node = NULL;

	user_node = user_get_by_name(user_name,&user_rdlock);
	if(user_node != NULL){
		dispat_node = dispat_get_by_uuid(user_node,uuid,dispat_rdlock);
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}
	return dispat_node;
}*/
static dispat_node_t * dispat_get_by_name_in_node(user_node_t *user_node,const char *uuid, int *dispat_rdlock)
{
	dispat_node_t *dispat_node = NULL,*tmp_node=NULL;
	void *val;
	switch_hash_index_t *first=NULL,*next=NULL;


	if(user_node == NULL || user_node->dispat_hash == NULL){
		return NULL;
	}

	switch_mutex_lock(user_node->mutex);

	if(uuid == NULL){
		dispat_node = user_node->current;
	}
	else
	{

		dispat_node = switch_core_hash_find(user_node->dispat_hash,uuid);

		if(dispat_node == NULL){
			if((first = switch_hash_first(NULL,user_node->dispat_hash)) != NULL){
				switch_hash_this(first,NULL,NULL,&val);
				tmp_node = (dispat_node_t *)val;
				if(tmp_node !=NULL && !strcmp(uuid, tmp_node->conference_name))
				{
					dispat_node = tmp_node;
				}
			
				while(dispat_node == NULL &&  (next = switch_hash_next(first)) != NULL)
				{
					first = next;
					switch_hash_this(first,NULL,NULL,&val);
					tmp_node = (dispat_node_t *)val;
					if(tmp_node !=NULL && !strcmp(uuid, tmp_node->conference_name))
						{
							dispat_node = tmp_node;
						}
			
				}

			}//if(first != NULL)
		}//if(dispat_node == NULL)
	}//else
	
	if(dispat_node != NULL){
		if(dispat_rdlock != NULL){
			switch_thread_rwlock_rdlock(dispat_node->rwlock);
			(*dispat_rdlock) ++;
		}
	}
	switch_mutex_unlock(user_node->mutex);
	
	return dispat_node;
}

static dispat_node_t * dispat_get_by_name(const char *user_name, const char * name,int *dispat_rdlock)
{

	user_node_t *user_node = NULL;
	int user_rdlock = 0;
    dispat_node_t *dispat_node = NULL;

	user_node = user_get_by_name(user_name,&user_rdlock);
	if(user_node != NULL){
		if(name != NULL && strcasecmp(name,"NULL")){
			dispat_node = dispat_get_by_name_in_node(user_node,name,dispat_rdlock);
		}else{
			dispat_node = dispat_get_by_name_in_node(user_node,NULL,dispat_rdlock);
		}
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}
	return dispat_node;
}

/*

static void * user_get_member_id(user_node_t * user_node,const char *call)
{
	char * member_id = NULL;
	dispat_node_t * dispat_node =NULL;
	int dispat_rdlock = 0;

	dispat_node = dispat_get_by_name_in_node(user_node,call,&dispat_rdlock);
	if(dispat_node != NULL){
		member_id = dispat_node->conference_member_id;
		if(dispat_rdlock){
			switch_thread_rwlock_unlock(dispat_node->rwlock);
		}
	}
	return member_id;
}

static void * user_get_conference_name(user_node_t * user_node,const char *call)
{
	char * conference_name = NULL;
	dispat_node_t *dispat_node=NULL;
	int dispat_rdlock = 0;

	dispat_node = dispat_get_by_name_in_node(user_node,call,&dispat_rdlock);
	if(dispat_node != NULL){
		conference_name = dispat_node->conference_name;
		if(dispat_rdlock){
			switch_thread_rwlock_unlock(dispat_node->rwlock);
		}
	}

	
	return conference_name;
}

*/



/*
 *diapt queue mothods
 */

static switch_status_t dispat_queue_private_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	
	if(node && event && node->private_event_queue){
		switch_mutex_lock(node->mutex);
		(*event)->event_id = SWITCH_EVENT_PRIVATE_COMMAND;
		if(switch_queue_trypush(node->private_event_queue,*event) == SWITCH_STATUS_SUCCESS){
			(*event)= NULL;
			status = SWITCH_STATUS_SUCCESS;
		} 
		switch_mutex_unlock(node->mutex);
	}

	return status;
}
static switch_status_t dispat_dequeue_private_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	void *pop;

	if(node && event && node->private_event_queue){
		switch_mutex_lock(node->mutex);
		if((status = (switch_status_t)switch_queue_trypop(node->private_event_queue,&pop)) == SWITCH_STATUS_SUCCESS){
			*event = (switch_event_t *)pop;
		}
		switch_mutex_unlock(node->mutex);
	}

	return status;
}
#if GUJUN_CHANGE_HOLD
#if 0
static switch_status_t dispat_queue_hold_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	
	if(node && event && node->hold_event_queue){
		switch_mutex_lock(node->mutex);
		(*event)->event_id = SWITCH_EVENT_PRIVATE_COMMAND;
		if(switch_queue_trypush(node->hold_event_queue,*event) == SWITCH_STATUS_SUCCESS){
			(*event)= NULL;
			status = SWITCH_STATUS_SUCCESS;
		} 
		switch_mutex_unlock(node->mutex);
	}

	return status;
}
#endif
static switch_status_t dispat_dequeue_hold_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	void *pop;

	if(node && event && node->hold_event_queue){
		switch_mutex_lock(node->mutex);
		if((status = (switch_status_t)switch_queue_trypop(node->hold_event_queue,&pop)) == SWITCH_STATUS_SUCCESS){
			*event = (switch_event_t *)pop;
		}
		switch_mutex_unlock(node->mutex);
	}

	return status;
}

#endif


#if GUJUN_CHANGE_FORWARDING
static switch_status_t dispat_queue_add_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	
	if(node && event && node->add_event_queue){
		switch_mutex_lock(node->mutex);
		(*event)->event_id = SWITCH_EVENT_PRIVATE_COMMAND;
		if(switch_queue_trypush(node->add_event_queue,*event) == SWITCH_STATUS_SUCCESS){
			(*event)= NULL;
			status = SWITCH_STATUS_SUCCESS;
		} 
		switch_mutex_unlock(node->mutex);
	}

	return status;
}
static switch_status_t dispat_dequeue_add_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	void *pop;

	if(node && event && node->add_event_queue){
		switch_mutex_lock(node->mutex);
		if((status = (switch_status_t)switch_queue_trypop(node->add_event_queue,&pop)) == SWITCH_STATUS_SUCCESS){
			*event = (switch_event_t *)pop;
		}
		switch_mutex_unlock(node->mutex);
	}

	return status;
}


//del
static switch_status_t dispat_queue_del_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	
	if(node && event && node->del_event_queue){
		switch_mutex_lock(node->mutex);
		(*event)->event_id = SWITCH_EVENT_PRIVATE_COMMAND;
		if(switch_queue_trypush(node->del_event_queue,*event) == SWITCH_STATUS_SUCCESS){
			(*event)= NULL;
			status = SWITCH_STATUS_SUCCESS;
		} 
		switch_mutex_unlock(node->mutex);
	}

	return status;
}

static switch_status_t dispat_dequeue_del_event(dispat_node_t *node, switch_event_t **event)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	void *pop;

	if(node && event && node->del_event_queue){
		switch_mutex_lock(node->mutex);
		if((status = (switch_status_t)switch_queue_trypop(node->del_event_queue,&pop)) == SWITCH_STATUS_SUCCESS){
			*event = (switch_event_t *)pop;
		}
		switch_mutex_unlock(node->mutex);
	}

	return status;
}
#endif
#define DISPAT_PRIVATE_API_CMD "dispat_private_api_cmd"
#define DISPAT_PRIVATE_API_DATA "dispat_private_api_data"

static switch_status_t dispat_parse_private_event(dispat_node_t *node,switch_event_t *event)
{
	switch_stream_handle_t stream = { 0 };
	switch_status_t status = SWITCH_STATUS_FALSE;
	char *api_cmd=NULL;
	char *api_data=NULL,*p_data = NULL;
	char *reply = NULL,*freply = NULL;
	
	if(node == NULL || event == NULL){
		return SWITCH_STATUS_SUCCESS;
	}
	
	api_cmd = switch_event_get_header(event,DISPAT_PRIVATE_API_CMD);
	p_data = switch_event_get_header(event,DISPAT_PRIVATE_API_DATA);
	
	if(api_cmd == NULL){
		return status;
	}
	if(p_data != NULL){
		if((api_data = switch_event_expand_headers(node->variables_event,p_data)) == NULL){
			return status;
		}
	}
	SWITCH_STANDARD_STREAM(stream);
	
	if((status = switch_api_execute(api_cmd,api_data,NULL,&stream)) == SWITCH_STATUS_SUCCESS){
		reply = stream.data;
	} else {
		freply = switch_mprintf("%s: Command not found!\n",api_cmd);
		reply = freply;
	}
	if(!reply){
		reply = "Command returned no output!";
	}
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,reply);
	switch_safe_free(stream.data);
	switch_safe_free(freply);
	if(api_data && api_data != p_data){
		switch_safe_free(api_data);
	}
	return status;
}

static int dispat_private_event_count(dispat_node_t *node)
{
	int len = 0;
	
	if(node && node->private_event_queue){
		switch_mutex_lock(node->mutex);
		len = switch_queue_size(node->private_event_queue);
		switch_mutex_unlock(node->mutex);
	}

	return len;
}
static switch_status_t dispat_parse_next_private_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node && dispat_dequeue_private_event(node,&event) == SWITCH_STATUS_SUCCESS) {
		dispat_parse_private_event(node,event);
		switch_event_destroy(&event);
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}
static switch_status_t dispat_parse_all_private_event(dispat_node_t *node)
{
	while(dispat_parse_next_private_event(node) == SWITCH_STATUS_SUCCESS);
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t dispat_clear_all_private_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node){
		while(dispat_dequeue_private_event(node,&event) == SWITCH_STATUS_SUCCESS) {
			switch_event_destroy(&event);
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}

#if GUJUN_CHANGE_HOLD
static int dispat_hold_event_count(dispat_node_t *node)
{
	int len = 0;
	
	if(node && node->hold_event_queue){
		switch_mutex_lock(node->mutex);
		len = switch_queue_size(node->hold_event_queue);
		switch_mutex_unlock(node->mutex);
	}

	return len;
}
static switch_status_t dispat_parse_next_hold_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node && dispat_dequeue_hold_event(node,&event) == SWITCH_STATUS_SUCCESS) {
		dispat_parse_private_event(node,event);
		switch_event_destroy(&event);
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}
static switch_status_t dispat_parse_all_hold_event(dispat_node_t *node)
{
	while(dispat_parse_next_hold_event(node) == SWITCH_STATUS_SUCCESS);
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t dispat_clear_all_hold_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node){
		while(dispat_dequeue_hold_event(node,&event) == SWITCH_STATUS_SUCCESS) {
			switch_event_destroy(&event);
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}
#endif

#if GUJUN_CHANGE_FORWARDING

static int dispat_add_event_count(dispat_node_t *node)
{
	int len = 0;
	
	if(node && node->add_event_queue){
		switch_mutex_lock(node->mutex);
		len = switch_queue_size(node->add_event_queue);
		switch_mutex_unlock(node->mutex);
	}

	return len;
}
static int dispat_del_event_count(dispat_node_t *node)
{
	int len = 0;
	
	if(node && node->del_event_queue){
		switch_mutex_lock(node->mutex);
		len = switch_queue_size(node->del_event_queue);
		switch_mutex_unlock(node->mutex);
	}

	return len;
}


static switch_status_t dispat_parse_next_add_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node && dispat_dequeue_add_event(node,&event) == SWITCH_STATUS_SUCCESS) {
		dispat_parse_private_event(node,event);
		switch_event_destroy(&event);
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}
static switch_status_t dispat_parse_next_del_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node && dispat_dequeue_del_event(node,&event) == SWITCH_STATUS_SUCCESS) {
		dispat_parse_private_event(node,event);
		switch_event_destroy(&event);
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}

static switch_status_t dispat_parse_all_add_event(dispat_node_t *node)
{
	while(dispat_parse_next_add_event(node) == SWITCH_STATUS_SUCCESS);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t dispat_parse_all_del_event(dispat_node_t *node)
{
	while(dispat_parse_next_del_event(node) == SWITCH_STATUS_SUCCESS);
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t dispat_clear_all_add_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node){
		while(dispat_dequeue_add_event(node,&event) == SWITCH_STATUS_SUCCESS) {
			switch_event_destroy(&event);
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}
static switch_status_t dispat_clear_all_del_event(dispat_node_t *node)
{
	switch_event_t *event;
	
	if(node){
		while(dispat_dequeue_del_event(node,&event) == SWITCH_STATUS_SUCCESS) {
			switch_event_destroy(&event);
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	return SWITCH_STATUS_FALSE;
}

#endif

static switch_status_t dispat_node_free(dispat_node_t *node)
{
	int wrlock = 0;

    if(node != NULL){
    	
        if(switch_thread_rwlock_wrlock(node->rwlock) ==  SWITCH_STATUS_SUCCESS){
			wrlock++;
		}
	    dispat_clear_all_private_event(node);
#if GUJUN_CHANGE_HOLD
		dispat_clear_all_hold_event(node);
#endif

#if GUJUN_CHANGE_FORWARDING
		dispat_clear_all_add_event(node);
		dispat_clear_all_del_event(node);
#endif
	    switch_event_destroy(&node->variables_event);
         
		if(wrlock){
			switch_thread_rwlock_unlock(node->rwlock);
		}
         
        //switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"destroy dispather:%s\n",node->name);
        //switch_core_destroy_memory_pool(&node->pool);
    }

    return SWITCH_STATUS_SUCCESS;
}
static switch_status_t dispat_hash_free(switch_hash_t *dispat_hash)
{
	switch_hash_index_t *hi = NULL;
	void *val = NULL;
	dispat_node_t *dispat_node = NULL;

	if(dispat_hash != NULL){

		while((hi = switch_hash_first(NULL,dispat_hash)) != NULL){
			switch_hash_this(hi,NULL,NULL,&val);
			dispat_node = (dispat_node_t *)val;
			if(dispat_node != NULL){
				switch_core_hash_delete(dispat_hash,dispat_node->uuid);
				dispat_node_free(dispat_node);
				dispat_node = NULL;
			}
		}
				
	}
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t user_node_free(user_node_t *node)
{
	int wrlock = 0;

	 if(node != NULL){
    	
        if(switch_thread_rwlock_wrlock(node->rwlock) == SWITCH_STATUS_SUCCESS){
			wrlock++;
		}
		
		dispat_hash_free(node->dispat_hash);
		node->dispat_num = 0;
		switch_core_hash_destroy(&node->dispat_hash);
		
		if(wrlock){
			switch_thread_rwlock_unlock(node->rwlock);
		}
	     

        switch_core_destroy_memory_pool(&node->pool);

 
    }

    return SWITCH_STATUS_SUCCESS;
}




#define CONFERENCE_ACTION_HEADER "Action"
#define CONFERENCE_ADD_ACTION "add-member"
#define CONFERENCE_DEL_ACTION "del-member"
#define CONFERENCE_KICK_ACTION "kick-member"
#define CONFERENCE_START_TALKING_ACTION "start-talking"
#define CONFERENCE_STOP_TALKING_ACTION "stop-talking"
#define UUID_EVENT_HEADER "Unique-ID"
#define GATEWAY_NAME "Gateway-Name"

#define CONFERENCE_NAME_HEADER "Conference-Name"
#define CONFERENCE_MEMBER_ID_HEADER "Member-ID"


/*
 *handle conference event
 */
static switch_status_t dispat_handle_add(dispat_node_t *dispat_node,switch_event_t *event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	int len = 0;
	//char *uuid =  NULL;
	char *conference_name = NULL;
	char *conference_member_id = NULL;

	/*update variable_event*/

	//uuid = switch_event_get_header(event,UUID_EVENT_HEADER);
	conference_name = switch_event_get_header(event,CONFERENCE_NAME_HEADER);
	conference_member_id = switch_event_get_header(event,CONFERENCE_MEMBER_ID_HEADER);

	/*if(uuid!=NULL){
		node->uuid =switch_core_strdup(node->pool,uuid);
	}*/

	//	switch_mutex_lock(node->mutex);
	if(conference_name != NULL && (dispat_node->conference_name == NULL ||
								   strcmp(conference_name,dispat_node->conference_name) )
	   ){
			dispat_node->conference_name = switch_core_strdup(dispat_node->pool,conference_name);
		
		switch_event_del_header(dispat_node->variables_event,"conference_name");
		switch_event_add_header(dispat_node->variables_event,SWITCH_STACK_BOTTOM,"conference_name","%s",dispat_node->conference_name);

		
	}

	if(conference_member_id != NULL && (dispat_node->conference_member_id == NULL ||
										strcmp(conference_member_id,dispat_node->conference_member_id) )
	   ){
			dispat_node->conference_member_id = switch_core_strdup(dispat_node->pool,conference_member_id);
		switch_event_del_header(dispat_node->variables_event,"conference_member_id");
		switch_event_add_header(dispat_node->variables_event,SWITCH_STACK_BOTTOM,"conference_member_id","%s",dispat_node->conference_member_id);

		

	}
	//switch_mutex_unlock(node->mutex);


	/*handle private event*/
#if GUJUN_CHANGE_FORWARDING
	len = dispat_add_event_count(dispat_node);
	if(len > 0){
		dispat_parse_all_add_event(dispat_node);
	}
#endif
	len = dispat_private_event_count(dispat_node);
	if(len > 0){
		dispat_parse_all_private_event(dispat_node);
	}

	//if(check_dispatcher_by_id(dispat_node->name) == SWITCH_STATUS_SUCCESS){

		if(dispat_node->state == DISPAT_NONE){
			//dispat_set_state(dispat_node,DISPAT_GROUP);
			dispat_node->state = DISPAT_GROUP;
		}
		//}

	return status;
}
static switch_status_t dispat_handle_del(dispat_node_t *dispat_node,switch_event_t *event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	/*int len = 0;*/
	/*update variable_event*/
	/*conference_name*/
	/*conference_member_id*/
	
	
	
	/*handle private event*/
#if GUJUN_CHANGE_FORWARDING
	if(dispat_del_event_count(dispat_node)>0){
		dispat_parse_all_del_event(dispat_node);
	}
#endif
	/*len = dispat_private_event_count(node);
	if(len > 0){
		dispat_parse_all_private_event(node);
	}*/
	
	/*destroy dispat_node*/
#if GUJUN_CHANGE_FORWARDING
	{
		int none = 0;
		if(dispat_node->uuid != NULL){
			dispat_node->session = switch_core_session_locate(dispat_node->uuid);
			if(dispat_node->session != NULL){
				switch_channel_t * channel = switch_core_session_get_channel(dispat_node->session);
				if(channel != NULL ){
					if (switch_channel_get_variable_dup(channel, DISPAT_NODE_STATE_RESET_VAR,SWITCH_FALSE)) {
						switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"%s var %s\n",dispat_node->name,
										  DISPAT_NODE_STATE_RESET_VAR);						
						none++;
						switch_channel_set_variable(channel,DISPAT_NODE_STATE_RESET_VAR,NULL);
					}
					
				}
				switch_core_session_rwunlock(dispat_node->session);
			}
		}

		if(none){
			if(dispat_node->state != DISPAT_HOVER){
				dispat_node->state =  DISPAT_NONE;
			}
		}else{
			if(dispat_node->state != DISPAT_HOVER){
				dispat_node->state =  DISPAT_END;
			}
		}
		
	}
#else
	dispat_node->state =  DISPAT_END;
#endif

    //dispat_set_state(dispat_node,DISPAT_END);
	
	return status;
}
/*
static switch_status_t dispat_handle_kick(dispat_node_t *dispat_node,switch_event_t *event)
{
	return SWITCH_STATUS_SUCCESS;
}
*/

 /*
static switch_status_t dispat_handle_conference_event(dispat_node_t *dispat_node,switch_event_t *event)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char *action  = NULL;
	switch_assert(dispat_node != NULL);
	switch_assert(event != NULL);
	
	action = switch_event_get_header(event,CONFERENCE_ACTION_HEADER);
	if(action == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s receive null action\n",dispat_node->name);
		status = SWITCH_STATUS_FALSE;
		goto done;
	} else {
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s receive %s action\n",dispat_node->name,action);
	}
	if(!strcasecmp(action,CONFERENCE_ADD_ACTION)){
		status = dispat_handle_add(dispat_node,event);
	}else if(!strcasecmp(action,CONFERENCE_DEL_ACTION)){
		status = dispat_handle_del(dispat_node,event);
	}
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"%s receive %s action\n",dispat_node->name,action);
	

done:
	return status;
}*/

  /*
  *DISPAT_END node will be destroyed
  *
  */
  
static dispat_node_t *next_active_dispat_node(user_node_t *user_node)
 {
	dispat_node_t *dispat_node = NULL,*tmp_node=NULL;
	void *val;
	switch_hash_index_t *first=NULL,*next=NULL;

	switch_channel_t *channel = NULL;

	if(user_node == NULL || user_node->dispat_hash == NULL){
		return NULL;
	}
	switch_mutex_lock(user_node->mutex);

	if((first = switch_hash_first(NULL,user_node->dispat_hash)) != NULL)
	{
				switch_hash_this(first,NULL,NULL,&val);
				tmp_node = (dispat_node_t *)val;
				if(tmp_node !=NULL && tmp_node->state != DISPAT_END && tmp_node->uuid != NULL)
				{
					tmp_node->session = switch_core_session_locate(tmp_node->uuid);
					if(tmp_node->session != NULL){
						channel = switch_core_session_get_channel(tmp_node->session);
						   if(channel != NULL 
						   &&!switch_channel_test_flag(channel,CF_HOLD)
						   ){
										dispat_node = tmp_node;
						   }
						   switch_core_session_rwunlock(tmp_node->session);
					}
				}
			
			while(dispat_node == NULL &&  (next = switch_hash_next(first)) != NULL)
			{
				first = next;
				switch_hash_this(first,NULL,NULL,&val);
				tmp_node = (dispat_node_t *)val;
				if(tmp_node !=NULL && tmp_node->state != DISPAT_END && tmp_node->uuid != NULL)
				{
					tmp_node->session = switch_core_session_locate(tmp_node->uuid);
					if(tmp_node->session != NULL){
						channel = switch_core_session_get_channel(tmp_node->session);
						if(channel != NULL 
						   && !switch_channel_test_flag(channel,CF_HOLD)
						   ){
							dispat_node = tmp_node;
					   	}
						switch_core_session_rwunlock(tmp_node->session);
					}
				}

			
			}//while

	}
	
	switch_mutex_unlock(user_node->mutex);

	 return dispat_node;
 }

  /*
   *assert(dispat != NULL);
   *if cur == dispat then cur = next_active();
   *if cur != dispat then do nothing;
   *
   *if(send_event)
   */
 static void user_current_none(user_node_t *user_node,dispat_node_t *dispat_node,int send_event)
{
	
	 if(user_node == NULL || dispat_node == NULL){
		 return;
	 }
	 //switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s cur none\n",user_node->name);
	 switch_mutex_lock(user_node->mutex);	 
	
	 if(dispat_node == user_node->current){
		 
		 user_node->current = next_active_dispat_node(user_node);
		 if(send_event && (check_dispatcher_by_id(dispat_node->name) == SWITCH_STATUS_SUCCESS)){
			 if(user_node->current == NULL){
			 send_dispatcher_state_event(dispat_node->name,dispat_node->uuid,dispat_node->state,DISPAT_NONE);
			 }
			 else {
				 //switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new cur not null\n");
			 send_dispatcher_state_event(dispat_node->name,dispat_node->uuid,dispat_node->state,user_node->current->state);
			 }
		 }
	 }
	 
	 switch_mutex_unlock(user_node->mutex);
	 

}

  /*
   *assert(dispat != NULL);
   * 
   *if cur == dispat then do nothing 
   *if cur != dispat  then cur = dispatt
   *
   *if(send_event)
   */
static void user_current_shift(user_node_t *user_node,dispat_node_t *dispat_node,int send_event){
	 dispat_node_t *old_node=NULL;

	 if(user_node == NULL || dispat_node == NULL){
		 return;
	 }

	 switch_mutex_lock(user_node->mutex);	 
	 old_node = user_node->current;

	 user_node->current = dispat_node;

	
	 if(send_event && (check_dispatcher_by_id(dispat_node->name) == SWITCH_STATUS_SUCCESS)){ 
		 if(old_node == NULL){
	
			 send_dispatcher_state_event(dispat_node->name,dispat_node->uuid,DISPAT_NONE,dispat_node->state);
	
		 }else{
		 
   			 send_dispatcher_state_event(dispat_node->name,dispat_node->uuid,old_node->state,dispat_node->state);
		 

		 }
	 }
	 
	 switch_mutex_unlock(user_node->mutex);
	 

}

static switch_status_t user_handle_conference_event(user_node_t *user_node,switch_event_t *event) 
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char *uuid = NULL;
	char *channel_name = NULL;

	char *action  = NULL;
	dispat_node_t *dispat_node=NULL;
	int dispat_rdlock = 0;
	int dispat_destroy = 0;

	switch_assert(user_node != NULL);
	switch_assert(event != NULL);

	uuid = switch_event_get_header(event,UUID_EVENT_HEADER);
	channel_name = switch_event_get_header(event,NODE_NAME_EVENT_HEADER);

	if(channel_name == NULL || uuid == NULL){
		//print something
		return status;
	}
	dispat_node = dispat_get_by_uuid_or_create(user_node,uuid,channel_name,&dispat_rdlock);
	if(dispat_node == NULL){
		return status;
	}

	action = switch_event_get_header(event,CONFERENCE_ACTION_HEADER);
	if(action == NULL){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s receive null action\n",dispat_node->name);
		
		return status;
	}
    else {
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s receive %s action\n",dispat_node->name,action);
	}



	if(!strcasecmp(action,CONFERENCE_ADD_ACTION)){
		status = dispat_handle_add(dispat_node,event);

		user_current_shift(user_node,dispat_node,1);

	}else if(!strcasecmp(action,CONFERENCE_DEL_ACTION)){

		status = dispat_handle_del(dispat_node,event);

		user_current_none(user_node,dispat_node,1);
	}else if(!strcasecmp(action,CONFERENCE_START_TALKING_ACTION)){
		if(dispat_node != user_node->current){
			user_current_shift(user_node,dispat_node,1);
		}
	}else if(!strcasecmp(action,CONFERENCE_STOP_TALKING_ACTION)){
		
		if(dispat_node == user_node->current){
			//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s deal stop talking action\n",dispat_node->name);
			dispat_node->session = switch_core_session_locate(uuid);
			
			if(dispat_node->session != NULL){
				switch_channel_t * channel = switch_core_session_get_channel(dispat_node->session);
				//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s session not null\n",dispat_node->name);
						if(channel != NULL){ 
							//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s channel not null\n",dispat_node->name);
							if(switch_channel_test_flag(channel,CF_HOLD)){
								//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"%s channel hold\n",dispat_node->name);
							user_current_none(user_node,dispat_node,1);
							}
						}
				switch_core_session_rwunlock(dispat_node->session);
			}
		}
	}
		

	if(dispat_node->state == DISPAT_END){
		dispat_destroy++;
	}


	if(dispat_rdlock){
		switch_thread_rwlock_unlock(dispat_node->rwlock);
	}
	if(dispat_destroy){
		switch_mutex_lock(user_node->mutex);
		//remove from hahs
		switch_core_hash_delete(user_node->dispat_hash,dispat_node->uuid);

		user_node->dispat_num--;
		//dispat_node_free
		switch_mutex_unlock(user_node->mutex);

		dispat_node_free(dispat_node);
	}
	return status; 
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
			p_end = (char *)switch_stristr("@",name_dup);
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
		
			//if(!switch_strlen_zero(gateway_name)){
			//	user_id = switch_mprintf("%s-%s",gateway_name,p_start);
			//}
			//else{
				user_id = strdup(p_start);
				//}
		
		}
		switch_safe_free(name_dup);
		
	}
	return user_id;
}


static void conference_event_handler(switch_event_t *event)
{
	char *uuid = NULL;
	/*switch_core_session_t *session = NULL;*/
	char *channel_name = NULL;
	char *id = NULL;
	char *node_name = NULL;
	char *gateway_name = NULL;

	user_node_t *user_node = NULL;
	//int user_conference_num = 0;
	int user_rdlock = 0;
	int user_destroy = 0;

	switch_assert(event != NULL);

	/*get caller_id_number*/
	channel_name = switch_event_get_header(event,NODE_NAME_EVENT_HEADER);
	uuid = switch_event_get_header(event,UUID_EVENT_HEADER);
	gateway_name = switch_event_get_header(event,GATEWAY_NAME);

	if(channel_name == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"%s is NULL\n",NODE_NAME_VARIABLE);
		return;
	}
	if(uuid == NULL){
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"%s is null\n",UUID_EVENT_HEADER);
		return;
	}
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"%s has event to mod_dispat\n",node_name);
	id = dup_user_id_from_channel_name(channel_name);
	if(id == NULL){
		return;
	}
	if(switch_strlen_zero(gateway_name)){
		node_name = id;
	}else{
		char * conference_name = switch_event_get_header(event,CONFERENCE_NAME_HEADER);
		char * conference_member_id = switch_event_get_header(event,CONFERENCE_MEMBER_ID_HEADER);
		if(switch_strlen_zero(conference_name) ||
		   switch_strlen_zero(conference_member_id)){
			goto DONE;
		}
		node_name = switch_mprintf("%s-%s-%s-%s",id,gateway_name,conference_name,conference_member_id);
	}
	//node_name = dup_user_id_from_channel_name(channel_name);
	user_node = user_get_by_name_or_create(node_name,&user_rdlock);
	if(user_node == NULL){
		goto DONE;
	}

	user_handle_conference_event(user_node,event);
	if(user_node->dispat_num <=0){
		user_destroy++;
	}
	if(user_rdlock){
		switch_thread_rwlock_unlock(user_node->rwlock);
	}
	
	if(user_destroy){
		switch_mutex_lock(globals.user_mutex);

		switch_core_hash_delete(globals.user_hash,user_node->name);

		switch_mutex_unlock(globals.user_mutex);
		
		user_node_free(user_node);		
	}
 DONE:
	
	if(node_name != NULL && node_name != id){
		
		switch_safe_free(node_name);
    }
	
	switch_safe_free(id);
}



/*
 *handle xml reload event
 */
static void xml_event_handler(switch_event_t *event){
         load_conf();
		 load_call_conf();
}
static void dispat_cfg_event_handler(switch_event_t *event){
	load_call_conf();
}
static void channel_event_handler(switch_event_t *event){                                                                           
    const char *bound = switch_event_get_header(event,"Call-Direction");                                                            
    const char *context_extension = switch_event_get_header(event,"variable_context_extension");                                    
                                                                                                                                    
                                                                                                                                    
    if(bound && context_extension 
	   && !strcmp(bound,"inbound") 
	   && !strcmp(context_extension,"hover") 
	   ){                                                                                   
        //switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel event:%s,%s\n",bound,context_extension);                     
        const char *tmp = switch_event_get_header(event,CONFERENCE_ACTION_HEADER);
		char *tmpDup=NULL;
		if(tmp){
			tmpDup = strdup(tmp);
			switch_event_del_header(event,CONFERENCE_ACTION_HEADER);
		}
		switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,CONFERENCE_ACTION_HEADER,CONFERENCE_DEL_ACTION);
		conference_event_handler(event);                          
		switch_event_del_header(event,CONFERENCE_ACTION_HEADER);
        if(tmpDup){
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,CONFERENCE_ACTION_HEADER,tmpDup);
			switch_safe_free(tmpDup);
		}                                                                                                                   
    }                                                                                                                               
                                                                                                                                    
                                                                                                                                    
                                                                                                                                    
                                                                                                                                    
}
//static void pres_event_handler(switch_event_t *event)
//{
	/*char *to =switch_event_get_header(event,"dispat");
	char *node_name = NULL;

	if(!globals.running){
		return;
	}
	
	node_name= strdup(to);
	switch_assert(node_name != NULL );
	


	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"presence request for %s",node_name);


	switch_safe_free(node_name);*/
//}




/*
 *cmd from keyboard
 */
static switch_status_t dispat_hover_bridge_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;
	
	switch_caller_extension_t *extension = NULL;
    	char * conference_app_data = NULL;

	char * user_num = NULL;
	char *user_dial_str = NULL;
	char *user_dial_str_exp=NULL;
	
    	char * conference_name = NULL;

	if(argc < 1 || argv[0] == NULL 
		|| node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_num = argv[0];

      	if(strlen(user_num) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

      channel = switch_core_session_get_channel(session);

      if(channel == NULL || !switch_channel_ready(channel)){
      		if(stream != NULL){
				stream->write_function(stream,"err,channel null!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,channel null!\n");
		}
      		
		return SWITCH_STATUS_FALSE;
      	}

	/*maybe get_user_id_from_channel_name(node->name)*/
	//char * caller_id = dup_user_id_from_channel_name(node->name);
	conference_name = switch_mprintf("%s[dispatcall]-%s",node->name,user_num);
	//if(caller_id  != NULL && caller_id != node->name){
	//		switch_safe_free(caller_id);
	//}

	if(conference_name == NULL){
		return SWITCH_STATUS_FALSE;
	}
	
	user_dial_str = switch_mprintf("user/%s@${domain_name}",user_num);
	if(user_dial_str){
			user_dial_str_exp = switch_channel_expand_variables(channel,user_dial_str);
	}
	if(user_dial_str_exp == NULL){
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}
	
	if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

			/* app*/
			
			/*switch_channel_set_variable(channel, "conference_member_flags", "endconf");*/
			switch_safe_free(conference_app_data);
			conference_app_data = switch_mprintf("bridge:%s@default:%s+flags{endconf}",conference_name,user_dial_str_exp);
			
			if(conference_app_data == NULL) {
				status = SWITCH_STATUS_MEMERR;
				switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
				goto done;
			}

			switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);
		    switch_caller_extension_add_application(session,extension,RECOVER_APP_COMMAND,RECOVER_APP_COMMAND);

			switch_channel_set_caller_extension(channel,extension);
			switch_channel_set_flag(channel,CF_RESET);
			
			//node->state = DISPAT_BRIDGE;
#if GUJUN_CHANGE_GROUP
			dispat_set_state(node,DISPAT_GROUP);
#else
			dispat_set_state(node,DISPAT_BRIDGE);
#endif
			switch_safe_free(conference_app_data);
			
	}else {

			
			if(stream != NULL){
				stream->write_function(stream,"err,new caller_extension fail!\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
			}
			status = SWITCH_STATUS_MEMERR;
	}	

		
		
done:
	if(user_dial_str_exp && user_dial_str_exp != user_dial_str){
		switch_safe_free(user_dial_str_exp);
	}
	switch_safe_free(user_dial_str);
	switch_safe_free(conference_name);
	return status;
}
#if GUJUN_CHANGE_ANSWER
static int query_callback(void *pArg, int argc, char **argv, char **columnNames){
	char ** uuid = (char **)pArg;
	int idx = -1;
	
	if(uuid == NULL){
		return 0;
	}
	for (int i = 0; i < argc; i++) {
		char *name = columnNames[i];
		if (!name) {
			continue;
		}
		if(!strcasecmp(name,"uuid")){
			idx = i;
			break;
		}
	}
	if(idx < argc){
		*uuid = strdup(argv[idx]);
	}

	return 0;
}

static switch_status_t get_uuid_from_user_input(char **uuid_output,const char *input,switch_call_direction_t direction){
	char sql[1024];
	char *errmsg = NULL;
	switch_cache_db_handle_t *db = NULL;
	char * uuid=NULL;
	
	if(switch_strlen_zero(input)){
		return SWITCH_STATUS_FALSE;
	}
	if (switch_core_db_handle(&db) != SWITCH_STATUS_SUCCESS) {
		
		return SWITCH_STATUS_FALSE;
	}
	
	//select uuid from channels where uuid like '%s' and direction like 'outbound'
	if(direction == SWITCH_CALL_DIRECTION_OUTBOUND){
		switch_snprintf(sql, sizeof(sql), "select uuid from channels where uuid like '%s' and direction like 'outbound'",
					input);
	}
	else{
		switch_snprintf(sql, sizeof(sql), "select uuid from channels where uuid like '%s' and direction like 'inbound'",
					input);
	}
	switch_cache_db_execute_sql_callback(db, sql, query_callback, &uuid, &errmsg);

	if(errmsg){
		switch_core_db_free(errmsg);
		errmsg = NULL;
	}
	if(uuid != NULL){
		goto done;
	}
	//select uuid from channels where dest like '%s' and direction like 'outbound'
	if(direction == SWITCH_CALL_DIRECTION_OUTBOUND){
		switch_snprintf(sql, sizeof(sql), "select uuid from channels where dest like '%s' and presence_id like '%%%s@%%' and direction like 'outbound'",
						input,input);
	}
	else{
		switch_snprintf(sql, sizeof(sql), "select uuid from channels where cid_num like '%s' and presence_id like '%%%s@%%' and direction like 'inbound'",
						input,input);
	}
	switch_cache_db_execute_sql_callback(db, sql, query_callback, &uuid, &errmsg);

	if(errmsg){
		switch_core_db_free(errmsg);
		errmsg = NULL;
	}
 done:
	if (db) {
		switch_cache_db_release_db_handle(&db);
	}
	if(uuid != NULL){
		*uuid_output = uuid;
	}
	return SWITCH_STATUS_SUCCESS;
}
static switch_status_t dispat_hover_answer_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;
	switch_event_t *event=NULL;	
	switch_caller_extension_t *extension = NULL;
	switch_caller_extension_t *user_extension = NULL;
    char * conference_app_data = NULL;

	char * user_num = NULL;
	char * user_uuid = NULL;
	switch_core_session_t * user_session = NULL;
	switch_channel_t *user_channel=NULL;
	
	char *user_dial_str = NULL;
	char *user_dial_str_exp=NULL;
	
    char * conference_name = NULL;
    //switch_call_direction_t direction;
		
	switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"ok,enter answer!\n");
	  


	if(argc < 1 || argv[0] == NULL 
	   || node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
			stream->write_function(stream,"err,input fails!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,input fails!\n");
		}
		
		return SWITCH_STATUS_SUCCESS;
	}
	user_num = argv[0];

	if(strlen(user_num) == 0){
		if(stream != NULL){
			stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,strlen(user_name) is 0!\n");
		}
		return SWITCH_STATUS_SUCCESS;
	}
	
	
	channel = switch_core_session_get_channel(session);

	if(channel == NULL || !switch_channel_ready(channel)){
		if(stream != NULL){
			stream->write_function(stream,"err,channel null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,channel null!\n");
		}
		
		return SWITCH_STATUS_SUCCESS;
	}

	/*maybe get_user_id_from_channel_name(node->name)*/
	//char * caller_id = dup_user_id_from_channel_name(node->name);
	conference_name = switch_mprintf("%s[dispatanswer]-%s",node->name,user_num);
	//if(caller_id  != NULL && caller_id != node->name){
	//switch_safe_free(caller_id);
	//}

	if(conference_name == NULL){
		return SWITCH_STATUS_SUCCESS;
	}
	
	user_dial_str = switch_mprintf("user/%s@${domain_name}",user_num);
	if(user_dial_str){
		user_dial_str_exp = switch_channel_expand_variables(channel,user_dial_str);
	}
	if(user_dial_str_exp == NULL){
		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}
	
	if(get_uuid_from_user_input(&user_uuid,user_num,SWITCH_CALL_DIRECTION_INBOUND) != SWITCH_STATUS_SUCCESS || user_uuid == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,uuid  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,uuid is null!\n");
		}
		status =  SWITCH_STATUS_SUCCESS;
		goto done;
	}
	user_session = switch_core_session_locate(user_uuid);
	if(user_session == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,user_session  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_session is null!\n");
		}
		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}
	user_channel = switch_core_session_get_channel(user_session);
	if(user_channel == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel is null!\n");
		}
		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}
	if(switch_channel_direction(user_channel) != SWITCH_CALL_DIRECTION_INBOUND){
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  is not inbound!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel is not inbound!\n");
		}
		status = SWITCH_STATUS_SUCCESS;
		goto done;
		
	}

	if (switch_channel_test_flag(user_channel, CF_ANSWERED)) {
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  is answered!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel is answered!\n");
		}

		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}
	if(!switch_channel_ready(user_channel)){
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  is not ready!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel is not ready!\n");
		}

		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}


	conference_app_data = switch_mprintf("%s@default",conference_name);
	
	if(conference_app_data == NULL) {
		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}
	if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} min 2");
		if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			
			switch_event_destroy(&event);
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  event err!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel  event err!\n");
		}

			status = SWITCH_STATUS_SUCCESS;
		}
	} else {

		status = SWITCH_STATUS_SUCCESS;
	}
	   	
	if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

		
		switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);

		switch_channel_set_caller_extension(channel,extension);
		switch_channel_set_flag(channel,CF_RESET);
		
		//node->state = DISPAT_BRIDGE;
		
		//switch_safe_free(conference_app_data);
		
	}else {

		
		if(stream != NULL){
			stream->write_function(stream,"err,new extension fail!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new extension fail!\n");
		}
		status = SWITCH_STATUS_SUCCESS;
		goto done;
	}

	if((user_extension = switch_caller_extension_new(user_session,conference_name,conference_name)) != 0) {

		switch_channel_set_variable(user_channel,DISPAT_NODE_FORWARDING_VAR,conference_name);
		switch_caller_extension_add_application(user_session,user_extension,RECOVER_APP_COMMAND,"");

		switch_channel_set_caller_extension(user_channel,user_extension);
		switch_channel_set_flag(user_channel,CF_RESET);

		switch_channel_set_variable(user_channel,CLEAR_FLAG_NOT_READY_VAR,"true");
		switch_channel_set_flag(user_channel,CF_NOT_READY);
	
	
	}else {

		
		if(stream != NULL){
			stream->write_function(stream,"err,new caller_extension fail!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
		}
		status = SWITCH_STATUS_SUCCESS;
		switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
		goto done;
	}
#if GUJUN_CHANGE_GROUP
	dispat_set_state(node,DISPAT_GROUP);
#else	
	dispat_set_state(node,DISPAT_BRIDGE);
#endif
 done:
	
	if(user_dial_str_exp && user_dial_str_exp != user_dial_str){
		switch_safe_free(user_dial_str_exp);
	}
	if(user_uuid && user_uuid != user_num){
		switch_safe_free(user_uuid);
	}
	if(user_session){
		switch_core_session_rwunlock(user_session);
	}
	
	switch_safe_free(conference_app_data);
	switch_safe_free(user_dial_str);
	switch_safe_free(conference_name);
	return status;
}
static switch_status_t dispat_hover_replace_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;
	switch_event_t *event=NULL;	
	switch_caller_extension_t *extension = NULL;
	switch_caller_extension_t *caller_extension = NULL;
    char * conference_app_data = NULL;

	char * user_num = NULL;
	char * user_uuid = NULL;
	switch_core_session_t * user_session = NULL;
	switch_channel_t *user_channel=NULL;
	
	const char * caller_uuid = NULL;
	switch_core_session_t * caller_session = NULL;
	switch_channel_t *caller_channel=NULL;
	
	char *user_dial_str = NULL;
	char *user_dial_str_exp=NULL;
	
    char * conference_name = NULL;

	if(argc < 1 || argv[0] == NULL 
	   || node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
			stream->write_function(stream,"err,input fails!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,input fails!\n");
		}
		
		return SWITCH_STATUS_FALSE;
	}
	user_num = argv[0];

	if(strlen(user_num) == 0){
		if(stream != NULL){
			stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,strlen(user_name) is 0!\n");
		}
		return SWITCH_STATUS_FALSE;
	}
	
	
	channel = switch_core_session_get_channel(session);

	if(channel == NULL || !switch_channel_ready(channel)){
		if(stream != NULL){
			stream->write_function(stream,"err,channel null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,channel null!\n");
		}
		
		return SWITCH_STATUS_FALSE;
	}

	/*maybe get_user_id_from_channel_name(node->name)*/
	//char * caller_id = dup_user_id_from_channel_name(node->name);
	conference_name = switch_mprintf("%s[dispatreplace]-%s",node->name,user_num);
	//if(caller_id  != NULL && caller_id != node->name){
	//switch_safe_free(caller_id);
	//}

	if(conference_name == NULL){
		return SWITCH_STATUS_FALSE;
	}
	
	user_dial_str = switch_mprintf("user/%s@${domain_name}",user_num);
	if(user_dial_str){
		user_dial_str_exp = switch_channel_expand_variables(channel,user_dial_str);
	}
	if(user_dial_str_exp == NULL){
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}
	
	if(get_uuid_from_user_input(&user_uuid,user_num,SWITCH_CALL_DIRECTION_OUTBOUND) != SWITCH_STATUS_SUCCESS || user_uuid == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,uuid  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,uuid is null!\n");
		}
		status =  SWITCH_STATUS_FALSE;
		goto done;
	}
	user_session = switch_core_session_locate(user_uuid);
	if(user_session == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,user_session  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_session is null!\n");
		}
		status = SWITCH_STATUS_FALSE;
		goto done;
	}
	user_channel = switch_core_session_get_channel(user_session);
	if(user_channel == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel is null!\n");
		}
		status = SWITCH_STATUS_FALSE;
		goto done;
	}
	if(switch_channel_direction(user_channel) != SWITCH_CALL_DIRECTION_OUTBOUND){
		if(stream != NULL){
			stream->write_function(stream,"err,user_channel  is not outbound!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,user_channel is not outbound!\n");
		}
		status = SWITCH_STATUS_FALSE;
		goto done;
		
	}

	caller_uuid = switch_channel_get_variable(user_channel, SWITCH_SIGNAL_BOND_VARIABLE);
	if(caller_uuid == NULL){
			if(stream != NULL){
				stream->write_function(stream,"err,caller uuid  is null!\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,caller uuid is null!\n");
			}
			status = SWITCH_STATUS_FALSE;
			goto done;
		}
	caller_session = switch_core_session_locate(caller_uuid);
	if(caller_session == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,caller_session  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,caller_session is null!\n");
		}
		status = SWITCH_STATUS_FALSE;
		goto done;
	}
	
	caller_channel = switch_core_session_get_channel(caller_session);
	if(caller_channel == NULL){
		if(stream != NULL){
			stream->write_function(stream,"err,caller_channel  is null!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,caller_channel is null!\n");
		}
		status = SWITCH_STATUS_FALSE;
		goto done;
	}

	if (switch_channel_test_flag(caller_channel, CF_ANSWERED)) {
		status = SWITCH_STATUS_FALSE;
		goto done;
	}
	if(!switch_channel_ready(caller_channel) || !switch_channel_ready(user_channel)){
		status = SWITCH_STATUS_FALSE;
		goto done;
	}


	conference_app_data = switch_mprintf("%s@default",conference_name);
	
	if(conference_app_data == NULL) {
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}
	
	if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} min 2");
		if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
	
			switch_event_destroy(&event);
			status = SWITCH_STATUS_FALSE;
		}
	} else {

		status = SWITCH_STATUS_MEMERR;
	}

	if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

		
		switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);

		switch_channel_set_caller_extension(channel,extension);
		switch_channel_set_flag(channel,CF_RESET);
		
		//node->state = DISPAT_BRIDGE;
		
		//switch_safe_free(conference_app_data);
		
	}else {

		
		if(stream != NULL){
			stream->write_function(stream,"err,new extension fail!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new extension fail!\n");
		}
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}

	if((caller_extension = switch_caller_extension_new(caller_session,conference_name,conference_name)) != 0) {

		switch_channel_set_variable(caller_channel,DISPAT_NODE_FORWARDING_VAR,conference_name);
		switch_caller_extension_add_application(caller_session,caller_extension,RECOVER_APP_COMMAND,"");

		switch_channel_set_caller_extension(caller_channel,caller_extension);
		switch_channel_set_flag(caller_channel,CF_RESET);

		switch_channel_set_variable(caller_channel,CLEAR_FLAG_NOT_READY_VAR,"true");
		switch_channel_set_flag(caller_channel,CF_NOT_READY);

				
	}else {

		
		if(stream != NULL){
			stream->write_function(stream,"err,new caller_extension fail!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
		}
		status = SWITCH_STATUS_MEMERR;
		switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
		goto done;
	}
	
	switch_channel_hangup(user_channel,SWITCH_CAUSE_NORMAL_CLEARING);
#if GUJUN_CHANGE_GROUP
	dispat_set_state(node,DISPAT_GROUP);
#else
	dispat_set_state(node,DISPAT_BRIDGE);
#endif
 done:
	
	if(user_dial_str_exp && user_dial_str_exp != user_dial_str){
		switch_safe_free(user_dial_str_exp);
	}
	if(user_uuid && user_uuid != user_num){
		switch_safe_free(user_uuid);
	}
	if(user_session){
		switch_core_session_rwunlock(user_session);
	}

	
	if(caller_session){
		switch_core_session_rwunlock(caller_session);
	}
	
	switch_safe_free(conference_app_data);
	switch_safe_free(user_dial_str);
	switch_safe_free(conference_name);
	return status;
}

#endif
#if GUJUN_CHANGE_DIRECT
static switch_status_t dispat_hover_direct_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	 switch_status_t status = SWITCH_STATUS_SUCCESS;

	 char *data = NULL;

	 char * user_name = NULL;
	 dispat_node_t * user_node=NULL;
	 char * conference_name = NULL;
	 char * conference_member_id = NULL;   
	 int user_rdlock = 0;

	 char * new_conference_name = NULL;

	 switch_core_session_t * session = NULL;       
	 switch_channel_t *channel = NULL;

	 switch_event_t *event=NULL;

	 switch_caller_extension_t *extension = NULL;


	 if(argc < 1 || argv[0] == NULL 
		|| node == NULL  || (session = node->session) == NULL ){

		 if(stream != NULL){
			 stream->write_function(stream,"err,input fails!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		 }
		 
		 return SWITCH_STATUS_FALSE;
	 }

	 user_name = argv[0];

	 if(strlen(user_name) == 0){
		 if(stream != NULL){
			 stream->write_function(stream,"err,strlen(user_num) is 0!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_num) is 0!\n");
		 }
		 return SWITCH_STATUS_FALSE;
	 }

	 user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
	 if(user_node == NULL){
		 if(stream != NULL){
			 stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		 }
		 return SWITCH_STATUS_FALSE;
	 }
	 else{

		 //conference_name = user_get_conference_name(user_node,NULL);
		 //conference_member_id = user_get_member_id(user_node,NULL);
		 conference_name = user_node->conference_name;
		 conference_member_id = user_node->conference_member_id;

	 }



	 if(conference_name == NULL || strlen(conference_name) == 0 || conference_member_id == NULL){
		 if(stream != NULL){
			 stream->write_function(stream,"err,dialogs of %s is NULL!\n",user_name);
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"dialogs of %s is NULL!\n",user_name);
		 }
		 status =  SWITCH_STATUS_FALSE;
		 goto done;
	 }

	 channel = switch_core_session_get_channel(session);

	 if(channel == NULL || !switch_channel_ready(channel)){
		 if(stream != NULL){
			 stream->write_function(stream,"err,channel null!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"err,channel null!\n");
		 }
		 
		 status =  SWITCH_STATUS_FALSE;
		 goto done;
	 }

	 new_conference_name = switch_mprintf("%s[dispatdirect]-%s",node->name,user_name);
	 if(new_conference_name == NULL){
		 status =  SWITCH_STATUS_FALSE;
		 goto done;
	 }
	 	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} back  %s",conference_name);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} min 2");
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "%s nomin",conference_name);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }


	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "%s transfer ${conference_name} %s",conference_name,conference_member_id);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "%s min 2",conference_name);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }


	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} endconf");
		 if(dispat_queue_del_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }

	 switch_safe_free(data);


	 if((extension = switch_caller_extension_new(node->session,new_conference_name,new_conference_name)) != 0 &&
        (data = switch_mprintf("%s",new_conference_name)) != NULL) {

		 switch_caller_extension_add_application(node->session,extension,CONFERENCE_APP_COMMAND,data);

		 switch_channel_set_caller_extension(channel,extension);
		 switch_channel_set_flag(channel,CF_RESET);

		 //node->state = DISPAT_GRAB;
		 dispat_set_state(node,DISPAT_GROUP);

	 }else {

		 dispat_clear_all_add_event(node);
		 dispat_clear_all_del_event(node);

		 if(stream != NULL){
			 stream->write_function(stream,"err,new caller_extension fail!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
		 }
		 status = SWITCH_STATUS_MEMERR;
	 }
            
	
 done:
		 if(user_node && user_rdlock){
			 user_rdlock = 0;
			 switch_thread_rwlock_unlock(user_node->rwlock);
		 }
	 
	 switch_safe_free(new_conference_name); 
	switch_safe_free(data);
	 	
	return status;
}

#endif

static switch_status_t dispat_hover_hear_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;

	/*switch_event_t *event=NULL;*/
	
	switch_caller_extension_t *extension = NULL;
    	char * conference_app_data = NULL;

	char * user_name = NULL;
    	dispat_node_t * user_node=NULL;
    	char * conference_name = NULL;
    	int user_rdlock = 0;

	/*int data_len = 0;*/

	if(argc < 1 || argv[0] == NULL 
		|| node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

		user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
	if(user_node == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}
	else{

		conference_name = user_node->conference_name;
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}

	if(switch_strlen_zero(conference_name)){
		if(stream != NULL){
				stream->write_function(stream,"err,dialogs of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"dialogs of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}

      channel = switch_core_session_get_channel(session);
      
	if(channel && switch_channel_ready(channel)){
		
		if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

			conference_app_data = switch_mprintf("%s+flags{mute}",conference_name);
			
			if(conference_app_data == NULL) {
				status = SWITCH_STATUS_MEMERR;
				goto done;
			}

			switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);
			
			switch_channel_set_caller_extension(channel,extension);
			switch_channel_set_flag(channel,CF_RESET);

			//node->state = DISPAT_HEAR;
#if GUJUN_CHANGE_GROUP
			dispat_set_state(node,DISPAT_GROUP);
#else
			dispat_set_state(node,DISPAT_HEAR);
#endif		
		}else {

			
			if(stream != NULL){
				stream->write_function(stream,"err,new caller_extension fail!\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
			}
			status = SWITCH_STATUS_MEMERR;
		}	
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel null!\n");
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(conference_app_data);
done:
	
	return status;
	
}

static switch_status_t dispat_hover_insert_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;

	/*switch_event_t *event=NULL;*/
	
	switch_caller_extension_t *extension = NULL;
    	char * conference_app_data = NULL;

	char * user_name = NULL;
    	dispat_node_t * user_node=NULL;
    	char * conference_name = NULL;
    	int user_rdlock = 0;

		switch_event_t *event=NULL;
	/*int data_len = 0;*/

	if(argc < 1 || argv[0] == NULL 
		|| node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

		user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
	if(user_node == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}
	else{

		//conference_name = user_get_conference_name(user_node,NULL);
		conference_name = user_node->conference_name;
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}

	if(switch_strlen_zero(conference_name)){
		if(stream != NULL){
				stream->write_function(stream,"err,dialogs of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"dialogs of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}

      channel = switch_core_session_get_channel(session);
      
	if(channel && switch_channel_ready(channel)){
			if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
				 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
				 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} min 2");
				 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
					 dispat_clear_all_add_event(node);
					 switch_event_destroy(&event);
					 status = SWITCH_STATUS_FALSE;
				 }
			 } else {

				 status = SWITCH_STATUS_MEMERR;
			 }

		if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

			conference_app_data = switch_mprintf("%s",conference_name);
			
			if(conference_app_data == NULL) {
				status = SWITCH_STATUS_MEMERR;
				goto done;
			}

			switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);
			
			switch_channel_set_caller_extension(channel,extension);
			switch_channel_set_flag(channel,CF_RESET);

			//node->state = DISPAT_HEAR;
#if GUJUN_CHANGE_GROUP
			dispat_set_state(node,DISPAT_GROUP);
#else

			dispat_set_state(node,DISPAT_INSERT);
#endif			
		}else {

			
			if(stream != NULL){
				stream->write_function(stream,"err,new caller_extension fail!\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
			}
			status = SWITCH_STATUS_MEMERR;
		}	
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel null!\n");
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(conference_app_data);
done:
	
	return status;
	
}

static switch_status_t dispat_hover_grab_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;

	switch_event_t *event=NULL;
	
	switch_caller_extension_t *extension = NULL;
    	char * conference_app_data = NULL;

	char * user_name = NULL;
    	dispat_node_t * user_node=NULL;
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

   
    	int user_rdlock = 0;

	/*int data_len = 0;*/

	if(argc < 1 || argv[0] == NULL 
		|| node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}
		user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
	if(user_node == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}
	else{

		//conference_name = user_get_conference_name(user_node,NULL);
		//conference_member_id = user_get_member_id(user_node,NULL);
		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_member_id;

		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}

	if(conference_name == NULL || strlen(conference_name) == 0 || conference_member_id == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,dialogs of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"dialogs of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}

      channel = switch_core_session_get_channel(session);
      
	if(channel && switch_channel_ready(channel)){
		
		/*prepare private_event for node*/
		if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
			switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "%s kick %s",conference_name,conference_member_id);
#if 0 //GUJUN_CHANGE_FORWARDING
			if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
#else
				if(dispat_queue_private_event(node, &event) != SWITCH_STATUS_SUCCESS){
#endif
				
				switch_event_destroy(&event);
				status = SWITCH_STATUS_FALSE;
				goto done;
			}
		} else {
			status = SWITCH_STATUS_MEMERR;
			goto done;
		}

		if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} unmute ${conference_member_id}");
#if 0 // GUJUN_CHANGE_FORWARDING
			if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
				dispat_clear_all_add_event(node);
#else
				if(dispat_queue_private_event(node, &event) != SWITCH_STATUS_SUCCESS){
					dispat_clear_all_private_event(node);
#endif

				switch_event_destroy(&event);
				status = SWITCH_STATUS_FALSE;
				goto done;
			}
		} else {

			status = SWITCH_STATUS_MEMERR;
			goto done;
		}	

		
		/*prepare app for session*/
		if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

			conference_app_data = switch_mprintf("%s+flags{mute}",conference_name);
			
			if(conference_app_data == NULL) {
				status = SWITCH_STATUS_MEMERR;
				goto done;
			}

			switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);
			
			switch_channel_set_caller_extension(channel,extension);
			switch_channel_set_flag(channel,CF_RESET);

			//node->state = DISPAT_GRAB;
#if GUJUN_CHANGE_GROUP
			dispat_set_state(node,DISPAT_GROUP);
#else
			dispat_set_state(node,DISPAT_GRAB);
#endif			
		}else {
			
			dispat_clear_all_private_event(node);
			
			
			if(stream != NULL){
				stream->write_function(stream,"err,new caller_extension fail!\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
			}
			status = SWITCH_STATUS_MEMERR;
		}	
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel null!\n");
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(conference_app_data);
done:
	return status;
}

static switch_status_t dispat_hover_unbridge_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	/*switch_event_t *event=NULL;*/
	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
    dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;
    	
    	int user_rdlock = 0;
	
	/*int data_len = 0;*/

	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}
	else{

		//		conference_name = user_get_conference_name(user_node,NULL);
		//conference_member_id = user_get_member_id(user_node,NULL);
		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_member_id;
		
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}


	SWITCH_STANDARD_STREAM(mystream);
	/*kick user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s kick all",conference_name);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
	       /*not change dispat state*/
		
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;
}

static switch_status_t dispat_hover_group_call_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	switch_core_session_t * session = NULL;
	switch_channel_t *channel=NULL;

	/*switch_event_t *event=NULL;*/
	
	switch_caller_extension_t *extension = NULL;
    	char * conference_app_data = NULL;

	char * group_name = NULL;
    	char * conference_name = NULL;

	if(argc < 1 || argv[0] == NULL 
		|| node == NULL  || (session = node->session) == NULL ){

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       group_name = argv[0];

      	if(strlen(group_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

	/*maybe get_user_id_from_channel_name(node->name)*/
	
	conference_name = switch_mprintf("%s[dispatgroupcall]-%s",node->name,group_name);
	

	if(conference_name == NULL){
		return SWITCH_STATUS_FALSE;
	}

      channel = switch_core_session_get_channel(session);
      
	if(channel && switch_channel_ready(channel)){
		

		if((extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {
			

			/*1 app*/
			switch_caller_extension_add_application(session,extension,SET_APP_COMMAND,"conference_auto_outcall_flags=mute");

			/*2 app*/
			conference_app_data = switch_mprintf("${group_call(%s)}",group_name);
			
			if(conference_app_data == NULL) {
				status = SWITCH_STATUS_MEMERR;
				switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
				goto done;
			}

			switch_caller_extension_add_application(session,extension,CONFERENCE_SET_AUTO_OUTCALL,conference_app_data);

			/*3 app*/
			switch_safe_free(conference_app_data);
			conference_app_data = switch_mprintf("%s@default+flags{endconf}",conference_name);
			
			if(conference_app_data == NULL) {
				status = SWITCH_STATUS_MEMERR;
				switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
				goto done;
			}

			switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);

			switch_channel_set_caller_extension(channel,extension);
			switch_channel_set_flag(channel,CF_RESET);


			//node->state = DISPAT_GROUP;
			dispat_set_state(node,DISPAT_GROUP);
			
		}else {

			
			if(stream != NULL){
				stream->write_function(stream,"err,new caller_extension fail!\n");
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
			}
			status = SWITCH_STATUS_MEMERR;
		}	
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"channel null!\n");
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(conference_app_data);
	
done:
	switch_safe_free(conference_name);
	return status;
}
/*
static switch_status_t dispat_hover_add_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_hover_bridge_function(stream,node,argv,argc);
	return status;
}

static switch_status_t dispat_hover_kick_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	return status;
}

static switch_status_t dispat_hover_mute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	return status;
}
static switch_status_t dispat_hover_unmute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	return status;
}
static switch_status_t dispat_hover_moderator_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	return status;
}
static switch_status_t dispat_hover_unmoderator_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	
	return status;
}
*/
static dispat_api_t dispat_hover_api_s[]={
	{"call",(dispat_api_function_t)&dispat_hover_bridge_function,"dispatcher_number <call> user_number"},
#if GUJUN_CHANGE_ANSWER
	{"answer",(dispat_api_function_t)&dispat_hover_answer_function,"dispatcher_number <answer> user_uuid|user_number"},
	{"replace",(dispat_api_function_t)&dispat_hover_replace_function,"dispatcher_number <replace> user_uuid|user_number"},
#endif
#if GUJUN_CHANGE_DIRECT
	{"direct",(dispat_api_function_t)&dispat_hover_direct_function,"dispatcher_number <direct> user_uuid|user_number"},
#endif
	{"hear",(dispat_api_function_t)&dispat_hover_hear_function,"dispatcher_number <hear> user_number"},
	{"insert",(dispat_api_function_t)&dispat_hover_insert_function,"dispatcher_number <hear> user_number"},
	{"grab",(dispat_api_function_t)&dispat_hover_grab_function,"dispatcher_number <grab> user_number"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
	{"group_call",(dispat_api_function_t)&dispat_hover_group_call_function,"dispatcher_number <group_call> group_name"},
/*
	{"add",(dispat_api_function_t)&dispat_hover_add_member_function,"dispatcher_number <add> user_number"},
	{"kick",(dispat_api_function_t)&dispat_hover_kick_member_function,"dispatcher_number <kick> user_number"},
	{"mute",(dispat_api_function_t)&dispat_hover_mute_member_function,"dispatcher_number <mute> user_number"},
	{"unmute",(dispat_api_function_t)&dispat_hover_unmute_member_function,"dispatcher_number <unmute> user_number"},
	{"moderator",(dispat_api_function_t)&dispat_hover_moderator_member_function,"dispatcher_number <moderator> user_number"},
	{"unmoderator",(dispat_api_function_t)&dispat_hover_unmoderator_function,"dispatcher_number <unmoderator>"},
*/
	{NULL,NULL,NULL}
};
#if GUJUN_CHANGE_HOLD
#define HOVER_APP_COMMAND "hover"
#define HOVER_APP_DATA "local_stream://moh"
static switch_status_t dispat_group_hold_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	 switch_status_t status = SWITCH_STATUS_SUCCESS;

	 char *data = NULL;
	 switch_stream_handle_t mystream = { 0 };
	 
	 
	 char * dispat_conference_name = NULL;
       
	 switch_channel_t *channel = NULL;

	 switch_caller_extension_t *extension = NULL;


	 //dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
	 

	 if(switch_strlen_zero(node->conference_name) ||
		switch_strlen_zero(node->uuid) ||
		switch_strlen_zero(node->conference_member_id) ||
		node->session == NULL ||
		(channel = switch_core_session_get_channel(node->session)) == NULL ||
		!switch_channel_ready(channel)){
		 return SWITCH_STATUS_FALSE;
	 }
	 dispat_conference_name = node->conference_name;

	 

	 if((extension = switch_caller_extension_new(node->session,dispat_conference_name,dispat_conference_name)) != 0) {

		 switch_caller_extension_add_application(node->session,extension,HOVER_APP_COMMAND,HOVER_APP_DATA);
		 //switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,data);

		 switch_channel_set_caller_extension(channel,extension);
		 switch_channel_set_variable(channel,DISPAT_NODE_STATE_RESET_VAR,"true");
		 switch_channel_set_flag(channel,CF_RESET);

		 //node->state = DISPAT_GRAB;
		 //dispat_set_state(node,DISPAT_GRAB);

	 }else {

		 if(stream != NULL){
			 stream->write_function(stream,"err,new caller_extension fail!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
		 }
		 status = SWITCH_STATUS_MEMERR;
		 goto done;
	 }
            
	 
	 SWITCH_STANDARD_STREAM(mystream);
	 /*gbdail user*/
	 //memset(cmdbuf,0,sizeof(cmdbuf));
	 //switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s nomin",dispat_conference_name);
	 switch_safe_free(data);
	 data = switch_mprintf("%s nomin",dispat_conference_name);
	 if((status = switch_api_execute(CONFERENCE_API_COMMAND,data,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		 /*not change dispat state*/
		 
	 } else {
		 status = SWITCH_STATUS_FALSE;
	 }
	        
	 switch_safe_free(data);
	 data = switch_mprintf("%s kick %s",dispat_conference_name,node->conference_member_id);
	 if((status = switch_api_execute(CONFERENCE_API_COMMAND,data,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		 /*not change dispat state*/
		 
	 } else {
		 status = SWITCH_STATUS_FALSE;
	 }
	 switch_safe_free(mystream.data);
	 
 done:	 
	 switch_safe_free(data);
	 
	
	return status;


}
#endif 

#if GUJUN_CHANGE_FORWARDING
 static switch_status_t dispat_group_forwarding_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
 {
	 switch_status_t status = SWITCH_STATUS_SUCCESS;

	 char *data = NULL;
#if 1
	 switch_stream_handle_t mystream = { 0 };
	 switch_caller_extension_t *extension = NULL;
#else

#endif	 
	 char * user_num = NULL;
	 char *user_dial_str = NULL;
	 char *user_dial_str_exp=NULL;
	 
	 
	 char * dispat_conference_name = NULL;
	 char * new_conference_name = NULL;
       
	 switch_channel_t *channel = NULL;
	 const char *var = NULL;
	 char * new_var = NULL;


	 switch_event_t *event=NULL;




	 if(argc < 1 || argv[0] == NULL || node == NULL ) {

		 if(stream != NULL){
			 stream->write_function(stream,"err,input fails!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		 }
		 
		 return SWITCH_STATUS_FALSE;
	 }
	 user_num = argv[0];

	 if(strlen(user_num) == 0){
		 if(stream != NULL){
			 stream->write_function(stream,"err,strlen(user_num) is 0!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_num) is 0!\n");
		 }
		 return SWITCH_STATUS_FALSE;
	 }


	 //dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
	 

	 if(switch_strlen_zero(node->conference_name) ||
		switch_strlen_zero(node->uuid) ||
		switch_strlen_zero(node->conference_member_id) ||
		node->session == NULL ||
		(channel = switch_core_session_get_channel(node->session)) == NULL ||
		!switch_channel_ready(channel)){
		 return SWITCH_STATUS_FALSE;
	 }
	 dispat_conference_name = node->conference_name;

	 
	 /*prepare user dial string*/
	 user_dial_str = switch_mprintf("user/%s@${domain_name}",user_num);
	 if(user_dial_str ){
		 user_dial_str_exp = switch_channel_expand_variables(channel,user_dial_str);
	 }
	 if(user_dial_str_exp == NULL){
		 status =  SWITCH_STATUS_FALSE;
		 goto done;
	 }

	 new_conference_name = switch_mprintf("%s[dispatcall]-%s",dispat_conference_name,user_num);
	 if(new_conference_name == NULL){
		 status =  SWITCH_STATUS_FALSE;
		 goto done;
	 }
	 
	 //DISPAT_NODE_FORWARDING_VAR
	 var = switch_channel_get_variable(channel, DISPAT_NODE_FORWARDING_VAR);
	 if(var != NULL){
		 new_var = switch_mprintf("%s%c%s",dispat_conference_name,VAR_DELIM,var);
	 }else{
		 new_var =  dispat_conference_name;
	 }
	 
	 
	 
	 
	 

	 
	 

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "%s min 2",dispat_conference_name);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {
		 status = SWITCH_STATUS_MEMERR;
	 }

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} back  %s",dispat_conference_name);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} min 2");
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }
#if 1
	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} bgdial  %s",user_dial_str_exp);
		 if(dispat_queue_add_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }
#else  
	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, DISPAT_API_COMMAND);
		 switch_event_add_header(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "%s call %s",node->name,user_num);
		 if(dispat_queue_hold_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 dispat_clear_all_add_event(node);
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }
#endif
	 switch_safe_free(data);
#if 1
	 if((extension = switch_caller_extension_new(node->session,new_conference_name,new_conference_name)) != 0 &&
        (data = switch_mprintf("%s",new_conference_name)) != NULL) {

		 switch_caller_extension_add_application(node->session,extension,CONFERENCE_APP_COMMAND,data);
		 switch_caller_extension_add_application(node->session,extension,RECOVER_APP_COMMAND,RECOVER_APP_COMMAND);

		 switch_channel_set_caller_extension(channel,extension);
		 switch_channel_set_variable(channel,DISPAT_NODE_FORWARDING_VAR,new_var);
		 switch_channel_set_variable(channel,DISPAT_NODE_FORWARDING_CURRENT_VAR,new_conference_name);
		 switch_channel_set_variable(channel,DISPAT_NODE_STATE_RESET_VAR,"true");
		 switch_channel_set_flag(channel,CF_RESET);

		 //node->state = DISPAT_GRAB;
		 //dispat_set_state(node,DISPAT_GRAB);

	 }else {

		 dispat_clear_all_add_event(node);


		 if(stream != NULL){
			 stream->write_function(stream,"err,new caller_extension fail!\n");
		 } else {
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
		 }
		 status = SWITCH_STATUS_MEMERR;
	 }
            
	 
	 SWITCH_STANDARD_STREAM(mystream);
	 /*gbdail user*/
	 //memset(cmdbuf,0,sizeof(cmdbuf));
	 //switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s nomin",dispat_conference_name);
	 switch_safe_free(data);
	 data = switch_mprintf("%s nomin",dispat_conference_name);
	 if((status = switch_api_execute(CONFERENCE_API_COMMAND,data,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		 /*not change dispat state*/
		 
	 } else {
		 status = SWITCH_STATUS_FALSE;
	 }
	        
	 switch_safe_free(data);
	 data = switch_mprintf("%s kick %s",dispat_conference_name,node->conference_member_id);
	 if((status = switch_api_execute(CONFERENCE_API_COMMAND,data,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		 /*not change dispat state*/
		 
	 } else {
		 status = SWITCH_STATUS_FALSE;
	 }
	 switch_safe_free(mystream.data);
#else

	 switch_channel_set_variable(channel,DISPAT_NODE_FORWARDING_VAR,new_var);
	 switch_channel_set_variable(channel,DISPAT_NODE_FORWARDING_CURRENT_VAR,new_conference_name);

	 dispat_group_hold_function(stream,node,argv,argc);

#endif	 
	 //dispat_set_state(node,DISPAT_FORWARD);
	 dispat_set_state(node,DISPAT_GROUP);

 done:
	 	 
	 switch_safe_free(data);
	 
	 if(new_var &&  new_var != dispat_conference_name){
	      switch_safe_free(new_var);
      }
     switch_safe_free(new_conference_name);
 
    if(user_dial_str_exp &&  user_dial_str_exp != user_dial_str){
		switch_safe_free(user_dial_str_exp);
	}
	switch_safe_free(user_dial_str);
	
	return status;
}

static switch_status_t dispat_group_cancel_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char *data = NULL;
	switch_stream_handle_t mystream = { 0 };
	 
	char * dispat_conference_name = NULL;

	switch_channel_t *channel = NULL;

	switch_event_t *event=NULL;
	switch_caller_extension_t *extension = NULL;
	char *forward_var = NULL;

	int margc = 0;
	char *margv[25]={0};

	if(node == NULL ) {

		if(stream != NULL){
			stream->write_function(stream,"err,input fails!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
		 
		return SWITCH_STATUS_FALSE;
	}


	if(switch_strlen_zero(node->conference_name) ||
	   switch_strlen_zero(node->uuid) ||
	   switch_strlen_zero(node->conference_member_id) ||
	   node->session == NULL ||
	   (channel = switch_core_session_get_channel(node->session)) == NULL ||
	   !switch_channel_ready(channel)){
		return SWITCH_STATUS_FALSE;
	}

	forward_var = (char *)switch_channel_get_variable(channel, DISPAT_NODE_FORWARDING_VAR);
	if(switch_strlen_zero(forward_var)){
		//dispat_set_state(node,DISPAT_GROUP);
		return SWITCH_STATUS_SUCCESS;
	}
	margc = switch_separate_string(forward_var,VAR_DELIM,margv,(sizeof(margv)/sizeof(margv[0])));
	if(margc <= 1){
		//dispat_set_state(node,DISPAT_GROUP);
	}
	
	dispat_conference_name = node->conference_name;


	if((extension = switch_caller_extension_new(node->session,RECOVER_APP_COMMAND,RECOVER_APP_COMMAND)) != 0) {

		switch_caller_extension_add_application(node->session,extension,RECOVER_APP_COMMAND,"");
		 
		switch_channel_set_caller_extension(channel,extension);
		switch_channel_set_flag(channel,CF_RESET);

	}else {

		if(stream != NULL){
			stream->write_function(stream,"err,new caller_extension fail!\n");
		} else {
			switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"new caller_extension fail!\n");
		}
		status = SWITCH_STATUS_MEMERR;
	}

	 if (switch_event_create(&event, SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_CMD, CONFERENCE_API_COMMAND);
		 switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, DISPAT_PRIVATE_API_DATA, "${conference_name} endconf");
		 if(dispat_queue_del_event(node, &event) != SWITCH_STATUS_SUCCESS){
			 switch_event_destroy(&event);
			 status = SWITCH_STATUS_FALSE;
		 }
	 } else {

		 status = SWITCH_STATUS_MEMERR;
	 }


	SWITCH_STANDARD_STREAM(mystream);

	data = switch_mprintf("%s noback",dispat_conference_name);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,data,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		/*not change dispat state*/
		 
	} else {
		status = SWITCH_STATUS_FALSE;
	}
	switch_safe_free(data);
    
	 
	data = switch_mprintf("%s kick all",dispat_conference_name);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,data,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		/*not change dispat state*/
		 
	} else {
		status = SWITCH_STATUS_FALSE;
	}
	switch_safe_free(data);
	switch_safe_free(mystream.data);

	return status;

}

#endif
static switch_status_t dispat_group_add_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	
	char * user_num = NULL;
	char *user_dial_str = NULL;
	char *user_dial_str_exp=NULL;

	
    	char * dispat_conference_name = NULL;
    

	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_num = argv[0];

      	if(strlen(user_num) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_num) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_num) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}


		//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		dispat_conference_name = node->conference_name;

	if(dispat_conference_name == NULL || strlen(dispat_conference_name) == 0){
		return SWITCH_STATUS_FALSE;
	}
	
	/*prepare user dial string*/
	user_dial_str = switch_mprintf("user/%s@${domain_name}",user_num);
	if(user_dial_str && node->session){
		switch_channel_t *channel = switch_core_session_get_channel(node->session);
		if(channel){
			user_dial_str_exp = switch_channel_expand_variables(channel,user_dial_str);
		}
	}
	if(user_dial_str_exp == NULL){
		switch_safe_free(user_dial_str);
		return SWITCH_STATUS_FALSE;
	}
	
	SWITCH_STANDARD_STREAM(mystream);
	/*gbdail user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s bgdial %s",dispat_conference_name,user_dial_str_exp);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
	       /*not change dispat state*/
		
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);
	if(user_dial_str_exp != user_dial_str){
		switch_safe_free(user_dial_str_exp);
	}
	switch_safe_free(user_dial_str);
	return status;
}

static switch_status_t dispat_group_kick_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
    dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

    	char * dispat_conference_name = NULL;
  	
    	int user_rdlock = 0;


	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}


		//conference_name = user_get_conference_name(user_node,NULL);
		//conference_member_id = user_get_member_id(user_node,NULL);
		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_member_id;

	if(user_rdlock){
		switch_thread_rwlock_unlock(user_node->rwlock);
	}


	//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
	dispat_conference_name = node->conference_name;
	if(strcmp(conference_name,dispat_conference_name)){
		return SWITCH_STATUS_FALSE;
	}

	SWITCH_STANDARD_STREAM(mystream);
	/*kick user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s kick %s",conference_name,conference_member_id);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
	     /*not change dispat state*/
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;
}

static switch_status_t dispat_group_mute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
    dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

    	char * dispat_conference_name = NULL;
    	char * dispat_conference_member_id = NULL;
    	
    	int user_rdlock = 0;


	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

		//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		//dispat_conference_member_id = switch_event_get_header(node->variables_event,"conference_member_id");
		dispat_conference_name = node->conference_name;
		dispat_conference_member_id = node->conference_member_id;

      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
      		/*load_conf();*/
      		
      		/*if(prefs.can_dispatch_dispatcher){
			char * user_id = NULL;
			user_id = dup_user_id_from_channel_name(node->name);

			if(user_id && !strcmp(user_id,user_name)){
				conference_name = dispat_conference_name;
				conference_member_id = dispat_conference_member_id;
			}
			switch_safe_free(user_id);
      		}*/
      		//else {
      			
			return SWITCH_STATUS_FALSE;
      		//}
	}
	else{

		//conference_name = user_get_conference_name(user_node,NULL);
		//conference_member_id = user_get_member_id(user_node,NULL);
		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_member_id;

		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
		
		if(strcmp(conference_name,dispat_conference_name)){
			return SWITCH_STATUS_FALSE;
		}
	}

	if(conference_name == NULL || conference_member_id == NULL){
		return SWITCH_STATUS_FALSE;
	}
	
	SWITCH_STANDARD_STREAM(mystream);
	/*mute user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s mute %s",conference_name,conference_member_id);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		/*not change dispat state*/
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;
}
static switch_status_t dispat_group_unmute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
   	 dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

    	char * dispat_conference_name = NULL;
    	char * dispat_conference_member_id = NULL;
    	
    	int user_rdlock = 0;


	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

		//      	dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		//dispat_conference_member_id = switch_event_get_header(node->variables_event,"conference_member_id");
		dispat_conference_name = node->conference_name;
		dispat_conference_member_id = node->conference_member_id;


      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
      		
      		/*load_conf();*/
      		/*
      		if(prefs.can_dispatch_dispatcher){
			char * user_id = NULL;
			user_id = dup_user_id_from_channel_name(node->name);

			if(user_id && !strcmp(user_id,user_name)){
				conference_name = dispat_conference_name;
				conference_member_id = dispat_conference_member_id;
			}
			switch_safe_free(user_id);
      		}*/
      		//else {
      			
			return SWITCH_STATUS_FALSE;
      		//}

	}
	else{

		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_member_id;
		
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
		
		if(strcmp(conference_name,dispat_conference_name)){
			return SWITCH_STATUS_FALSE;
		}
	}

	if(conference_name == NULL || conference_member_id == NULL){
		return SWITCH_STATUS_FALSE;
	}
	SWITCH_STANDARD_STREAM(mystream);
	/*unmute user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s unmute %s",conference_name,conference_member_id);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
		/*not change dispat state*/
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;
}
static switch_status_t dispat_group_moderator_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
    dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

    	char * dispat_conference_name = NULL;
    	char * dispat_conference_member_id = NULL;
    	
    	int user_rdlock = 0;


	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

		//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		//dispat_conference_member_id = switch_event_get_header(node->variables_event,"conference_member_id");
		dispat_conference_name = node->conference_name;
		dispat_conference_member_id = node->conference_member_id;

      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
      		/*load_conf();*/
      		/*
      		if(prefs.can_dispatch_dispatcher){
			char * user_id = NULL;
			user_id = dup_user_id_from_channel_name(node->name);

			if(user_id && !strcmp(user_id,user_name)){
				conference_name = dispat_conference_name;
				conference_member_id = dispat_conference_member_id;
			}
			switch_safe_free(user_id);
      		}
      		else {*/
      			
			return SWITCH_STATUS_FALSE;
      		//}
	}
	else{

		//conference_name = user_get_conference_name(user_node,NULL);
		//conference_member_id = user_get_member_id(user_node,NULL);
		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_member_id;

		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
		
		if(strcmp(conference_name,dispat_conference_name)){
			return SWITCH_STATUS_FALSE;
		}
	}

	if(conference_name == NULL || conference_member_id == NULL){
		return SWITCH_STATUS_FALSE;
	}
	
	SWITCH_STANDARD_STREAM(mystream);
	/*mute  all*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s mute all",conference_name);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
	       /*unmute this user*/
		memset(cmdbuf,0,sizeof(cmdbuf));
		switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s unmute %s",conference_name,conference_member_id);

		if(conference_member_id  !=  dispat_conference_member_id &&
			!prefs.can_dispatch_dispatcher ){
			if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
			memset(cmdbuf,0,sizeof(cmdbuf));
			switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s unmute %s",conference_name,dispat_conference_member_id);
			if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
				
			/*not change dispat state*/
			}
			else {
				status = SWITCH_STATUS_FALSE;
				}
			} else {
			status = SWITCH_STATUS_FALSE;
			}

	  }
		
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;
}
static switch_status_t dispat_group_unmoderator_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };


    	char * dispat_conference_name = NULL;
    	char * dispat_conference_member_id = NULL;
  
		//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		//dispat_conference_member_id = switch_event_get_header(node->variables_event,"conference_member_id");
		dispat_conference_name = node->conference_name;
		dispat_conference_member_id = node->conference_member_id;

	if(dispat_conference_name == NULL || strlen(dispat_conference_name)==0){
		return SWITCH_STATUS_FALSE;
	}

	SWITCH_STANDARD_STREAM(mystream);
	/*unmute  all*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s unmute all",dispat_conference_name);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
	      /*not change dispate state*/
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;

}

static dispat_api_t dispat_group_api_s[]={
	{"add",(dispat_api_function_t)&dispat_group_add_member_function,"dispatcher_number <add> user_number"},
	{"kick",(dispat_api_function_t)&dispat_group_kick_member_function,"dispatcher_number <kick> user_number"},
	{"mute",(dispat_api_function_t)&dispat_group_mute_member_function,"dispatcher_number <mute> user_number"},
	{"unmute",(dispat_api_function_t)&dispat_group_unmute_member_function,"dispatcher_number <unmute> user_number"},
	{"moderator",(dispat_api_function_t)&dispat_group_moderator_member_function,"dispatcher_number <moderator> user_number"},
	{"unmoderator",(dispat_api_function_t)&dispat_group_unmoderator_function,"dispatcher_number <unmoderator>"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
#if GUJUN_CHANGE_HOLD
	{"hold",(dispat_api_function_t)&dispat_group_hold_function,"dispatcher_number <hold> "},
#endif
#if GUJUN_CHANGE_FORWARDING
	{"forward",(dispat_api_function_t)&dispat_group_forwarding_function,"dispatcher_number <forward> user_number"},
	{"cancel",(dispat_api_function_t)&dispat_group_cancel_function,"dispatcher_number <forward> user_number"},
#endif
	{NULL,NULL,NULL}
};


static switch_status_t dispat_insert_add_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_add_member_function(stream,node, argv, argc);
	return status;
}

static switch_status_t dispat_insert_kick_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_kick_member_function(stream,node, argv, argc);
	return status;
}

static switch_status_t dispat_insert_mute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_mute_member_function(stream,node, argv, argc);
	return status;
}
static switch_status_t dispat_insert_unmute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_unmute_member_function(stream,node, argv, argc);
	return status;
}
static switch_status_t dispat_insert_moderator_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_moderator_member_function(stream,node, argv, argc);
	return status;
}
static switch_status_t dispat_insert_unmoderator_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_unmoderator_function(stream,node, argv, argc);
	return status;
}
static dispat_api_t dispat_insert_api_s[]={
	{"add",(dispat_api_function_t)&dispat_insert_add_member_function,"dispatcher_number <add> user_number"},
	{"kick",(dispat_api_function_t)&dispat_insert_kick_member_function,"dispatcher_number <kick> user_number"},
	{"mute",(dispat_api_function_t)&dispat_insert_mute_member_function,"dispatcher_number <mute> user_number"},
	{"unmute",(dispat_api_function_t)&dispat_insert_unmute_member_function,"dispatcher_number <unmute> user_number"},
	{"moderator",(dispat_api_function_t)&dispat_insert_moderator_member_function,"dispatcher_number <moderator> user_number"},
	{"unmoderator",(dispat_api_function_t)&dispat_insert_unmoderator_function,"dispatcher_number <unmoderator>"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
#if GUJUN_CHANGE_HOLD
	{"hold",(dispat_api_function_t)&dispat_group_hold_function,"dispatcher_number <hold> "},
#endif
#if GUJUN_CHANGE_FORWARDING
	{"forward",(dispat_api_function_t)&dispat_group_forwarding_function,"dispatcher_number <forward> user_number"},
#endif
	{NULL,NULL,NULL}
};


static switch_status_t dispat_bridge_add_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_add_member_function(stream,node, argv, argc);
	return status;
}

static switch_status_t dispat_bridge_kick_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_kick_member_function(stream,node, argv, argc);
	return status;
}

static switch_status_t dispat_bridge_mute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_mute_member_function(stream,node, argv, argc);
	return status;
}
static switch_status_t dispat_bridge_unmute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_unmute_member_function(stream,node, argv, argc);
	return status;
}
static switch_status_t dispat_bridge_moderator_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_moderator_member_function(stream,node, argv, argc);
	return status;
}
static switch_status_t dispat_bridge_unmoderator_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_unmoderator_function(stream,node, argv, argc);
	return status;
}
static dispat_api_t dispat_bridge_api_s[]={
	{"add",(dispat_api_function_t)&dispat_bridge_add_member_function,"dispatcher_number <add> user_number"},
	{"kick",(dispat_api_function_t)&dispat_bridge_kick_member_function,"dispatcher_number <kick> user_number"},
	{"mute",(dispat_api_function_t)&dispat_bridge_mute_member_function,"dispatcher_number <mute> user_number"},
	{"unmute",(dispat_api_function_t)&dispat_bridge_unmute_member_function,"dispatcher_number <unmute> user_number"},
	{"moderator",(dispat_api_function_t)&dispat_bridge_moderator_member_function,"dispatcher_number <moderator> user_number"},
	{"unmoderator",(dispat_api_function_t)&dispat_bridge_unmoderator_function,"dispatcher_number <unmoderator>"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
#if GUJUN_CHANGE_HOLD
	{"hold",(dispat_api_function_t)&dispat_group_hold_function,"dispatcher_number <hold> "},
#endif
#if GUJUN_CHANGE_FORWARDING
	{"forward",(dispat_api_function_t)&dispat_group_forwarding_function,"dispatcher_number <forward> user_number"},
	{"cancel",(dispat_api_function_t)&dispat_group_cancel_function,"dispatcher_number <forward> user_number"},
#endif
	{NULL,NULL,NULL}
};

static switch_status_t dispat_grab_add_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_add_member_function(stream,node,argv,argc);
	return status;
}

static switch_status_t dispat_grab_kick_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_kick_member_function(stream,node,argv,argc);
	return status;
}

static switch_status_t dispat_grab_mute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_mute_member_function(stream,node,argv,argc);
	return status;
}
static switch_status_t dispat_grab_unmute_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_unmute_member_function(stream,node,argv,argc);
	return status;
}
static switch_status_t dispat_grab_moderator_member_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_moderator_member_function(stream,node,argv,argc);
	return status;
}
static switch_status_t dispat_grab_unmoderator_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	status = dispat_group_unmoderator_function(stream,node,argv,argc);
	return status;
}
static dispat_api_t dispat_grab_api_s[]={
	{"add",(dispat_api_function_t)&dispat_grab_add_member_function,"dispatcher_number <add> user_number"},
	{"kick",(dispat_api_function_t)&dispat_grab_kick_member_function,"dispatcher_number <kick> user_number"},
	{"mute",(dispat_api_function_t)&dispat_grab_mute_member_function,"dispatcher_number <mute> user_number"},
	{"unmute",(dispat_api_function_t)&dispat_grab_unmute_member_function,"dispatcher_number <unmute> user_number"},
	{"moderator",(dispat_api_function_t)&dispat_grab_moderator_member_function,"dispatcher_number <moderator> user_number"},
	{"unmoderator",(dispat_api_function_t)&dispat_grab_unmoderator_function,"dispatcher_number <unmoderator>"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
#if GUJUN_CHANGE_HOLD
	{"hold",(dispat_api_function_t)&dispat_group_hold_function,"dispatcher_number <hold> "},
#endif
#if GUJUN_CHANGE_FORWARDING
	{"forward",(dispat_api_function_t)&dispat_group_forwarding_function,"dispatcher_number <forward> user_number"},
#endif
	{NULL,NULL,NULL}
};
static switch_status_t dispat_hear_grab_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	/*switch_event_t *event=NULL;*/
	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
    dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

    	char * dispat_conference_name = NULL;
    	char * dispat_conference_member_id = NULL;
    	
    	int user_rdlock = 0;
	
	/*int data_len = 0;*/

	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
			if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
			}
			return SWITCH_STATUS_FALSE;
		}
		else{

			//conference_name = user_get_conference_name(user_node,NULL);
			//conference_member_id = user_get_member_id(user_node,NULL);
			conference_name = user_node->conference_name;
			conference_member_id = user_node->conference_member_id;

			if(user_rdlock){
				switch_thread_rwlock_unlock(user_node->rwlock);
			}
		}
		//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		//dispat_conference_member_id = switch_event_get_header(node->variables_event,"conference_member_id");
		dispat_conference_name = node->conference_name;
		dispat_conference_member_id = node->conference_member_id;

		if(strcmp(conference_name,dispat_conference_name)){
			return SWITCH_STATUS_FALSE;
		}

	SWITCH_STANDARD_STREAM(mystream);
	/*kick user*/
	memset(cmdbuf,0,sizeof(cmdbuf));
	switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s kick %s",conference_name,conference_member_id);
	if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
	       /*unmute dispat*/
		memset(cmdbuf,0,sizeof(cmdbuf));
		switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s unmute %s",dispat_conference_name,dispat_conference_member_id);

		if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
			//node->state = DISPAT_GRAB;
#if GUJUN_CHANGE_GROUP
			dispat_set_state(node,DISPAT_GROUP);
#else
			dispat_set_state(node,DISPAT_GRAB);
#endif
		} else {
			status = SWITCH_STATUS_FALSE;
		}
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	switch_safe_free(mystream.data);

	return status;
}

static switch_status_t dispat_hear_insert_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	/*switch_event_t *event=NULL;*/
	char cmdbuf[128]; 
	switch_stream_handle_t mystream = { 0 };
	char * user_name = NULL;
    dispat_node_t * user_node=NULL;
    	
    	char * conference_name = NULL;
    	char * conference_member_id = NULL;

    	char * dispat_conference_name = NULL;
    	char * dispat_conference_member_id = NULL;
    	
    	int user_rdlock = 0;
	
	/*int data_len = 0;*/

	if(argc < 1 || argv[0] == NULL || node == NULL ) {

		if(stream != NULL){
				stream->write_function(stream,"err,input fails!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"input fails!\n");
		}
			
		return SWITCH_STATUS_FALSE;
	}
       user_name = argv[0];

      	if(strlen(user_name) == 0){
      		if(stream != NULL){
				stream->write_function(stream,"err,strlen(user_name) is 0!\n");
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"strlen(user_name) is 0!\n");
		}
      		return SWITCH_STATUS_FALSE;
      	}

      	user_node = dispat_get_by_name(user_name,argc>1?argv[1]:NULL,&user_rdlock);
      	
      	if(user_node == NULL){
		if(stream != NULL){
				stream->write_function(stream,"err,user_node of %s is NULL!\n",user_name);
		} else {
				switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"user_node of %s is NULL!\n",user_name);
		}
		return SWITCH_STATUS_FALSE;
	}
	else{

		//conference_name = user_get_conference_name(user_node,NULL);
		//conference_member_id = user_get_member_id(user_node,NULL);
		conference_name = user_node->conference_name;
		conference_member_id = user_node->conference_name;

		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
	}

		//dispat_conference_name = switch_event_get_header(node->variables_event,"conference_name");
		//dispat_conference_member_id = switch_event_get_header(node->variables_event,"conference_member_id");
		dispat_conference_name = node->conference_name;
		dispat_conference_member_id = node->conference_member_id;
		
	if(strcmp(conference_name,dispat_conference_name)){
		return SWITCH_STATUS_FALSE;
	}

	SWITCH_STANDARD_STREAM(mystream);
	
	/*unmute dispat,BUT NOT KICK anybody*/
		memset(cmdbuf,0,sizeof(cmdbuf));
		switch_snprintf(cmdbuf,sizeof(cmdbuf),"%s unmute %s",dispat_conference_name,dispat_conference_member_id);

		if((status = switch_api_execute(CONFERENCE_API_COMMAND,cmdbuf,NULL,&mystream)) == SWITCH_STATUS_SUCCESS){
			//node->state = DISPAT_INSERT;
#if GUJUN_CHANGE_GROUP
			dispat_set_state(node,DISPAT_GROUP);
#else
			dispat_set_state(node,DISPAT_INSERT);
#endif
		} else {
			status = SWITCH_STATUS_FALSE;
		}
	

	switch_safe_free(mystream.data);

	return status;
}
/*
static switch_status_t dispat_hear_unbridge_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	if((status = dispat_hover_unbridge_function( stream, node, argv, argc)) == SWITCH_STATUS_SUCCESS){
		if(node){
			//node->state = DISPAT_END;
			dispat_set_state(node,DISPAT_END);
		}
	}

	return status;
}
*/

static dispat_api_t dispat_hear_api_s[]={
	{"grab",(dispat_api_function_t)&dispat_hear_grab_function,"dispatcher_number <grab> user_number"},
	{"insert",(dispat_api_function_t)&dispat_hear_insert_function,"dispatcher_number <grab> user_number"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
	{NULL,NULL,NULL}
};


static dispat_api_t dispat_forward_api_s[]={
	{"add",(dispat_api_function_t)&dispat_group_add_member_function,"dispatcher_number <add> user_number"},
	{"kick",(dispat_api_function_t)&dispat_group_kick_member_function,"dispatcher_number <kick> user_number"},
	{"mute",(dispat_api_function_t)&dispat_group_mute_member_function,"dispatcher_number <mute> user_number"},
	{"unmute",(dispat_api_function_t)&dispat_group_unmute_member_function,"dispatcher_number <unmute> user_number"},
	{"moderator",(dispat_api_function_t)&dispat_group_moderator_member_function,"dispatcher_number <moderator> user_number"},
	{"unmoderator",(dispat_api_function_t)&dispat_group_unmoderator_function,"dispatcher_number <unmoderator>"},
	{"unbridge",(dispat_api_function_t)&dispat_hover_unbridge_function,"dispatcher_number <unbridge> user_number"},
#if GUJUN_CHANGE_HOLD
	{"hold",(dispat_api_function_t)&dispat_group_hold_function,"dispatcher_number <hold> "},
#endif
#if GUJUN_CHANGE_FORWARDING
	{"forward",(dispat_api_function_t)&dispat_group_forwarding_function,"dispatcher_number <forward> user_number"},
	{"cancel",(dispat_api_function_t)&dispat_group_cancel_function,"dispatcher_number <forward> user_number"},
#endif
	{NULL,NULL,NULL}
};


static dispat_state_api_set_t  dispat_state_api_set_s[]={
	{DISPAT_NONE,NULL},
	{DISPAT_HOVER,dispat_hover_api_s},
	{DISPAT_GROUP,dispat_group_api_s},
	{DISPAT_FORWARD,dispat_forward_api_s},
	{DISPAT_BRIDGE,dispat_bridge_api_s},
	{DISPAT_INSERT,dispat_insert_api_s},
	{DISPAT_GRAB,dispat_grab_api_s},
	{DISPAT_HEAR,dispat_hear_api_s},
	{DISPAT_END,NULL}
};


static switch_status_t dispat_conf_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	const char *err;
	switch_xml_t xml_root;

	
	if(argc > 0 ){
		if( !strcasecmp(argv[0],"reloadxml")){
			if ((xml_root = switch_xml_open_root(1, &err))) {
				switch_xml_free(xml_root);
			}
		}
	}

	load_conf();
	load_call_conf(); 
	
	stream->write_function(stream,"ok,dispat configuration reload.\n");
	
	return status;
}

static switch_status_t dispat_id_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
 	char *dispatcher = NULL;
 	dispat_node_t *dispat_node = NULL;
	//user_node_t *user_node = NULL;
	switch_hash_index_t *hi = NULL,*next = NULL;
	void *val = NULL;
	int rdlock = 0;
	
	stream->write_function(stream,"<dispatchers>\n");
	switch_mutex_lock(globals.dispatchers_mutex);
	/*dispat_node*/
	if((hi = switch_hash_first(NULL,globals.dispatchers_hash)) != NULL){
		switch_hash_this(hi,NULL,NULL,&val);
		dispatcher = (char  *)val;
		
		if(dispatcher != NULL){
			stream->write_function(stream,"\t<dispatcher id=\"%s\"",dispatcher);
			rdlock = 0;
			dispat_node = dispat_get_by_name(dispatcher,NULL, &rdlock);
			if(dispat_node != NULL){
				stream->write_function(stream," status=\"%s\"",dispat_state_discription[dispat_node->state]);
				if(rdlock){
					switch_thread_rwlock_unlock(dispat_node->rwlock);
				}
			}
			else{
				stream->write_function(stream," status=\"none\"");
			}

			stream->write_function(stream,"/>\n");
		}
		while((next = switch_hash_next(hi)) != NULL) {
			hi = next;
			switch_hash_this(hi,NULL,NULL,&val);
			dispatcher = (char  *)val;
			if(dispatcher != NULL){
				stream->write_function(stream,"\t<dispatcher id=\"%s\"",dispatcher);
				rdlock = 0;
				dispat_node = dispat_get_by_name(dispatcher, NULL,&rdlock);
				if(dispat_node != NULL){
					stream->write_function(stream," status=\"%s\"",dispat_state_discription[dispat_node->state]);
					if(rdlock){
						switch_thread_rwlock_unlock(dispat_node->rwlock);
					}
				}
				else{
					stream->write_function(stream," status=\"none\"");
				}

				stream->write_function(stream,"/>\n");
			
			}
		}
	}
	
	stream->write_function(stream,"</dispatchers>\n");
    	switch_mutex_unlock(globals.dispatchers_mutex);
	return status;
}

static switch_status_t dispat_calls_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
 	user_node_t  *user_node = NULL;
	switch_hash_index_t *hi = NULL,*next = NULL;
	void *val = NULL;
	//char *id = NULL;
	
	switch_mutex_lock(globals.user_mutex);
	if((hi = switch_hash_first(NULL,globals.user_hash)) != NULL){
		switch_hash_this(hi,NULL,NULL,&val);
		 user_node = (user_node_t *)val;
        if(user_node != NULL){
			//id = dup_user_id_from_channel_name(user_node->name);
        		stream->write_function(stream,"\t%s,%s,%d\n",
        			user_node->name,user_node->name,user_node->dispat_num);
        		//switch_safe_free(id);
        }

		while((next = switch_hash_next(hi)) != NULL){
			hi = next;
			switch_hash_this(hi,NULL,NULL,&val);
			user_node = (user_node_t *)val;
			if(user_node != NULL){
				
				//id = dup_user_id_from_channel_name(user_node->name);
        			stream->write_function(stream,"\t%s,%s,%d\n",
        				user_node->name,user_node->name,user_node->dispat_num);
					//	switch_safe_free(id);
			}
		}
		
	}
	switch_mutex_unlock(globals.user_mutex);
	
	return status;
}

static switch_status_t dispat_line_function(switch_stream_handle_t *stream,dispat_node_t *node, char *argv[],int argc)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	user_node_t *user_node = NULL;
 	dispat_node_t *dispat_node = NULL;
	switch_hash_index_t *hi = NULL,*next = NULL;
	void *val = NULL;
	//char *id = NULL;
	switch_mutex_lock(globals.user_mutex);
	/*dispat_node*/
	if((hi = switch_hash_first(NULL,globals.user_hash)) != NULL){
		switch_hash_this(hi,NULL,NULL,&val);
		user_node = (user_node_t *)val;
		
		if(user_node != NULL && (dispat_node = dispat_get_by_name_in_node(user_node,NULL,NULL)) != NULL){
			//id = dup_user_id_from_channel_name(dispat_node->name);
			stream->write_function(stream,"%s,%s,%d\n",dispat_node->name,dispat_node->channel_name,dispat_node->state);
			//switch_safe_free(id);
		}
		while((next = switch_hash_next(hi)) != NULL) {
			hi = next;
			switch_hash_this(hi,NULL,NULL,&val);
			user_node = (user_node_t *)val;
			if(user_node != NULL && (dispat_node = dispat_get_by_name_in_node(user_node,NULL,NULL)) != NULL){
				//id = dup_user_id_from_channel_name(dispat_node->name);
				stream->write_function(stream,"%s,%s,%d\n",dispat_node->name,dispat_node->channel_name,dispat_node->state);
				//switch_safe_free(id);
			}
		}
	}
    switch_mutex_unlock(globals.user_mutex);
	return status;
}


static dispat_api_t dispat_api_s[]={
		{"id",dispat_id_function,"show all dispatchers id"},
		{"calls",dispat_calls_function,"calls show"},
		{"line",dispat_line_function,"line show all diaptchers on line"},
		{"conf",dispat_conf_function,"conf reload"}
};

#define DISPATCHERS_CALL_TRANSFER_SYNTAX "<dispatchers_call> domain"
SWITCH_STANDARD_API(dispatchers_call_transfer_function)
{
	//const char *domain = NULL;

	int ok = 0;
	char *data = NULL;


	/*
	if(cmd != NULL && !switch_strlen_zero(cmd)){
		domain = cmd;
	}
	else{
		domain = switch_core_get_variable("domain");
	}

	if(domain == NULL || switch_strlen_zero(domain)){
		goto end;
	}*/
	if(prefs.direct_transfer == NULL){
		goto end;
	}


	if((data = strdup(prefs.direct_transfer)) != NULL){
		char c = end_of(data);
		char *p;
		
		if(c == ',' || c == '|'){
			end_of(data) = '\0';
		}

		for(p = data;p && *p; p++){
			if(*p == '{'){
				*p = '[';
			}else if(*p == '}'){
				*p = ']';
			}
		}
		stream->write_function(stream,"%s",data);
		ok++;
		free(data);
	}
 end:


	if(!ok){
		stream->write_function(stream,"error/NO_ROUTE_DESTINATION");
	}
	return SWITCH_STATUS_SUCCESS;

}
#define DISPATCHERS_CALL_NORMAL_SYNTAX "<dispatchers_call> domain"
SWITCH_STANDARD_API(dispatchers_call_normal_function)
{
	const char *domain = NULL;

	int ok = 0;

	switch_stream_handle_t dstream = {0};

 	char *dispatcher = NULL;
	switch_hash_index_t *hi = NULL,*next = NULL;
	void *val = NULL;

	char *call_delim = ",";

	if(!switch_strlen_zero(prefs.call_delim)){
		call_delim = prefs.call_delim;
	}
	if(cmd != NULL && !switch_strlen_zero(cmd)){
		domain = cmd;
	}
	else{
		domain = switch_core_get_variable("domain");
	}

	if(domain == NULL || switch_strlen_zero(domain)){
		goto end;
	}
	
	SWITCH_STANDARD_STREAM(dstream);

	switch_mutex_lock(globals.dispatchers_mutex);
	/*dispat_node*/
	if((hi = switch_hash_first(NULL,globals.dispatchers_hash)) != NULL){
		switch_hash_this(hi,NULL,NULL,&val);
		dispatcher = (char  *)val;
		
		if(dispatcher != NULL){
			dstream.write_function(&dstream,"user/%s@%s",dispatcher,domain);
			ok++;
   		}
		while((next = switch_hash_next(hi)) != NULL) {
			hi = next;
			switch_hash_this(hi,NULL,NULL,&val);
			dispatcher = (char  *)val;
			if(dispatcher != NULL){
				if(ok){
					dstream.write_function(&dstream,"%s",call_delim);
				}
				dstream.write_function(&dstream,"user/%s@%s",dispatcher,domain);
				ok++;
			}
		}
	}
	
    switch_mutex_unlock(globals.dispatchers_mutex);

	if(ok && dstream.data){
		char *data = (char *)dstream.data;
		char c = end_of(data);
		char *p;
		
		if(c == ',' || c == '|'){
			end_of(data) = '\0';
		}

		for(p = data;p && *p; p++){
			if(*p == '{'){
				*p = '[';
			}else if(*p == '}'){
				*p = ']';
			}
		}
		stream->write_function(stream,"%s",data);
		free(dstream.data);
	} else {
		ok = 0;
	}
 end:


	if(!ok){
		stream->write_function(stream,"error/NO_ROUTE_DESTINATION");
	}
	return SWITCH_STATUS_SUCCESS;
}
static int check_period(const char *now,const char *start,const char *end){
    int ret = 0;
	char *now_dup= NULL,*start_dup=NULL,*end_dup = NULL;
    char *h = NULL,*m = NULL;
    int now_h = 0,now_m = 0,now_t = 0;
    int start_h = 0,start_m = 0,start_t = 0;
    int end_h =0 ,end_m = 0,end_t = 0;

    if(switch_strlen_zero(now) || switch_strlen_zero(start) || switch_strlen_zero(end)){
        return 0;
    }
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "check %s  between %s~%s ",now,start,end);
	 
#define getHM(S,D,H,M,T) \
	D = strdup(S); \
	if(D == NULL){ \
		goto done; \
    } \
    h = D; \
    m = strchr(h,':'); \
    if(m == NULL){ \
		goto done; \
    } \
    *m++='\0'; \
    H = atoi(h); \
    H = H%24; \
    M = atoi(m); \
    M = M%60; \
	T = H*60+M;

	getHM(now,now_dup,now_h,now_m,now_t);
	getHM(start,start_dup,start_h,start_m,start_t);
	getHM(end,end_dup,end_h,end_m,end_t);

	if(end_t >= start_t){
		if(start_t <= now_t && now_t <= end_t){
			ret  = 1;
		}
	}
	else{
		if(now_t >= start_t || now_t <= end_t){
			ret = 1;
		}
	}
 done:
	if(now_dup){
		free(now_dup);
	}
	if(start_dup){
		free(start_dup);
	}
	if(end_dup){
		free(end_dup);
	}
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, ", ret is %d\n",ret);
	return ret;
}
static int check_direct_transfer_period(const char *period){

	int argc;
    char *argv[25] = { 0 };
    char *lbuf = NULL;

    switch_time_t start_time;
    switch_time_exp_t start_time_exp;
    char time_str[80]="";
    switch_size_t retsize;

    char *start,*end;
    int ret = 0;

    if(switch_strlen_zero(period)){
        return 1;
    }
    if(!(lbuf = strdup(period))){
        return 0;
    }
    argc = switch_separate_string(lbuf,',',argv,(sizeof(argv)/sizeof(argv[0])));
    if(argc < 1){
        goto done;
    }
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "separte %s into %d period\n",lbuf,argc);
    start_time = switch_micro_time_now();
    switch_time_exp_lt(&start_time_exp,start_time);
    switch_strftime_nocheck(time_str,&retsize,sizeof(time_str),"%H:%M",&start_time_exp);//"%Y-%m-%d %H:%M:%S",&start_time_exp);

    for(int i = 0 ; i < argc ; i++){
        start = argv[i];
        if(start == NULL){
            continue;
        }
        end = strchr(start,'-');
        if(end == NULL){
            continue;
        }
        *end++ = '\0';

        if((ret = check_period(time_str,start,end))){
            break;
        }
    }

 done:
    if(lbuf){
        free(lbuf);
    }
    return ret;
}

#define DISPATCHERS_CALL_SYNTAX "<dispatchers_call> domain"
SWITCH_STANDARD_API(dispatchers_call_function)
{
	int transfer_ok = 0;
	
	if(prefs.direct_transfer_enable && prefs.direct_transfer){
		if(switch_strlen_zero(prefs.direct_transfer_period)){
			transfer_ok++;
		} else {
			if(check_direct_transfer_period(prefs.direct_transfer_period)){
				transfer_ok++;
			}
		}
	}
	if(transfer_ok){
		return dispatchers_call_transfer_function(cmd,session,stream);
	}
	return dispatchers_call_normal_function(cmd,session,stream);

}

#define DISPAT_SYNTAX "<dispatch> <who>  hear|grab|unbridge|group_call  <data>"
SWITCH_STANDARD_API(dispat_function)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	char *lbuf = NULL;
	int argc;
	char *argv[25] = { 0 };
	
	int api_classes = 0;
	dispat_api_t * api_s = NULL;
	char *api_cmd  = NULL;
	int called = 0;

	/*switch_event_t *event;*/
	dispat_node_t *node=NULL;
	int rdlock = 0;

	user_node_t *user_node=NULL;
	int user_rdlock = 0;

	const char *usage_string = "USAGE:\n"
		"--------------------------------------------------------------------------------\n"
		"dispat conf [reloadxml]\n"
		"dispat id\n"
		"dispat calls\n"
		"dispat line\n"
		"dispat <num [uuid]>  hear|grab|unbridge|group_call  <num data> uuid\n"
		"--------------------------------------------------------------------------------\n";
	

    if(switch_strlen_zero(cmd)){
        stream->write_function(stream,"%s",usage_string);
        return status;
    }
	if(!(lbuf = strdup(cmd))){
		return status;
	}
	argc = switch_separate_string(lbuf,' ',argv,(sizeof(argv)/sizeof(argv[0])));
	if(argc < 1){
		stream->write_function(stream,"%s",usage_string);
		switch_safe_free(lbuf);
		return status;
	}
	api_classes = sizeof(dispat_api_s)/sizeof(dispat_api_s[0]);
	for(int i =0;i<api_classes;i++){
		if( !strcasecmp(dispat_api_s[i].api_command,argv[0])){
			dispat_api_function_t fn = dispat_api_s[i].api_function;
			if(fn != NULL){
					status = fn(stream,NULL,&(argv[1]),argc-1);
					
			}
			else{
				stream->write_function(stream,"internal error\n");
			}
			switch_safe_free(lbuf);
			return status;
		}
	}
	
	/*
	switch_mutex_lock(globals.dispat_mutex);
	node = switch_core_hash_find(globals.dispat_hash,argv[0]);
	if(node != NULL){
		if(switch_thread_rwlock_rdlock(node->rwlock) ==  SWITCH_STATUS_SUCCESS){
			rdlock++;
		}
	}
	switch_mutex_unlock(globals.dispat_mutex);*/

	//if(argc < 5){
	if(argc < 3){
		stream->write_function(stream,"%s",usage_string);
		goto free_lbuf_and_done;
	}

	/*check that whether this node is dispatcher or not*/
	if(check_dispatcher_by_id(argv[0]) != SWITCH_STATUS_SUCCESS){
		stream->write_function(stream,"err,%s not dispatcher\n",argv[0]);
		goto free_lbuf_and_done;		
	}

	user_node=user_get_by_name(argv[0],&user_rdlock);
	if(user_node == NULL){
		stream->write_function(stream,"err,this dispatcher %s not call in yet\n",argv[0]);
		goto free_lbuf_and_done;
	}
	switch_mutex_lock(user_node->mutex);
	//node = dispat_get_by_name(argv[0],argv[1],&rdlock);
	node = dispat_get_by_name_in_node(user_node,NULL,&rdlock);
	if(node == NULL){
		stream->write_function(stream,"err,this dispatcher %s not call in yet\n",argv[0]);
		goto unlock_user_node_and_done;
	}
	if(node->uuid == NULL){
		stream->write_function(stream,"err,this dispatcher %s uuid is null\n",argv[0]);
		goto unlock_dispat_node_and_done;
	}
	
	node->session = switch_core_session_locate(node->uuid);
	if(node->session == NULL){
		stream->write_function(stream,"err,this dispatcher %s session is null\n",argv[0]);
		goto unlock_dispat_node_and_done;
	}
	
	/*cmd*/
	/*status = hear(node,"1010-1001-192.168.17.7",stream);*/
	api_classes = sizeof(dispat_state_api_set_s)/sizeof(dispat_state_api_set_s[0]);
	
	for(int i =0;i<api_classes;i++){
		if(node->state == dispat_state_api_set_s[i].state){
			api_s = dispat_state_api_set_s[i].api_s;
			break;
		}
	}
	if(api_s != NULL){
		for(int i = 0;api_s[i].api_command != NULL;i++){
			//if(!strcasecmp(api_s[i].api_command,argv[2])){
			if(!strcasecmp(api_s[i].api_command,argv[1])){
				dispat_api_function_t fn = (dispat_api_function_t)api_s[i].api_function;
				api_cmd = api_s[i].api_command;

				if(fn != NULL){
					//status = fn(stream,node,&(argv[3]),argc-3);
					status = fn(stream,node,&(argv[2]),argc-2);
				}
				called++;
				break;
			}
		}
	}
	/*let node know*/
	/*if(called){
		if(switch_event_create(&event,SWITCH_EVENT_COMMAND) == SWITCH_STATUS_SUCCESS) {
			switch_event_add_header_string(event,SWITCH_STACK_BOTTOM,"cmd",api_cmd);
			switch_core_session_queue_private_event(node->session,&event);
		}
	}*/

	switch_core_session_rwunlock(node->session);

 unlock_dispat_node_and_done:
	if(rdlock){
		switch_thread_rwlock_unlock(node->rwlock);
	}
	/*if(status == SWITCH_STATUS_SUCCESS){
		stream->write_function(stream,"ok\n");
	}*/

 unlock_user_node_and_done:
	 switch_mutex_unlock(user_node->mutex);
	 if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
	 }


 free_lbuf_and_done:
	switch_safe_free(lbuf);
	return status;
}



/****************app********************/

SWITCH_STANDARD_APP(voicetip_function)
{
	//switch_input_args_t args = { 0 };
	switch_channel_t *channel = switch_core_session_get_channel(session);
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	int loops = 1;
	switch_call_cause_t cause = SWITCH_CAUSE_NONE;
	char *cause_str = NULL;

	if(channel == NULL ){
		return;
	}
	
	cause_str = (char *)switch_channel_get_variable(channel,"originate_disposition");
	if(cause_str != NULL){
		//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"%s ori disp:%s\n",switch_channel_get_name(channel),cause_str);
		cause = switch_channel_str2cause(cause_str);
	}
	//switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_DEBUG,"%s voicetip\n",switch_channel_get_name(channel));
	if(cause == SWITCH_CAUSE_NONE || cause == SWITCH_CAUSE_SUCCESS ||
	   cause == SWITCH_CAUSE_NORMAL_CLEARING){
		return;
	}
	
	
	//args.input_callback = on_dtmf;

	//switch_channel_set_variable(channel, SWITCH_PLAYBACK_TERMINATOR_USED, "" );

	if(switch_channel_answer(channel) != SWITCH_STATUS_SUCCESS){
		return;
	}

	if(!switch_strlen_zero(data)){
		loops = atoi(data);
	}
	
	while(loops-- > 0){
		switch_ivr_sleep(session,1000,SWITCH_TRUE,NULL);
		switch(cause){
		case SWITCH_CAUSE_USER_NOT_REGISTERED:
			status = switch_ivr_play_file(session,NULL,"lily/not_registered.wav",NULL);
			break;
		case SWITCH_CAUSE_USER_BUSY:
			status = switch_ivr_play_file(session,NULL,"lily/busy.wav",NULL);
			break;
		case SWITCH_CAUSE_NO_ANSWER:
			status = switch_ivr_play_file(session,NULL,"lily/no_answer.wav",NULL);
			break;
		case SWITCH_CAUSE_NO_USER_RESPONSE:
		default:
			status = switch_ivr_play_file(session,NULL,"lily/unavailable.wav",NULL);
			break;
		}
	}
	//status = switch_ivr_play_file(session, NULL, data, &args);

	switch (status) {
	case SWITCH_STATUS_SUCCESS:
	case SWITCH_STATUS_BREAK:
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "FILE PLAYED");
		break;
	case SWITCH_STATUS_NOTFOUND:
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "FILE NOT FOUND");
		break;
	default:
		switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "PLAYBACK ERROR");
		break;
	}
	
	switch_channel_hangup(channel,cause);
}


/*#define HEAR_SYNTAX " "
#define GRAB_SYNTAX "[-bleg] <uuid>"
SWITCH_STANDARD_APP(grab_function)
{
	int argc;
	char *argv[4] = { 0 };
	char *mydata;
	char *uuid;
	switch_bool_t bleg = SWITCH_FALSE;

	if (!switch_strlen_zero(data) && (mydata = switch_core_session_strdup(session, data))) {
		switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"data: %s\n",mydata);
		if ((argc = switch_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) >= 1) {
			if (!strcasecmp(argv[0], "-bleg")) {
				if (argv[1]) {
					uuid = argv[1];
					bleg = SWITCH_TRUE;
				} else {
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Usage: %s\n", GRAB_SYNTAX);
					return;
				}
			} else {
				uuid = argv[0];
			}

			switch_ivr_intercept_session(session, uuid, bleg);
		}
		return;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Usage: %s\n", GRAB_SYNTAX);
}
*/

static switch_status_t hover_on_dtmf(switch_core_session_t *session, void *input, switch_input_type_t itype, void *buf, unsigned int buflen)
{
	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF:
		{
			switch_dtmf_t *dtmf = (switch_dtmf_t *) input;
			//switch_channel_t *channel = switch_core_session_get_channel(session);
			//const char *caller_exit_key = switch_channel_get_variable(channel, "fifo_caller_exit_key");
			dtmf_handler_data_t *data = (dtmf_handler_data_t *)buf;


			data->dtmf[data->dtmfNum] = *dtmf;
			data->dtmfNum ++;
			return SWITCH_STATUS_BREAK;
			
		}
		break;
	default:
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}


static switch_status_t caller_read_frame_callback(switch_core_session_t *session, switch_frame_t *frame, void *user_data)
{
	dispat_node_t *node = (dispat_node_t *) user_data;

	if (!node) {
		return SWITCH_STATUS_SUCCESS;
	}

	if(node->state == DISPAT_HOVER ||
	   node->state == DISPAT_NONE){

		return SWITCH_STATUS_SUCCESS;
	}
			 switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_INFO,"dispatnode state is not hover!\n");	
	return SWITCH_STATUS_FALSE;
}

#define HOVER_SYNTAX " <data>"

SWITCH_STANDARD_APP(hover_function)
{
	int argc=0;
	char *argv[5]={0};
	user_node_t *user_node = NULL;
	int user_rdlock = 0;
	int user_destroy = 0;

	dispat_node_t *dispat_node = NULL;
	switch_channel_t *channel = NULL;
	char *uuid = NULL;
	int dispat_rdlock = 0;
	//int dispat_destroy = 0;

	switch_caller_profile_t *caller_profile=NULL;
	char *caller_id_number = NULL;	
	char *mydata = NULL;
	char *moh = NULL;
	char * user_id = NULL;
	
	dtmf_handler_data_t buf={0};
	switch_input_args_t args = {0};

	switch_status_t status=SWITCH_STATUS_SUCCESS;
	
	/*session check*/
	
	/*get channel*/
	channel = switch_core_session_get_channel(session);
	switch_assert(channel != NULL);
	/*get caller profile*/
	caller_profile = switch_channel_get_caller_profile(channel);
	/*get caller_id_number*/
	/*caller_id_number = (char *)switch_channel_get_variable(channel,"caller_id_number");*/
	caller_id_number = switch_channel_get_name(channel);

	//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "caller_id_number-->%s",caller_id_number);
	if(caller_id_number == NULL){
		switch_channel_answer(channel);
		if(switch_channel_ready(channel)){
			switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
		}

		return;
	}
	
	user_id = dup_user_id_from_channel_name(caller_id_number);
	//check is dispatcher
	if(check_dispatcher_by_id(user_id) != SWITCH_STATUS_SUCCESS ||
	   (user_node = user_get_by_name_or_create(user_id,&user_rdlock)) == NULL){
		switch_channel_answer(channel);
		if(switch_channel_ready(channel)){
			switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
		}
		switch_safe_free(user_id);
		return;
	}

	uuid = switch_core_session_get_uuid(session);
	dispat_node = dispat_get_by_uuid_or_create(user_node,uuid,caller_id_number,&dispat_rdlock);

	if(dispat_node == NULL){
		switch_channel_answer(channel);
		if(switch_channel_ready(channel)){
			switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
		}
		switch_safe_free(user_id);
		if(user_node->dispat_num <= 0){
			user_destroy++;
		}
		if(user_rdlock){
			switch_thread_rwlock_unlock(user_node->rwlock);
		}
		if(user_destroy){
			switch_mutex_lock(globals.user_mutex);

			switch_core_hash_delete(globals.user_hash,user_node->name);

			switch_mutex_unlock(globals.user_mutex);

			user_node_free(user_node);
		}
		return;
	}

	dispat_node->session = session;

	dispat_node->state = DISPAT_HOVER;
	
	user_current_shift(user_node,dispat_node,1);
	
	//dispat_set_state(dispat_node,DISPAT_HOVER);
	
	/*set channel private */
	switch_channel_set_private(channel,HOVER_ID_VARIABLE,dispat_node->name);//channel name

	moh = (char *)switch_channel_get_variable(channel,"hold_music");

	if(data != NULL) {
		mydata = switch_core_session_strdup(session,data);
		argc = switch_separate_string(mydata,' ',argv,(sizeof(argv)/sizeof(argv[0])));
	}

	if(argc > 0) {
		moh = argv[0];
	}
	if(moh && !strcasecmp(moh,"silence")) {
		moh = NULL;
	}
	
	
	switch_channel_answer(channel);

	buf.dtmfNum = 0;
		
	args.input_callback = hover_on_dtmf;
		//args.input_callback = NULL;
	args.buf = &buf;
	args.buflen = sizeof(dtmf_handler_data_t);

	args.read_frame_callback = caller_read_frame_callback;
	args.user_data = dispat_node;


	while(globals.running && switch_channel_ready(channel) && (dispat_node->state == DISPAT_HOVER)) {
			
#if GUJUN_CHANGE_FORWARDING
		if(dispat_hold_event_count(dispat_node)>0){
			dispat_parse_all_hold_event(dispat_node);
			continue;
		}
#endif
		if(buf.dtmfNum == 0){
			
			if(moh) {
				 status = switch_ivr_play_file(session,NULL,moh,&args);
				
			}else {
				
				status = switch_ivr_collect_digits_callback(session,&args,MAX_DIAL_DURATION,0);
			}

		
		}
		else{
			status = switch_ivr_collect_digits_callback(session,&args,MAX_DIAL_DURATION,0);
		}

		if(status == SWITCH_STATUS_SUCCESS){
			break;
		}
		if(buf.dtmfNum > 0 && buf.dtmf[buf.dtmfNum-1].digit == '#' ){
			buf.dtmfNum --;
			break;
		}
		if(buf.dtmfNum==MAX_PHONE_NUM_LEN){
			break;
		}
		
		if(!SWITCH_READ_ACCEPTABLE(status)) {
				break;
		}
		
		/*else {
		
			switch_ivr_collect_digits_callback(session,&args,0);
		}
		
		if(*buf == '*'){
			break
		}else if(*buf == '#') {
			break;
		}*/
		
		/*deal private event*/
		/*if(switch_core_session_private_event_count(session)){
			switch_core_session_flush_private_events(session);	
			break;
		}		
		*/
		switch_yield(200000);
	}/*while*/
	if(switch_channel_ready(channel)){
		if(dispat_node->state == DISPAT_HOVER){
			if(buf.dtmfNum > 0){
				char user_num[MAX_PHONE_NUM_LEN+1] = {0};
				char *arg[] ={user_num};
				memset(user_num,0,sizeof(user_num));
				for(int i =0; i < buf.dtmfNum && i<MAX_PHONE_NUM_LEN; i++){
					user_num[i] = buf.dtmf[i].digit;
				}
				//switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "call %s\n",arg[0]);
				dispat_hover_bridge_function(NULL,dispat_node,arg,1);
			}
			else{
				switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
				//dispat_set_state(dispat_node,DISPAT_END);
				//dispat_node->state = DISPAT_END;
			}
		}
	}

	//	else{
		//destroy dispat_node
		//dispat_set_state(dispat_node,DISPAT_END);
		//dispat_node->state = DISPAT_END;
		//}

	//if(dispat_node->state == DISPAT_END){
	//	dispat_destroy++;
	//}

	if(dispat_rdlock){
		switch_thread_rwlock_unlock(dispat_node->rwlock);
	}
	
	/*	if(dispat_destroy){
		switch_mutex_lock(user_node->mutex);
		//remove from hahs
		switch_core_hash_delete(user_node->dispat_hash,dispat_node->uuid);
		
		user_node->dispat_num--;
		
		user_current_none(user_node,dispat_node,1);
		//dispat_node_free
		switch_mutex_unlock(user_node->mutex);

		dispat_node_free(dispat_node);
	}
	
	if(user_node->dispat_num <=0){
		user_destroy++;
	}
*/
	if(user_rdlock){
		switch_thread_rwlock_unlock(user_node->rwlock);
	}
	/*	
	if(user_destroy){
		switch_mutex_lock(globals.user_mutex);

		switch_core_hash_delete(globals.user_hash,user_node->name);

		switch_mutex_unlock(globals.user_mutex);

		user_node_free(user_node);				
	}
   */
	
	/*	if(!aborted&&switch_channel_ready(channel)){
			hear(session,"1010-1001-192.168.17.7",NULL);
			goto done;
		}
	*/
	/*	if(switch_channel_ready(channel)){
			switch_channel_hangup(channel,SWITCH_CAUSE_NORMAL_CLEARING);
		}
	*/	
	
	/*gather dtmf while playing somthing*/
	

	switch_safe_free(user_id);
	
	return;
}


#define RECOVER_SYNTAX "<data>"
SWITCH_STANDARD_APP(recover_function)
{
	switch_channel_t *channel = switch_core_session_get_channel(session);

	/*switch_event_t *event=NULL;*/
	
	switch_caller_extension_t *extension = NULL;
    char * conference_app_data = NULL;
    char * conference_name = NULL;
    
	char *forward_var = NULL;
	char *new_var = NULL;
	
	int argc=0;
	char *argv[25]={0};
	
	int more = 0;

	if(channel == NULL ){
		return;
	}
	if(switch_channel_get_variable_dup(channel,CLEAR_FLAG_NOT_READY_VAR,SWITCH_FALSE)){
		switch_channel_clear_flag(channel, CF_NOT_READY);
		switch_channel_set_variable(channel,CLEAR_FLAG_NOT_READY_VAR,NULL);
	}
	if(!switch_channel_ready(channel) ){
		return;
	}
	forward_var = (char *)switch_channel_get_variable(channel, DISPAT_NODE_FORWARDING_VAR);
	if(switch_strlen_zero(forward_var)){
		return;
	}
	argc = switch_separate_string(forward_var,VAR_DELIM,argv,(sizeof(argv)/sizeof(argv[0])));
	if(argc < 1){
		return;
	}
	conference_name = argv[0];

	if(argc > 1){
		for(int i = 1;i<argc;i++){
			char *var = argv[i];
			char *old = new_var;
			
			if(switch_strlen_zero(var)){
				continue;
			}
			if(old){
				new_var = switch_mprintf("%s%c%s",old,VAR_DELIM,var);
				switch_safe_free(old);
			}else{
				new_var = switch_mprintf("%s",var);
			}
		}
		if(new_var){
			switch_channel_set_variable(channel,DISPAT_NODE_FORWARDING_VAR,new_var);
			switch_safe_free(new_var);
			more++;
		}
	}
	else{
		switch_channel_set_variable(channel,DISPAT_NODE_FORWARDING_VAR,NULL);
	}
	
	if(!switch_strlen_zero(conference_name) && 
	   (extension = switch_caller_extension_new(session,conference_name,conference_name)) != 0) {

		conference_app_data = switch_mprintf("%s",conference_name);
		
		if(conference_app_data == NULL) {
			goto done;
		}

		switch_caller_extension_add_application(session,extension,CONFERENCE_APP_COMMAND,conference_app_data);
		if(more){
			switch_caller_extension_add_application(session,extension,RECOVER_APP_COMMAND,RECOVER_APP_COMMAND);
		}
		switch_channel_set_caller_extension(channel,extension);
		switch_channel_set_flag(channel,CF_RESET);

		switch_safe_free(conference_app_data);
		//dispat_set_state(node,DISPAT_GROUP);
		
	}
	
	
 done:
	
	return;
}


SWITCH_MODULE_LOAD_FUNCTION(mod_dispat_load)
{
	switch_application_interface_t *app_interface;
	switch_api_interface_t *commands_api_interface;
	
	/* create /register cumstom event message type */
	if(switch_event_reserve_subclass(DISPAT_EVENT) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!",DISPAT_EVENT);
		return SWITCH_STATUS_TERM;
	}

	/* Subscribe to presence request events*/
	if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,CONF_EVENT_MAINT,
				conference_event_handler,NULL,&globals.conference_event_node) != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}
	if(switch_event_bind_removable(modname, SWITCH_EVENT_CUSTOM,DISPAT_CFG_EVENT,
				dispat_cfg_event_handler,NULL,&globals.dispat_cfg_event_node) != SWITCH_STATUS_SUCCESS) {
		return SWITCH_STATUS_GENERR;
	}

	/*if(switch_event_bind_removable(modname,SWITCH_EVENT_PRESENCE_PROBE, SWITCH_EVENT_SUBCLASS_ANY, 
									pres_event_handler,NULL, &globals.node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Coundn't subscribe to SWITCH_EVENT_PRESENCE_PROBE events!\n");
		return SWITCH_STATUS_GENERR;
	}
	*/
	if (switch_event_bind_removable(modname, SWITCH_EVENT_RELOADXML, NULL, xml_event_handler, NULL, &globals.xml_event_node) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
		return SWITCH_STATUS_GENERR;
	}
	if(switch_event_bind_removable(modname, SWITCH_EVENT_CHANNEL_HANGUP, NULL, channel_event_handler, NULL, &globals.channel_hangup_event_node)!= SWITCH_STATUS_SUCCESS){                                                                                              
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");                                                
        return SWITCH_STATUS_GENERR;                                                                                                
    } 
	/*init globals structure*/
	switch_core_new_memory_pool(&globals.pool);
	//	switch_core_hash_init(&globals.dispat_hash,globals.pool);
	switch_core_hash_init(&globals.user_hash, globals.pool);
	switch_core_hash_init(&globals.dispatchers_hash,globals.pool);
	//switch_mutex_init(&globals.dispat_mutex,SWITCH_MUTEX_NESTED,globals.pool);
	switch_mutex_init(&globals.user_mutex, SWITCH_MUTEX_NESTED,globals.pool);
	switch_mutex_init(&globals.dispatchers_mutex,SWITCH_MUTEX_NESTED,globals.pool);
	
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	//SWITCH_ADD_APP(app_interface, "grab", "grab", "grab", grab_function, GRAB_SYNTAX, SAF_NONE);
	SWITCH_ADD_APP(app_interface,"hover","hover","hover",hover_function, HOVER_SYNTAX,SAF_NONE);
	SWITCH_ADD_APP(app_interface,"recover","recover","recover",recover_function, RECOVER_SYNTAX,SAF_NONE);
	SWITCH_ADD_APP(app_interface,"voicetip","voicetip","voicetip",voicetip_function,"voicetip <loops>",SAF_NONE);
	

	SWITCH_ADD_API(commands_api_interface,"dispat","dispat",dispat_function,DISPAT_SYNTAX);
	SWITCH_ADD_API(commands_api_interface,"dispatchers_call","dispatchers_call",dispatchers_call_function,DISPATCHERS_CALL_SYNTAX);
	// SWITCH_ADD_API(commands_api_interface,"dispat_get_conference_name","dispat_get_conference_name",dispat_get_conference_name,DISPAT_GET_CONFERENCE_NAME_SYNTAX);
	//SWITCH_ADD_API(commands_api_interface,"dispat_conference","dispat_conference",dispat_conference,DISPAT_CONFERENCE_SYNTAX);
	switch_console_set_complete("add dispat");

	memset(&prefs,0,sizeof(prefs));
	load_conf();
	load_call_conf();
	
	globals.running = 1;


	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_dispat_shutdown)
{
	switch_hash_index_t *hi=NULL;
	void *val=NULL;
	//	dispat_node_t *dispat_node=NULL;
	user_node_t *user_node = NULL;

	switch_memory_pool_t *pool = globals.pool;

	//switch_mutex_t *dispat_mutex = globals.dispat_mutex;
	switch_mutex_t *user_mutex = globals.user_mutex;
	switch_mutex_t *dispatchers_mutex = globals.dispatchers_mutex;

	switch_event_unbind(&globals.node);
	switch_event_unbind(&globals.conference_event_node);
	switch_event_unbind(&globals.dispat_cfg_event_node);
    switch_event_unbind(&globals.xml_event_node);
	switch_event_unbind(&globals.channel_hangup_event_node);
       
	switch_event_free_subclass(DISPAT_EVENT);

	globals.running = 0;

	/*	switch_mutex_lock(dispat_mutex);

	while((hi = switch_hash_first(NULL,globals.dispat_hash)) != NULL){
		switch_hash_this(hi,NULL,NULL,&val);
		dispat_node = (dispat_node_t *)val;
		if(dispat_node !=NULL){
			switch_core_hash_delete(globals.dispat_hash,dispat_node->name);
	
			dispat_node_free(dispat_node);
			dispat_node = NULL;
		}
	}
	switch_core_hash_destroy(&globals.dispat_hash);
	switch_mutex_unlock(dispat_mutex);
*/
	switch_mutex_lock(user_mutex);
	
	 while((hi = switch_hash_first(NULL,globals.user_hash)) != NULL){
        switch_hash_this(hi,NULL,NULL,&val);
        user_node = (user_node_t *)val;
        if(user_node !=NULL){
            switch_core_hash_delete(globals.user_hash,user_node->name);
	
            user_node_free(user_node);
            user_node = NULL;
        }
    }
	switch_core_hash_destroy(&globals.user_hash);
	switch_mutex_unlock(user_mutex);

	switch_mutex_lock(dispatchers_mutex);
	switch_core_hash_destroy(&globals.dispatchers_hash);
	switch_mutex_unlock(dispatchers_mutex);
	
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
