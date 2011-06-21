/*
 * CARI mod in FreeSWITCH
 * Copyright(C)2005-2009,jim Gu <jim.ritchie.gu@gmail.com>
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
 * jime Gu <jim.ritchie.gu@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * mod_cfg.c -- configuration api  Module
 */
#include <switch.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_cfg_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cfg_shutdown);
SWITCH_MODULE_DEFINITION(mod_cfg,mod_cfg_load,mod_cfg_shutdown,NULL);

#define DISPAT_CONF_FILE "dispat_call.conf"
#define DISPAT_CFG_EVENT "custom::cfg::dispat"

static struct{
  switch_memory_pool_t *pool;
}globals;
typedef  switch_status_t (*sub_api_function_t)(switch_stream_handle_t *stream, char  **argv, int argc);

struct sub_api{

  char *api_command;

  struct sub_api *sub;

  sub_api_function_t api_function;
  
  char *api_syntax;
  
};

static switch_status_t load_config(switch_memory_pool_t *pool)
{
  memset(&globals, 0, sizeof(globals));

  globals.pool = pool;

  return SWITCH_STATUS_SUCCESS;
}

static switch_status_t help_function(switch_stream_handle_t *stream,struct sub_api  *sub_api_s)
{
  
  if(stream == NULL || sub_api_s == NULL){
    return SWITCH_STATUS_SUCCESS;
  }
 
  for(int i = 0; sub_api_s[i].api_command != NULL;i++){
    stream->write_function(stream,"\t%s",sub_api_s[i].api_command);
    if(sub_api_s[i].api_syntax == NULL){
      stream->write_function(stream,"\n");
    }else {
      stream->write_function(stream,"\t%s\n",sub_api_s[i].api_syntax);
    }
  }
  
  return SWITCH_STATUS_SUCCESS;
}

static switch_status_t sub_api_function(switch_stream_handle_t *stream,struct sub_api  *sub_api_s, char **argv, int argc){
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  int called = 0;

  if(stream == NULL || sub_api_s == NULL || argv == NULL || argc < 1 || argv[0] == NULL){
    return status;
  }

  if(!strcasecmp("?",argv[0])){
    help_function(stream,sub_api_s);
    called++;
    goto done;
  }

  // api_classes = sizeof(cfg_sub_api_s)/sizeof(cfg_sub_api_s[0]);
  for(int i =0;sub_api_s[i].api_command != NULL;i++){
    if( !strcasecmp(sub_api_s[i].api_command,argv[0])){
      sub_api_function_t fn = sub_api_s[i].api_function;
      struct sub_api *sub = sub_api_s[i].sub;
      if(fn != NULL){
	status = fn(stream,&(argv[1]),argc-1);
      }
      if(sub != NULL){
	status = sub_api_function(stream,sub,&(argv[1]),argc-1);
      }

      called++;
      goto done;
    }
  }
  
done:
  return status;
}

char * getFullPathFile(){
  //conf/autoload_configs/
  return switch_mprintf("%s%s%s%s%s%s",SWITCH_GLOBAL_dirs.conf_dir, SWITCH_PATH_SEPARATOR, 
			"autoload_configs",SWITCH_PATH_SEPARATOR,
			DISPAT_CONF_FILE,".xml");
}
static switch_status_t save_dispat_dt_cfg()
{
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_xml_t cfg=NULL,xml=NULL;
  char *xmlstr = NULL;
  char *file = NULL;//getAutoPath();

  if((xml = switch_xml_open_cfg(DISPAT_CONF_FILE,&cfg,NULL))==NULL){
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open of %s failed\n",DISPAT_CONF_FILE);
    status = SWITCH_STATUS_FALSE;
  }else{
    if(cfg){
      xmlstr = switch_xml_toxml(cfg,SWITCH_FALSE);
    }
    switch_xml_free(xml);
  }
  file = getFullPathFile();
  if(xmlstr && file){
     //	stream->write_function(stream,"%s",xmlstr);
    //remove(file);
    int fd = -1;
#ifdef _MSC_VER
      if ((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) > -1) {
#else
	if ((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) > -1) {
#endif
	  int wrote;
	  wrote = write(fd, xmlstr, (unsigned) strlen(xmlstr));
	  close(fd);
	  fd = -1;
	}
    

  }

   switch_safe_free(file);
   switch_safe_free(xmlstr);
  return status;
}

static switch_status_t set_dispat_dt_function(switch_stream_handle_t *stream, char **argv,int argc)
{
  char *enable = NULL;
  char *dt = NULL;
  char *dt_period = NULL;
  int changed= 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_xml_t cfg=NULL, xml=NULL, settings=NULL,param=NULL;

  if(stream == NULL || argv == NULL || argc < 1 || argv[0]==NULL){
    return status;
  }
  
  enable = switch_true(argv[0])?"true":"false";

  if(argc >= 3){
    dt = argv[1];
    dt_period = argv[2];
  }else if(argc == 2){
    dt = argv[1];
  }
  if(dt && !strcasecmp(dt,"null")){
    dt = "";
  }
  if(dt_period && !strcasecmp(dt_period,"null")){
    dt_period = "";
  }
  if((xml = switch_xml_open_cfg(DISPAT_CONF_FILE,&cfg,NULL))==NULL){
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open of %s failed\n",DISPAT_CONF_FILE);
    status = SWITCH_STATUS_FALSE;
  }else{
    if(cfg && (settings = switch_xml_child(cfg,"settings"))){
      for(param = switch_xml_child(settings,"param");param;param=param->next){
	char *var = (char *)switch_xml_attr_soft(param,"name");
	char *val = (char *)switch_xml_attr_soft(param,"value");

	
	if(!strcmp(var,"direct-transfer-enable")){
	  if(enable && switch_true(enable) != switch_true(val)){
	    switch_xml_set_attr_d(param,"value",enable);
	    changed++;
	  }

	} else if(!strcmp(var,"direct-transfer")){
	  if(dt && strcmp(dt,val)){
	    switch_xml_set_attr_d(param,"value",dt);
	    changed++;
	  }
	} else if(!strcmp(var,"direct-transfer-period")){
	  if(dt_period && strcmp(dt_period,val)){
	    switch_xml_set_attr_d(param,"value",dt_period);
	    changed++;
	  }
	}

      }
    }
    switch_xml_free(xml);
  }
  
  if(changed){
    switch_event_t *event=NULL;

    if(switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, DISPAT_CFG_EVENT) == SWITCH_STATUS_SUCCESS) {
      switch_event_fire(&event);
    }
    save_dispat_dt_cfg();
  }
  
  
  return status;
}

static switch_status_t show_dispat_dt_function(switch_stream_handle_t *stream, char **argv,int argc)
{
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_xml_t cfg=NULL,xml=NULL;

  if(stream == NULL){
    return status;
  }
  if((xml = switch_xml_open_cfg(DISPAT_CONF_FILE,&cfg,NULL))==NULL){
    switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR,"Open of %s failed\n",DISPAT_CONF_FILE);
    status = SWITCH_STATUS_FALSE;
  }else{
    if(cfg){
      char *xmlstr = switch_xml_toxml(cfg,SWITCH_FALSE);
      if(xmlstr){
	stream->write_function(stream,"%s",xmlstr);
	switch_safe_free(xmlstr);
      }
    }
    switch_xml_free(xml);
  }

  return status;
}

///////////////////////////////////////////////////////////////////////////////
static struct sub_api set_dispat_sub_api_s[]={
  {"dt",NULL,set_dispat_dt_function,"set about dispat direct transfer"},
  {NULL,NULL,NULL}
};

static struct sub_api set_sub_api_s[]={
  {"dispat",set_dispat_sub_api_s,NULL,"set about dispat"},
  {NULL,NULL,NULL}
};
/////////////////////////////////////////////////////////////////////////////
static struct sub_api show_dispat_sub_api_s[]={
  {"dt",NULL,show_dispat_dt_function,"show about dispat direct transfer"},
  {NULL,NULL,NULL}
};


static struct sub_api  show_sub_api_s[]={
  {"dispat",show_dispat_sub_api_s,NULL,"show about dispat"},
  {NULL,NULL,NULL}
};

/////////////////////////////////////////////////////////////////////////////
static struct sub_api cfg_sub_api_s[]={
  {"set",set_sub_api_s,NULL,"set data"},
  {"show",show_sub_api_s,NULL,"show data"},
  {NULL,NULL,NULL}
};

#define CFG_USAGE "cfg class subclass <data>"
SWITCH_STANDARD_API(cfg_api_function)
{
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  char *lbuf = NULL;
  char *argv[25] = { 0 };
  int argc= 0;
 
  if(switch_strlen_zero(cmd)){
    //stream->write_function(stream,"%s",usage_string);
    return status;
  }

  if(!(lbuf = strdup(cmd))){
    return status;
  }

  argc = switch_separate_string(lbuf,' ',argv,(sizeof(argv)/sizeof(argv[0])));
  if(argc < 1){
    //stream->write_function(stream,"%s",usage_string);
    switch_safe_free(lbuf);
    return status;
  }

  sub_api_function(stream,cfg_sub_api_s,argv,argc);

  switch_safe_free(lbuf);
  return status;
}
SWITCH_MODULE_LOAD_FUNCTION(mod_cfg_load)
{
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_api_interface_t *api_interface;

  load_config(pool);

  if(switch_event_reserve_subclass(DISPAT_CFG_EVENT) != SWITCH_STATUS_SUCCESS){
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,"Cound't register subclass %s!",DISPAT_CFG_EVENT);
    return status;
  }

  *module_interface = switch_loadable_module_create_module_interface(pool, modname);
  SWITCH_ADD_API(api_interface, "cfg", "cfg", cfg_api_function, CFG_USAGE);

  return status;

}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_cfg_shutdown)
{
  switch_event_free_subclass(DISPAT_CFG_EVENT);

  return SWITCH_STATUS_SUCCESS;
}
