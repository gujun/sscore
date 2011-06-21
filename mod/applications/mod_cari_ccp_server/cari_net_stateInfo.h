#ifndef  STATEINFO_H
#define  STATEINFO_H


//�Ƿ���Ҫ����ת��
#define CODE_TRANSFER_FLAG 1


/*---�궨�庯��--------------------------------------------------------*/
#define CARICCP_MAX(num1,num2) (num1>num2 ? num1:num2)							//���������
#define CARICCP_MIN(num1,num2) (num1>num2 ? num2:num1)							//������С��

//////////////////////////////////////////////////////////////////////////
//����ʹ�õĺ궨��
#define CARI_NET_PRO_RESULT				   "cari_net_proResult"
#define CARI_NET_PRO_NOTIFY				   "cari_net_proNotify"
#define CARI_NET_SOFIA_CMD_HANDLER	       "cari_net_sofia_cmd_handler"


#define CARICCP_SOCKET_BUFFER_SIZE			1024*4 //4K��
#define CARICCP_MAX_CMD_NUM					0xFFFF //�첽������������������
#define CARICCP_MAX_MODULE_NUM				32     //֧����ģ������

#define CARICCP_MAX_SHORT					0x7FFF //short���͵����ֵ  32767
#define CARICCP_MAX_INT						0xFFFFFFFF //int���͵����ֵ(0xFFFF???)

#define CARICCP_MAX_CREATTHREAD_NUM			3     //�����̵߳�������
#define CARICCP_MAX_CPU_WAIT_TIME			10    //�����������ʱ����(��λ:����),����һ�·�����,��ֹ����

#define CARICCP_MAX_NAME_LENGTH				32
#define CARICCP_MAX_TIME_LENGTH				20
#define CARICCP_MAX_DESC					200
#define CARICCP_MAX_LENGTH					1024
#define CARICCP_MAX_TABLEHEADER				150   //��ͷ����󳤶�
#define CARICCP_MAX_TABLECONTEXT			CARICCP_SOCKET_BUFFER_SIZE \
	- sizeof(common_ResultRespFrame_Header) \
	- sizeof(u_int) \
	- CARICCP_MAX_TABLEHEADER

#define CARICCP_MAX_STR_LEN(str1,str2)      CARICCP_MAX(strlen(str1),strlen(str2)) //�����ִ�,����Ǹ��ִ��ĳ���           


#define MAX_INTLENGTH                       100     //int�������ݵ���󳤶� ���ķ�Χ��0~4294967295,0~65535

#define CARICCP_MAX_ERRRORFRAME_COUNT       10      //���յ��Ĵ���֡�Ĵ���
#define CARICCP_MAX_CLIENT_NUM              128     //���յ��Ĵ���֡�Ĵ���

//////////////////////////////////////////////////////////////////////////
//�û�����й���Ϣ
#define CARICCP_MAX_GROUPS_NUM              1024
#define CARICCP_MAX_MOBUSER_IN_GROUP_NUM    1024
#define CARICCP_MAX_GATEWAY_NUM             100

#define CARICCP_STRING_LENGTH_16			16
#define CARICCP_STRING_LENGTH_32			32
#define CARICCP_STRING_LENGTH_64			64
#define CARICCP_STRING_LENGTH_128			128
#define CARICCP_STRING_LENGTH_256			256
#define CARICCP_STRING_LENGTH_512			512

#define CARICCP_NULL_MARK					""						//���ִ�
#define CARICCP_SPACEBAR					" "						//�ո��
#define CARICCP_ENTER_KEY					"\r\n"					//�س����з�

//�س���,����ϵͳ�Ĳ�ͬ,�漰���ַ�����Ҳ��ͬ
//#if defined(_WIN32) || defined(WIN32)
//#define CARICCP_NEWLINE_LF				"\r"
//#else
#define CARICCP_NEWLINE_CR					"\r"
#define CARICCP_NEWLINE_LF					"\n"
//#endif

#define CARICCP_TAB_MARK					"\t"					//tab��
#define CARICCP_EQUAL_MARK					"="						//�Ⱥ�
#define CARICCP_CHAR_EQUAL_MARK				'='					    //�ַ� :�Ⱥ�
#define CARICCP_COMMA_MARK					","                     //����
#define CARICCP_CHAR_COMMA_MARK				','                     //�ַ����� :����
#define CARICCP_QUO_MARK                    "\""                    //����,ʹ��ת���
#define CARICCP_COMMA_FLAG					","
#define CARICCP_BLACKSLASH_MARK		        "\\"                    //��б��

#ifdef CODE_TRANSFER_FLAG
        //utf8ת��,���鲻Ҫ���������˫�ֽ��ַ�,������Ҫת��һ��,�ٷ�װ��¼...
	    #define CARICCP_SPECIAL_SPLIT_FLAG  "/\\"/*"��""$"*/        //ʹ�ô��������,��ʶ"������"֮��ļ��,��ҪӦ���ڲ�ѯ������صĽ��
#else
	    #define CARICCP_SPECIAL_SPLIT_FLAG	"��"                    //ʹ�ô��������,��ʶ"������"֮��ļ��,��ҪӦ���ڲ�ѯ������صĽ��
#endif

#define CARICCP_CHAR_SPECIAL_SPLIT_FLAG	   /* '��'*/'$'				//�ַ�����:ʹ�ô��������,��ʶ"������"֮��ļ��,��ҪӦ���ڲ�ѯ������صĽ��
#define CARICCP_PARAM_SPLIT_FLAG            CARICCP_COMMA_FLAG

#define CARICCP_LEFT_SQUARE_BRACKETS		"["						//������
#define CARICCP_RIGHT_SQUARE_BRACKETS		"]"						//�ҷ�����
#define CARICCP_LEFT_ROUND_BRACKETS		    "("						//��Բ����
#define CARICCP_RIGHT_ROUND_BRACKETS		")"						//��Բ����
#define CARICCP_LEFT_BIG_BRACKETS		    "{"						//�������
#define CARICCP_RIGHT_BIG_BRACKETS		    "}"						//�Ҵ�����
#define CARICCP_COLON_MARK					":"                     //ð��
#define CARICCP_SEMICOLON_MARK              ";"						//�ֺ�
#define CARICCP_OR_MARK						"|"						//��
#define CARICCP_XOR_MARK					"^"						//���
#define CARICCP_DOLLAR_MARK					"$"						//��Ԫ��
#define CARICCP_LINK_MARK					"-"						//-
#define CARICCP_EMPTY_MARK					"" 
#define CARICCP_MUTIL_VALUE_MARK			"&"						//��ֵ��ʶ
#define CARICCP_PERCENT_MARK                "%"                     //�ٷֱȺ�
#define CARICCP_DOT_MARK                    "."                     //dot���


//////////////////////////////////////////////////////////////////////////
//profile������
#define CARICCP_SIP_PROFILES_INTERNAL       "internal"
#define CARICCP_XML_FILE_TYPE               ".xml"
#define CARI_CCP_EVENT_NAME                 "cariccp:state"

//"����֡"�漰���ؼ���ʶ
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


//Ȩ������:����ԽС,����Խ��,Ŀǰֻ��3,4,5����
#define USERPRIORITY_SPECIAL                 0 //�����ļ���,ר�Ÿ�����Աʹ��,����:���к������Ȩ��
#define USERPRIORITY_INTERNATIONAL	         1
#define USERPRIORITY_DOMESTIC		         2
#define USERPRIORITY_OUT			         3
#define USERPRIORITY_LOCAL			         4
#define USERPRIORITY_ONLY_DISPATCH	         5
#define DEFAULT_USERPRIORITY	             USERPRIORITY_LOCAL

//�����·��:�������̨
#define ROUTER_TO_DISPATCH_DESK              100

//�������Ӧ��
enum CmdCode
{
	//�ڲ�������:ģ���ڲ�ֱ�ӷ���
	CARICCP_QUERY_CLIENTID				=  99,      //��ѯ���clientID
	CARICCP_LOGIN						=  100,     //��¼
	CARICCP_RECONNECT                   =  101,     //����
	CARICCP_REBOOT				        =  102,     //����
	CARICCP_SHAKEHAND    		        =  103,     //����
	CARICCP_QUERY_MOB_USER				=  150,		//��ѯMOB�û�,��Ҫ��Լ����������ѯ,������ֻ��Ҫ�����û���

	CARICCP_QUERY_SYSINFO		        =  160,		//��ѯlinuxϵͳ�İ汾��Ϣ
	CARICCP_QUERY_UPTIME                =  161,     //��ѯlinuxϵͳ���û���
	CARICCP_QUERY_CPU_INFO				=  162,		//��ѯϵͳCPU��Ϣ
	CARICCP_QUERY_MEMORY_INFO			=  163,     //��ѯϵͳ�ڴ�ʹ��״��
	CARICCP_QUERY_DISK_INFO				=  164,     //��ѯ����ʹ�����
	CARICCP_QUERY_CPU_RATE				=  165,     //��ѯCPU rate


	//һ����������:�������ļ������õ�����
	CARICCP_MOB_USER_BEGIN              =  190,     //mob userģ���������ʼ-----------------------

	//һ����������:�������ļ������õ�����
	CARICCP_ADD_OP_USER					=  196,		//���Ӳ����û�
	CARICCP_DEL_OP_USER					=  197,		//ɾ�������û�
	CARICCP_MOD_OP_USER					=  198,		//�޸Ĳ����û�
	CARICCP_LST_OP_USER					=  199,		//��ѯ�����û�

	CARICCP_ADD_MOB_USER				=  201,		//����MOB�û�
	CARICCP_DEL_MOB_USER				=  202,		//ɾ��MOB�û�
	CARICCP_MOD_MOB_USER				=  203,		//�޸�MOB�û�
	CARICCP_LST_MOB_USER				=  204,		//��ѯMOB�û�,��������û���,˵���ȶ�����Ϣ
	CARICCP_LST_MOB_STATE			    =  205,		//��ѯMOB�û�������״̬��Ϣ

	CARICCP_LST_USER_CALL_TRANSFER      =  207,     //��ѯ��ǰ�󶨵ĺ���ת�Ƶ���Ϣ
	CARICCP_SET_USER_CALL_TRANSFER      =  208,     //�û��������ת��
	CARICCP_UNBIND_CALL_TRANSFER        =  209,     //����û��������ת��

	CARICCP_ADD_MOB_GROUP			    =  210,		//����MOB�û���
	CARICCP_DEL_MOB_GROUP				=  211,		//ɾ��MOB�û���
	CARICCP_MOD_MOB_GROUP				=  212,		//�޸�MOB�û���
	CARICCP_LST_MOB_GROUP				=  213,		//��ѯMOB�û���,�������
	CARICCP_SET_MOB_GROUP				=  214,		//����MOB�û���,�������

	CARICCP_ADD_DISPATCHER				=  215,		//���ӵ���Ա
	CARICCP_DEL_DISPATCHER				=  216,		//ɾ������Ա
	CARICCP_LST_DISPATCHER				=  217,		//��ѯ����Ա

	CARICCP_ADD_RECORD				    =  218,		//���¼���û�
	CARICCP_DEL_RECORD				    =  219,		//ɾ��¼���û�
	CARICCP_LST_RECORD				    =  220,		//��ѯ¼���û�

	CARICCP_MOB_USER_END                =  499,     //mob userģ����������-----------------------

	CARICCP_ADD_EQUIP_NE				=  500,		//�����豸��Ԫ
	CARICCP_DEL_EQUIP_NE				=  501,		//ɾ���豸��Ԫ
	CARICCP_MOD_EQUIP_NE				=  502,		//�޸��豸��Ԫ
	CARICCP_LST_EQUIP_NE				=  503,		//��ѯ�豸��Ԫ

	CARICCP_SET_ROUTER				    =  504,		//����·��
	CARICCP_DEL_ROUTER                  =  505,     //ɾ��·��
	CARICCP_LST_ROUTER				    =  506,		//��ѯ·��

	//·����Ϣ������,�������ฺ��
	CARICCP_ADD_GATEWAY                 =  510,     //��������
	CARICCP_DEL_GATEWAY                 =  511,     //ɾ������
	CARICCP_MOD_GATEWAY                 =  512,     //�޸�����
	CARICCP_LST_GATEWAY                 =  513,     //��ѯ����


	//��������
	CARICCP_SEND_MESSAGE                =  999,     //���Ͷ���Ϣ
};

////MOB�û�������״̬,�ο�switch��״̬��
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
//enum MOB_USER_STATUS//������渴�ӵ��������һ��
//{
//	CARICCP_MOB_USER_NEW						=  0,		 //�½��û�
//	CARICCP_MOB_USER_INIT						=  1,        //�Ѿ�ע��,��ʼ��
//	CARICCP_MOB_USER_CALLING					=  2,        //�û�call����
//	CARICCP_MOB_USER_CONNECT					=  3,        //�û�����
//	CARICCP_MOB_USER_HANGUP                     =  4,        //�һ�
//	CARICCP_MOB_USER_DESTROY					=  5,        //��DESTROY,ǿ����ֹ
//};


//����֪ͨ���͵���,�����/ɾ/�޸ĵȲ�������,�Լ������û���ʵʱ���
//���鲻Ҫ������,������������ģ���ҲҪ�ദ����
enum Notify_Type_Code
{ 
	CARICCP_NOTIFY_CODE_BEGIN				= 100000,   //��ʼ

	CARICCP_NOTIFY_RECCONECT_SUCCEED	    = 100101,   //�����ɹ�
	CARICCP_NOTIFY_REBOOT				    = 100102,   //����֪ͨ

	CARICCP_NOTIFY_MOBUSER_REALTIME_CALL	= 100199,   //�û����е�ʵʱ���
	CARICCP_NOTIFY_MOBUSER_STATE_CHANGED	= 100200,   //�û�������״̬�ı��֪ͨ�¼�,��Ҫ����ϸ��,����μ� MOB_USER_STATUS�Ķ���

	CARICCP_NOTIFY_ADD_MOBUSER				= 100201,   //����MOB�û��㲥֪ͨ   
	CARICCP_NOTIFY_DEL_MOBUSER				= 100202,	//ɾ��MOB�û��㲥֪ͨ  
	CARICCP_NOTIFY_MOD_MOBUSER				= 100203,	//�޸�MOB�û��㲥֪ͨ  

	CARICCP_NOTIFY_ADD_EQUIP_NE				= 100500,   //�����豸��Ԫ�㲥֪ͨ   
	CARICCP_NOTIFY_DEL_EQUIP_NE				= 100501,	//ɾ���豸��Ԫ�㲥֪ͨ  
	CARICCP_NOTIFY_MOD_EQUIP_NE				= 100502,	//�޸��豸��Ԫ�㲥֪ͨ  

	CARICCP_NOTIFY_CODE_END					= 111111,   //���� 
};

//�ڲ���������
//�ڲ�������������(��Χ :1~1000),���ǽ�������,һ�ִ������Ӧ�ض��Ĵ���˵��
enum innerError
{
	CARICCP_INNER_ERROR_CODE            =  001,     //�ڲ�����
	CARICCP_TIMEOUT_STATE_CODE			=  002,     //���ʱ
	CARICCP_NET_EXCEPTION_CODE			=  003,		//�����쳣
	CARICCP_DB_ERROR_CODE               =  004,     //���ݿ�ִ��ʧ��
	CARICCP_MEMORY_ERROR_CODE           =  005,     //�ڴ�������
};

//���������(״̬��)���ձ�
//�ڲ�������������(��Χ :1~1000),������
//����ִ�д���(>1000)
enum stateCode
{
	CARICCP_ERROR_STATE_CODE			= -1,		//һ�����
	CARICCP_SUCCESS_STATE_CODE			= 0,		//�ɹ�״̬
	CARICCP_RECORD_NULL_CODE            = 10,       //��¼Ϊ��

	//������һ�����
	CARICCP_MOBUSER_EXIST_CODE          = 1001,     //�û��Ѿ�����
	CARICCP_MOBUSER_NOEXIST_CODE        = 1002,     //�û�������
};

//ģ��ı��:��ͻ��˵���ģ���Ų�һ��,�Ϳͻ��˵���ͼ���Ҳ��һһ��Ӧ
//��Ҫ�Ǹ���ҵ���ܻ���,��Բ�ͬ�Ĵ���class
enum MODULES_CLASS_ID
{
	BASE_MODULE_CLASSID						= 1,  //�����
	USER_MODULE_CLASSID						= 11, //�û�����/�û�״̬���ģ��
	ROUTE_MODULE_CLASSID					= 12, //·�ɹ���ģ��
	SYSINFO_MODULE_CLASSID                  = 13, //ϵͳ��Ϣͳ��ģ��
	NE_MANAGER_MODULE_CLASSID               = 14, //��Ԫ����ģ��
	MAX_MODULE_ID							= 128,//��ģ�����Ϊ128
};

//���ؽ������ʽ,������ͨ�������,֪ͨ�Լ������ϱ���������
enum result_type
{
	COMMON_RES_TYPE =0,//��ͨ������������
	NOTIFY_RES_TYPE,   //֪ͨ����
	DOWN_RES_TYPE,     //����,�����ϱ�
};


#endif
