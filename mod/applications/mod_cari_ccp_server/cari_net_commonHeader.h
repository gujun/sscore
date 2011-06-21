#ifndef COMMANDHEADER_H
#define COMMANDHEADER_H 

#pragma warning(disable:4189) //局部变量已初始化但不引用
#pragma warning(disable:4245) //"=":从"stateCode"转换到“"u_int",有符号/无符号不匹配
#pragma warning(disable:4244) //"=":从"u_int"转换到"u_short",可能丢失数据


#pragma warning(disable:4786) //屏蔽map是有关warning
#pragma warning(disable:4267)
#pragma warning(disable:4996)
#pragma warning(disable:6385)

//#pragma pack(1)

//#include<afxsock.h>//包含此头文件,否则找不到socket方面的库
//另外必须放置在此位置,否则编译有问题


//替代 #include<afxsock.h>
#include <switch.h>

#include <string>
#include <vector>
#include <list>
#include <map>

#include "stdio.h"
//#include "stdarg.h"

//////////////////////////////////////////////////////////////////////////
#include "cari_net_stateInfo.h"

using namespace std;

#ifdef WIN32
#else
	#include <stdio.h>
	#include <assert.h>
	#include <iconv.h>
	#include <iostream>

	#include   <sys/types.h>   
	#include   <sys/stat.h>

	/************************************************************************/
	/*                    代码转换操作类                                    */
	/************************************************************************/
	class CodeConverter {
	private:
		iconv_t cd;

	public:
		//构造
		CodeConverter(const char *from_charset,const char *to_charset);

		//析构
		~CodeConverter();

		//转换输出
		int convert(char *inbuf,int inlen,char *outbuf,int outlen);
	};

#endif

/*using namespace boost;*/



#define OUTLEN 1024*2
#define CONVERT_OUTLEN 255

#define DEFINE_WIN32 (defined(_WIN32) || defined(WIN32))

#define CARI_CCP_MACRO_INI_FILE			"cari_ccp_macro.ini"
#define CARI_CCP_NE_XML_FILE			"cari_ccp_ne.xml"
#define CARI_CCP_OPUSER_XML_FILE        "cari_ccp_opuser.xml"
#define CARI_CCP_DISPAT_XML_FILE		"dispat.conf.xml"
#define CARI_CCP_DIAPLAN_DEF_XML_FILE	"default.xml"
#define CARI_CCP_OUT_DIAPLAN_XML_FILE	"00_out_dial.xml"

#define PORT_SERVER				        33333	//起始端口号,范围不能超出u_short,最好不要太大
#define NUM_CLIENTS				        128		//连接客户端数量
#define MAX_BIND_PORT_NUM_COUNT	        3       //端口数量
#define MAX_BIND_COUNT			        6		//最多绑定bind的次数,另外,绑定端口失败,可以考虑划分某个段,如[33333~33343]
										        //如果这个端口失败,再重尝试另外的一个端口


/*----全局变量-------------------------------------------------------------------------------------------------------------*/
extern map<string,string> global_define_vars_map;	//存放cari_ccp_macro.ini文件中宏定义变量的map
extern int  global_port_num;                        //port号
extern bool loopFlag;
extern int  global_client_num;                      //client数目

//自定义基本类型
typedef unsigned short u_short;
typedef unsigned int u_int;


/*---宏定义函数----------------------------------------------------------------------------------*/

#define CARI_CCP_VOS_NEW(classType) new classType;
#define CARI_CCP_VOS_DEL(obj)    if (NULL != obj)  \
{       \
 delete obj;  \
 obj = NULL;  \
}        

//线程休眠时间,转换成"秒"
#if defined(_WIN32) || defined(WIN32)
#define CARI_SLEEP(t)  Sleep(t)				//单位为毫秒,如果CPU的频率高,此时间间隔要"长",否则大批量分批上报的数据会出现问题!!!
#else
#define CARI_SLEEP(t)  switch_sleep(t*1000)//使用switch的提供的函数处理,单位为微秒
#endif

//关闭socket
#if defined(_WIN32) || defined(WIN32)
#define CARI_CLOSE_SOCKET(s) closesocket(s)
#else
#define CARI_CLOSE_SOCKET(s) close(s)
#endif

/*内部参数的具体结构
 */
typedef struct simpleParamStruct
{
  u_short sParaSerialNum;//参数号
  string strParaName;	//参数名字
  string strParaValue;	//参数值,先以字符串形式保存
}simpleParamStruct;



//客户端和服务器端的帧结构必须一致,否则传输转换会出现问题!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*---请求帧 :为了方面计算长度,将通用帧分割成 帧头/命名区/参数区 三个部分------------------------*/
/*请求帧"头"结构
 */
typedef struct common_CmdReqFrame_Header
{
  u_short sClientID;      //客户端的ID号
  u_short m_NEID   ;      //网元号,保留字段
  u_short sClientChildModID;    //客户端的子模块号

  char strLoginUserName[CARICCP_MAX_NAME_LENGTH];//登录用户名

  char timeStamp[CARICCP_MAX_TIME_LENGTH];  //时间戳

  //bool网络传输有问题,如果把这两个字段设置到common_CmdReqFrame_Body
  //由于timeStamp为字符数组,传输的帧转换导致数据异常???????????????????????
  //u_short      bSynchro;    //命令是否为同步命令
  //u_short      bBroadcast;     //命令是否为广播型:此命令的处理结果是否要广播给其他注册的客户端
  //此种类型为:涉及到结果变更通知
  //字段保留,考虑是否由客户端决定,还是在server端配置???????
}common_CmdReqFrame_Header;


/*请求帧的"命令区"结构
 */
typedef struct common_CmdReqFrame_Body
{
  u_short bSynchro;         //命令是否为同步命令
  /*u_short       bBroadcast;        //命令是否为广播型:此命令的处理结果是否要广播给其他注册的客户端*/

  //更改为通知类型
  u_short bNotify;     
  u_int iCmdCode;      //命令码
  //string    strCmdName;     //命令名
  char strCmdName[CARICCP_MAX_NAME_LENGTH];
}common_CmdReqFrame_Body;


/*请求帧的"参数"区域
 */
typedef struct common_CmdReqFrame_Param
{
  //参数区,是否可以设置为map进行存放??发送后接收端如何解析？？
  //先设置为"字符串"类型,内部进行封装
  //string strParamRegion;
  char strParamRegion[CARICCP_MAX_LENGTH];    
  //map<string,string> m_mapParam; //在网络上发送此结构有问题

  //预留的参数格式
  //simpleParamStruct params[128];//以128个参数
}common_CmdReqFrame_Param;


/*内部请求帧的"参数"区域
 */
typedef struct inner_CmdReqFrame_Param
{
  //map<string,string> m_mapParam;//key为命令名,value为命令值
  vector<simpleParamStruct> m_vecParam;  //命令的内部结构
  //参数的结构为simpleParamStruct类型
}inner_CmdReqFrame_Param;


/*请求帧结构,客户端和服务器端,以及模块和模块之间进行相互交互使用
 *的通用帧
 */
// typedef struct common_CmdReq_Frame 
// {
//  unsigned short   sClientID;      //客户端的ID号   
//  unsigned short   sSourceModuleID;      //源模块号
//  unsigned short   sDestModuleID;     //目的模块号
//  //string    strLoginUserName;     //登录用户名
//  char    strLoginUserName[CARICCP_MAX_NAME_LENGTH];
//  //string    timeStamp;      //时间戳
//     char    timeStamp[CARICCP_MAX_TIME_LENGTH];
//  
//  bool       bSynchro;      //命令是否为同步命令
//     
//  u_int  iCmdCode;      //命令码
//  //string    strCmdName;    //命令名
//     char    strCmdName[CARICCP_MAX_NAME_LENGTH];
// 
//  //参数区,是否可以设置为map进行存放??发送后接收端如何解析？？
//  //先设置为"字符串"类型,内部进行封装
//  //string strParamRegion;
//     char  strParamRegion[CARICCP_MAX_LENGTH];    
//  //map<string,string> m_mapParam; //在网络上发送此结构有问题
//  
// }common_CmdReq_Frame;




/*请求帧结构,客户端和服务器端,以及模块和模块之间进行相互交互使用
 *的通用帧
 */
typedef struct common_CmdReq_Frame
{
  common_CmdReqFrame_Header header;  //帧头
  common_CmdReqFrame_Body body; //命令区
  common_CmdReqFrame_Param param ;  //参数区
}common_CmdReq_Frame;



/*---响应帧 :为了方面计算长度,将通用帧分割成 帧头/参数区 两个部分------------------------*/
/*模块内部使用的请求帧结构
 */
typedef struct inner_CmdReq_Frame
{
  common_CmdReqFrame_Header header;    //帧头
  common_CmdReqFrame_Body body;      //命令区

  inner_CmdReqFrame_Param innerParam ;  //参数区,更换成map的存储方式,key为参数名,value为参数的值

  //预留一个socket字段,为的结果要发送给对应的客户端,如何和clientID对应起来?????
  //或者和三元组进行对应
  u_int socketClient;		  //SOCKET
  u_short sSourceModuleID;    //源模块号,涉及到子模块之间的交互
  u_short sDestModuleID;      //目的模块号
}inner_CmdReq_Frame;


//typedef struct ADD
//{
// //序列化必须的操作:2个步骤
////  friend class boost::serialization::access;
////  template<class Archive>
////  void serialize(Archive &ar, const unsigned int)
////  {
////   ar & BOOST_SERIALIZATION_NVP(user_names);
////  }
//
// //std::vector<std::string> user_names; 
// int i;
// //string pwd;
// char str1[10];
// char str[10];
//}ADD;




/*响应帧"头"结构
 */
typedef struct common_ResultRespFrame_Header
{
  //序列化必须的操作:2个步骤
  //  friend class boost::serialization::access;
  //  template<class Archive>
  //  void serialize(Archive &ar, const unsigned int)
  //   {
  //   ar & BOOST_SERIALIZATION_NVP(sClientID);
  //   ar & BOOST_SERIALIZATION_NVP(sClientSubModuleID);
  //   ar & BOOST_SERIALIZATION_NVP(strLoginUserName);
  //   ar & BOOST_SERIALIZATION_NVP(timeStamp);
  //   ar & BOOST_SERIALIZATION_NVP(bBroadcast);
  //   ar & BOOST_SERIALIZATION_NVP(iCmdCode);
  //   ar & BOOST_SERIALIZATION_NVP(strCmdName);
  //   ar & BOOST_SERIALIZATION_NVP(iResultCode);
  //   ar & BOOST_SERIALIZATION_NVP(strResuleDesc);
  //   ar & BOOST_SERIALIZATION_NVP(isFirst);
  //   ar & BOOST_SERIALIZATION_NVP(isLast);
  //   } 

  u_short sClientID;      //客户端的ID号 
  u_short m_NEID   ;      //网元号,保留字段
  u_short sClientChildModID;      //源模块号
  //u_short   sDestModuleID;      //目的模块号


  //  string   strLoginUserName;   //登录用户名
  //  string   timeStamp;    
  //  char   strLoginUserName[CARICCP_MAX_NAME_LENGTH];
  char timeStamp[CARICCP_MAX_TIME_LENGTH];    //时间戳:处理完命令,结果上报的时间点

  u_short bSynchro;       //命令是否为同步命令
  //bool网络传输有问题
  //  u_short   bBroadcast;    //命令是否为广播型:此命令的处理结果是否要广播给其他注册的客户端
  //           //此种类型为:涉及到结果变更通知
  /*----------------------------------*/
  //标识通知类型
  u_short bNotify;

  u_int iCmdCode;     //命令码
  //string   strCmdName;     //命令名
  char strCmdName[CARICCP_MAX_NAME_LENGTH];

  /*----------------------------------*/
  //通知类型的消息,此时代表Notify_Type_Code类型
  u_int iResultCode;     //返回码
  //string   strResuleDesc;  //结果描述
  char strResuleDesc[CARICCP_MAX_DESC];


  /* char Flag[6]; 标志
   int Index; 数据包编号
   int Size; 包长度
   int CRC; CRC校验码
   char Data[1024] 1K字节的数据包(可以变长)
  */

  //bool传世可能有问题,换成u_short
  //bool    isFirst;         //报文是否第一条(结果是否需要再继续分块发送显示)
  //bool    isLast;       //报文是否最后一条(结果是否需要再继续分块发送显示)
  u_short isFirst;      
  u_short isLast;
}common_ResultRespFrame_Header;


/*响应帧结构
*备注:
*  记录的行和行之间使用"回车换行\r\n"区分,
*  字段和字段之间使用特殊符号"〒"区分.
*/
typedef struct common_ResultResponse_Frame
{
  //序列化必须的操作:2个步骤
  //  friend class boost::serialization::access;
  //  template<class Archive>
  //  void serialize(Archive &ar, const unsigned int)
  //   {
  //    ar & BOOST_SERIALIZATION_NVP(header);
  //    ar & BOOST_SERIALIZATION_NVP(iResultNum);
  //    ar & BOOST_SERIALIZATION_NVP(m_vectTableName);
  //    ar & BOOST_SERIALIZATION_NVP(m_vectTableValue);
  //   } 


  common_ResultRespFrame_Header header;    //帧头



  /*主要针对查询类命令或者需要返回某些参数的值给客户端使用的命令
   *表头格式为 :  用户名 用户号 级别
   *结果值  :  mike〒8001〒1 
   *    : tom〒1345〒2 
   *
   *记录的行和行之间使用"回车换行\r\n"区分,
   *字段和字段之间使用特殊符号"〒"区分.
   */
  u_int iResultNum;       //结果数量,相当于显示的行数
  char strTableName[CARICCP_MAX_TABLEHEADER];
  char strTableContext[CARICCP_MAX_TABLECONTEXT];
  //std::vector<string>   m_vectTableName;   //结果名:  显示的列数,相当于表头
  //std::vector<string>   m_vectTableValue;    //结果值
}common_ResultResponse_Frame;


/*响应帧结构*/
typedef struct inner_ResultResponse_Frame
{
  common_ResultRespFrame_Header header;    //响应帧头

  //预留一个socket字段,为的结果要发送给对应的客户端,如何和clientID对应起来?????
  //或者和三元组进行对应
  u_int socketClient;
  //下面字段的意义和请求帧的意义应该相反,逆过程
  u_short sSourceModuleID;      //源模块号,涉及到子模块之间的交互
  u_short sDestModuleID;        //目的模块号


  /*结果集 :主要针对查询类命令或者需要返回某些参数的值给客户端使用的命令*/
  u_int iResultNum;      //结果数量,相当于显示的行数

  char strTableName[CARICCP_MAX_TABLEHEADER]; //表头名,格式 :用户名〒用户号〒级别

  //值填充的格式如下:
  //m_vectTableValue.push_back("小张〒123〒1\r\n");
  //m_vectTableValue.push_back("小李〒xxllxx〒0\r\n");
  vector<string> m_vectTableValue;   //结果值 :对应完整的记录
  /*用户名 用户号 级别
            mike〒8001〒1\r\n
            tom〒1345〒2\r\n
            返回客户端由其进行解析,去除〒进行处理
          */

  //增加一个保留字段 :结果是否要立即发送给client
  u_short isImmediateSend;
}inner_ResultResponse_Frame;


/*关于socket元素的三元组的结构体
*/
typedef struct socket3element
{
  u_int socketClient;  //客户端对应的socket

#if defined(_WIN32) || defined(WIN32)
  struct sockaddr_in addr; //对应客户端的ip地址等信息
#else
  struct sockaddr addr;
  //struct in_addr   addr;
#endif

  u_short sClientID;     //客户端号
}socket3element;


//--------------------------------------------------------------------------------------------------//
//公共函数
//初始化锁
void initmutex();

//异步命令处理线程的入口函数
void* asynProCmd(void* attr);


/*-------------------------------------声明一些通用的函数处理--------------------------------------*/
string getValue(string name, inner_CmdReq_Frame*& reqFrame);

/*自定义的优化的内存拷贝,主要应用于字符串的拷贝使用.
*如果source的长度过长,会被截掉
*/
void myMemcpy(char* dest, const char* source, int sizeDest);

/*判断map容器中的key是否存在
*/
//template<typename TKey,typename TValue>
//bool isValidKey(const map<TKey,TValue> &mapContainer,TKey key);
bool isValidKey();

//是否有效数字
bool isNumber(string strvalue);

//判断是由有效的IP地址
bool isValidIPV4(string strIP);

/*按照分割字符将字符串分割成多个子串
*@param strSource :源字符串
*@param strSplit  :分割字符
*@param resultVec :输出参数:存放分割后的字串
*/
//template<typename T>
void splitString(const string& strSource, const string& strSplit, vector<string>& resultVec, bool bOutNullFlag = false);

/* 过滤相同的字串,如:A,A,B,应返回A,B
*/
string filterSameStr(string strSource,string strSplit);

/*自定义取出字符首尾空格的方法
*/
string& trim(string& str);

string& trimRight(string& str);

char* trimChar(char* chBuf);

bool isNullStr(const char* chBuf);

/*使用指定的字串替换字符(串)
*/
string& replace_all(string& str, const   string& old_mark, const   string& new_mark);  

/*判断两个字符串是否相等,忽略大小写
*/
bool isEqualStr(const string str1, const string str2);

string stradd(char* str, char c);

/*格式化新的字符串,方面打印日志使用
*/
string formatStr(const char* sourceStr, ...);

/*把整形转换成bool类型
*/
bool intToBool(int ivalue);

/*把int类型的值转换为string类型
*/
string intToString(int ivalue);

/*把string类型转换为int类型
*/
int stringToInt(const char* strValue);

/*转换成小写字符
*/
void stringToLower(string &strSource);

/*转换成小写
*/
char tolower(char t);  

/*转换成大写
*/
char toupper(char t);   

/*转换成百分数
*/
string convertToPercent(string strMolecule,string strDenominator,bool bContainedDot=false);

/*获得本地的时间戳
*/
char* getLocalTime(char* strTime);

/*格式化结果
*/
void formatResult(const string& msg, map<string, string>& mapRes);

/*长整数进行增加
*/
void addLongInteger(char operand1[],char operand2[],char result[]);   

/*封装0开头的字符串
*/
string encapSpecialStr(string sourceStr,int oriLen);

/*封装结果,根据vec中存放的信息进行封装
*/
string encapResultFromVec(string splitkey, vector<string>& vec);

/*从map容器中获得对应的值
*/
string cari_net_map_getvalue(map<string, string>& mapRes, string key, string& value);

/*从vec容器中删除某个元素,如果元素不存在则返回-1
*/
int eraseVecElement(vector<string>& vec, string element);

/*封装标准的每一行的xml的内容,防止格式出现混乱
*/
string encapSingleXmlString(string strsource, int ispacnum);

/*封装标准的xml内容,主要用于写入到xml文件中,防止格式出现混乱
*/
string encapStandXMLContext(string sourceContext);

/*获得宏定义变量的值,具体参见配置文件cari_ccp_macro.ini
*/
/*char**/ string getValueOfDefinedVar(string strVar);

/*根据vector,获得某个索引值的字段名
*/
string getValueOfVec(int index,vector<string>& vec);

/*根据string,获得某个索引值的字段名
*/
string getValueOfStr(string splitkey,int index,string strSource);

/*根据索引号插入元素到vector容器中
*/
void insertElmentToVec(int index,string elment,vector<string> &vec);


/*-------------------------------------帧结构的初始化----------------------------------------------*/
/*初始化请求帧的结构
*/
void initCommonRequestFrame(common_CmdReq_Frame*& common_reqFrame);

void initInnerRequestFrame(inner_CmdReq_Frame*& innner_reqFrame);

/*初始化响应帧的结构
*/
void initCommonResponseFrame(common_ResultResponse_Frame*& common_respFrame);

/*初始化内部响应帧,并从请求帧拷贝有关信息
*/
void initInnerResponseFrame(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_respFrame);

/*初始化内部响应帧,从switch_event_t拷贝有关信息
*/
void initInnerResponseFrame(inner_ResultResponse_Frame*& inner_respFrame, switch_event_t* event);

//字符串转换成simpleParamStruct结构进行存放
void covertStrToParamStruct(char* strParameter, vector<simpleParamStruct>& vecParam, const char* splitflag = CARICCP_PARAM_SPLIT_FLAG);

void setValueToReqFrame(common_CmdReq_Frame& reqFrame);

void initSocket3element(socket3element& element);

/*初始化event对象
*/
void initEventObj(inner_CmdReq_Frame*& reqFrame, switch_event_t*& event);

//从数据库中获得记录
int getRecordTableFromDB(switch_core_db_t * db,const char * sql,vector<string>& resultVec,int * nrow,int * ncolumn, char ** errmsg);

/*初始化"主动上报的响应帧"
*/
void initDownRespondFrame(inner_ResultResponse_Frame *&inner_respFrame,int clientModuleID,int cmdCode);
/*初始化命令类型的结构
*/
//void initCmdStruct(commandStruct &cmdStruct);

/*初始化参数类型的结构
*/
//void initParameterStruct(parameterStruct &paraStruct);

#if defined(_WIN32) || defined(WIN32)
	//编码转换
	void UTF_8ToUnicode(wchar_t* pOut,char *pText);
	void UnicodeToUTF_8(char* pOut,wchar_t* pText);
	void UnicodeToGB2312(char* pOut,wchar_t uData);  
	void Gb2312ToUnicode(wchar_t* pOut,char *gbBuffer);
	void GB2312ToUTF_8(string& pOut,char *pText, int pLen);
	void UTF_8ToGB2312(string &pOut, char *pText, int pLen);
#else
	#include   <iconv.h>     

	//代码转换:从一种编码转为另一种编码   
	int  code_convert(char   *from_charset,char   *to_charset,char   *inbuf,int   inlen,char   *outbuf,int   outlen);

	//UNICODE码转为GB2312码   
	int u2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//GB2312码转为UNICODE码   
	int g2u(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//GB2312码转为UNICODE码   
	int utf2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//GB2312码转为UTF-8码   
	int g2utf(char *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//ascii码转为UTF-8码   
	int asc2utf(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen); 

	//转换帧的格式
	void convertFrameCode(common_ResultResponse_Frame *&common_respFrame,char *sourceCode,char *dstSource);

#endif

/************************************************************************/
/*																		*/
/************************************************************************/

//统一的接口函数
#ifdef __cplusplus
extern "C"
{
#endif

  extern int  shutdownFlag;  //平台是否按下shutdown命令操作

  //////////////////////////////////////////////////////////////////////////
  //声明通用函数,负责通知cari net server.由其他的模块负责直接调用
  //初始化环境信息
  void cari_net_init();

  //初始加载变量定义的文件vars.conf.ini
  bool loadVarsConfigCmd();

  void setLoopFlag(switch_bool_t bf);

  //获得有效的字符串,linux系统的工具获得的结果有可能含有不识别的字符
  void cari_common_getValidStr(string &strSouce);

  //linux C程序中获取shell脚本输出(如获取system命令输出)
  void cari_common_getSysCmdResult(const char *syscmd,string &strRes);

  //执行系统命令,不需要返回结果
  int cari_common_excuteSysCmd(const char *syscmd);

  /*用户的状态的改变通知Cari同步更改
  */
  //void cari_switch_state_machine_notify(const char *mobUserID,
  //               const char *mobUserState,
  //               const char *mobUserInfo);

  ////数据库的连接
  //SWITCH_DECLARE(bool) cari_switch_db_pro(char *sql,
  //             switch_odbc_handle_t *master_odbc_handle,
  //             SQLHSTMT stmt,
  //             char *errorMsg);

  //通用的函数
  void cari_common_convertStrToEvent(const char* context, const char* split, switch_event_t* event);

  //使用字符为分割符,特殊字符出现问题???
  //SWITCH_DECLARE(void) cari_common_convertStrToEvent(const char *context, 
  //               char split,
  //               switch_event_t *event);

  //自定义取出字符首尾空格/tab/\r\n的方法
  char* cari_common_trimStr(char* ibuf); //C++文件调用此函数出现问题????????

  //是否含有中文字符
  bool cari_common_isExsitedChinaChar(const char* ibuf);

  //创建新的目录
  bool cari_common_creatDir(char* dir, const char** err);

  //删除目录
  bool cari_common_delDir(char* dir, const char** err);

  //修改目录
  bool cari_common_modDir(char* sourceDir, char* destDir, const char** err);

  //创建新的xml文件
  bool cari_common_creatXMLFile(char* file, char* filecontext, const char** err);
  
  //删除xml文件
  bool cari_common_delFile(char* file, const char** err);

  //复制文件
  bool cari_common_cpFile(char* sourceDir, char* destDir, const char** err);

  //文件(目录)是否存在
  bool cari_common_isExistedFile(char* filename);

  //获得用户组
  void cari_common_getMobUserGroupInfo(char* mobusername, const char** mobGroupInfo);

  //根据文件的内容,获得对应的xml结构,封装switch_xml_parse_fd()函数
  //此返回的xml结构必须要释放
  switch_xml_t cari_common_parseXmlFromFile(const char* file);

  //解析xml文件,类似switch_xml_parse_file()函数
  //switch_xml_t cari_common_copy_parse_file(const char* sourcefile, const char* detestfile);

  //重新设置main root对应的xml文件的结构
  int cari_common_reset_mainroot(const char** err);

  //使用容器进行存放函数的句柄
  //switch_hash_t *cari_func_hash;


#ifdef __cplusplus
}
#endif






#endif
