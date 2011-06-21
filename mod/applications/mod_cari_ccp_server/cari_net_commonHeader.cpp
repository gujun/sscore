#pragma warning(disable:4996)
#include "cari_net_commonHeader.h"

#if defined(_WIN32) || defined(WIN32)
	#include <direct.h>

#else //linuxϵͳʹ��

	#include <sys/types.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	

///************************************************************************/
///*                    ����ת��������                                    */
///************************************************************************/
//class CodeConverter {
//private:
//	iconv_t cd;
//public:
//	//����
//	CodeConverter(const char *from_charset,const char *to_charset) {
//		cd = iconv_open(to_charset,from_charset);
//	}
//
//	//����
//	~CodeConverter() {
//		iconv_close(cd);
//	}
//
//	//ת�����
//	int convert(char *inbuf,int inlen,char *outbuf,int outlen) {
//		char **pin = &inbuf;
//		char **pout = &outbuf;
//
//		memset(outbuf,0,outlen);
//		return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
//	}
//};


/************************************************************************/
/*                    ����ת��������                                    */
/************************************************************************/

//����
CodeConverter::CodeConverter(const char *from_charset,const char *to_charset) 
{
	cd = iconv_open(to_charset,from_charset);
}

//����
CodeConverter::~CodeConverter() 
{
	iconv_close(cd);
}

//ת�����
int CodeConverter::convert(char *inbuf,int inlen,char *outbuf,int outlen) 
{
	char **pin = &inbuf;
	char **pout = &outbuf;

	memset(outbuf,0,outlen);
	return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
}


#endif

#include "cari_net_commonHeader.h"
#include "CMsgProcess.h"
#include "cari_net_stateInfo.h"

//ȫ�ֱ����Ķ���
bool loopFlag	= true;
int  global_port_num = PORT_SERVER; 
int  global_client_num = 0;
int  shutdownFlag; //ƽ̨�Ƿ���shutdown�������

//�궨���map
map<string,string> global_define_vars_map;

pthread_mutex_t mutex_syscmd ;//pthread�Դ��Ļ�����

//--------------------------------------------------------------------------------------------------//
//��������

//��ʼ����
void initmutex()
{
	pthread_mutex_init(&mutex_syscmd, NULL);
}

//�첽������̵߳���ں���
void * asynProCmd(void *attr)
{
	int					iResult			= CARICCP_SUCCESS_STATE_CODE;
	inner_CmdReq_Frame	*inner_reqFrame	= NULL;

	//ʹ��ѭ����ʶ����
	while (loopFlag) {
		//�Ӷ�����ȡ��һ��Ԫ��(��ǰ��ŵ�"����֡")
		iResult = CMsgProcess::getInstance()->popCmdFromQueue(inner_reqFrame);

		if (CARICCP_ERROR_STATE_CODE == iResult || NULL == inner_reqFrame) {
			//����"����֡"
			CARI_CCP_VOS_DEL(inner_reqFrame);
			continue;
		}

		//��������֡,�ڲ�������뱣֤˳����,��:ͬ������
		//�½�һ��"��Ӧ֡",������һЩ��Ҫ�Ĳ���
		inner_ResultResponse_Frame	*inner_respFrame	= CARI_CCP_VOS_NEW(inner_ResultResponse_Frame);

		//��ʼ���ڲ���Ӧ֡,����ͨ������֡�����й����ݸ���Ӧ֡
		initInnerResponseFrame(inner_reqFrame, inner_respFrame);

		//ֱ�ӽ����߼����� :��Զ����е�ÿ��������ϻ���ͬ������
		CMsgProcess::getInstance()->syncProCmd(inner_reqFrame, inner_respFrame);

		//�����ֱ֡�ӷ��͸���Ӧ�Ŀͻ���,������֡�����ڷ��õ�������
		//������ɿͻ��˿��ܻ�ȴ��ϳ���һ��ʱ������Ӧ���
		CMsgProcess::getInstance()->commonSendRespMsg(inner_respFrame);

		//���� "����֡"
		CARI_CCP_VOS_DEL(inner_reqFrame);

		//���� "��Ӧ֡"
		CARI_CCP_VOS_DEL(inner_respFrame);
	}


//	if (NULL != attr) {
//		pthread_attr_t	attr_Thd;
//		attr_Thd = *((pthread_attr_t *) attr); //shutdown��ʱ���������??
//
//#if defined(_WIN32) || defined(WIN32)
//		pthread_exit(attr_Thd);//�߳��˳�?????
//#else
//#endif
//	} else {
//		pthread_exit(NULL);
//	}

	return NULL;	  //returnҲ�ᵼ�±��������ڵ��߳��˳�
}

/*��ʼ������֡
*/
void initCommonRequestFrame(common_CmdReq_Frame *&common_reqFrame)
{
	memset(common_reqFrame, 0, sizeof(common_CmdReq_Frame));
}

/*��ʼ���ڲ�"����֡"
*/
void initInnerRequestFrame(inner_CmdReq_Frame *&innner_reqFrame)
{
	memset(innner_reqFrame, 0, sizeof(inner_CmdReq_Frame));
}

void initCommonResponseFrame(common_ResultResponse_Frame *&common_respFrame)
{
	memset(common_respFrame, 0, sizeof(common_ResultResponse_Frame));
}

/*��ʼ���ڲ�"��Ӧ֡",�������й�����
*/
void initInnerResponseFrame(inner_CmdReq_Frame *&inner_reqFrame, //�ڲ�����֡
							inner_ResultResponse_Frame *&inner_respFrame) //������� :��ʼ����"�ڲ���Ӧ֡"
{
	//��ʼ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����Ӧ֡��ֵ
	inner_respFrame->header.sClientID = inner_reqFrame->header.sClientID;
	inner_respFrame->header.m_NEID = inner_reqFrame->header.m_NEID;
	inner_respFrame->header.sClientChildModID = inner_reqFrame->header.sClientChildModID;
	inner_respFrame->header.iCmdCode = inner_reqFrame->body.iCmdCode;
	inner_respFrame->header.bSynchro = inner_reqFrame->body.bSynchro;
	inner_respFrame->header.bNotify = inner_reqFrame->body.bNotify;

	inner_respFrame->header.isFirst = true;
	inner_respFrame->header.isLast = true;

	//����Ƿ���Ҫ�������ظ�client
	inner_respFrame->isImmediateSend = true; 

	//�����ؼ���Ϣ����
	inner_respFrame->socketClient = inner_reqFrame->socketClient;

	// 	memcpy(inner_respFrame->header.strLoginUserName,
	// 		inner_reqFrame->header.strLoginUserName,
	// 		sizeof(inner_respFrame->header.strLoginUserName));
	// 	memcpy(inner_respFrame->header.timeStamp,
	// 		inner_reqFrame->header.timeStamp,
	// 		sizeof(inner_respFrame->header.timeStamp));

	//ʹ��myMemcpy�滻memcpy
	myMemcpy(inner_respFrame->header.strCmdName, inner_reqFrame->body.strCmdName, sizeof(inner_respFrame->header.strCmdName));
}

/*��ʼ���ڲ���Ӧ֡,��switch_event_t�����й���Ϣ,��Ҫ������"֪ͨ"���͵��¼�����
*/
void initInnerResponseFrame(inner_ResultResponse_Frame *&inner_respFrame, switch_event_t *event)
{
	////����"������/������"�ȹؼ���Ϣ,��ǰ�����µĲ����ֶ�,����
	//const char	*socketid	= switch_event_get_header(event, SOCKET_ID);
	const char	*clientId	= switch_event_get_header(event, CLIENT_ID);
	//const char	*neId		= switch_event_get_header(event, CARICCP_NE_ID);
	//const char	*csubmodId	= switch_event_get_header(event, CARICCP_SUBCLIENTMOD_ID);
	const char	*cmdCode	= switch_event_get_header(event, CARICCP_CMDCODE);
	const char	*cmdName	= switch_event_get_header(event, CARICCP_CMDNAME);
	const char	*bnotify	= switch_event_get_header(event, CARICCP_NOTIFY);		//֪ͨ��ʶ

	//���صĽ����Ϣ.ע:��������ڴ˳�����
	const char	*returnCode	= switch_event_get_header(event, CARICCP_RETURNCODE);	//��������/֪ͨ��
	const char	*resuleDesc	= switch_event_get_header(event, CARICCP_RESULTDESC);

	////���ڲ���Ӧ֡���и�ֵ

	//����ı�ʶӰ�쵽�ͻ��˵���ʾ����
	inner_respFrame->header.isFirst = true;
	inner_respFrame->header.isLast = true;
	inner_respFrame->isImmediateSend = true; //����Ƿ���Ҫ�������ظ�client!!!!!!!!!!!!!!!!!!!!!!


	////�����ؽ���йؼ�����Ϣ���õ��ڲ���Ӧ֡��
	//inner_respFrame->socketClient = (u_int) stringToInt(socketid);
	//inner_respFrame->header.sClientChildModID = (u_short) stringToInt(csubmodId);
	//inner_respFrame->header.iCmdCode = stringToInt(cmdCode);
	inner_respFrame->header.bNotify = (u_short) stringToInt(bnotify);    //֪ͨ��ʶ,true��false
	inner_respFrame->header.iResultCode = stringToInt(returnCode);       //֪ͨ��򷵻���

	if (NULL != clientId) {
		inner_respFrame->header.sClientID = (u_short) stringToInt(clientId);
	}
	if (NULL != cmdName) {
		memcpy(inner_respFrame->header.strCmdName, cmdName, sizeof(inner_respFrame->header.strCmdName));
	}
	if (NULL != cmdCode){
		inner_respFrame->header.iCmdCode = stringToInt(cmdCode);
	}
	if (NULL != resuleDesc) {
		memcpy(inner_respFrame->header.strResuleDesc, resuleDesc, sizeof(inner_respFrame->header.strResuleDesc));
	}
}

/*�������͵�ת��
*@param  strParam :
*@param  vecParam :�������,��Ų���֡�Ľṹ
*@param  splitflag :�ָ��
*/
void covertStrToParamStruct(char *strParameter, 
							vector<simpleParamStruct> &vecParam,//�������:��Ų����ṹ������
							const char *splitflag)
{
	string strParam(strParameter); //������

	int				iPara;
	string			strTmp, strName;
	string			strS, strBegin, strMid, strValue;
	vector< string>	resultVec;

	size_t			beginIndex, midIndex, endIndex;

	bool	bTmpFlag	= false;//��ʶ:��ʾ������ֵ�Ƿ�������

	//��һ��������뿼��,������������ݺ��ж���","�����???
	//�� :name ="ab,cd";
	splitString(strParam, splitflag, resultVec);//����֮��ķָ����","
	//���ʽ�� :name:1 = stu , password:2 =123

	for (vector< string>::iterator iter = resultVec.begin(); iter != resultVec.end(); ++iter) {
		/*------------------------------------------*/
		simpleParamStruct	paramStruct;//�Ƿ���Ҫnew???

		strS = *iter;
		trim(strS);  //1:name = stu CARICCP_EQUAL_MARK

		//����������
		beginIndex = strS.find_first_of(CARICCP_EQUAL_MARK);		 //"="��ʶ��
		if (string::npos != beginIndex) {
			strBegin = strS.substr(0, beginIndex);			//������:������
			strValue = strS.substr(beginIndex + 1);			//����ֵ

			//�Ѳ����źͲ�������������
			midIndex = strBegin.find_last_of(CARICCP_COLON_MARK);	//":"
			if (string::npos != midIndex) {
				//������
				strTmp = strBegin.substr(0, midIndex);		//������:������
				trim(strTmp);
				iPara = atoi(strTmp.c_str());
				//������
				strTmp = strBegin.substr(midIndex + 1);
				trim(strTmp);

				//
				paramStruct.strParaName = strTmp;
				paramStruct.sParaSerialNum = iPara;
			} else {
				trim(strBegin);
				//ֻ�в�����
				paramStruct.strParaName = strBegin;
				paramStruct.sParaSerialNum = CARICCP_MAX_SHORT;		//��Чֵ
			}

			trim(strValue);

			//�����"�ı�"��"����"���͵Ĳ���,��ֵ���ŷ�װ
			//��: name="tiom"
			beginIndex = strValue.find_first_of(CARICCP_QUO_MARK);	//��һ��"\""��ʶ��
			if (string::npos != beginIndex) {
				endIndex = strValue.find_last_of(CARICCP_QUO_MARK);	//���һ��"\""��ʶ��
				if (string::npos != endIndex) {
					//�п������,��: "aaade
					if (beginIndex < endIndex) {
						//��ö�Ӧ��������ֵ
						strValue = strValue.substr(beginIndex + 1, endIndex - beginIndex - 1);	 //����ֵ
					} else {
						//��ö�Ӧ��������ֵ,ȥ��������װ�ĵ�һ������
						strValue = strValue.substr(beginIndex + 1);	  //����ֵ
					}
				}//end ������"\""

				//�������,ֱ�ӵ��ɲ�����ֵ����
			}

			paramStruct.strParaValue = strValue;

			vecParam.push_back(paramStruct);				//��ŵ������б�������
		}
		/*---�˷�֧�Ĵ���Ƚ�����,��Բ����� "ֵ"�к��ж��ź����ŵ��������---------*/
		//end ����ֵ������"="��,��ʱ���Ե������ϸ�������ֵ����
		else {
			//��ȡ���ϸ�����(������)
			vector< simpleParamStruct>::reference	lastParam	= vecParam.back();

			if (bTmpFlag) {
				lastParam.strParaValue.append(CARICCP_QUO_MARK);
				bTmpFlag = false;
			}

			//����ϸ�������Ӧ��ֵ
			lastParam.strParaValue.append(splitflag);

			//�����"�ı�"��"����"���͵Ĳ���,��ֵ����������,��Ҫȥ��
			endIndex = strS.find_last_of(CARICCP_QUO_MARK);//"\""
			if (string::npos != endIndex) {
				string	strTmp	= "";
				size_t	iTmp	= endIndex + 1;
				if (strS.length() == iTmp) {
					bTmpFlag = true;
					strTmp = strS.substr(0, endIndex);	 //����ֵ,��λ��0��ʼ,ȡǰendIndex��ֵ
					lastParam.strParaValue.append(strTmp);
				}
			} else {
				lastParam.strParaValue.append(strS);
			}
		}
	}
}

/*��ʼ��Ԫ��
 */
void initSocket3element(socket3element &element)
{
	//�˴���Ҫ��ʼ���ͻ��˺�
	element.sClientID = CARICCP_MAX_SHORT;
}

/*��ʼ���¼�����
*/
void initEventObj(inner_CmdReq_Frame *&reqFrame, switch_event_t *&event)
{
	//��Ҫ���漰��SOCKET/�ͻ��˺�/�Ƿ�㲥�ȹؼ���Ϣ
	string	strSocketID				= intToString(reqFrame->socketClient);
	string	strClientID				= intToString(reqFrame->header.sClientID);
	string	strClientSubModuleID	= intToString(reqFrame->header.sClientChildModID);
	string	strSynchro				= intToString(reqFrame->body.bSynchro);
	string	strCmdCode				= intToString(reqFrame->body.iCmdCode);
	string	strCmdName				= reqFrame->body.strCmdName;
	string	strNotify				= intToString(reqFrame->body.bNotify);

	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, SOCKET_ID, strSocketID.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CLIENT_ID, strClientID.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_SUBCLIENTMOD_ID, strClientSubModuleID.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_CMDCODE, strCmdCode.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_CMDNAME, strCmdName.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_SYNCHRO, strSynchro.c_str());
	switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, CARICCP_NOTIFY, strNotify.c_str());
}

///*���ݿ���õĻص�����,Ϊʲô����ִ��???
//*/
//static int db_callback(void *pArg, int argc, char **argv, char **columnNames)
//{
//	char **r = (char **) pArg;
//	*r = strdup(argv[0]);
//
//	return 0;
//}

/*�����ݿ��л�ü�¼
*/
int getRecordTableFromDB(switch_core_db_t *db,		//�ɵ����߸����ͷ�
						 const char *sql, 
						 vector<string>& resultVec,	//����Ľ�����ݱ�
						 int *nrow,                 //��¼������
						 int *ncolumn,              //��¼������
						 char **errMsg) //Ŀǰ�ݲ�ʹ��
{
	int iRes = CARICCP_ERROR_STATE_CODE;
	char *errmsg = NULL;
	char **resultp = NULL;
	int iCount = 0;

#ifdef SWITCH_HAVE_ODBC
#else
#endif

	//switch_core_db_exec(db, sql, db_callback, NULL, &errmsg);//���ûص������ķ�ʽ,�����ɹ�???
	switch_core_db_get_table(db,sql,&resultp,nrow,ncolumn,&errmsg);
	if (errmsg){
		goto end;
	}

	//��õļ�¼������
	//ע��:���������ݴ�nrow��ʼ
	iCount = (*nrow) + (*nrow) * (*ncolumn);
	for(int i = *nrow; i < iCount; i++){
		char *chRes = resultp[i];
		resultVec.push_back(chRes);
	}

	//while(resultp && *resultp){//��ѭ����������(*nrow) * (*ncolumn),��ʱ��ָ�����������
	//	char *r1 = *resultp;
	//	//printf("%s\n",r1);
	//	resultp++;
	//}

	//�ͷŲ�ѯ��õļ�¼�ڴ�
	switch_core_db_free_table(resultp);
	iRes = CARICCP_SUCCESS_STATE_CODE;

end:
	switch_core_db_free(errmsg);
	return iRes;
}

/*��ʼ��"�����ϱ�����Ӧ֡"
*/
void initDownRespondFrame(inner_ResultResponse_Frame *&inner_respFrame, 
						  int clientModuleID,	 
						  int cmdCode)
{
	//��ʼ��"�ڲ���Ӧ֡"�ṹ��
	memset(inner_respFrame, 0, sizeof(inner_ResultResponse_Frame));

	//��ʼ����ֵ,������"֪ͨ���͵ı�ʶ"��"֪ͨ��"
	//����ı�ʶӰ�쵽�ͻ��˵���ʾ����
	inner_respFrame->header.isFirst = true;
	inner_respFrame->header.isLast = true;
	inner_respFrame->isImmediateSend = true; //����Ƿ���Ҫ�������ظ�client!!!!!!!!!!!!!!!!!!!!!!

	////�����ؽ���йؼ�����Ϣ���õ��ڲ���Ӧ֡��
	//inner_respFrame->socketClient = (u_int) stringToInt(socketid);
	inner_respFrame->header.sClientChildModID = (u_short) clientModuleID;//�ͻ��˸������ģ��
	inner_respFrame->header.iCmdCode = cmdCode;
	inner_respFrame->header.bNotify = 0;    //֪ͨ��ʶ,true��false
	inner_respFrame->header.iResultCode = 0;//������
}

/*���ݲ�������ö�Ӧ�Ĳ���ֵ
*/
string getValue(string name, inner_CmdReq_Frame *&reqFrame)
{
	string										strV	= "";
	simpleParamStruct							simpleParam;
	vector<simpleParamStruct>::const_iterator	iter	= reqFrame->innerParam.m_vecParam.begin();
	for (; iter != reqFrame->innerParam.m_vecParam.end(); ++iter) {
		simpleParam = *iter;
		if (isEqualStr(name, simpleParam.strParaName))//���ҵ���������
		{
			strV = simpleParam.strParaValue;
			break;
		}
	}

	return strV;
}

/*�Զ�����Ż����ڴ濽��,��ҪӦ�����ַ����Ŀ���ʹ��.
*���source�ĳ��ȹ���,�ᱻ�ص�
*/
void myMemcpy(char *dest, const char *source, int sizeDest)
{
	if(!source){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "myMemcpy error,source is NULL \n");
		memcpy(dest, "", 1);
		return;
	}

	int	sizeSource	= (int) strlen(source);
	int	size		= CARICCP_MIN(sizeDest, sizeSource);
	memcpy(dest, source, size);
}

/*�ж�map�����е�key�Ƿ����
*/
// template<typename TKey,typename TValue>
// bool isValidKey(const map<TKey,TValue> &mapContainer,TKey key)
// {
// 	map<TKey,TValue>::const_iterator it = mapContainer.find(key);
// 	if (it != mapContainer.end())
// 	{
// 		return true;//keyԪ�ش���
// 	}
// 	
// 	return false;
// }

bool isValidKey()
{
	return true;
}

/*�ж��ִ��Ƿ�Ϊ��Ч����
*/
bool isNumber(string strvalue)
{
	char ch;
	int iLength = strvalue.length();
	for (int i = 0; i < iLength; i++) {
		ch = strvalue.at(i);
		if ('0' > ch || '9' < ch) {
			return false;
		}
	}

	return true;
}

/*�ж�������Ч��IP��ַ
*/
bool isValidIPV4(string strIP)
{
	vector<string> vec;
	string str1,str2,str3,str4;
	int iFiled1,iFiled2,iFiled3,iFiled4;

	trim(strIP);
	if (0 == strIP.length()){
		return false;
	}
	splitString(strIP,CARICCP_DOT_MARK,vec,true);
	if (4 != vec.size()){//ip��ַ��Ч
		return false;
	}

	str1 = getValueOfVec(0,vec);
	str2 = getValueOfVec(1,vec);
	str3 = getValueOfVec(2,vec);
	str4 = getValueOfVec(3,vec);
	trim(str1);
	trim(str2);
	trim(str3);
	trim(str4);
	if (0 == str1.length()
		|| 0 == str2.length()
		|| 0 == str3.length()
		|| 0 == str4.length()){
		return false;
	}

	//�ж��Ƿ���Ч����,λ�����ܳ���3,��ֵ���ܳ���255
	if (!isNumber(str1.c_str()) 
		|| !isNumber(str2.c_str())
		|| !isNumber(str3.c_str())
		|| !isNumber(str4.c_str())){
		return false;
	}
	if ( 3<str1.length() 
		|| 3<str2.length()
		|| 3<str3.length()
		|| 3<str4.length()){
		return false;
	}

	//ÿ����ε�ȡֵ��Χ
	iFiled1 = stringToInt(str1.c_str());
	iFiled2 = stringToInt(str2.c_str());
	iFiled3 = stringToInt(str3.c_str());
	iFiled4 = stringToInt(str4.c_str());
	if (255 < iFiled1
		|| 255 < iFiled2
		|| 255 < iFiled3
		|| 255 < iFiled4){
		return false;
	}

	//��һ���ֶβ���Ϊ0
	if(0 == iFiled1){
		return false;
	}
	
	return true;
}

/*���շָ��ַ����ַ����ָ�ɶ���Ӵ�
*@param strSource :Դ�ַ���
*@param strSplit  :�ָ��ַ�
*@param resultVec :�������:��ŷָ����ִ�
*/
//template<typename T>
void splitString(const string &strSource, 
				 const string &strSplit, 
				 vector<string> &resultVec, 
				 bool bOutNullFlag)
{
	string	strTmp;
	size_t	p1 = 0, p2	= 0, len = 0;
	len = strlen(strSplit.c_str());
	if(0 == len){
		return;
	}

	//��Ҫ���ǿմ�"",���䵱��������ֵ��ŵ�������,���ڲ�ѯ���͵Ľ��������Ҫ�������
	if (bOutNullFlag) {
		while (string::npos != (p2 = strSource.find(strSplit, p1))) {
			strTmp = strSource.substr(p1, p2 - p1);

			//////////////////////////////////////////////////////////////////////////
			//trimRight(strTmp);
			//if (0 != strTmp.size()) {
			resultVec.push_back(strTmp);//��ŵ�������
			//}

			p1 = p2 + len;
		}
	}
	else{//����Ҫ���ǿմ�"",��Ч����,����
		while (string::npos != (p2 = strSource.find(strSplit, p1))) {
			strTmp = strSource.substr(p1, p2 - p1);
			trim(strTmp);//��trim����һ��
			if (0 != strTmp.size()) {
				resultVec.push_back(strTmp);//��ŵ�������
			}

			p1 = p2 + len;
		}
	}

	//�������һ���ִ�
    strTmp = strSource.substr(p1);
	if (bOutNullFlag){
		resultVec.push_back(strTmp);
	}
	else{
		trim(strTmp);
		if(0 != strTmp.size()){
			resultVec.push_back(strTmp);
		}
	}
}

/* ������ͬ���ִ�,��:A,A,B,Ӧ����A,B
*/
string filterSameStr(string strSource,
					 string strSplit)
{
	string str1,str2,strResult="";
	bool bSame = false;
	vector<string> resultVec,newVec;
	int isize =0,i=0;
	splitString(strSource,strSplit,resultVec);

	for (vector<string>::iterator it = resultVec.begin(); resultVec.end() != it; it++){
		bSame = false;
		str1 = *it;
		for (vector<string>::iterator it2 = newVec.begin(); newVec.end() != it2; it2++){
			str2 = *it2;
			if (isEqualStr(str1,str2)){
				bSame = true;
				break;
			}
		}
		if (!bSame){
			newVec.push_back(str1);
		}
	}

    isize = newVec.size();
	for (vector<string>::iterator it2 = newVec.begin(); newVec.end() != it2; it2++){
		i++;
		str2 = *it2;
		strResult.append(str2);
		if (isize != i){
			strResult.append(strSplit);
		}
	}

	return strResult;
}

/*�Զ���ȡ���ַ���β�ո�ķ���
*/
string & trim(string &str)
{
	if (str.empty()) {
		return str;
	}

	//���滻������� tab
	replace_all(str, CARICCP_TAB_MARK, CARICCP_SPACEBAR);

	//\r\nһ�������ĩβ��
	replace_all(str, CARICCP_ENTER_KEY, CARICCP_SPACEBAR);

	//�����Ż�һ��
	replace_all(str, "\n", CARICCP_SPACEBAR);
	replace_all(str, "\r", CARICCP_SPACEBAR);

	//���ҵĿո��
	str.erase(0, str.find_first_not_of(CARICCP_SPACEBAR));
	str.erase(str.find_last_not_of(CARICCP_SPACEBAR) + 1);

	return str;
}

/*ֻȥ���ұߵķ���
*/
string & trimRight(string &str)
{
	if (str.empty()) {
		return str;
	}


	//�ҵĿո��
	str.erase(str.find_last_not_of(CARICCP_SPACEBAR) + 1);
	return str;
}

/*���غ���
*/
char * trimChar(char *ibuf) //���ʹ��char[]���͵Ĳ���
{
	int	nlen, i;

	/*�ж��ַ�ָ���Ƿ�Ϊ�ռ��ַ������Ƿ�Ϊ0*/
	if (ibuf == NULL) {
		return CARICCP_NULL_MARK;
	}

	nlen = strlen(ibuf);
	if (nlen == 0)
		return CARICCP_NULL_MARK;

	/*ȥǰ�ո�*/
	i = 0;
	while (i < nlen) {
		/*if (ibuf[i]!=' ')*/
		if ((ibuf[i] != 0x20) && (ibuf[i] != '\t') && (ibuf[i] != '\r') && (ibuf[i] != '\n')) {
			break;
		}

		i ++;
	}

	if (i == nlen) {
		ibuf[0] = 0;
		return CARICCP_NULL_MARK;
	}

	if (i != 0) {
		nlen = nlen - i + 1;
		memcpy(ibuf, &ibuf[i], nlen);
	}

	/*ȥ��ո�*/
	nlen = strlen(ibuf) - 1;
	while (nlen >= 0) {
		/*if (ibuf[nlen] != ' ')*/
		if ((ibuf[nlen] != 0x20) && (ibuf[nlen] != '\t') && (ibuf[nlen] != '\r') && (ibuf[nlen] != '\n')) {
			ibuf[nlen + 1] = 0;

			break;
		}
		nlen --;
	}

	return ibuf;
}

/*�ַ��Ƿ�Ϊ��
*/
bool isNullStr(const char* chBuf)
{
	if (NULL == chBuf){
		return true;
	}

	string str = chBuf;
	if (0 == str.length()){
		return true;
	}
	return false;
}

/*�滻�ַ�����ָ�����ַ�
*@param str 	 :Դ�ִ�
*@param old_mark :���滻�ľ��ִ�
*@paramnew_mark  :�µ��ִ�
*/
string & replace_all(string &str, const string &old_mark, const string &new_mark)
{
	int	iOldLen	= old_mark.length();
	for (; ;) {
		string::size_type	pos	(0);   
		if (string::npos != (pos = str.find(old_mark))) {
			str.replace(pos, iOldLen, new_mark);
		} else {
			break;
		}
	}  

	return   str;
}   

/*�ж������ַ����Ƿ����,���Դ�Сд
*/
bool isEqualStr(const string str1, const string str2)
{
	int		iResult	= 0;
	size_t	max	= CARICCP_MAX(str1.length(), str2.length());
	iResult = strncasecmp(str1.c_str(), str2.c_str(), max);
	if (0 == iResult) {
		return true;
	}

	return false;
}

/*
 */
string stradd(char *str, char c) //strΪԭ�ַ���,cΪҪ׷�ӵ��ַ�
{
	//strcat(str, (const char *)c); 
	//	int n = strlen(str);
	// 	str[n] = c; //׷���ַ�
	// 	str[n+1] = 0; //��ӽ�����
	string strTest(str);
	char	szText[sizeof(str) + 1]	= {
		0
	};
	memset(szText, 0, sizeof(szText));
	strcpy(szText, strTest.c_str());
	szText[sizeof(str)] = c;

	string	s	= &(szText[0]); 
	return trim(s);
} 


#if defined(_WIN32) || defined(WIN32)
/*
* A simple printf function. Only support the following format:
* Code Format
* %c character
* %d signed integers
* %i signed integers
* %s a string of characters
* %o octal
* %x unsigned hexadecimal
*/
int my_printf(const char *format, ...)
{
	va_list	arg;
	int		done	= 0;

	va_start(arg, format);
	//done = vfprintf (stdout, format, arg);

	while (*format != '\0') {
		if (*format == '%') {
			if (*(format + 1) == 'c') {
				char	c	= (char) va_arg(arg, int);
				putc(c, stdout);
			} else if (*(format + 1) == 'd' || *(format + 1) == 'i') {
				char	store[20];
				int		i	= va_arg(arg, int);
				char	*str	= store;
				itoa(i, store, 10);
				while (*str != '\0')
					putc(*str++, stdout);
			} else if (*(format + 1) == 'o') {
				char	store[20];
				int		i	= va_arg(arg, int);
				char	*str	= store;
				itoa(i, store, 8);
				while (*str != '\0')
					putc(*str++, stdout);
			} else if (*(format + 1) == 'x') {
				char	store[20];
				int		i	= va_arg(arg, int);
				char	*str	= store;
				itoa(i, store, 16);
				while (*str != '\0')
					putc(*str++, stdout);
			} else if (*(format + 1) == 's') {
				char *str	= va_arg(arg, char *);
				while (*str != '\0')
					putc(*str++, stdout);
			}

			// Skip this two characters.

			format += 2;
		} else {
			putc(*format++, stdout);
		}
	}

	va_end(arg);

	return done;
} 
#endif

/* һ���򵥵�������printf��ʵ��,ͨ�����ʵ��
* Code Format
* %c character
* %d signed integers
* %i signed integers
* %s a string of characters
* %o octal
* %x unsigned hexadecimal
*
* ע :�ַ������Ͳ�������ʱ��ѡʹ��c_str()��ʽת��
*/
//string formatStr(const char *fmt, ...)  	  //һ���򵥵�������printf��ʵ��
//											 //ע:����Ϊ�ַ�������ʹ�ñ���,����ת��Ϊconst char*��ʽ
//{
//	vector< char>	vecCh;
//	vecCh.clear();
//
//	//char* pArg=NULL;  			 //�ȼ���ԭ����va_list
//	va_list	pArg;
//
//	char	c;
//	//pArg = (char*) &fmt;  		//ע�ⲻҪд��p = fmt !!��Ϊ����Ҫ�Բ���ȡַ,������ȡֵ
//	//pArg += sizeof(fmt);  		//�ȼ���ԭ����va_start  	   
//
//	va_start(pArg, fmt);
//
//	do {
//		c = *fmt;
//		if (c != '%') {
//			vecCh.push_back(c);
//
//			//��ԭ������ַ�
//			//putchar(c);   		 
//		} else {
//			//����ʽ�ַ����"����"
//			switch (*(++fmt)) {
//			case 'd':
//				//ʮ������
//				 {
//					//printf("%d",*((int*)pArg));  
//
//					int		sc	= (int) *((int *) pArg);
//					char	sTmp[16];//16����λ�����㹻��
//					sprintf(sTmp, "%d", sc);
//
//
//					char *sT	= sTmp;
//					while ('\0' != *sT) {
//						vecCh.push_back(*sT);
//						sT++;
//					}
//
//					//pArg += sizeof(int); 
//					va_arg(pArg, int);
//					break;
//				}
//
//			case 'x':
//				//printf("%#x",*((int*)pArg));
//				//pArg += sizeof(int);
//				va_arg(pArg, int);
//				break;
//
//			case 'f':
//				//printf("%f",*((float*)pArg));
//				//pArg += sizeof(int);
//				va_arg(pArg, int);
//				break;
//			case 's':
//				//�ַ���
//				 {
//					char *ch	= *((char **) pArg);
//					//printf("%s",ch);
//
//					while ('\0' != *ch) {
//						vecCh.push_back(*ch++);
//					}
//
//					//pArg += sizeof(char*);
//					va_arg(pArg, char*);
//					break;
//				}
//			default:
//				break;
//			}
//
//			//pArg += sizeof(int);  			 //�ȼ���ԭ����va_arg
//			//va_arg(pArg,int);
//		}
//
//		++fmt;
//	} while ('\0' != *fmt);
//
//
//	//pArg = NULL;  							 //�ȼ���va_end
//	va_end(pArg);
//
//	//��vecCh�������ַ���
//
//	string	strResult	= "";
//	char	chTmp;
//	for (vector< char>::iterator iter = vecCh.begin(); iter != vecCh.end(); ++iter) {
//		chTmp = *iter;
//		if ('\0' != chTmp) {
//			strResult += chTmp;
//		}
//	}
//
//	return  strResult;
//}

string formatStr(const char* fmt,...) //ע:����Ϊ�ַ�������ʹ�ñ���,����ת��Ϊconst char*��ʽ.string��ʹ��c_str()��������
{
	vector<char> vecCh;
	vecCh.clear();

	char* pArg=NULL;               //�ȼ���ԭ����va_list
	//va_list pArg;

	char c;
	pArg = (char*) &fmt;          //ע�ⲻҪд��p = fmt !!��Ϊ����Ҫ�Բ���ȡַ,������ȡֵ
	pArg += sizeof(fmt);          //�ȼ���ԭ����va_start         

	//va_start(pArg,fmt);

	do
	{
		c =*fmt;
		if (c != '%')
		{
			vecCh.push_back(c);

			//��ԭ������ַ�
			//putchar(c);            
		}
		else
		{
			//����ʽ�ַ����"����"
			switch(*(++fmt))
			{
			case 'd'://ʮ������
				{
					int sc = (int)*((int*)pArg);
					char sTmp[16];			//16����λ�����㹻��
					sprintf(sTmp,"%d",sc);  //��װ���µ��ִ� sTmp


					char *sT =sTmp;
					while('\0' != *sT)
					{
						vecCh.push_back(*sT);
						sT++;
					}

					pArg += sizeof(int); 
					break;
				}

			case 'x':
				pArg += sizeof(int); 
				break;

			case 'f':
				printf("%f",*((float*)pArg));
				pArg += sizeof(int); 
				break;
			case 's'://�ַ���
				{
					char* ch = *((char**)pArg);

					//�Ż�����
					//if (NULL == ch){
					//	break;
					//}

					while('\0' != *ch)
					{
						vecCh.push_back(*ch++);
					}

					pArg += sizeof(char*); 
					break;
				}
			default:

				break;
			}

			//pArg += sizeof(int);               //�ȼ���ԭ����va_arg
			//va_arg(pArg,int);
		}

		++fmt;

	}while ('\0' != *fmt);


	pArg = NULL;                                //�ȼ���va_end
	//va_end(pArg);

	//��vecCh�������ַ���

	string strResult = "";
	char chTmp;
	for(vector<char>::iterator iter=vecCh.begin();iter!=vecCh.end();++iter)
	{
		chTmp = *iter;
		if ('\0' != chTmp)
		{
			strResult += chTmp;
		}
	}

	return  strResult;
}

/*��int���͵�ֵת��Ϊbool����
*/
bool intToBool(int ivalue)
{
	if (0 == ivalue) {
		return false;
	}

	return true;
}

/*��int���͵�ֵת��Ϊstring����
*/
string intToString(int ivalue)
{
	char	buffer[10];//���ȶ������<=10
	memset(buffer, 0, sizeof(buffer));

#if defined(_WIN32) || defined(WIN32)
	itoa(ivalue, buffer, 10);
#else
	sprintf(buffer,"%d",ivalue);
#endif
	
	string str(buffer);
	return str;
}

/*��string����ת��Ϊint����
*/
int stringToInt(const char *strValue)
{
	if (NULL == strValue) {
		return 0;
	}
	//return atoi(strValue);
	return strtoul(strValue,NULL,10);
}

/*ת����Сд�ַ�
*/
void stringToLower(string &strSource)
{
#if defined(_WIN32) || defined(WIN32)
	strlwr((char*)strSource.c_str()); //ת����Сд 
#else
	string newstr = "";
	char buf[8];
	int ilen = strSource.length();
	for (int i = 0;i < ilen; i++){
		char ch = strSource.at(i);
		ch = tolower(ch);
		sprintf(buf,"%c",ch);
		newstr.append(buf);
	}
	strSource = newstr;
#endif
}

/*�ַ�ת����Сд
*/
char tolower(char t)
{   
	if(t>='A'&&t<='Z')   
		t+='a'-'A';   
	return t;   
}   

char toupper(char t)
{   
	if(t>='a'&&t<='z')   
		t-='a'-'A';   
	return   t;   
}

/*ת���ɰٷ���
*/
string convertToPercent(string strMolecule,   //����
						string strDenominator,//��ĸ
						bool bContainedDot)   //�Ƿ����С����
{
	int i_molecule,i_denominator,iRes=0;
	double dRtio;
	string strRes="";
	char buf[16];
	if (!isNumber(strMolecule.c_str()) 
		|| !isNumber(strDenominator.c_str())){
		goto end;
	}

	i_molecule = stringToInt(strMolecule.c_str());
	i_denominator = stringToInt(strDenominator.c_str());
	if (0 == i_denominator){
		goto end;
	}

	dRtio = i_molecule * 1.0/i_denominator;

	//�Ƿ�Ҫ����С����
	if (bContainedDot){
		iRes = (int)(dRtio*100);
	}
	else{
		iRes = (int)(dRtio*100);
	}

	sprintf(buf,"%d%s",iRes,CARICCP_PERCENT_MARK);
	strRes = buf;

end:
	return strRes;
}

/*���ʱ���
*/
char * getLocalTime(char *strTime)
{
	struct tm	*ptr;
	time_t		now;

	time(&now);
	ptr = localtime(&now);

	strftime(strTime, CARICCP_MAX_TIME_LENGTH, "%Y-%m-%d %H:%M:%S", ptr);//��-��-�� ʱ:��:��

	return strTime;
}

/*��ʽ����õĽ��
*/
void formatResult(const string &msg, map< string,string> &mapRes)//�������
{
	if (0 == msg.length()) {
		return;
	}

	vector< string>	vecRes;
	splitString(msg, CARICCP_SPECIAL_SPLIT_FLAG, vecRes);
	string	strTmp, strKey, strValue;
	size_t	beginIndex;

	for (vector< string>::const_iterator iter = vecRes.begin(); iter != vecRes.end(); iter++) {
		strTmp = *iter;

		beginIndex = strTmp.find_first_of(CARICCP_EQUAL_MARK);
		if (string::npos != beginIndex) {
			strKey = strTmp.substr(0, beginIndex);
			strValue = strTmp.substr(beginIndex + 1);
		} else {
			strKey = strTmp;
			strValue = CARICCP_NULL_MARK;
		}

		trim(strKey);
		trim(strValue);

		mapRes.insert(map< string,string>::value_type(strKey, strValue));
	}
}

/*����������ʼ��"0"��ʼ������,�������Ӳ���
*/
void addLongInteger(char operand1[],
					char operand2[],
					char result[])   
{   
	int i,e,d;   
	int n,m;   
	char temp; 

	//��ֹ���Ĳ���,�˴�ʹ�þֲ���������
	char tmpOperand[MAX_INTLENGTH];
	memset(tmpOperand,0,MAX_INTLENGTH);
	myMemcpy(tmpOperand,operand1,MAX_INTLENGTH);
	
	n=strlen(operand1);   
	m=strlen(operand2);   
	for(i=0;i<n/2;i++){
		temp=tmpOperand[i];
		tmpOperand[i]=tmpOperand[n-1-i];
		tmpOperand[n-1-i]=temp;
	}   
	for(i=0;i<m/2;i++){
		temp=operand2[i];
		operand2[i]=operand2[m-1-i];
		operand2[m-1-i]=temp;
	}   

	e=0;   
	for(i=0;i<n&&i<m;i++){   
		d=tmpOperand[i]-'0'+operand2[i]-'0'+e;   
		e=d/10;   
		result[i]=d%10+'0';   
	}   
	if(i<m){   
		for(;i<m;i++){   
			d=operand2[i]-'0'+e;   
			e=d/10;   
			result[i]=d%10+'0';   
		}   
	}   
	else{   
		for(;i<n;i++){   
			d=tmpOperand[i]-'0'+e;   
			e=d/10;   
			result[i]=d%10+'0';   
		}   
	}   
	if(e){
		result[i++]=e+'0'; 
	}
	result[i]=0;   
	n=i;   

	for(i=0;i<n/2;i++){
		temp=result[i];
		result[i]=result[n-1-i];
		result[n-1-i]=temp;
	}   
} 

/*��װ0��ͷ���ַ���
*/
string encapSpecialStr(string sourceStr,
					   int oriLen)
{
	int ilen = sourceStr.length();
	string str="";
	//��ʼ��װ0��ͷ
	for (int i=0; i<(oriLen-ilen);i++){
		str.append("0");
	}
	str.append(sourceStr);

	return str;
}

/*��װ���,����vec�д�ŵ���Ϣ���з�װ
*/
string encapResultFromVec(string splitkey,    //�ָ��
						  vector<string>& vec)//����
{
	string strRes = "",strTmp;
	int icout = 0,isize;
	isize = vec.size();
	if (0 == isize){
		return strRes;
	}

	for (vector<string>::const_iterator it=vec.begin();it!=vec.end();it++){
		icout++;//������
		strTmp = *it;

		//��Ҫȥ���ո��Ż�
		strRes.append(strTmp);
		if(isize !=icout){
			strRes.append(splitkey);//����β����Ҫ�����Ӵ�splitkey
		}
	}

	return strRes;
}

/*����map��key,��ö�Ӧ��value
*/
string cari_net_map_getvalue(map< string,string> &mapRes, string key, string &value)
{
	map< string,string>::const_iterator	it	= mapRes.find(key);
	if (it != mapRes.end()) {
		value = it->second;
	}

	//value = mapContainer[key]  //�����������!!!!!
	return value;
}

/*��vec������ɾ��ĳ��Ԫ��,���Ԫ�ز������򷵻�-1
*/
int eraseVecElement(vector< string> &vec, string element)
{
	int		ipos	= 0;
	string	strT	= "";
	for (vector< string>::iterator it = vec.begin(); it != vec.end(); it++) {
		strT = *it;

		//λ�ô�0��ʼ����
		ipos++;

		if (!strncasecmp(strT.c_str(), element.c_str(), CARICCP_MAX_STR_LEN(strT.c_str(), element.c_str()))) {
			//ɾ����Ԫ��
			vec.erase(it); //��ע :��ʱit����Ұָ����

			return ipos; //�������ڵ�λ������
		}
	}

	return -1;
}

/*��װ��׼��ÿһ�е�xml������
*/
string encapSingleXmlString(string strsource, int ispacnum)
{
	string	strT	= "";
	for (int i = 0; i < ispacnum; i++) {
		strT.append("  ");//��װ
	}

	strT.append(strsource);

	return strT;
}

/*��װ��׼��xml����,��Ҫ����д�뵽xml�ļ���.������Ծ������������з�װ����
*/
string encapStandXMLContext(string sourceContext)
{
	int				ipos		= 0;
	string			strNew		= "",strTmp	= "";
	string			strLine, strBefore,strSave,strChidname;
	vector< string>	vec, vecSavedChildName, vecNewStr;
	int				beginIndex	= 0,midIndex = 0,endIndex = 0;

	/*splitString(sourceContext, "\n", vec);*/
	splitString(sourceContext, "<", vec);//�˷�����֤�ɿ�
	for (vector< string>::iterator it = vec.begin(); it != vec.end(); it++) {
		strTmp = *it;
		trim(strTmp);
		if (0 == strTmp.length()) {
			continue;
		}

		//����������߼��鿴�Ƿ���Ҫ�����child������
		strBefore="";
		beginIndex = strTmp.find(" ");
		if (0 > beginIndex){
			beginIndex = strTmp.find(">");
			if (0 < beginIndex){
				strBefore =  strTmp.substr(0, beginIndex);
			}
			else{
				//�Ǳ�׼�ĸ�ʽ
				continue;
			}
		}
		else{
			strBefore =  strTmp.substr(0, beginIndex);
		}
		trim(strBefore);
		if (0 == strBefore.length()) {
			continue;
		}

		//��ɾ������"<"�������
		strLine = "";
		strLine.append("<");
		strLine.append(strTmp);

		beginIndex = strLine.find("<!--");//ע����
		if (0 == beginIndex) {
			//Ҳ���Ǳ���ԭ��
			ipos = vecSavedChildName.size();
			ipos++;//��ʾ��λ��ƫ��һ��
			strTmp = encapSingleXmlString(strLine, ipos);
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		beginIndex = strLine.find("-->");//ע����
		if (0 == beginIndex) {
			ipos = vecSavedChildName.size();
			ipos++;//��ʾ��λ��ƫ��һ��
			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		beginIndex = strLine.find("</");//child��ʶ�Ľ�����
		if (0 <= beginIndex) {
			//���ұ�ʶ��,"</"ռ2���ַ���λ��
			midIndex = strLine.find(">");
			strChidname = strLine.substr(beginIndex + 2, midIndex - (beginIndex + 2));//</variables>,ע������

			//�Ƿ���vecSavedChildName�а���????,һ������,����϶�������

			//�鿴����,�������д�ŵ�λ��,ʹ�ÿո��װ�ִ�
			//vecSavedChildName������ȥ��,newstrvec���������װ������ִ�
			ipos = eraseVecElement(vecSavedChildName, strChidname);
			if (0 > ipos){//��ֹ��������,��ǿ����һ��
				ipos = vecSavedChildName.size();
			}

			ipos--;//λ�ü���һ��

			//�ϲ��Ż�һ��,��ʽ�ϼ��
			if (0 != strSave.length()
				&& (!strcmp(strChidname.c_str(),strSave.c_str()))){
				int idex_0 = 0;
				
				//�ϴ��Ѿ���װ�õı�����������
				strTmp = vecNewStr.back();
				idex_0 = strTmp.find(">");
				strTmp = strTmp.substr(0, idex_0);
				strTmp.append("/>");

				//��ɾ��,������
				vecNewStr.pop_back();
			}
			else{
				strTmp = encapSingleXmlString(strLine, ipos);
			}

			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		//�����һ��������xml��,��:<param name=\"password\" value=\"%s\"/>
		//<A>��<A/>��<A name= >��<A name = />
		endIndex = strLine.find("/>");
		if (0 < endIndex){ 
			//�鿴vecSavedChildName�к��м���Ԫ��,������Ӽ����ո�,��װһ��
			ipos = vecSavedChildName.size();
			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}

		//child��������
		strChidname = strBefore;

		//�Ƿ���vecSavedChildName���Ѿ�����strChidname��ʶ
		//�������,vecSavedChildName������ȥ��
		ipos = eraseVecElement(vecSavedChildName, strChidname);
		if (-1 != ipos) {
			//��װ����
			strTmp = encapSingleXmlString(strLine, ipos);//��Ҫ�ǻ�ô�ŵ�λ��,���и�ʽ��װ
			vecNewStr.push_back(strTmp);

			strSave = "";
			continue;
		}
		
		//�Ȼ�õ�ǰ��Ԫ�ظ���,ȷ��λ��
		ipos = vecSavedChildName.size();
		strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
		vecNewStr.push_back(strTmp);

		//vecSavedChildName�����child��name��ʶ����
		vecSavedChildName.push_back(strChidname);

		//-----------------��������---------------------
		strSave = strBefore;
		//----------------------------------------------
	}

	//���ݱ���õķ�װ�õ��µ��ִ�,���,�˴��ٽ����Ż�����һ��
	for (vector< string>::iterator it2 = vecNewStr.begin(); it2 != vecNewStr.end(); it2++) {
		strLine = *it2;
		strNew.append(strLine);
		strNew.append("\n");
	}

	return strNew;
}

///*��װ��׼��xml����,��Ҫ����д�뵽xml�ļ���.������Ծ������������з�װ����
//*/
//string encapStandXMLContext(string sourceContext)
//{
//	int				ipos		= 0;
//	string			strNew		= "",strTmp	= "";
//	string			strLine, strChidname;
//	vector< string>	vec, vecSavedChildName, vecNewStr;
//	int				beginIndex	= 0,midIndex = 0,endIndex = 0;
//
//	/*splitString(sourceContext, "\n", vec);*/
//	splitString(sourceContext, "<", vec);//�˷�����֤�ɿ�
//	for (vector< string>::iterator it = vec.begin(); it != vec.end(); it++) {
//		strTmp = *it;
//		trim(strTmp);
//		if (0 == strTmp.length()) {
//			continue;
//		}
//
//		//��ȥ������"<"�������
//		strLine = "";
//		strLine.append("<");
//		strLine.append(strTmp);
//
//		beginIndex = strLine.find("<!--");//ע����
//		if (0 == beginIndex) {
//			//Ҳ���Ǳ���ԭ��
//			ipos = vecSavedChildName.size();
//			ipos++;//��ʾ��λ��ƫ��һ��
//			strTmp = encapSingleXmlString(strLine, ipos);
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//		beginIndex = strLine.find("-->");//ע����
//		if (0 == beginIndex) {
//			ipos = vecSavedChildName.size();
//			ipos++;//��ʾ��λ��ƫ��һ��
//			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//			vecNewStr.push_back(strTmp);
//			continue;
//		}
//
//		beginIndex = strLine.find("</");//child��ʶ�Ľ�����
//		if (0 == beginIndex) {
//			//���ұ�ʶ��
//			midIndex = strLine.find(">");
//			strChidname = strLine.substr(beginIndex + 2, midIndex - beginIndex - 2);//</variables>
//
//			//�Ƿ���vecSavedChildName�а���????,һ������,����϶�������
//
//			//�鿴����,�������д�ŵ�λ��,ʹ�ÿո��װ�ִ�
//			//vecSavedChildName������ȥ��,newstrvec���������װ������ִ�
//			ipos = eraseVecElement(vecSavedChildName, strChidname);
//			if (0 > ipos)//��ֹ��������,��ǿ����һ��
//			{
//				ipos = vecSavedChildName.size();
//			}
//
//			ipos--;//λ�ô�0��ʼ�����
//			strTmp = encapSingleXmlString(strLine, ipos);
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//
//		beginIndex = strLine.find("<");
//		if (0 != beginIndex)//����<��ͷ,��ʶ��Ϊע��������
//		{
//			ipos = vecSavedChildName.size();
//			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//			vecNewStr.push_back(strTmp);
//			continue;
//		}
//
//		//<A>��<A/>��<A name= >��<A name = />
//		endIndex = strLine.find("/>");
//		if (0 < endIndex) //һ��������xml��,��:<param name=\"password\" value=\"%s\"/>
//		{
//			//�鿴vecSavedChildName�к��м���Ԫ��,������Ӽ����ո�,��װһ��
//			ipos = vecSavedChildName.size();
//			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//
//		//Ҫ�����������: <X-PRE-PROCESS cmd="include" data="default/*.xml"></X-PRE-PROCESS>
//		//				  <X-PRE-PROCESS></X-PRE-PROCESS>
//		midIndex = strLine.find(" ");
//		if (0 > midIndex) {
//			endIndex = strLine.find("</");//����������һ���������
//			if (0 > endIndex) {
//				endIndex = strLine.find(">");
//
//				//��ʽ:<action>
//				if (0 > endIndex) {
//					//��Ч����,Ϊ�˱�������,�������-----------------------
//					ipos = vecSavedChildName.size();
//					strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//					vecNewStr.push_back(strTmp);
//
//					continue;
//				}
//
//				strChidname = strLine.substr(beginIndex + 1, endIndex - beginIndex - 1);
//			} else//������������г�
//			{
//				ipos = vecSavedChildName.size();
//				strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//				vecNewStr.push_back(strTmp);
//
//				continue;
//			}
//		}
//		//�����Ƿ���������
//		else {
//			endIndex = strLine.find("</");
//			if (-1 != endIndex)//������xml��
//			{
//				ipos = vecSavedChildName.size();
//				strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//				vecNewStr.push_back(strTmp);
//
//				continue;
//			}
//
//			strChidname = strLine.substr(beginIndex + 1, midIndex - beginIndex - 1);//��ʽ:<action 
//		}
//
//		//�Ƿ���vecSavedChildName���Ѿ�����strChidname��ʶ
//		//�������,vecSavedChildName������ȥ��
//		ipos = eraseVecElement(vecSavedChildName, strChidname);
//		if (-1 != ipos) {
//			//��װ����
//			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//			vecNewStr.push_back(strTmp);
//			continue;
//		}
//
//		//���������,���ǵ�һ�γ���
//		beginIndex = strLine.find("/>");
//		if (0 < beginIndex)//����Ϊһ��������
//		{
//			//�鿴vecSavedChildName�к��м���Ԫ��,������Ӽ����ո�,��װһ��
//			ipos = vecSavedChildName.size();
//			ipos--;//λ�ô�0��ʼ����
//			strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//			vecNewStr.push_back(strTmp);
//
//			continue;
//		}
//
//		//�Ȼ�õ�ǰ��Ԫ�ظ���,ȷ��λ��
//		ipos = vecSavedChildName.size();
//		strTmp = encapSingleXmlString(strLine, ipos);//����λ�ý��з�װ
//		vecNewStr.push_back(strTmp);
//
//		//vecSavedChildName����������child��name��ʶ����
//		vecSavedChildName.push_back(strChidname);
//	}
//
//
//	//���ݱ���õķ�װ�õ��µ��ִ�,���
//	for (vector< string>::iterator it2 = vecNewStr.begin(); it2 != vecNewStr.end(); it2++) {
//		strLine = *it2;
//		strNew.append(strLine);
//		strNew.append("\n");
//	}
//
//	return strNew;
//}


/*��ú궨�������ֵ,����μ������ļ�cari_ccp_macro.ini
*/
string getValueOfDefinedVar(string strVar)
{
	char *res="";
	string strVal = "",strKey = "";
	strKey = strVar;

	//mod by xxl 2010-9-25 :���ؼ���ת����Сд
	stringToLower(strKey);

	map<string,string>::const_iterator it = global_define_vars_map.find(strKey);
	if (it != global_define_vars_map.end()){
		 strVal = it->second;
	}

	//�Ż�����һ��:���val������,�򷵻�var��ֵ
	if (0 == strVal.length()){
		strVal = strVar;
	}
	return strVal;
}

/*����vector,���ĳ������ֵ���ֶ���
*/
string getValueOfVec(int index,           //����ֵ��0��ʼ
					 vector<string>& vec) //����
{
    int i=0;
	string strVal = "";

	vector<string>::const_iterator it = vec.begin();
	for (;it != vec.end(); it++)
	{
	  if (i == index)
	  {
	    strVal = *it;
		break;
	  }

	  i++;
	}

	return strVal;
}

/*����string,���ĳ������ֵ���ֶ���
*/
string getValueOfStr(string splitkey,  //�ָ��
					 int index,		   //����ֵ��0��ʼ
					 string strSource) //Դ�ִ�      
{  
	string strTmp,strValue;
	vector<string> vec;
	splitString(strSource,splitkey,vec);
	strValue = getValueOfVec(index,vec);
	return strValue;
}

/*���������Ų���Ԫ�ص�vector������
*/
void insertElmentToVec(int index,
					   string elment,
					   vector<string> &vec)
{
	int iSize = vec.size();
	if (index>=iSize){
		vec.push_back(elment);
		return;
	}

	int icount = -1;
	for (vector<string>::iterator it = vec.begin(); it != vec.end();it++){
		icount++;
		if (index == icount){
			vec.insert(it,1,elment);
			break;
		}
	}
}


/************************************************************************/
/*  																	*/
/************************************************************************/

/*�Զ���ȡ���ַ���β�ո�ķ���
*/
string & trimSpace(string &str)
{
	if (str.empty()) {
		return str;
	}

	//���ҵĿո��
	str.erase(0, str.find_first_not_of(CARICCP_SPACEBAR));
	str.erase(str.find_last_not_of(CARICCP_SPACEBAR) + 1);

	str.erase(0, str.find_first_not_of(" "));
	str.erase(str.find_last_not_of(" ") + 1);

	return str;
}

/*�����û���״̬�����仯,֪ͨcari net serverͬ������
*/
//void cari_switch_state_machine_notify(const char *mobUserID,    //�û���
//													  const char *mobUserState, //�û�״̬
//													  const char *mobUserInfo)  //�û�����Ϣ
//{
//	char *strNotifyCode						= NULL;
//	switch_stream_handle_t stream			= { 0 };
//	int istatus;
//	char * pSendInfo						=NULL;
//
//	if (NULL == mobUserID 
//		|| NULL == mobUserState)
//	{
//		return;
//	}
//
//	//֪ͨ��ͽ����
//	strNotifyCode  = switch_mprintf("%d", CARICCP_NOTIFY_MOBUSER_STATE_CHANGED); //֪ͨ����
//
//	//���ش������Ϣ��ʾ�����û�:�ն��û�������
//	pSendInfo = switch_mprintf("%s%s%s%s",				CARICCP_NOTIFY,			CARICCP_EQUAL_MARK, "1",			CARICCP_SPECIAL_SPLIT_FLAG); //֪ͨ��ʶ
//	pSendInfo = switch_mprintf("%s%s%s%s%s", pSendInfo, CARICCP_RETURNCODE,		CARICCP_EQUAL_MARK, strNotifyCode,	CARICCP_SPECIAL_SPLIT_FLAG); //������=֪ͨ��
//	pSendInfo = switch_mprintf("%s%s%s%s%s", pSendInfo, CARICCP_MOBUSERNAME,	CARICCP_EQUAL_MARK, mobUserID,		CARICCP_SPECIAL_SPLIT_FLAG); //�û��ĺ���
//	pSendInfo = switch_mprintf("%s%s%s%s%s", pSendInfo, CARICCP_MOBUSERSTATE,	CARICCP_EQUAL_MARK, mobUserState,	CARICCP_SPECIAL_SPLIT_FLAG); //�û���״̬
//	pSendInfo = switch_mprintf("%s%s%s%s",   pSendInfo, CARICCP_MOBUSERINFO,	CARICCP_EQUAL_MARK, mobUserInfo);   					 //�û�����Ϣ
//
//
//	//����֪ͨ��Ϣ,�û���״̬�ı�.����ϵͳ����ģ��Ķ���ӿں���
//	SWITCH_STANDARD_STREAM(stream);
//	istatus = switch_api_execute(CARI_NET_PRO_NOTIFY, pSendInfo, NULL, &stream);
//	if (SWITCH_STATUS_SUCCESS != istatus)//��������ִ��������
//	{
//		//��ӡ��־
//	}
//}

#if defined(_WIN32) || defined(WIN32)

void UTF_8ToUnicode(wchar_t* pOut,char *pText)
{
	char* uchar = (char *)pOut;

	uchar[1] = ((pText[0] & 0x0F) << 4) + ((pText[1] >> 2) & 0x0F);
	uchar[0] = ((pText[1] & 0x03) << 6) + (pText[2] & 0x3F);

	return;
}

void UnicodeToUTF_8(char* pOut,wchar_t* pText)
{
	// ע�� WCHAR�ߵ��ֵ�˳��,���ֽ���ǰ�����ֽ��ں�
	char* pchar = (char *)pText;

	pOut[0] = (0xE0 | ((pchar[1] & 0xF0) >> 4));
	pOut[1] = (0x80 | ((pchar[1] & 0x0F) << 2)) + ((pchar[0] & 0xC0) >> 6);
	pOut[2] = (0x80 | (pchar[0] & 0x3F));

	return;
}

void UnicodeToGB2312(char* pOut,wchar_t uData)
{
	WideCharToMultiByte(CP_ACP,NULL,&uData,1,pOut,sizeof(wchar_t),NULL,NULL);
	return;
}     

void Gb2312ToUnicode(wchar_t* pOut,char *gbBuffer)
{
	::MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,gbBuffer,2,pOut,1);
	return ;
}

void GB2312ToUTF_8(string& pOut,char *pText, int pLen)
{
	if (!pText
		||'\0' == (*pText)){
			return;
	}

	char buf[4];
	int nLength = pLen* 3;
	char* rst = new char[nLength];

	memset(buf,0,4);
	memset(rst,0,nLength);

	int i = 0;
	int j = 0;      
	while(i < pLen)
	{
		//�����Ӣ��ֱ�Ӹ���
		if( *(pText + i) >= 0)
		{
			rst[j++] = pText[i++];
		}
		else
		{
			wchar_t pbuffer;
			Gb2312ToUnicode(&pbuffer,pText+i);

			UnicodeToUTF_8(buf,&pbuffer);

			unsigned short int tmp = 0;
			tmp = rst[j] = buf[0];
			tmp = rst[j+1] = buf[1];
			tmp = rst[j+2] = buf[2];    

			j += 3;
			i += 2;
		}
	}
	//rst[j] = ' ';

	//���ؽ��
	pOut = rst;             
	delete []rst;   

	return;
}

void UTF_8ToGB2312(string &pOut, char *pText, int pLen)
{
	if (!pText
		||'\0' == (*pText)){
			return;
	}

	char * newBuf = new char[pLen];
	char Ctemp[4];

	memset(Ctemp,0,4);
	memset(newBuf,0,pLen);

	int i =0;
	int j = 0;

	while(i < pLen)
	{
		//if(pText > 0)
		if( *(pText + i) >= 0)
		{
			newBuf[j++] = pText[i++];                       
		}
		else                 
		{
			WCHAR Wtemp;
			UTF_8ToUnicode(&Wtemp,pText + i);

			UnicodeToGB2312(Ctemp,Wtemp);

			newBuf[j] = Ctemp[0];
			newBuf[j + 1] = Ctemp[1];

			i += 3;    
			j += 2;   
		}
	}

	/*newBuf[j] = ' ';*/ 
	//newBuf[j] = '\0';

	pOut = newBuf;

	delete []newBuf;

	return; 
}

//linuxϵͳ�µĲ�����ʽ
#else

//����ת��:��һ�ֱ���תΪ��һ�ֱ���   
int code_convert(char   *from_charset,char   *to_charset,char   *inbuf,int   inlen,char   *outbuf,int   outlen)   
{   
	iconv_t   cd;   
	int   rc;   
	char   **pin   =   &inbuf;   
	char   **pout   =   &outbuf;   

	cd   =   iconv_open(to_charset,from_charset);   
	if   (cd==0)   return   -1;   
	memset(outbuf,0,outlen);   
	//printf("inlen=%d\n",inlen);   
	if (iconv(cd,pin,(size_t*)&inlen,pout,(size_t*)&outlen)==-1)   
		return   -1;   
	iconv_close(cd); 

	return   0;   
}   
//UNICODE��תΪGB2312��   
int u2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("UNICODE","gb2312",inbuf,inlen,outbuf,outlen);   
	//return   code_convert("UTF-8","GB2312",inbuf,inlen,outbuf,outlen);   
}   
//GB2312��תΪUNICODE��   
int g2u(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	//return   code_convert("GB2312","UTF-8",inbuf,inlen,outbuf,outlen);   
	return   code_convert("GB2312","UNICODE",inbuf,inlen,outbuf,outlen);   
} 

//GB2312��תΪUNICODE��   
int utf2g(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("UTF-8","GB2312",inbuf,inlen,outbuf,outlen);   
} 

//GB2312��תΪUTF-8��   
int g2utf(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("GB2312","UTF-8",inbuf,inlen,outbuf,outlen);     
} 

//ascii��תΪUTF-8��   
int asc2utf(char   *inbuf,size_t   inlen,char   *outbuf,size_t   outlen)   
{   
	return   code_convert("ASCII","UTF-8",inbuf,inlen,outbuf,outlen);     
} 

void convertFrameCode(common_ResultResponse_Frame *&common_respFrame,
					  char *sourceCode,
					  char *dstSource)
{

	//test ��������ת��
	//CodeConverter cc = CodeConverter(sourceCode,dstSource);
	int ilen = 0;
	string str;


	//printf("back before: %s\n",common_respFrame->strTableContext);

	//if (!switch_strlen_zero(common_respFrame->header.strResuleDesc)){
	//	ilen = sizeof(common_respFrame->header.strResuleDesc);
	//	char outDesc[sizeof(common_respFrame->header.strResuleDesc)];
	//	memset(outDesc,0,sizeof(outDesc));
	//	//cc.convert(common_respFrame->header.strResuleDesc, ilen,outDesc,ilen);
	//	g2utf(common_respFrame->header.strResuleDesc,ilen,outDesc,sizeof(outDesc));
	//	memset(common_respFrame->header.strResuleDesc,0,sizeof(common_respFrame->header.strResuleDesc));
	//	myMemcpy(common_respFrame->header.strResuleDesc,outDesc,sizeof(outDesc));
	//}

	//if (!switch_strlen_zero(common_respFrame->strTableName)){
	//	//printf("back before: %s\n",common_respFrame->strTableContext);
	//	ilen = sizeof(common_respFrame->strTableName);
	//	char outTableName[sizeof(common_respFrame->strTableName)];
	//	memset(outTableName,0,sizeof(outTableName));
	//	cc.convert(common_respFrame->strTableName, ilen,outTableName,ilen);

	//	//printf("back convert: %s\n",out);
	//	memset(common_respFrame->strTableName,0,sizeof(common_respFrame->strTableName));
	//	myMemcpy(common_respFrame->strTableName,outTableName,sizeof(outTableName));
	//	//printf("back after: %s\n",common_respFrame->strTableContext);
	//}

	//if (!switch_strlen_zero(common_respFrame->strTableContext)){
	//	ilen = sizeof(common_respFrame->strTableContext);
	//	char outTableContext[sizeof(common_respFrame->strTableContext)];
	//	memset(outTableContext,0,sizeof(outTableContext));
	//	cc.convert(common_respFrame->strTableContext, ilen,outTableContext,ilen);

	//	//printf("back convert: %s\n",out);
	//	memset(common_respFrame->strTableContext,0,sizeof(common_respFrame->strTableContext));
	//	myMemcpy(common_respFrame->strTableContext,outTableContext,sizeof(outTableContext));
	//}
}
#endif


/************************************************************************/
/*  																	*/
/************************************************************************/

//��ʼ��������Ϣ,������һЩ�ļ���Ŀ¼
void cari_net_init()
{
	string strPort;

	//1 �½�һ��\conf\directory\groupsĿ¼
	const char	*err[1]	= {
		0
	};
	char		*groupspath=NULL, *diagroup_xmlfile=NULL, *context=NULL;
	groupspath = switch_mprintf("%s%s%s%s%s", 
		SWITCH_GLOBAL_dirs.conf_dir, 
		SWITCH_PATH_SEPARATOR, 
		USER_DIRECTORY, 
		SWITCH_PATH_SEPARATOR, 
		DIR_DEFINE_GROUPS); //groups

	//�ж�Ŀ¼�Ƿ��Ѿ�����
	if (!cari_common_isExistedFile(groupspath)) {
		cari_common_creatDir(groupspath, err);
	}
	switch_safe_free(groupspath);

	////2 �½�һ��\conf\dialplan\02_group_dial.xml�ļ�
	//diagroup_xmlfile = switch_mprintf("%s%s%s%s%s%s%s%s", 
	//	SWITCH_GLOBAL_dirs.conf_dir, 
	//	SWITCH_PATH_SEPARATOR, 
	//	DIAPLAN_DIRECTORY, 
	//	SWITCH_PATH_SEPARATOR,
	//	DEFAULT_DIRECTORY, 
	//	SWITCH_PATH_SEPARATOR, 
	//	DIAPLAN_GROUP_XMLFILE,  //�̶����ļ��� 02_group_dial
	//	CARICCP_XML_FILE_TYPE);


	//context = "<include>\n" \
	//"</include>";

	////�ж��ļ��Ƿ��Ѿ�����
	//if (!cari_common_isExistedFile(diagroup_xmlfile)) {
	//	cari_common_creatXMLFile(diagroup_xmlfile, context, err);
	//}
	//switch_safe_free(diagroup_xmlfile);

	//3 �������õı�����Ϣ
	loadVarsConfigCmd();

	//4 ���ö˿ڵ���Ϣ
	strPort = getValueOfDefinedVar("port");
	if (isNumber(strPort)){
		global_port_num = stringToInt(strPort.c_str());
	}

	//5 �������ݿ�ͱ���Ϣ
}


/*��ʼ���ر���������ļ�cari_ccp_macro.ini
*/
bool loadVarsConfigCmd()
{
	//����vars�����ļ�,��ÿ�����������Ӧ�����ݽṹ,���������
	string filePath = SWITCH_GLOBAL_dirs.conf_dir;
	filePath.append(SWITCH_PATH_SEPARATOR);
	filePath.append(CARI_CCP_MACRO_INI_FILE);
	//string filePath = "\\conf\\cari_ccp_macro.ini";

	vector<string> vecFile;//��ʱ����ļ�����
	string strLine,strKey,strVal;
	size_t index;
	vecFile.clear();

    int read_fd = -1;
	switch_size_t cur = 0, ml = 0;
	char buf[2048];
	int line = 0;
	int ilen = 0;
	
	if ((read_fd = open(filePath.c_str(), O_RDONLY, 0)) < 0) {
		const char *reason = strerror(errno);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldnt open %s (%s)\n", filePath.c_str(), reason);
		
		return false;
	}

	//ѭ����ȡԴ�ļ���ÿһ����Ϣ
	while ((cur = switch_fd_read_line(read_fd, buf, sizeof(buf))) > 0) {
		strLine = buf;
		trim(strLine);

		vecFile.push_back(strLine);//�������ļ���ÿһ�����ݴ������
		continue;
	}

	close(read_fd);

	//ifstream fin;
	//fin.open(filePath.c_str());

	////�ļ���·���в�Ҫ����"����"�ַ�
	//if (fin){//���ļ��ɹ�
	//	while(getline(fin,strLine)){    
	//		trim(strLine);
	//		vecFile.push_back(strLine);//�������ļ���ÿһ�����ݴ������
	//	}

	//	fin.clear();
	//	fin.close();
	//}
	//else{	
	//	//���ļ�ʧ��
	//	return false;
	//}


	if (0 == vecFile.size()){
		//�ļ���û�����ñ���
	}

	//��ϸ����ÿһ��
	for(vector<string>::iterator iterLine=vecFile.begin(); iterLine!=vecFile.end(); ++iterLine){
		strLine = *iterLine;
		index = strLine.find_first_of(CARICCP_SEMICOLON_MARK);      //";"ע���в���Ҫ����
		if (0 == index
			|| 0 ==strLine.size()){ //���в���Ҫ����
			continue;
		}
		index = strLine.find_first_of(CARICCP_LEFT_SQUARE_BRACKETS);//"["
		if (0 == index){
			continue;
		}

		index = strLine.find_first_of(CARICCP_EQUAL_MARK);          //"="
		if (0 >= index){
			continue;
		}

		//ȡ�ñ������Ͷ�Ӧ�ı���ֵ
		strKey = strLine.substr(0,index);
		strVal = strLine.substr(index + 1);
		trim(strKey);
		trim(strVal);

		if (0 == strKey.length()){
			continue;
		}

		//ʹ������Ĳ�����ʽ������쳣,���,���齫�����ļ�cari_ccp_macro.ini����Ϊutf-8��ʽ����
//#ifdef CODE_TRANSFER_FLAG
//	#if defined(_WIN32) || defined(WIN32)
//		string strConvert="";
//		GB2312ToUTF_8(strConvert,(char*)strVal.c_str(),strlen(strVal.c_str()));
//		strVal = strConvert;
//	#else
//		char chOut[CONVERT_OUTLEN];
//		//Ҫ���б���ת��,linux����ϵͳĬ��Ϊutf-8
//		memset(chOut,0,sizeof(chOut));
//		ilen = /*strlen(strVal.c_str());*/strVal.length();
//		g2utf((char*)strVal.c_str(),(size_t)ilen,chOut,CONVERT_OUTLEN);
//		strVal = chOut;
//	#endif
//#endif

		//mod by xxl 2010-9-25 :��keyת����Сд��ʽ
		stringToLower(strKey);

		//���궨�������ŵ�������
		global_define_vars_map.insert(map<string,string>::value_type(strKey,strVal));

	}//end  �����ļ�����ȫ���������

	vecFile.clear();

	return true;
}

/*ѭ����ʶ
*/
void setLoopFlag(switch_bool_t bf)
{
	if (bf){
		loopFlag = true;
	}
	else{
		loopFlag = false;
	}
}

/*�����Ч���ַ���,linuxϵͳ�Ĺ��߻�õĽ���п��ܺ��в�ʶ����ַ�
*/
void cari_common_getValidStr(string &strSouce)
{
	string strNew = "";
	char ch;
	char	buffer[6];
	int iLength = strSouce.length();
	for (int i = 0; i < iLength; i++) {
		ch = strSouce.at(i);
		if ((32 <= ch && 126 >= ch)//ascii��
			//||(8<=ch && 13 >= ch)//�����ַ�:�˸�����Ʊ�������з��ͻس���
			||10==ch   //CR,
			||13 == ch //LF
			||(0> ch)){//�����ַ�
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer,"%c",ch);
			strNew.append(buffer);
		}
		//else{
		//	//printf("%c",ch);
		//}
	}

	//printf("\n");
	strSouce = strNew;
}

/*ִ��shell ����ؽ����Ϣ,�˺����漰�����߳�����
*/
void cari_common_getSysCmdResult(const char *syscmd,
								 string &strRes)//�������:����ִ�еĽ��
{
	//�ȼ���
	pthread_mutex_lock(&mutex_syscmd);

#if defined(_WIN32) || defined(WIN32)
#else
	FILE   *fp; 
	char   buf[OUTLEN];
	//char   out[OUTLEN];
	memset(buf, '\0', sizeof(buf));//��ʼ��buf,�������д�����뵽�ļ���

	//�� syscmd ��������ͨ���ܵ���ȡ(��r������)��FILE* fp
	//���лᱻ�ض�,����fgets()��������,�μ������˵��
	fp = popen(syscmd, "r" ); 
	//fgets()�����Ӳ���stream��ָ���ļ��ڶ����ַ����浽����s��ָ���ڴ�ռ�,
	//ֱ�����ֻ����ַ��������ļ�β�����Ѷ���size-1���ַ�Ϊֹ,�������NULL��Ϊ�ַ���������
	//fgets(buf,sizeof(buf),fp);

	//ʹ��fread�滻
	fread(buf,sizeof(char), sizeof(buf),fp); //���ո�FILE* stream����������ȡ��buf��

	////�ܵ�����Ľ����ʾ�ַ�������������,utf-8-->gb2312(�˴�ת����Ч???client����gb2312��ʾ)
	//CodeConverter cc = CodeConverter("utf-8","gb2312");
	//cc.convert(buf,sizeof(buf),out,OUTLEN);
	//strRes = out;

	strRes = buf;

	//��װ�Ż�һ��
	cari_common_getValidStr(strRes);
	pclose(fp); 

	////ͨ����д�ļ��ķ�ʽ���д���
	//FILE   *fp;  
	//FILE    *wp;
	//char   buf[OUTLEN]; 
	//memset( buf, '\0', sizeof(buf) );//��ʼ��buf,�������д�����뵽�ļ���
	//fp = popen(syscmd, "r" ); //��syscmd �������� ͨ���ܵ���ȡ����r����������FILE* fp
	//wp = fopen("syscmd_popen.txt", "w+"); //�½�һ����д���ļ�

	//fread(buf, sizeof(char), sizeof(buf),fp);  //���ո�FILE* fp����������ȡ��buf��
	//fwrite(buf, 1, sizeof(buf), wp);//��buf�е�����д��FILE  *wp��Ӧ������,Ҳ��д���ļ���

	//pclose(stream);  
	//fclose(wstream);

	//strRes = buf;

#endif
	
	//�ͷ���
	pthread_mutex_unlock(&mutex_syscmd);
}

/*ִ��ϵͳ����,����Ҫ���ؽ��
*/
int cari_common_excuteSysCmd(const char *syscmd)
{
	if (NULL == syscmd){
		return -1;
	}
	int	iRes = 0;
	iRes = system(syscmd);
	if (0 != iRes) {
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
//����ͨ�õĺ���
void cari_common_convertStrToEvent(const char *context, const char *split, switch_event_t *event)
{
	string	strSource	= context;
	string	strSplit	= split;
	string	strTmp, strKey, strVal, strMid;
	size_t	p1			= 0, p2	= 0, p3	= 0;

	if (NULL == context || NULL == event) {
		return;
	}

	while (string::npos != (p2 = strSource.find(strSplit, p1))) {
		strTmp = strSource.substr(p1, p2 - p1);
		trimSpace(strTmp);
		if (0 != strTmp.size()) {
			//resultVec.push_back(strTmp);//��ŵ�������
			//��Ҫ�ټ�������,key��value
			if (string::npos != (p3 = strTmp.find(CARICCP_EQUAL_MARK, 0))) {
				strKey = strTmp.substr(0, p3);
				strVal = strTmp.substr(p3 + 1);
				trimSpace(strKey);
				trimSpace(strVal);

				//�������
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, strKey.c_str(), strVal.c_str());
			}
		}

		p1 = p2 + strlen(strSplit.c_str()); //
	}

	//�������һ���ִ�
	strTmp = strSource.substr(p1);
	trimSpace(strTmp);
	if (0 != strTmp.size()) {
		//resultVec.push_back(strTmp);
		if (string::npos != (p3 = strTmp.find(CARICCP_EQUAL_MARK, 0))) {
			strKey = strTmp.substr(0, p3);
			strVal = strTmp.substr(p3 + 1);
			trimSpace(strKey);
			trimSpace(strVal);

			//�������
			switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, strKey.c_str(), strVal.c_str());
		}
	}
}

//
////����ͨ�õĺ���
//SWITCH_DECLARE(void) cari_common_convertStrToEvent(const char *context,	
//												   char split,
//												   switch_event_t *event)
//{
//	int i= 0;
//	char *array[1024] = { 0 };		//���1024��Ԫ��
//	char *key_value[2] = { 0 };		//���2��,key��value
//	int size = (sizeof(key_value) / sizeof(key_value[0]));
//
//	int splitNum_1 = 0;
//	int splitNum_2 = 0;
//
//	splitNum_1 = switch_separate_string((char *)context, split, array, sizeof(array) / sizeof(array[0]));
//	//���δ��������Ϣ
//	for(i=0; i<splitNum_1;i++)
//	{
//		splitNum_2 = switch_separate_string(array[i], CARICCP_CHAR_EQUAL_MARK, key_value, size);
//		if (2 > splitNum_2)
//		{
//			continue;
//		}
//
//		//�������
//		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, key_value[0],  key_value[1]); 
//	}
//}

/*�����µ�Ŀ¼ :�������ϵͳ��������в���.�����ǰ��Ŀ¼�Ѿ�����,���ǿ�ƴ���???
*/
bool cari_common_creatDir(char *dir, const char **err)
{
	//�ж�Ŀ¼�Ƿ��Ѿ�����
	if (cari_common_isExistedFile(dir)) {
		return true;
	}

#if defined(_WIN32) || defined(WIN32)

	//�˴���Ҫ���δ���һ��,���·����ĳ��Ŀ¼���пո�,����windows�´�����Ĳ���������
	//ֱ�ӵ���ϵͳ����
	mkdir(dir);
	return true;

#else

	bool bRes = false;

	//���������ַ�������Ŀ¼����������,ֱ��ʹ��ϵͳ�ṩ�ĺ������д���
	//int	iRes		= 0;
	//char *pDir = NULL;
	//char *comaddDir	= "mkdir %s";
	//pDir = switch_mprintf(comaddDir, dir);
	//iRes = system(pDir);
	//if (0 != iRes) {
	//	bRes = false;
	//}

	//switch_safe_free(pDir);

	//���ǵ�"����"Ŀ¼��������
	int ires = mkdir(dir,0755);//Ȩ�� :rwxr-xr-x 
	if (0 == ires){
		bRes = true;
	}
	return bRes;

#endif
}

/*ɾ��Ŀ¼ :�������ϵͳ��������в���
*/
bool cari_common_delDir(char *dir, const char **err)
{
	if (NULL ==dir){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "rm dir failed, dir is null!");
		return false;
	}

	bool bRes = true;
	int	iRes  = 0;
	char *syscmd = NULL,*comdelDir=NULL;

	*err = "rm dir error!";

#if defined(_WIN32) || defined(WIN32)
	//iRes = rmdir(dir); 
	//if (0 != iRes){
	//	bRes = false;
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "rm dir %s error!",dir);
	//}
	comdelDir	= "rmdir /s /q %s";// /s /q ��ʶ��̲�����ʾĿ¼�е��ļ�Ҳ��һ��ɾ��,Ҳ������Ŀ¼�� 
#else
	comdelDir	= "rm -rf %s";     // -rf��ʶ��̲�����ʾĿ¼�е��ļ�Ҳ��һ��ɾ��,Ҳ������Ŀ¼��
#endif
	syscmd = switch_mprintf(comdelDir, dir);
	iRes = system(syscmd);
	if (0 != iRes) {
		bRes = false;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "rm dir %s error!",dir);
	}

	switch_safe_free(syscmd);
	return bRes;
}

/*�޸�Ŀ¼
*/
bool cari_common_modDir(char *sourceDir, 
						char *destDir, 
						const char **err)
{
	if (NULL ==sourceDir 
		|| NULL == destDir){
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "mv dir failed, dir is null!");
		return false;
	}

	bool	bRes		= false;
	char			*commodDir	= "";//��̲�����ʾĿ¼�е��ļ�Ҳ��һ��ɾ��,Ҳ������Ŀ¼��
	char			*oldDirAll	= NULL,*syscmd = NULL;
	int	iRes		= 0;

	//����ķ���renִ�в��ɹ�
	//#if defined(_WIN32) || defined(WIN32)
	//	commodDir="ren %s %s";
	//#else
	//	commodDir="mv %s %s";
	//#endif
	//
	//	commodDir = switch_mprintf(commodDir,sourceDir,destDir);


	//�ȴ����µ�Ŀ¼
	bRes = cari_common_creatDir(destDir, err);
	if (!bRes) {
		goto end;
	}

	//�Ѿ�Ŀ¼�е������û��ļ��������µ�Ŀ¼��
	oldDirAll = switch_mprintf("%s%s%s", sourceDir, SWITCH_PATH_SEPARATOR, "*.*");

#if defined(_WIN32) || defined(WIN32)
	commodDir = "move /y %s %s";//ǿ�и������е������ļ��Ļ�,��move���ϸ� /Y �Ŀ���
#else
	commodDir = "mv %s %s";
#endif

	syscmd = switch_mprintf(commodDir, oldDirAll, destDir);

	//�˴����������:destDir������,oldDirAll�����û�xml�ļ�,ֱ�Ӵ�������
	iRes = system(syscmd);
	if (0 != iRes) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s error!",syscmd);
	}

	//ɾ���ɵ�Ŀ¼
	bRes = cari_common_delDir(sourceDir, err);
	if (!bRes) {
		goto end;
	}

end:
	switch_safe_free(oldDirAll);
	switch_safe_free(syscmd);

	bRes = true;
	return bRes;
}

/*�����µ��ļ�,��Ҫ�����xml�ļ��Ĳ���
*/
bool cari_common_creatXMLFile(char *file, //�������ļ���
							  char *filecontext, //�ļ�������   
							  const char **err) //�����������Ϣ
{
	int				write_fd	= -1; //�ļ��Ķ�д���,��ʱ���йر�
	switch_size_t	cur			= 0;
	//char filebuf[2048],ebuf[8192]; //�Ѿ������˳���,���Ƽ�
	char			*var		= NULL,*errMsg = NULL;
	vector< string>	vec;
	string			newStr;
	if (!filecontext ||(!file)) {
		*err = "file name is null.";
		return true;
	}

	//���·�װxml�ļ�������,����xml�Ľڵ�˳��,�˴�ͳһ���д���
	newStr = encapStandXMLContext(filecontext);
	var = (char *)(newStr.c_str());

	//��ʼ����xml�ļ�
	write_fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (0 > write_fd) {
		*err = "Cannot open file";
		return false;
	}

	//ת����char[]�ṹ
	//memset(filebuf,0,sizeof(filebuf));
	//switch_copy_string(filebuf,filecontext,sizeof(filebuf));
	//ע�� :�˷�����������ַ�"$${"�Ĵ���!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//var = switch_xml_get_expand_vars(filebuf, ebuf, sizeof(ebuf), &cur);

	//var = filecontext;
	cur = strlen(var);

	if (write(write_fd, var, (unsigned) cur) != (int) cur) {
		*err = "Write file failed:";

		//�رվ��
		close(write_fd);
		write_fd = -1;
		return false;
	}

	*err = "Success";

	//�رվ��
	close(write_fd);
	write_fd = -1;

	return true;
}

/*ɾ��xml�ļ�,�˲�����Ҫ���ɾ���ļ�,ɾ��Ŀ¼���⴦��
*/
bool cari_common_delFile(char *file, //�����Ǿ���·��
						 const char **err)
{
	//��ʹ��remove(),���ʧ������ʹ��ϵͳ������ del XXX
	bool bres = false;
	int	iRes	= 0;
	if (!file) {
		goto end;
	}
	*err = "del file failed.";
	iRes = remove(file);//�ȵ���api����ɾ������
	if (0 != iRes) {
		char *ch = NULL;
		char* comdel;

#if defined(_WIN32) || defined(WIN32)
		ch = "del  ";
#else
		ch	= "rm -rf ";     // -rf��ʶǿ��ɾ��
#endif
		comdel = switch_mprintf("%s%s", ch, file);
		if (!comdel) {
			goto end;
		}

		//������֪��ִ�еĽ���Ƿ�ɹ�,�˴�ֻ�Ƿ��غ����Ĵ�����
		iRes = system(comdel);
		if (0 != iRes) {
			*err = "del file failed.";
		}
		switch_safe_free(comdel);
		goto end;
	}
	bres = true;

end:
	
	return bres;
}

/*�����ļ�
*/
bool cari_common_cpFile(char* sourceDir,
						char* destDir,
						const char** err)
{
	bool bres = false;
	int	iRes	= 0;
	char *ch = NULL,*comdel = NULL;
	if (!sourceDir 
		|| !destDir) {
		goto end;
	}

	*err = "copy file failed.";

#if defined(_WIN32) || defined(WIN32)
		ch = "copy /Y  %s %s";
#else
		ch	= "cp  %s %s";
#endif
	comdel = switch_mprintf(ch, sourceDir, destDir);
	if (!comdel) {
		goto end;
	}
	iRes = system(comdel);
	if (0 != iRes) {
		*err = "copy file failed.";
	}
	else{			
		bres = true;
	}
	switch_safe_free(comdel);

end:

	return bres;
}

/*�ж��ļ���Ŀ¼�Ƿ����
*/
bool cari_common_isExistedFile(char *filename)
{
	/*0-����ļ��Ƿ����		 
	1-����ļ��Ƿ������		
	2-����ļ��Ƿ��д����    
	4-����ļ��Ƿ�ɶ�����     
	6-����ļ��Ƿ�ɶ�/д���� 
	*/
	if (0 == access(filename, 0)) {
		return true;
	}

	return false;
}

/*����û���
*/
void cari_common_getMobUserGroupInfo(char *mobusername, const char **mobGroupInfo)
{
	if (switch_strlen_zero(mobusername)) {
		return;
	}

	//����1:ͨ��defaultĿ¼�µ�xml��<variable name="callgroup" value="techsupport" /> 
	//������û������Ϣ

	//����2:����root xml�ṹ�����
}

/*�����ļ�������,��ö�Ӧ��xml�ṹ,��װswitch_xml_parse_fd()����
 *�˷��ص�xml�ṹ����Ҫ�ͷ�
*/
switch_xml_t cari_common_parseXmlFromFile(const char *file)
{
	int				read_fd	= -1;
	switch_xml_t	x_xml	= NULL;

	//�Ȼ�ô��û���xml�ļ���������Ϣ
	read_fd = open(file, O_RDONLY, 0);
	if (0 > read_fd) {
		goto end;
	}

	//��ö�Ӧ��xml�ṹ
	x_xml = switch_xml_parse_fd(read_fd);

	//�ر����ľ��
	close(read_fd);
	read_fd = -1;

	end : return x_xml;
}

/*�Ƿ���������ַ�
*/
bool cari_common_isExsitedChinaChar(const char *ibuf)
{
	char	ch;
	if (NULL == ibuf) {
		return false;
	}

	ch = *ibuf;
	while ('\0' != ch) {
		//���ں�����2���ֽ�,�����ж��Ը���Ϊ׼
		//if (ch   >=   0x4e00   &&   ch   <=   0x9fa5) 
		if (0 > ch) {
			return  true;
		}

		ibuf++;
		ch = *ibuf;
	}

	return false;
}

/*�Զ���ȡ���ַ���β�ո�/tab/\r\n�ķ���
*/
char * cari_common_trimStr(char *ibuf) //������ö���Ϊchar[]����
{
	size_t	nlen, i;

	/*�ж��ַ�ָ���Ƿ�Ϊ�ռ��ַ������Ƿ�Ϊ0*/
	if (switch_strlen_zero(ibuf)) {
		return CARICCP_NULL_MARK;
	}

	nlen = strlen(ibuf);
	if (nlen == 0)
		return CARICCP_NULL_MARK;

	/*ȥǰ�ո�*/
	i = 0;
	while (i < nlen) {
		/*if (ibuf[i]!=' ')*/
		if ((ibuf[i] != 0x20) && (ibuf[i] != '\t') && (ibuf[i] != '\r') && (ibuf[i] != '\n')) {
			break;
		}

		i ++;
	}

	if (i == nlen) {
		ibuf[0] = 0;
		return CARICCP_NULL_MARK;
	}

	if (i != 0) {
		nlen = nlen - i + 1;
		memcpy(ibuf, &ibuf[i], nlen);
	}

	/*ȥ��ո�*/
	nlen = strlen(ibuf) - 1;
	while (nlen >= 0) {
		/*if (ibuf[nlen] != ' ')*/
		if ((ibuf[nlen] != 0x20) && (ibuf[nlen] != '\t') && (ibuf[nlen] != '\r') && (ibuf[nlen] != '\n')) {
			ibuf[nlen + 1] = 0;

			break;
		}
		nlen --;
	}

	return ibuf;
}

////����xml�ļ�,����switch_xml_parse_file()����
//switch_xml_t cari_common_copy_parse_file(const char *sourcefile, //Դ�ļ�	
//										 const char *detestfile)  //Ŀ���ļ�
//{
//	int				fd			= -1, write_fd = -1;
//	switch_xml_t	xml			= NULL;
//	char			*new_file	= NULL;
//
//	if (switch_strlen_zero(sourcefile) || switch_strlen_zero(detestfile)) {
//		return NULL;
//	}
//
//	//����ֱ�Ӹ�ֵ,�����ڴ�����ʾ����
//	//new_file = (char *)detestfile;
//	if (!(new_file = switch_mprintf("%s", detestfile))) {
//		return NULL;
//	}
//
//	//��ʼ����(open)�µ�(O_CREAT)�ļ�
//	if ((write_fd = open(detestfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
//		goto done;
//	}
//
//	//�ȽϹؼ��Ĵ���,��Դ�ļ������ݿ������µ�Ŀ���ļ���,�����ض�Ӧ��xml�ṹ
//	if (switch_xml_preprocess(SWITCH_GLOBAL_dirs.conf_dir, sourcefile, write_fd, 0) > -1) {
//		close(write_fd);
//		write_fd = -1;
//		if ((fd = open(new_file, O_RDONLY, 0)) > -1) {
//			//����xml�ṹ
//			if ((xml = switch_xml_parse_fd(fd))) {
//				xml->free_path = new_file;
//				new_file = NULL;
//			}
//			close(fd);
//			fd = -1;
//		}
//	}
//
//	done:
//	if (write_fd > -1) {
//		close(write_fd);
//	}
//	if (fd > -1) {
//		close(fd);
//	}
//
//	switch_safe_free(new_file);
//	return xml;
//}

static char not_so_threadsafe_error_buffer[256] = "";

///*�Զ�����ؼ���root xml�ļ�,ȥ���߳���,��Ҫ��"д��",�߳���������������
//*/
//static switch_xml_t  cari_switch_xml_open_root(uint8_t reload, 
//											   const char **err)
//{
//	char path_buf[1024];
//	uint8_t hasmain = 0, errcnt = 0;
//	switch_xml_t new_main, r = NULL;
//
//	switch_mutex_t *xml_lock = switch_xml_get_xml_lock();
//
//	switch_mutex_lock(xml_lock);
//
//	//��ԭʼ�������ļ�freeswitch.xml
//	switch_snprintf(path_buf, sizeof(path_buf), "%s%s%s", SWITCH_GLOBAL_dirs.conf_dir, SWITCH_PATH_SEPARATOR, "freeswitch.xml");
//
//	if ((new_main = switch_xml_parse_file(path_buf))) {
//		*err = switch_xml_error(new_main);
//		switch_copy_string(not_so_threadsafe_error_buffer, *err, sizeof(not_so_threadsafe_error_buffer));
//		*err = not_so_threadsafe_error_buffer;
//		if (!switch_strlen_zero(*err)) {
//			switch_xml_free(new_main);
//			new_main = NULL;
//			errcnt++;
//		} else {
//			//switch_xml_t old_root;
//			*err = "Success";
//
//			//���������ڴ�
//			switch_xml_reset_rootxml(new_main);
//
//			//��ǰ���߼�
//			//old_root = /*MAIN_XML_ROOT*/switch_xml_root();
//			//MAIN_XML_ROOT = new_main; //!!!!!!!!!!!!!!!!!!!!!���½���xml�ṹ���浽 MAIN_XML_ROOT��
//			//switch_set_flag(MAIN_XML_ROOT, SWITCH_XML_ROOT);
//			//switch_xml_free(old_root);
//			/* switch_xml_free_in_thread(old_root); */
//		}
//	} else {
//		*err = "Cannot Open log directory or XML Root!";
//		errcnt++;
//	}
//
//	if (errcnt == 0) {
//		switch_event_t *event;
//		if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
//			if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
//				switch_event_destroy(&event);
//			}
//		}
//		r = switch_xml_root();
//	}
//
//	switch_mutex_unlock(xml_lock);
//
//	return r;
//}

/*
*��������mainroot��xml�ļ��Ľṹ
*/
int cari_common_reset_mainroot(const char **err)
{
	////����һ�����ݵĹ���
	//char			source_rootxml_pathbuf[1024], dest_bak_rootxml_pathbuf[1024];

	////��Ҫ����logĿ¼��,����һ��ԭʼ���ļ�
	//switch_snprintf(source_rootxml_pathbuf, sizeof(source_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.log_dir, 
	//	SWITCH_PATH_SEPARATOR, 
	//	"freeswitch.xml.fsxml");
	//switch_snprintf(dest_bak_rootxml_pathbuf, sizeof(dest_bak_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.log_dir, 
	//	SWITCH_PATH_SEPARATOR, 
	//	"freeswitch.xml.fsxml.bak");

	////�����ļ�
	//cari_common_cpFile(source_rootxml_pathbuf,dest_bak_rootxml_pathbuf,err);


	////����0 :��������ķ���----------------------------------------------------------------------------------------
	//int	 ires	= SWITCH_STATUS_FALSE;
	//switch_xml_t xml_root;

	////���ֲ����Ƿ�����������ָ���ؼ���root��������???��Ҫ���߳�"��"д���ʱ������
	////�˲���Ҫע���ͷ��߳���
	//if ((xml_root = cari_switch_xml_open_root(1, err))) {		
	//	switch_xml_free(xml_root);
	//	*err = "Success";
	//	ires =  SWITCH_STATUS_SUCCESS;
	//}

	//����2 :������߼�--------------------------------------------------------------------------------------------
//	int				ires	= SWITCH_STATUS_SUCCESS;
//	char			source_rootxml_pathbuf[1024], dest_bak_rootxml_pathbuf[1024], error_buffer[256];
//	switch_xml_t	new_xml_root;
//	switch_mutex_t	*xmllock	= NULL;
//
//	xmllock = switch_xml_get_xml_lock();
//	if (xmllock) {
//		switch_mutex_lock(xmllock);
//	}
//
//	//�������¼���root xml�ļ�.��ԭʼ�������ļ�freeswitch.xml,��bak(����)
//	switch_snprintf(source_rootxml_pathbuf, sizeof(source_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.conf_dir, 
//		SWITCH_PATH_SEPARATOR, 
//		"freeswitch.xml");
//
//	switch_snprintf(dest_bak_rootxml_pathbuf, sizeof(dest_bak_rootxml_pathbuf), "%s%s%s", SWITCH_GLOBAL_dirs.log_dir, 
//		SWITCH_PATH_SEPARATOR, 
//		"freeswitch.new.xml.fsxml");
//
//	//���ݱ����ļ�freeswitch.bak.fsxml,���´���root xml�ļ��Ľṹ
//	new_xml_root = cari_common_copy_parse_file(source_rootxml_pathbuf, dest_bak_rootxml_pathbuf);
//	if (!new_xml_root) {
//		*err = "Copy  freeswitch.xml failed!";
//		ires = SWITCH_STATUS_FALSE;
//		goto end;
//	}
//
//	*err = (char*) switch_xml_error(new_xml_root);
//	switch_copy_string(error_buffer, *err, sizeof(error_buffer));
//	*err = error_buffer;
//	if (!switch_strlen_zero(*err)) {
//		switch_xml_free(new_xml_root);
//		new_xml_root = NULL;
//		ires = SWITCH_STATUS_FALSE;
//
//		goto end;
//	} 
//
//	//�������¼�
//	switch_event_t *event;
//	if (switch_event_create(&event, SWITCH_EVENT_RELOADXML) == SWITCH_STATUS_SUCCESS) {
//		if (switch_event_fire(&event) != SWITCH_STATUS_SUCCESS) {
//			switch_event_destroy(&event);
//		}
//	}
//
//	//��������root xml�Ľṹ,�ڴ�ĸ���,����ԭ�����ڴ治���ͷ�???!!!!!!!!!!!!!!!!!!!!!!!
//	switch_xml_reset_rootxml(new_xml_root);
//	*err = "Success";
//
//
//end:
//	if (xmllock) {
//		switch_mutex_unlock(xmllock);
//	}

	//----------------------------------------------------------------------------------------------------------------
	//����2 :ʹ��Ŀǰ�ṩ�ķ�ʽ,�����и�������������:switch_thread_rwlock_wrlock(RWLOCK);�˺�������ʱ����.
	int	 ires	= SWITCH_STATUS_FALSE;
	switch_xml_t xml_root;
	
	//�Ż���������
	//switch_thread_rwlock_t *rwlock= switch_xml_get_thread_lock();
	//if (rwlock){
	//	for (int i=0;i<6;i++){//���ͷż���,���Ǵ˴�������������
	//		switch_thread_rwlock_unlock(rwlock);
	//		switch_thread_rwlock_unlock(rwlock);
	//		switch_thread_rwlock_unlock(rwlock);
	//	}
	//}
	if ((xml_root = switch_xml_open_root(1, err))) {	
		switch_xml_free(xml_root);
		*err = "Success";
		ires =  SWITCH_STATUS_SUCCESS;
	}

	////����3 :��������ģ���ṩ��api����------------------------------------------------------------------------------
	//switch_status_t istatus;
	//switch_stream_handle_t stream	= { 0 };
	//SWITCH_STANDARD_STREAM(stream);
	//istatus = switch_api_execute("reloadxml", "reloadxml", NULL, &stream);
	//if (SWITCH_STATUS_SUCCESS != istatus){//��������ִ��������
	//	//��ӡ��־
	//	*err = "switch_api_execute :reload_acl_function failed!";
	//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "switch_api_execute :reload_acl_function failed! \n");
	//}
	//else{
	//	ires = SWITCH_STATUS_SUCCESS;
	//	*err = "Success";
	//}

	return ires;
}
