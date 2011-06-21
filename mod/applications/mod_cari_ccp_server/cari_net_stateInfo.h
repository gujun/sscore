#ifndef  STATEINFO_H
#define  STATEINFO_H


//是否需要进行转码
#define CODE_TRANSFER_FLAG 1


/*---宏定义函数--------------------------------------------------------*/
#define CARICCP_MAX(num1,num2) (num1>num2 ? num1:num2)							//查找最大数
#define CARICCP_MIN(num1,num2) (num1>num2 ? num2:num1)							//查找最小数

//////////////////////////////////////////////////////////////////////////
//函数使用的宏定义
#define CARI_NET_PRO_RESULT				   "cari_net_proResult"
#define CARI_NET_PRO_NOTIFY				   "cari_net_proNotify"
#define CARI_NET_SOFIA_CMD_HANDLER	       "cari_net_sofia_cmd_handler"


#define CARICCP_SOCKET_BUFFER_SIZE			1024*4 //4K的
#define CARICCP_MAX_CMD_NUM					0xFFFF //异步请求命令的最大存放数量
#define CARICCP_MAX_MODULE_NUM				32     //支持子模块数量

#define CARICCP_MAX_SHORT					0x7FFF //short类型的最大值  32767
#define CARICCP_MAX_INT						0xFFFFFFFF //int类型的最大值(0xFFFF???)

#define CARICCP_MAX_CREATTHREAD_NUM			3     //创建线程的最大次数
#define CARICCP_MAX_CPU_WAIT_TIME			10    //连续的命令发送时间间隔(单位:毫秒),缓冲一下发送流,防止阻塞

#define CARICCP_MAX_NAME_LENGTH				32
#define CARICCP_MAX_TIME_LENGTH				20
#define CARICCP_MAX_DESC					200
#define CARICCP_MAX_LENGTH					1024
#define CARICCP_MAX_TABLEHEADER				150   //表头的最大长度
#define CARICCP_MAX_TABLECONTEXT			CARICCP_SOCKET_BUFFER_SIZE \
	- sizeof(common_ResultRespFrame_Header) \
	- sizeof(u_int) \
	- CARICCP_MAX_TABLEHEADER

#define CARICCP_MAX_STR_LEN(str1,str2)      CARICCP_MAX(strlen(str1),strlen(str2)) //连个字串,最长的那个字串的长度           


#define MAX_INTLENGTH                       100     //int类型数据的最大长度 数的范围是0~4294967295,0~65535

#define CARICCP_MAX_ERRRORFRAME_COUNT       10      //接收到的错误帧的次数
#define CARICCP_MAX_CLIENT_NUM              128     //接收到的错误帧的次数

//////////////////////////////////////////////////////////////////////////
//用户组的有关信息
#define CARICCP_MAX_GROUPS_NUM              1024
#define CARICCP_MAX_MOBUSER_IN_GROUP_NUM    1024
#define CARICCP_MAX_GATEWAY_NUM             100

#define CARICCP_STRING_LENGTH_16			16
#define CARICCP_STRING_LENGTH_32			32
#define CARICCP_STRING_LENGTH_64			64
#define CARICCP_STRING_LENGTH_128			128
#define CARICCP_STRING_LENGTH_256			256
#define CARICCP_STRING_LENGTH_512			512

#define CARICCP_NULL_MARK					""						//空字串
#define CARICCP_SPACEBAR					" "						//空格键
#define CARICCP_ENTER_KEY					"\r\n"					//回车换行符

//回车符,操作系统的不同,涉及到字符可能也不同
//#if defined(_WIN32) || defined(WIN32)
//#define CARICCP_NEWLINE_LF				"\r"
//#else
#define CARICCP_NEWLINE_CR					"\r"
#define CARICCP_NEWLINE_LF					"\n"
//#endif

#define CARICCP_TAB_MARK					"\t"					//tab键
#define CARICCP_EQUAL_MARK					"="						//等号
#define CARICCP_CHAR_EQUAL_MARK				'='					    //字符 :等号
#define CARICCP_COMMA_MARK					","                     //逗号
#define CARICCP_CHAR_COMMA_MARK				','                     //字符类型 :逗号
#define CARICCP_QUO_MARK                    "\""                    //引号,使用转义符
#define CARICCP_COMMA_FLAG					","
#define CARICCP_BLACKSLASH_MARK		        "\\"                    //反斜杠

#ifdef CODE_TRANSFER_FLAG
        //utf8转码,建议不要采用特殊的双字节字符,否则需要转换一下,再封装记录...
	    #define CARICCP_SPECIAL_SPLIT_FLAG  "/\\"/*"〒""$"*/        //使用此特殊符号,标识"参数对"之间的间隔,主要应用在查询类命令返回的结果
#else
	    #define CARICCP_SPECIAL_SPLIT_FLAG	"〒"                    //使用此特殊符号,标识"参数对"之间的间隔,主要应用在查询类命令返回的结果
#endif

#define CARICCP_CHAR_SPECIAL_SPLIT_FLAG	   /* '〒'*/'$'				//字符类型:使用此特殊符号,标识"参数对"之间的间隔,主要应用在查询类命令返回的结果
#define CARICCP_PARAM_SPLIT_FLAG            CARICCP_COMMA_FLAG

#define CARICCP_LEFT_SQUARE_BRACKETS		"["						//左方括号
#define CARICCP_RIGHT_SQUARE_BRACKETS		"]"						//右方括号
#define CARICCP_LEFT_ROUND_BRACKETS		    "("						//左圆括号
#define CARICCP_RIGHT_ROUND_BRACKETS		")"						//右圆括号
#define CARICCP_LEFT_BIG_BRACKETS		    "{"						//左大括号
#define CARICCP_RIGHT_BIG_BRACKETS		    "}"						//右大括号
#define CARICCP_COLON_MARK					":"                     //冒号
#define CARICCP_SEMICOLON_MARK              ";"						//分号
#define CARICCP_OR_MARK						"|"						//或
#define CARICCP_XOR_MARK					"^"						//异或
#define CARICCP_DOLLAR_MARK					"$"						//美元符
#define CARICCP_LINK_MARK					"-"						//-
#define CARICCP_EMPTY_MARK					"" 
#define CARICCP_MUTIL_VALUE_MARK			"&"						//多值标识
#define CARICCP_PERCENT_MARK                "%"                     //百分比号
#define CARICCP_DOT_MARK                    "."                     //dot点号


//////////////////////////////////////////////////////////////////////////
//profile的名字
#define CARICCP_SIP_PROFILES_INTERNAL       "internal"
#define CARICCP_XML_FILE_TYPE               ".xml"
#define CARI_CCP_EVENT_NAME                 "cariccp:state"

//"命令帧"涉及到关键标识
#define SOCKET_ID							"socketID"
#define CLIENT_ID							"clientID"
#define CARICCP_NE_ID						"neID"
#define CARICCP_SUBCLIENTMOD_ID				"clientSubMoudleID"
#define CARICCP_CMDCODE						"cmdCode"
#define CARICCP_CMDNAME						"cmdName"
#define CARICCP_SYNCHRO						"synchro"
#define CARICCP_NOTIFY						"notify"
#define CARICCP_RETURNCODE					"returnCode"
#define CARICCP_RESULTDESC					"resuleDesc"
#define CARICCP_RESULTVALUE					"resuleValue"
#define CARICCP_MOBUSERNAME					"mobUserName"
#define CARICCP_MOBCALLER					"mobCaller"
#define CARICCP_MOBCALLEE				    "mobCallee"
#define CARICCP_MOBUSERSTATE				"mobUserState"
#define CARICCP_MOBCALLSTATE				"mobCallState"
#define CARICCP_MOBUSERINFO				    "mobUserInfo"
#define CARICCP_MOBCALLTYPE				    "mobCallType"


//权限设置:数字越小,级别越高,目前只用3,4,5三种
#define USERPRIORITY_SPECIAL                 0 //保留的级别,专门给调度员使用,包括:呼叫和软键盘权限
#define USERPRIORITY_INTERNATIONAL	         1
#define USERPRIORITY_DOMESTIC		         2
#define USERPRIORITY_OUT			         3
#define USERPRIORITY_LOCAL			         4
#define USERPRIORITY_ONLY_DISPATCH	         5
#define DEFAULT_USERPRIORITY	             USERPRIORITY_LOCAL

//特殊的路由:拨入调度台
#define ROUTER_TO_DISPATCH_DESK              100

//命令码对应表
enum CmdCode
{
	//内部命令区:模块内部直接发送
	CARICCP_QUERY_CLIENTID				=  99,      //查询获得clientID
	CARICCP_LOGIN						=  100,     //登录
	CARICCP_RECONNECT                   =  101,     //重连
	CARICCP_REBOOT				        =  102,     //重启
	CARICCP_SHAKEHAND    		        =  103,     //握手
	CARICCP_QUERY_MOB_USER				=  150,		//查询MOB用户,主要针对监控重连带查询,此命令只需要返回用户号

	CARICCP_QUERY_SYSINFO		        =  160,		//查询linux系统的版本信息
	CARICCP_QUERY_UPTIME                =  161,     //查询linux系统的用户数
	CARICCP_QUERY_CPU_INFO				=  162,		//查询系统CPU信息
	CARICCP_QUERY_MEMORY_INFO			=  163,     //查询系统内存使用状况
	CARICCP_QUERY_DISK_INFO				=  164,     //查询磁盘使用情况
	CARICCP_QUERY_CPU_RATE				=  165,     //查询CPU rate


	//一般配置命令:在配置文件中配置的命令
	CARICCP_MOB_USER_BEGIN              =  190,     //mob user模块的命令起始-----------------------

	//一般配置命令:在配置文件中配置的命令
	CARICCP_ADD_OP_USER					=  196,		//增加操作用户
	CARICCP_DEL_OP_USER					=  197,		//删除操作用户
	CARICCP_MOD_OP_USER					=  198,		//修改操作用户
	CARICCP_LST_OP_USER					=  199,		//查询操作用户

	CARICCP_ADD_MOB_USER				=  201,		//增加MOB用户
	CARICCP_DEL_MOB_USER				=  202,		//删除MOB用户
	CARICCP_MOD_MOB_USER				=  203,		//修改MOB用户
	CARICCP_LST_MOB_USER				=  204,		//查询MOB用户,此命令返回用户号,说明等多种信息
	CARICCP_LST_MOB_STATE			    =  205,		//查询MOB用户的在线状态信息

	CARICCP_LST_USER_CALL_TRANSFER      =  207,     //查询当前绑定的呼叫转移到信息
	CARICCP_SET_USER_CALL_TRANSFER      =  208,     //用户号码呼叫转移
	CARICCP_UNBIND_CALL_TRANSFER        =  209,     //解除用户号码呼叫转移

	CARICCP_ADD_MOB_GROUP			    =  210,		//增加MOB用户组
	CARICCP_DEL_MOB_GROUP				=  211,		//删除MOB用户组
	CARICCP_MOD_MOB_GROUP				=  212,		//修改MOB用户组
	CARICCP_LST_MOB_GROUP				=  213,		//查询MOB用户组,此命令返回
	CARICCP_SET_MOB_GROUP				=  214,		//设置MOB用户组,此命令返回

	CARICCP_ADD_DISPATCHER				=  215,		//增加调度员
	CARICCP_DEL_DISPATCHER				=  216,		//删除调度员
	CARICCP_LST_DISPATCHER				=  217,		//查询调度员

	CARICCP_ADD_RECORD				    =  218,		//添加录音用户
	CARICCP_DEL_RECORD				    =  219,		//删除录音用户
	CARICCP_LST_RECORD				    =  220,		//查询录音用户

	CARICCP_MOB_USER_END                =  499,     //mob user模块的命令结束-----------------------

	CARICCP_ADD_EQUIP_NE				=  500,		//增加设备网元
	CARICCP_DEL_EQUIP_NE				=  501,		//删除设备网元
	CARICCP_MOD_EQUIP_NE				=  502,		//修改设备网元
	CARICCP_LST_EQUIP_NE				=  503,		//查询设备网元

	CARICCP_SET_ROUTER				    =  504,		//设置路由
	CARICCP_DEL_ROUTER                  =  505,     //删除路由
	CARICCP_LST_ROUTER				    =  506,		//查询路由

	//路由信息类命令,由其他类负责
	CARICCP_ADD_GATEWAY                 =  510,     //增加网关
	CARICCP_DEL_GATEWAY                 =  511,     //删除网关
	CARICCP_MOD_GATEWAY                 =  512,     //修改网关
	CARICCP_LST_GATEWAY                 =  513,     //查询网关


	//测试试用
	CARICCP_SEND_MESSAGE                =  999,     //发送短信息
};

////MOB用户的在线状态,参考switch的状态机
//
///*!
//\enum switch_channel_state_t
//\brief Channel States (these are the defaults, CS_SOFT_EXECUTE, CS_EXCHANGE_MEDIA, and CS_CONSUME_MEDIA are often overridden by specific apps)
//<pre>
//CS_NEW				- Channel is newly created.
//CS_INIT				- Channel has been initilized.
//CS_ROUTING			- Channel is looking for an extension to execute.
//CS_SOFT_EXECUTE		- Channel is ready to execute from 3rd party control.
//CS_EXECUTE			- Channel is executing it's dialplan.
//CS_EXCHANGE_MEDIA	- Channel is exchanging media with another channel.
//CS_PARK				- Channel is accepting media awaiting commands.
//CS_CONSUME_MEDIA	- Channel is consuming all media and dropping it.
//CS_HIBERNATE		- Channel is in a sleep state.
//CS_RESET 			- Channel is in a reset state.
//CS_HANGUP			- Channel is flagged for hangup and ready to end.
//CS_REPORTING		- Channel is ready to collect call detail.
//CS_DESTROY		    - Channel is ready to be destroyed and out of the state machine
//</pre>
//*/
//enum MOB_USER_STATUS//针对上面复杂的情况精简一下
//{
//	CARICCP_MOB_USER_NEW						=  0,		 //新建用户
//	CARICCP_MOB_USER_INIT						=  1,        //已经注册,初始化
//	CARICCP_MOB_USER_CALLING					=  2,        //用户call呼叫
//	CARICCP_MOB_USER_CONNECT					=  3,        //用户连接
//	CARICCP_MOB_USER_HANGUP                     =  4,        //挂机
//	CARICCP_MOB_USER_DESTROY					=  5,        //被DESTROY,强行终止
//};


//增加通知类型的码,针对增/删/修改等操作命令,以及在线用户的实时监控
//建议不要随便更改,否则会造成其他模块的也要多处更改
enum Notify_Type_Code
{ 
	CARICCP_NOTIFY_CODE_BEGIN				= 100000,   //开始

	CARICCP_NOTIFY_RECCONECT_SUCCEED	    = 100101,   //重连成功
	CARICCP_NOTIFY_REBOOT				    = 100102,   //重启通知

	CARICCP_NOTIFY_MOBUSER_REALTIME_CALL	= 100199,   //用户呼叫的实时监控
	CARICCP_NOTIFY_MOBUSER_STATE_CHANGED	= 100200,   //用户的在线状态改变的通知事件,需要进行细分,具体参见 MOB_USER_STATUS的定义

	CARICCP_NOTIFY_ADD_MOBUSER				= 100201,   //增加MOB用户广播通知   
	CARICCP_NOTIFY_DEL_MOBUSER				= 100202,	//删除MOB用户广播通知  
	CARICCP_NOTIFY_MOD_MOBUSER				= 100203,	//修改MOB用户广播通知  

	CARICCP_NOTIFY_ADD_EQUIP_NE				= 100500,   //增加设备网元广播通知   
	CARICCP_NOTIFY_DEL_EQUIP_NE				= 100501,	//删除设备网元广播通知  
	CARICCP_NOTIFY_MOD_EQUIP_NE				= 100502,	//修改设备网元广播通知  

	CARICCP_NOTIFY_CODE_END					= 111111,   //结束 
};

//内部错误类型
//内部错误和网络错误(范围 :1~1000),考虑进行配置,一种错误码对应特定的错误说明
enum innerError
{
	CARICCP_INNER_ERROR_CODE            =  001,     //内部错误
	CARICCP_TIMEOUT_STATE_CODE			=  002,     //命令超时
	CARICCP_NET_EXCEPTION_CODE			=  003,		//网络异常
	CARICCP_DB_ERROR_CODE               =  004,     //数据库执行失败
	CARICCP_MEMORY_ERROR_CODE           =  005,     //内存分配错误
};

//结果返回码(状态码)对照表
//内部错误和网络错误(范围 :1~1000),见上面
//命令执行错误(>1000)
enum stateCode
{
	CARICCP_ERROR_STATE_CODE			= -1,		//一般错误
	CARICCP_SUCCESS_STATE_CODE			= 0,		//成功状态
	CARICCP_RECORD_NULL_CODE            = 10,       //记录为空

	//其他的一般错误
	CARICCP_MOBUSER_EXIST_CODE          = 1001,     //用户已经存在
	CARICCP_MOBUSER_NOEXIST_CODE        = 1002,     //用户不存在
};

//模块的编号:与客户端的子模块编号不一致,和客户端的视图编号也不一一对应
//主要是根据业务功能划分,针对不同的处理class
enum MODULES_CLASS_ID
{
	BASE_MODULE_CLASSID						= 1,  //基类号
	USER_MODULE_CLASSID						= 11, //用户管理/用户状态监控模块
	ROUTE_MODULE_CLASSID					= 12, //路由管理模块
	SYSINFO_MODULE_CLASSID                  = 13, //系统信息统计模块
	NE_MANAGER_MODULE_CLASSID               = 14, //网元管理模块
	MAX_MODULE_ID							= 128,//子模块最大为128
};

//返回结果的形式,包括普通的命令返回,通知以及主动上报三种类型
enum result_type
{
	COMMON_RES_TYPE =0,//普通的命令结果返回
	NOTIFY_RES_TYPE,   //通知类型
	DOWN_RES_TYPE,     //下行,主动上报
};


#endif
