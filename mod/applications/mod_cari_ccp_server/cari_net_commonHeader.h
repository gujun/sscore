#ifndef COMMANDHEADER_H
#define COMMANDHEADER_H 

#pragma warning(disable:4189) //�ֲ������ѳ�ʼ����������
#pragma warning(disable:4245) //"=":��"stateCode"ת������"u_int",�з���/�޷��Ų�ƥ��
#pragma warning(disable:4244) //"=":��"u_int"ת����"u_short",���ܶ�ʧ����


#pragma warning(disable:4786) //����map���й�warning
#pragma warning(disable:4267)
#pragma warning(disable:4996)
#pragma warning(disable:6385)

//#pragma pack(1)

//#include<afxsock.h>//������ͷ�ļ�,�����Ҳ���socket����Ŀ�
//�����������ڴ�λ��,�������������


//��� #include<afxsock.h>
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
	/*                    ����ת��������                                    */
	/************************************************************************/
	class CodeConverter {
	private:
		iconv_t cd;

	public:
		//����
		CodeConverter(const char *from_charset,const char *to_charset);

		//����
		~CodeConverter();

		//ת�����
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

#define PORT_SERVER				        33333	//��ʼ�˿ں�,��Χ���ܳ���u_short,��ò�Ҫ̫��
#define NUM_CLIENTS				        128		//���ӿͻ�������
#define MAX_BIND_PORT_NUM_COUNT	        3       //�˿�����
#define MAX_BIND_COUNT			        6		//����bind�Ĵ���,����,�󶨶˿�ʧ��,���Կ��ǻ���ĳ����,��[33333~33343]
										        //�������˿�ʧ��,���س��������һ���˿�


/*----ȫ�ֱ���-------------------------------------------------------------------------------------------------------------*/
extern map<string,string> global_define_vars_map;	//���cari_ccp_macro.ini�ļ��к궨�������map
extern int  global_port_num;                        //port��
extern bool loopFlag;
extern int  global_client_num;                      //client��Ŀ

//�Զ����������
typedef unsigned short u_short;
typedef unsigned int u_int;


/*---�궨�庯��----------------------------------------------------------------------------------*/

#define CARI_CCP_VOS_NEW(classType) new classType;
#define CARI_CCP_VOS_DEL(obj)    if (NULL != obj)  \
{       \
 delete obj;  \
 obj = NULL;  \
}        

//�߳�����ʱ��,ת����"��"
#if defined(_WIN32) || defined(WIN32)
#define CARI_SLEEP(t)  Sleep(t)				//��λΪ����,���CPU��Ƶ�ʸ�,��ʱ����Ҫ"��",��������������ϱ������ݻ��������!!!
#else
#define CARI_SLEEP(t)  switch_sleep(t*1000)//ʹ��switch���ṩ�ĺ�������,��λΪ΢��
#endif

//�ر�socket
#if defined(_WIN32) || defined(WIN32)
#define CARI_CLOSE_SOCKET(s) closesocket(s)
#else
#define CARI_CLOSE_SOCKET(s) close(s)
#endif

/*�ڲ������ľ���ṹ
 */
typedef struct simpleParamStruct
{
  u_short sParaSerialNum;//������
  string strParaName;	//��������
  string strParaValue;	//����ֵ,�����ַ�����ʽ����
}simpleParamStruct;



//�ͻ��˺ͷ������˵�֡�ṹ����һ��,������ת�����������!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*---����֡ :Ϊ�˷�����㳤��,��ͨ��֡�ָ�� ֡ͷ/������/������ ��������------------------------*/
/*����֡"ͷ"�ṹ
 */
typedef struct common_CmdReqFrame_Header
{
  u_short sClientID;      //�ͻ��˵�ID��
  u_short m_NEID   ;      //��Ԫ��,�����ֶ�
  u_short sClientChildModID;    //�ͻ��˵���ģ���

  char strLoginUserName[CARICCP_MAX_NAME_LENGTH];//��¼�û���

  char timeStamp[CARICCP_MAX_TIME_LENGTH];  //ʱ���

  //bool���紫��������,������������ֶ����õ�common_CmdReqFrame_Body
  //����timeStampΪ�ַ�����,�����֡ת�����������쳣???????????????????????
  //u_short      bSynchro;    //�����Ƿ�Ϊͬ������
  //u_short      bBroadcast;     //�����Ƿ�Ϊ�㲥��:������Ĵ������Ƿ�Ҫ�㲥������ע��Ŀͻ���
  //��������Ϊ:�漰��������֪ͨ
  //�ֶα���,�����Ƿ��ɿͻ��˾���,������server������???????
}common_CmdReqFrame_Header;


/*����֡��"������"�ṹ
 */
typedef struct common_CmdReqFrame_Body
{
  u_short bSynchro;         //�����Ƿ�Ϊͬ������
  /*u_short       bBroadcast;        //�����Ƿ�Ϊ�㲥��:������Ĵ������Ƿ�Ҫ�㲥������ע��Ŀͻ���*/

  //����Ϊ֪ͨ����
  u_short bNotify;     
  u_int iCmdCode;      //������
  //string    strCmdName;     //������
  char strCmdName[CARICCP_MAX_NAME_LENGTH];
}common_CmdReqFrame_Body;


/*����֡��"����"����
 */
typedef struct common_CmdReqFrame_Param
{
  //������,�Ƿ��������Ϊmap���д��??���ͺ���ն���ν�������
  //������Ϊ"�ַ���"����,�ڲ����з�װ
  //string strParamRegion;
  char strParamRegion[CARICCP_MAX_LENGTH];    
  //map<string,string> m_mapParam; //�������Ϸ��ʹ˽ṹ������

  //Ԥ���Ĳ�����ʽ
  //simpleParamStruct params[128];//��128������
}common_CmdReqFrame_Param;


/*�ڲ�����֡��"����"����
 */
typedef struct inner_CmdReqFrame_Param
{
  //map<string,string> m_mapParam;//keyΪ������,valueΪ����ֵ
  vector<simpleParamStruct> m_vecParam;  //������ڲ��ṹ
  //�����ĽṹΪsimpleParamStruct����
}inner_CmdReqFrame_Param;


/*����֡�ṹ,�ͻ��˺ͷ�������,�Լ�ģ���ģ��֮������໥����ʹ��
 *��ͨ��֡
 */
// typedef struct common_CmdReq_Frame 
// {
//  unsigned short   sClientID;      //�ͻ��˵�ID��   
//  unsigned short   sSourceModuleID;      //Դģ���
//  unsigned short   sDestModuleID;     //Ŀ��ģ���
//  //string    strLoginUserName;     //��¼�û���
//  char    strLoginUserName[CARICCP_MAX_NAME_LENGTH];
//  //string    timeStamp;      //ʱ���
//     char    timeStamp[CARICCP_MAX_TIME_LENGTH];
//  
//  bool       bSynchro;      //�����Ƿ�Ϊͬ������
//     
//  u_int  iCmdCode;      //������
//  //string    strCmdName;    //������
//     char    strCmdName[CARICCP_MAX_NAME_LENGTH];
// 
//  //������,�Ƿ��������Ϊmap���д��??���ͺ���ն���ν�������
//  //������Ϊ"�ַ���"����,�ڲ����з�װ
//  //string strParamRegion;
//     char  strParamRegion[CARICCP_MAX_LENGTH];    
//  //map<string,string> m_mapParam; //�������Ϸ��ʹ˽ṹ������
//  
// }common_CmdReq_Frame;




/*����֡�ṹ,�ͻ��˺ͷ�������,�Լ�ģ���ģ��֮������໥����ʹ��
 *��ͨ��֡
 */
typedef struct common_CmdReq_Frame
{
  common_CmdReqFrame_Header header;  //֡ͷ
  common_CmdReqFrame_Body body; //������
  common_CmdReqFrame_Param param ;  //������
}common_CmdReq_Frame;



/*---��Ӧ֡ :Ϊ�˷�����㳤��,��ͨ��֡�ָ�� ֡ͷ/������ ��������------------------------*/
/*ģ���ڲ�ʹ�õ�����֡�ṹ
 */
typedef struct inner_CmdReq_Frame
{
  common_CmdReqFrame_Header header;    //֡ͷ
  common_CmdReqFrame_Body body;      //������

  inner_CmdReqFrame_Param innerParam ;  //������,������map�Ĵ洢��ʽ,keyΪ������,valueΪ������ֵ

  //Ԥ��һ��socket�ֶ�,Ϊ�Ľ��Ҫ���͸���Ӧ�Ŀͻ���,��κ�clientID��Ӧ����?????
  //���ߺ���Ԫ����ж�Ӧ
  u_int socketClient;		  //SOCKET
  u_short sSourceModuleID;    //Դģ���,�漰����ģ��֮��Ľ���
  u_short sDestModuleID;      //Ŀ��ģ���
}inner_CmdReq_Frame;


//typedef struct ADD
//{
// //���л�����Ĳ���:2������
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




/*��Ӧ֡"ͷ"�ṹ
 */
typedef struct common_ResultRespFrame_Header
{
  //���л�����Ĳ���:2������
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

  u_short sClientID;      //�ͻ��˵�ID�� 
  u_short m_NEID   ;      //��Ԫ��,�����ֶ�
  u_short sClientChildModID;      //Դģ���
  //u_short   sDestModuleID;      //Ŀ��ģ���


  //  string   strLoginUserName;   //��¼�û���
  //  string   timeStamp;    
  //  char   strLoginUserName[CARICCP_MAX_NAME_LENGTH];
  char timeStamp[CARICCP_MAX_TIME_LENGTH];    //ʱ���:����������,����ϱ���ʱ���

  u_short bSynchro;       //�����Ƿ�Ϊͬ������
  //bool���紫��������
  //  u_short   bBroadcast;    //�����Ƿ�Ϊ�㲥��:������Ĵ������Ƿ�Ҫ�㲥������ע��Ŀͻ���
  //           //��������Ϊ:�漰��������֪ͨ
  /*----------------------------------*/
  //��ʶ֪ͨ����
  u_short bNotify;

  u_int iCmdCode;     //������
  //string   strCmdName;     //������
  char strCmdName[CARICCP_MAX_NAME_LENGTH];

  /*----------------------------------*/
  //֪ͨ���͵���Ϣ,��ʱ����Notify_Type_Code����
  u_int iResultCode;     //������
  //string   strResuleDesc;  //�������
  char strResuleDesc[CARICCP_MAX_DESC];


  /* char Flag[6]; ��־
   int Index; ���ݰ����
   int Size; ������
   int CRC; CRCУ����
   char Data[1024] 1K�ֽڵ����ݰ�(���Ա䳤)
  */

  //bool��������������,����u_short
  //bool    isFirst;         //�����Ƿ��һ��(����Ƿ���Ҫ�ټ����ֿ鷢����ʾ)
  //bool    isLast;       //�����Ƿ����һ��(����Ƿ���Ҫ�ټ����ֿ鷢����ʾ)
  u_short isFirst;      
  u_short isLast;
}common_ResultRespFrame_Header;


/*��Ӧ֡�ṹ
*��ע:
*  ��¼���к���֮��ʹ��"�س�����\r\n"����,
*  �ֶκ��ֶ�֮��ʹ���������"��"����.
*/
typedef struct common_ResultResponse_Frame
{
  //���л�����Ĳ���:2������
  //  friend class boost::serialization::access;
  //  template<class Archive>
  //  void serialize(Archive &ar, const unsigned int)
  //   {
  //    ar & BOOST_SERIALIZATION_NVP(header);
  //    ar & BOOST_SERIALIZATION_NVP(iResultNum);
  //    ar & BOOST_SERIALIZATION_NVP(m_vectTableName);
  //    ar & BOOST_SERIALIZATION_NVP(m_vectTableValue);
  //   } 


  common_ResultRespFrame_Header header;    //֡ͷ



  /*��Ҫ��Բ�ѯ�����������Ҫ����ĳЩ������ֵ���ͻ���ʹ�õ�����
   *��ͷ��ʽΪ :  �û��� �û��� ����
   *���ֵ  :  mike��8001��1 
   *    : tom��1345��2 
   *
   *��¼���к���֮��ʹ��"�س�����\r\n"����,
   *�ֶκ��ֶ�֮��ʹ���������"��"����.
   */
  u_int iResultNum;       //�������,�൱����ʾ������
  char strTableName[CARICCP_MAX_TABLEHEADER];
  char strTableContext[CARICCP_MAX_TABLECONTEXT];
  //std::vector<string>   m_vectTableName;   //�����:  ��ʾ������,�൱�ڱ�ͷ
  //std::vector<string>   m_vectTableValue;    //���ֵ
}common_ResultResponse_Frame;


/*��Ӧ֡�ṹ*/
typedef struct inner_ResultResponse_Frame
{
  common_ResultRespFrame_Header header;    //��Ӧ֡ͷ

  //Ԥ��һ��socket�ֶ�,Ϊ�Ľ��Ҫ���͸���Ӧ�Ŀͻ���,��κ�clientID��Ӧ����?????
  //���ߺ���Ԫ����ж�Ӧ
  u_int socketClient;
  //�����ֶε����������֡������Ӧ���෴,�����
  u_short sSourceModuleID;      //Դģ���,�漰����ģ��֮��Ľ���
  u_short sDestModuleID;        //Ŀ��ģ���


  /*����� :��Ҫ��Բ�ѯ�����������Ҫ����ĳЩ������ֵ���ͻ���ʹ�õ�����*/
  u_int iResultNum;      //�������,�൱����ʾ������

  char strTableName[CARICCP_MAX_TABLEHEADER]; //��ͷ��,��ʽ :�û������û��Ũ�����

  //ֵ���ĸ�ʽ����:
  //m_vectTableValue.push_back("С�Ũ�123��1\r\n");
  //m_vectTableValue.push_back("С�xxllxx��0\r\n");
  vector<string> m_vectTableValue;   //���ֵ :��Ӧ�����ļ�¼
  /*�û��� �û��� ����
            mike��8001��1\r\n
            tom��1345��2\r\n
            ���ؿͻ���������н���,ȥ�������д���
          */

  //����һ�������ֶ� :����Ƿ�Ҫ�������͸�client
  u_short isImmediateSend;
}inner_ResultResponse_Frame;


/*����socketԪ�ص���Ԫ��Ľṹ��
*/
typedef struct socket3element
{
  u_int socketClient;  //�ͻ��˶�Ӧ��socket

#if defined(_WIN32) || defined(WIN32)
  struct sockaddr_in addr; //��Ӧ�ͻ��˵�ip��ַ����Ϣ
#else
  struct sockaddr addr;
  //struct in_addr   addr;
#endif

  u_short sClientID;     //�ͻ��˺�
}socket3element;


//--------------------------------------------------------------------------------------------------//
//��������
//��ʼ����
void initmutex();

//�첽������̵߳���ں���
void* asynProCmd(void* attr);


/*-------------------------------------����һЩͨ�õĺ�������--------------------------------------*/
string getValue(string name, inner_CmdReq_Frame*& reqFrame);

/*�Զ�����Ż����ڴ濽��,��ҪӦ�����ַ����Ŀ���ʹ��.
*���source�ĳ��ȹ���,�ᱻ�ص�
*/
void myMemcpy(char* dest, const char* source, int sizeDest);

/*�ж�map�����е�key�Ƿ����
*/
//template<typename TKey,typename TValue>
//bool isValidKey(const map<TKey,TValue> &mapContainer,TKey key);
bool isValidKey();

//�Ƿ���Ч����
bool isNumber(string strvalue);

//�ж�������Ч��IP��ַ
bool isValidIPV4(string strIP);

/*���շָ��ַ����ַ����ָ�ɶ���Ӵ�
*@param strSource :Դ�ַ���
*@param strSplit  :�ָ��ַ�
*@param resultVec :�������:��ŷָ����ִ�
*/
//template<typename T>
void splitString(const string& strSource, const string& strSplit, vector<string>& resultVec, bool bOutNullFlag = false);

/* ������ͬ���ִ�,��:A,A,B,Ӧ����A,B
*/
string filterSameStr(string strSource,string strSplit);

/*�Զ���ȡ���ַ���β�ո�ķ���
*/
string& trim(string& str);

string& trimRight(string& str);

char* trimChar(char* chBuf);

bool isNullStr(const char* chBuf);

/*ʹ��ָ�����ִ��滻�ַ�(��)
*/
string& replace_all(string& str, const   string& old_mark, const   string& new_mark);  

/*�ж������ַ����Ƿ����,���Դ�Сд
*/
bool isEqualStr(const string str1, const string str2);

string stradd(char* str, char c);

/*��ʽ���µ��ַ���,�����ӡ��־ʹ��
*/
string formatStr(const char* sourceStr, ...);

/*������ת����bool����
*/
bool intToBool(int ivalue);

/*��int���͵�ֵת��Ϊstring����
*/
string intToString(int ivalue);

/*��string����ת��Ϊint����
*/
int stringToInt(const char* strValue);

/*ת����Сд�ַ�
*/
void stringToLower(string &strSource);

/*ת����Сд
*/
char tolower(char t);  

/*ת���ɴ�д
*/
char toupper(char t);   

/*ת���ɰٷ���
*/
string convertToPercent(string strMolecule,string strDenominator,bool bContainedDot=false);

/*��ñ��ص�ʱ���
*/
char* getLocalTime(char* strTime);

/*��ʽ�����
*/
void formatResult(const string& msg, map<string, string>& mapRes);

/*��������������
*/
void addLongInteger(char operand1[],char operand2[],char result[]);   

/*��װ0��ͷ���ַ���
*/
string encapSpecialStr(string sourceStr,int oriLen);

/*��װ���,����vec�д�ŵ���Ϣ���з�װ
*/
string encapResultFromVec(string splitkey, vector<string>& vec);

/*��map�����л�ö�Ӧ��ֵ
*/
string cari_net_map_getvalue(map<string, string>& mapRes, string key, string& value);

/*��vec������ɾ��ĳ��Ԫ��,���Ԫ�ز������򷵻�-1
*/
int eraseVecElement(vector<string>& vec, string element);

/*��װ��׼��ÿһ�е�xml������,��ֹ��ʽ���ֻ���
*/
string encapSingleXmlString(string strsource, int ispacnum);

/*��װ��׼��xml����,��Ҫ����д�뵽xml�ļ���,��ֹ��ʽ���ֻ���
*/
string encapStandXMLContext(string sourceContext);

/*��ú궨�������ֵ,����μ������ļ�cari_ccp_macro.ini
*/
/*char**/ string getValueOfDefinedVar(string strVar);

/*����vector,���ĳ������ֵ���ֶ���
*/
string getValueOfVec(int index,vector<string>& vec);

/*����string,���ĳ������ֵ���ֶ���
*/
string getValueOfStr(string splitkey,int index,string strSource);

/*���������Ų���Ԫ�ص�vector������
*/
void insertElmentToVec(int index,string elment,vector<string> &vec);


/*-------------------------------------֡�ṹ�ĳ�ʼ��----------------------------------------------*/
/*��ʼ������֡�Ľṹ
*/
void initCommonRequestFrame(common_CmdReq_Frame*& common_reqFrame);

void initInnerRequestFrame(inner_CmdReq_Frame*& innner_reqFrame);

/*��ʼ����Ӧ֡�Ľṹ
*/
void initCommonResponseFrame(common_ResultResponse_Frame*& common_respFrame);

/*��ʼ���ڲ���Ӧ֡,��������֡�����й���Ϣ
*/
void initInnerResponseFrame(inner_CmdReq_Frame*& inner_reqFrame, inner_ResultResponse_Frame*& inner_respFrame);

/*��ʼ���ڲ���Ӧ֡,��switch_event_t�����й���Ϣ
*/
void initInnerResponseFrame(inner_ResultResponse_Frame*& inner_respFrame, switch_event_t* event);

//�ַ���ת����simpleParamStruct�ṹ���д��
void covertStrToParamStruct(char* strParameter, vector<simpleParamStruct>& vecParam, const char* splitflag = CARICCP_PARAM_SPLIT_FLAG);

void setValueToReqFrame(common_CmdReq_Frame& reqFrame);

void initSocket3element(socket3element& element);

/*��ʼ��event����
*/
void initEventObj(inner_CmdReq_Frame*& reqFrame, switch_event_t*& event);

//�����ݿ��л�ü�¼
int getRecordTableFromDB(switch_core_db_t * db,const char * sql,vector<string>& resultVec,int * nrow,int * ncolumn, char ** errmsg);

/*��ʼ��"�����ϱ�����Ӧ֡"
*/
void initDownRespondFrame(inner_ResultResponse_Frame *&inner_respFrame,int clientModuleID,int cmdCode);
/*��ʼ���������͵Ľṹ
*/
//void initCmdStruct(commandStruct &cmdStruct);

/*��ʼ���������͵Ľṹ
*/
//void initParameterStruct(parameterStruct &paraStruct);

#if defined(_WIN32) || defined(WIN32)
	//����ת��
	void UTF_8ToUnicode(wchar_t* pOut,char *pText);
	void UnicodeToUTF_8(char* pOut,wchar_t* pText);
	void UnicodeToGB2312(char* pOut,wchar_t uData);  
	void Gb2312ToUnicode(wchar_t* pOut,char *gbBuffer);
	void GB2312ToUTF_8(string& pOut,char *pText, int pLen);
	void UTF_8ToGB2312(string &pOut, char *pText, int pLen);
#else
	#include   <iconv.h>     

	//����ת��:��һ�ֱ���תΪ��һ�ֱ���   
	int  code_convert(char   *from_charset,char   *to_charset,char   *inbuf,int   inlen,char   *outbuf,int   outlen);

	//UNICODE��תΪGB2312��   
	int u2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//GB2312��תΪUNICODE��   
	int g2u(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//GB2312��תΪUNICODE��   
	int utf2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//GB2312��תΪUTF-8��   
	int g2utf(char *inbuf,size_t   inlen,char   *outbuf,size_t   outlen);

	//ascii��תΪUTF-8��   
	int asc2utf(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen); 

	//ת��֡�ĸ�ʽ
	void convertFrameCode(common_ResultResponse_Frame *&common_respFrame,char *sourceCode,char *dstSource);

#endif

/************************************************************************/
/*																		*/
/************************************************************************/

//ͳһ�Ľӿں���
#ifdef __cplusplus
extern "C"
{
#endif

  extern int  shutdownFlag;  //ƽ̨�Ƿ���shutdown�������

  //////////////////////////////////////////////////////////////////////////
  //����ͨ�ú���,����֪ͨcari net server.��������ģ�鸺��ֱ�ӵ���
  //��ʼ��������Ϣ
  void cari_net_init();

  //��ʼ���ر���������ļ�vars.conf.ini
  bool loadVarsConfigCmd();

  void setLoopFlag(switch_bool_t bf);

  //�����Ч���ַ���,linuxϵͳ�Ĺ��߻�õĽ���п��ܺ��в�ʶ����ַ�
  void cari_common_getValidStr(string &strSouce);

  //linux C�����л�ȡshell�ű����(���ȡsystem�������)
  void cari_common_getSysCmdResult(const char *syscmd,string &strRes);

  //ִ��ϵͳ����,����Ҫ���ؽ��
  int cari_common_excuteSysCmd(const char *syscmd);

  /*�û���״̬�ĸı�֪ͨCariͬ������
  */
  //void cari_switch_state_machine_notify(const char *mobUserID,
  //               const char *mobUserState,
  //               const char *mobUserInfo);

  ////���ݿ������
  //SWITCH_DECLARE(bool) cari_switch_db_pro(char *sql,
  //             switch_odbc_handle_t *master_odbc_handle,
  //             SQLHSTMT stmt,
  //             char *errorMsg);

  //ͨ�õĺ���
  void cari_common_convertStrToEvent(const char* context, const char* split, switch_event_t* event);

  //ʹ���ַ�Ϊ�ָ��,�����ַ���������???
  //SWITCH_DECLARE(void) cari_common_convertStrToEvent(const char *context, 
  //               char split,
  //               switch_event_t *event);

  //�Զ���ȡ���ַ���β�ո�/tab/\r\n�ķ���
  char* cari_common_trimStr(char* ibuf); //C++�ļ����ô˺�����������????????

  //�Ƿ��������ַ�
  bool cari_common_isExsitedChinaChar(const char* ibuf);

  //�����µ�Ŀ¼
  bool cari_common_creatDir(char* dir, const char** err);

  //ɾ��Ŀ¼
  bool cari_common_delDir(char* dir, const char** err);

  //�޸�Ŀ¼
  bool cari_common_modDir(char* sourceDir, char* destDir, const char** err);

  //�����µ�xml�ļ�
  bool cari_common_creatXMLFile(char* file, char* filecontext, const char** err);
  
  //ɾ��xml�ļ�
  bool cari_common_delFile(char* file, const char** err);

  //�����ļ�
  bool cari_common_cpFile(char* sourceDir, char* destDir, const char** err);

  //�ļ�(Ŀ¼)�Ƿ����
  bool cari_common_isExistedFile(char* filename);

  //����û���
  void cari_common_getMobUserGroupInfo(char* mobusername, const char** mobGroupInfo);

  //�����ļ�������,��ö�Ӧ��xml�ṹ,��װswitch_xml_parse_fd()����
  //�˷��ص�xml�ṹ����Ҫ�ͷ�
  switch_xml_t cari_common_parseXmlFromFile(const char* file);

  //����xml�ļ�,����switch_xml_parse_file()����
  //switch_xml_t cari_common_copy_parse_file(const char* sourcefile, const char* detestfile);

  //��������main root��Ӧ��xml�ļ��Ľṹ
  int cari_common_reset_mainroot(const char** err);

  //ʹ���������д�ź����ľ��
  //switch_hash_t *cari_func_hash;


#ifdef __cplusplus
}
#endif






#endif
