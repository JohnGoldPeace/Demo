
#ifndef __PACKAGE__H
#define __PACKAGE__H
#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <fstream>
#include<io.h>
using namespace std;
#pragma comment(lib, "ws2_32.lib") 
#define DLLAPI __declspec(dllexport)
/******************************************************************/
struct phread_t
{
	int type;
	int length;
};
struct packheadV1
{
     phread_t Head;
     short ContentNumber;//包中内容块数量
     char SplitSymbol;//包中内容分割 0则无分割  //仅对字符串分割
     char defalut;//预留 字符对齐 
};

class  mtsClient
{
public:
	SOCKET         sockClient;
	SOCKADDR_IN	   addrSrv;
	USHORT         m_port;
	bool		   flagConnected;
	char *         pTempContent;
	int			   clientSession;
	int InnerInitClient();
	int InnerConnectServer();
	int InnerSendMessage(int type,int length,void *arg);
	phread_t InnerGetMessage();
	int InnerSendFile(char *PfilePath,int Type);
	int InnerGetRemindContent(char *pContent);
	int InnerSendString(char *Pstr,int StrType);
	int InnerGetDataHead(phread_t &ArgPhread);
	int InnerGetDataContent(int _ArgLength,void *_ArgPoint);
	int InnerSentDataHead(phread_t &ArgPhread);
	int InnerGetString(char *Pstr,int StrSize);
	//////////////////////////////V1/////////////////////////////
	int InnerSendMessageV1(packheadV1 ArgPackHead,void *Arg);
	int InnerSendStringV1(packheadV1 ArgPackHead,char* Pstr);
	int InnerGetDataHeadV1(packheadV1 &ArgPhread);
	int InnerSend(int ArgTotalByte,void *pData);
	int InnerGetStringV1(char *Pstr,int StrSize);
	int InnerSentDataHeadV1(packheadV1 &ArgPhread);
};
/**********************************************************/

/******************************************************************/

DLLAPI int _stdcall InitClient();
DLLAPI int _stdcall ConnectServer();
DLLAPI  int _stdcall SentMessage(int type,int length, void* arg);
DLLAPI phread_t _stdcall GetMMessage();
DLLAPI int _stdcall StartReadThread();
DLLAPI  int _stdcall SendFile(char *PfilePath,int Type);
DLLAPI int _stdcall StartSendThread();
void DeleteSendQueue();
 DLLAPI  int  _stdcall GetRemindContent(char *pContent);
 DLLAPI  int _stdcall SendString(char* Pstr,int StrType);
 DLLAPI int _stdcall Disconnect();
///////////////////////////////////V0/////////////////////////////////
//针对多客户端
//DLLAPI int _stdcall InitClientMul(const char* ipAddress,USHORT port,int &ReSession);
DLLAPI int _stdcall InitClientMul(const char* ipAddress,USHORT port,int RecTimeOut,int SenTimeOut,int &ReSession);
DLLAPI int _stdcall ConnectServerMul(int ClientSession);
DLLAPI  int _stdcall SentMessageMul(int type,int length, void* arg,int ClientSession);
DLLAPI phread_t _stdcall GetMMessageMul(int ClientSession);
DLLAPI  int _stdcall SendFileMul(char *PfilePath,int Type,int ClientSession);
DLLAPI  int  _stdcall GetRemindContentMul(char *pContent,int ClientSession);
DLLAPI  int _stdcall SendStringMul(char* Pstr,int StrType,int ClientSession);
DLLAPI int _stdcall ClientListIsHaveMsgV1(int ArgClientSession,int &IsHaveData);
//////////////////////////////////V1/////////////////////////////////////

#endif