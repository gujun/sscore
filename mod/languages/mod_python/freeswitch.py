# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.35
#
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _freeswitch
import new
new_instancemethod = new.instancemethod
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'PySwigObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


consoleLog = _freeswitch.consoleLog
consoleCleanLog = _freeswitch.consoleCleanLog
class IVRMenu(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, IVRMenu, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, IVRMenu, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_IVRMenu(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_IVRMenu
    __del__ = lambda self : None;
    def bindAction(*args): return _freeswitch.IVRMenu_bindAction(*args)
    def execute(*args): return _freeswitch.IVRMenu_execute(*args)
IVRMenu_swigregister = _freeswitch.IVRMenu_swigregister
IVRMenu_swigregister(IVRMenu)

class API(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, API, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, API, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_API(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_API
    __del__ = lambda self : None;
    def execute(*args): return _freeswitch.API_execute(*args)
    def executeString(*args): return _freeswitch.API_executeString(*args)
    def getTime(*args): return _freeswitch.API_getTime(*args)
API_swigregister = _freeswitch.API_swigregister
API_swigregister(API)

class input_callback_state_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, input_callback_state_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, input_callback_state_t, name)
    __repr__ = _swig_repr
    __swig_setmethods__["function"] = _freeswitch.input_callback_state_t_function_set
    __swig_getmethods__["function"] = _freeswitch.input_callback_state_t_function_get
    if _newclass:function = _swig_property(_freeswitch.input_callback_state_t_function_get, _freeswitch.input_callback_state_t_function_set)
    __swig_setmethods__["threadState"] = _freeswitch.input_callback_state_t_threadState_set
    __swig_getmethods__["threadState"] = _freeswitch.input_callback_state_t_threadState_get
    if _newclass:threadState = _swig_property(_freeswitch.input_callback_state_t_threadState_get, _freeswitch.input_callback_state_t_threadState_set)
    __swig_setmethods__["extra"] = _freeswitch.input_callback_state_t_extra_set
    __swig_getmethods__["extra"] = _freeswitch.input_callback_state_t_extra_get
    if _newclass:extra = _swig_property(_freeswitch.input_callback_state_t_extra_get, _freeswitch.input_callback_state_t_extra_set)
    __swig_setmethods__["funcargs"] = _freeswitch.input_callback_state_t_funcargs_set
    __swig_getmethods__["funcargs"] = _freeswitch.input_callback_state_t_funcargs_get
    if _newclass:funcargs = _swig_property(_freeswitch.input_callback_state_t_funcargs_get, _freeswitch.input_callback_state_t_funcargs_set)
    def __init__(self, *args): 
        this = _freeswitch.new_input_callback_state_t(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_input_callback_state_t
    __del__ = lambda self : None;
input_callback_state_t_swigregister = _freeswitch.input_callback_state_t_swigregister
input_callback_state_t_swigregister(input_callback_state_t)

S_HUP = _freeswitch.S_HUP
S_FREE = _freeswitch.S_FREE
S_RDLOCK = _freeswitch.S_RDLOCK
class DTMF(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, DTMF, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, DTMF, name)
    __repr__ = _swig_repr
    __swig_setmethods__["digit"] = _freeswitch.DTMF_digit_set
    __swig_getmethods__["digit"] = _freeswitch.DTMF_digit_get
    if _newclass:digit = _swig_property(_freeswitch.DTMF_digit_get, _freeswitch.DTMF_digit_set)
    __swig_setmethods__["duration"] = _freeswitch.DTMF_duration_set
    __swig_getmethods__["duration"] = _freeswitch.DTMF_duration_get
    if _newclass:duration = _swig_property(_freeswitch.DTMF_duration_get, _freeswitch.DTMF_duration_set)
    def __init__(self, *args): 
        this = _freeswitch.new_DTMF(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_DTMF
    __del__ = lambda self : None;
DTMF_swigregister = _freeswitch.DTMF_swigregister
DTMF_swigregister(DTMF)

class Stream(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Stream, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Stream, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_Stream(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_Stream
    __del__ = lambda self : None;
    def write(*args): return _freeswitch.Stream_write(*args)
    def get_data(*args): return _freeswitch.Stream_get_data(*args)
Stream_swigregister = _freeswitch.Stream_swigregister
Stream_swigregister(Stream)

class Event(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Event, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Event, name)
    __repr__ = _swig_repr
    __swig_setmethods__["event"] = _freeswitch.Event_event_set
    __swig_getmethods__["event"] = _freeswitch.Event_event_get
    if _newclass:event = _swig_property(_freeswitch.Event_event_get, _freeswitch.Event_event_set)
    __swig_setmethods__["serialized_string"] = _freeswitch.Event_serialized_string_set
    __swig_getmethods__["serialized_string"] = _freeswitch.Event_serialized_string_get
    if _newclass:serialized_string = _swig_property(_freeswitch.Event_serialized_string_get, _freeswitch.Event_serialized_string_set)
    __swig_setmethods__["mine"] = _freeswitch.Event_mine_set
    __swig_getmethods__["mine"] = _freeswitch.Event_mine_get
    if _newclass:mine = _swig_property(_freeswitch.Event_mine_get, _freeswitch.Event_mine_set)
    def __init__(self, *args): 
        this = _freeswitch.new_Event(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_Event
    __del__ = lambda self : None;
    def serialize(*args): return _freeswitch.Event_serialize(*args)
    def setPriority(*args): return _freeswitch.Event_setPriority(*args)
    def getHeader(*args): return _freeswitch.Event_getHeader(*args)
    def getBody(*args): return _freeswitch.Event_getBody(*args)
    def getType(*args): return _freeswitch.Event_getType(*args)
    def addBody(*args): return _freeswitch.Event_addBody(*args)
    def addHeader(*args): return _freeswitch.Event_addHeader(*args)
    def delHeader(*args): return _freeswitch.Event_delHeader(*args)
    def fire(*args): return _freeswitch.Event_fire(*args)
Event_swigregister = _freeswitch.Event_swigregister
Event_swigregister(Event)

class EventConsumer(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, EventConsumer, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, EventConsumer, name)
    __repr__ = _swig_repr
    __swig_setmethods__["events"] = _freeswitch.EventConsumer_events_set
    __swig_getmethods__["events"] = _freeswitch.EventConsumer_events_get
    if _newclass:events = _swig_property(_freeswitch.EventConsumer_events_get, _freeswitch.EventConsumer_events_set)
    __swig_setmethods__["e_event_id"] = _freeswitch.EventConsumer_e_event_id_set
    __swig_getmethods__["e_event_id"] = _freeswitch.EventConsumer_e_event_id_get
    if _newclass:e_event_id = _swig_property(_freeswitch.EventConsumer_e_event_id_get, _freeswitch.EventConsumer_e_event_id_set)
    __swig_setmethods__["node"] = _freeswitch.EventConsumer_node_set
    __swig_getmethods__["node"] = _freeswitch.EventConsumer_node_get
    if _newclass:node = _swig_property(_freeswitch.EventConsumer_node_get, _freeswitch.EventConsumer_node_set)
    __swig_setmethods__["e_callback"] = _freeswitch.EventConsumer_e_callback_set
    __swig_getmethods__["e_callback"] = _freeswitch.EventConsumer_e_callback_get
    if _newclass:e_callback = _swig_property(_freeswitch.EventConsumer_e_callback_get, _freeswitch.EventConsumer_e_callback_set)
    __swig_setmethods__["e_subclass_name"] = _freeswitch.EventConsumer_e_subclass_name_set
    __swig_getmethods__["e_subclass_name"] = _freeswitch.EventConsumer_e_subclass_name_get
    if _newclass:e_subclass_name = _swig_property(_freeswitch.EventConsumer_e_subclass_name_get, _freeswitch.EventConsumer_e_subclass_name_set)
    __swig_setmethods__["e_cb_arg"] = _freeswitch.EventConsumer_e_cb_arg_set
    __swig_getmethods__["e_cb_arg"] = _freeswitch.EventConsumer_e_cb_arg_get
    if _newclass:e_cb_arg = _swig_property(_freeswitch.EventConsumer_e_cb_arg_get, _freeswitch.EventConsumer_e_cb_arg_set)
    def __init__(self, *args): 
        this = _freeswitch.new_EventConsumer(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_EventConsumer
    __del__ = lambda self : None;
    def pop(*args): return _freeswitch.EventConsumer_pop(*args)
EventConsumer_swigregister = _freeswitch.EventConsumer_swigregister
EventConsumer_swigregister(EventConsumer)

class CoreSession(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, CoreSession, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, CoreSession, name)
    def __init__(self, *args, **kwargs): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    __swig_destroy__ = _freeswitch.delete_CoreSession
    __del__ = lambda self : None;
    __swig_setmethods__["session"] = _freeswitch.CoreSession_session_set
    __swig_getmethods__["session"] = _freeswitch.CoreSession_session_get
    if _newclass:session = _swig_property(_freeswitch.CoreSession_session_get, _freeswitch.CoreSession_session_set)
    __swig_setmethods__["channel"] = _freeswitch.CoreSession_channel_set
    __swig_getmethods__["channel"] = _freeswitch.CoreSession_channel_get
    if _newclass:channel = _swig_property(_freeswitch.CoreSession_channel_get, _freeswitch.CoreSession_channel_set)
    __swig_setmethods__["flags"] = _freeswitch.CoreSession_flags_set
    __swig_getmethods__["flags"] = _freeswitch.CoreSession_flags_get
    if _newclass:flags = _swig_property(_freeswitch.CoreSession_flags_get, _freeswitch.CoreSession_flags_set)
    __swig_setmethods__["allocated"] = _freeswitch.CoreSession_allocated_set
    __swig_getmethods__["allocated"] = _freeswitch.CoreSession_allocated_get
    if _newclass:allocated = _swig_property(_freeswitch.CoreSession_allocated_get, _freeswitch.CoreSession_allocated_set)
    __swig_setmethods__["cb_state"] = _freeswitch.CoreSession_cb_state_set
    __swig_getmethods__["cb_state"] = _freeswitch.CoreSession_cb_state_get
    if _newclass:cb_state = _swig_property(_freeswitch.CoreSession_cb_state_get, _freeswitch.CoreSession_cb_state_set)
    __swig_setmethods__["hook_state"] = _freeswitch.CoreSession_hook_state_set
    __swig_getmethods__["hook_state"] = _freeswitch.CoreSession_hook_state_get
    if _newclass:hook_state = _swig_property(_freeswitch.CoreSession_hook_state_get, _freeswitch.CoreSession_hook_state_set)
    __swig_setmethods__["cause"] = _freeswitch.CoreSession_cause_set
    __swig_getmethods__["cause"] = _freeswitch.CoreSession_cause_get
    if _newclass:cause = _swig_property(_freeswitch.CoreSession_cause_get, _freeswitch.CoreSession_cause_set)
    __swig_setmethods__["uuid"] = _freeswitch.CoreSession_uuid_set
    __swig_getmethods__["uuid"] = _freeswitch.CoreSession_uuid_get
    if _newclass:uuid = _swig_property(_freeswitch.CoreSession_uuid_get, _freeswitch.CoreSession_uuid_set)
    __swig_setmethods__["tts_name"] = _freeswitch.CoreSession_tts_name_set
    __swig_getmethods__["tts_name"] = _freeswitch.CoreSession_tts_name_get
    if _newclass:tts_name = _swig_property(_freeswitch.CoreSession_tts_name_get, _freeswitch.CoreSession_tts_name_set)
    __swig_setmethods__["voice_name"] = _freeswitch.CoreSession_voice_name_set
    __swig_getmethods__["voice_name"] = _freeswitch.CoreSession_voice_name_get
    if _newclass:voice_name = _swig_property(_freeswitch.CoreSession_voice_name_get, _freeswitch.CoreSession_voice_name_set)
    def answer(*args): return _freeswitch.CoreSession_answer(*args)
    def preAnswer(*args): return _freeswitch.CoreSession_preAnswer(*args)
    def hangup(*args): return _freeswitch.CoreSession_hangup(*args)
    def hangupState(*args): return _freeswitch.CoreSession_hangupState(*args)
    def setVariable(*args): return _freeswitch.CoreSession_setVariable(*args)
    def setPrivate(*args): return _freeswitch.CoreSession_setPrivate(*args)
    def getPrivate(*args): return _freeswitch.CoreSession_getPrivate(*args)
    def getVariable(*args): return _freeswitch.CoreSession_getVariable(*args)
    def process_callback_result(*args): return _freeswitch.CoreSession_process_callback_result(*args)
    def say(*args): return _freeswitch.CoreSession_say(*args)
    def sayPhrase(*args): return _freeswitch.CoreSession_sayPhrase(*args)
    def hangupCause(*args): return _freeswitch.CoreSession_hangupCause(*args)
    def getState(*args): return _freeswitch.CoreSession_getState(*args)
    def recordFile(*args): return _freeswitch.CoreSession_recordFile(*args)
    def originate(*args): return _freeswitch.CoreSession_originate(*args)
    def destroy(*args): return _freeswitch.CoreSession_destroy(*args)
    def setDTMFCallback(*args): return _freeswitch.CoreSession_setDTMFCallback(*args)
    def speak(*args): return _freeswitch.CoreSession_speak(*args)
    def set_tts_parms(*args): return _freeswitch.CoreSession_set_tts_parms(*args)
    def collectDigits(*args): return _freeswitch.CoreSession_collectDigits(*args)
    def getDigits(*args): return _freeswitch.CoreSession_getDigits(*args)
    def transfer(*args): return _freeswitch.CoreSession_transfer(*args)
    def read(*args): return _freeswitch.CoreSession_read(*args)
    def playAndGetDigits(*args): return _freeswitch.CoreSession_playAndGetDigits(*args)
    def streamFile(*args): return _freeswitch.CoreSession_streamFile(*args)
    def sleep(*args): return _freeswitch.CoreSession_sleep(*args)
    def flushEvents(*args): return _freeswitch.CoreSession_flushEvents(*args)
    def flushDigits(*args): return _freeswitch.CoreSession_flushDigits(*args)
    def setAutoHangup(*args): return _freeswitch.CoreSession_setAutoHangup(*args)
    def setHangupHook(*args): return _freeswitch.CoreSession_setHangupHook(*args)
    def ready(*args): return _freeswitch.CoreSession_ready(*args)
    def bridged(*args): return _freeswitch.CoreSession_bridged(*args)
    def answered(*args): return _freeswitch.CoreSession_answered(*args)
    def mediaReady(*args): return _freeswitch.CoreSession_mediaReady(*args)
    def waitForAnswer(*args): return _freeswitch.CoreSession_waitForAnswer(*args)
    def execute(*args): return _freeswitch.CoreSession_execute(*args)
    def sendEvent(*args): return _freeswitch.CoreSession_sendEvent(*args)
    def setEventData(*args): return _freeswitch.CoreSession_setEventData(*args)
    def getXMLCDR(*args): return _freeswitch.CoreSession_getXMLCDR(*args)
    def begin_allow_threads(*args): return _freeswitch.CoreSession_begin_allow_threads(*args)
    def end_allow_threads(*args): return _freeswitch.CoreSession_end_allow_threads(*args)
    def get_uuid(*args): return _freeswitch.CoreSession_get_uuid(*args)
    def get_cb_args(*args): return _freeswitch.CoreSession_get_cb_args(*args)
    def check_hangup_hook(*args): return _freeswitch.CoreSession_check_hangup_hook(*args)
    def run_dtmf_callback(*args): return _freeswitch.CoreSession_run_dtmf_callback(*args)
CoreSession_swigregister = _freeswitch.CoreSession_swigregister
CoreSession_swigregister(CoreSession)

console_log = _freeswitch.console_log
console_clean_log = _freeswitch.console_clean_log
msleep = _freeswitch.msleep
bridge = _freeswitch.bridge
hanguphook = _freeswitch.hanguphook
dtmf_callback = _freeswitch.dtmf_callback
class Session(CoreSession):
    __swig_setmethods__ = {}
    for _s in [CoreSession]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, Session, name, value)
    __swig_getmethods__ = {}
    for _s in [CoreSession]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, Session, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_Session(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_Session
    __del__ = lambda self : None;
    def begin_allow_threads(*args): return _freeswitch.Session_begin_allow_threads(*args)
    def end_allow_threads(*args): return _freeswitch.Session_end_allow_threads(*args)
    def check_hangup_hook(*args): return _freeswitch.Session_check_hangup_hook(*args)
    def destroy(*args): return _freeswitch.Session_destroy(*args)
    def run_dtmf_callback(*args): return _freeswitch.Session_run_dtmf_callback(*args)
    def setInputCallback(*args): return _freeswitch.Session_setInputCallback(*args)
    def unsetInputCallback(*args): return _freeswitch.Session_unsetInputCallback(*args)
    def setHangupHook(*args): return _freeswitch.Session_setHangupHook(*args)
    def ready(*args): return _freeswitch.Session_ready(*args)
    __swig_setmethods__["cb_function"] = _freeswitch.Session_cb_function_set
    __swig_getmethods__["cb_function"] = _freeswitch.Session_cb_function_get
    if _newclass:cb_function = _swig_property(_freeswitch.Session_cb_function_get, _freeswitch.Session_cb_function_set)
    __swig_setmethods__["cb_arg"] = _freeswitch.Session_cb_arg_set
    __swig_getmethods__["cb_arg"] = _freeswitch.Session_cb_arg_get
    if _newclass:cb_arg = _swig_property(_freeswitch.Session_cb_arg_get, _freeswitch.Session_cb_arg_set)
    __swig_setmethods__["hangup_func"] = _freeswitch.Session_hangup_func_set
    __swig_getmethods__["hangup_func"] = _freeswitch.Session_hangup_func_get
    if _newclass:hangup_func = _swig_property(_freeswitch.Session_hangup_func_get, _freeswitch.Session_hangup_func_set)
    __swig_setmethods__["hangup_func_arg"] = _freeswitch.Session_hangup_func_arg_set
    __swig_getmethods__["hangup_func_arg"] = _freeswitch.Session_hangup_func_arg_get
    if _newclass:hangup_func_arg = _swig_property(_freeswitch.Session_hangup_func_arg_get, _freeswitch.Session_hangup_func_arg_set)
    def setPython(*args): return _freeswitch.Session_setPython(*args)
    def setSelf(*args): return _freeswitch.Session_setSelf(*args)
Session_swigregister = _freeswitch.Session_swigregister
Session_swigregister(Session)



