// ClientDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "Package.h"
#include <map>
#include <iostream>
#define MAX_DATA_LENGTH 1024
bool bDebug = true;
void WriteLog(char *logContent);
HANDLE  m_handleThreadSem;
HANDLE hThread;
SOCKET g_sockClient;
queue<const phread_t*>g_Data;
queue<void *>g_SendData;
WSADATA g_wsaData;
SOCKADDR_IN g_addrSrv;
//HANDLE hMutex = NULL;
//HANDLE GetMessagehMutex = NULL;
bool g_FlagConnectServer = false;
int FileType=0;
HANDLE SendDataHandle;
//HANDLE SendMessageMutex = NULL;
//HANDLE GetRemindContentMutex =NULL;

queue<char*> g_Message; // remind Message
char *msgStr = NULL;
enum Code 
{
	CodeSendFile =1,
	CodeReceiveTPCFile = 2,
	CodeOK =3,
	CodeHeartBeat =4,
	AlarmParameterSetOK=5,
	CodeSTDF_init = 6,
	CodeSTDF_write_FAR = 7,
	CodeSTDF_write_ATR = 8,
	CodeSTDF_write_MIR = 9,
	CodeSTDF_write_SDR = 10,
	CodeSTDF_write_PIR = 11,
	CodeSTDF_write_PTR = 12,
	CodeSTDF_write_PRR = 13,
	CodeSTDF_write_HBR = 14,
	CodeSTDF_write_SBR = 15,
	CodeSTDF_write_TSR = 16,
	CodeSTDF_write_PCR = 17,
	CodeSTDF_write_MRR = 18,
	CodeSTDF_terminate = 19,
	CodeErrorConnect = 20,
    CodeCloseDataBase =21,
    CodeCreateDataBase=22,
    CodeFileName = 23,
    CodeAlarmStdev = 24,
    CodeAlarmCentralValue =25,
    CodeAlarmLimit =26,
    CodeAlarmSpecialBin=27,
    CodeAlarmSiteAverageSpan =28,
    CodeSingleProductFailStop =29,
    CodeLowYeildFailStop=30,
    CodeSite2SiteYeildStop=31,
    CodeContinueFailStop=32,
    CodeTestNumStop=33,
    CodeReadOperateSetting=34,
} _Code;
void* PackageData(int type ,int length,void *data)
{
    phread_t head;
    head.length = length;
    head.type = type;

    char* Pdata = (char*)malloc(sizeof(phread_t)+length);
    memset(Pdata,0,(sizeof(phread_t)+length));
    memcpy(Pdata,&head,sizeof(head));
    if(data)
    {
        memcpy(Pdata+sizeof(head),data,length);
    }
    return Pdata;
}
void *PackageDataV1(packheadV1 ArgPackHead,void *data)
{
	char* Pdata = (char*)malloc(sizeof(ArgPackHead)+ArgPackHead.Head.length);
    memset(Pdata,0,(sizeof(ArgPackHead)+ArgPackHead.Head.length));
	memcpy(Pdata,&ArgPackHead,sizeof(ArgPackHead));
	if(data&&ArgPackHead.Head.length>0)
	{
		memcpy(Pdata+sizeof(ArgPackHead),data,ArgPackHead.Head.length);
	}
	return Pdata;
}
unsigned int WINAPI ReadThread(LPVOID arg)
{
	while(1)
	{	
		phread_t *temp = new phread_t;
		//int _SO_ERROR=0; //读取TTL值
		//int Size_SO_ERROR = sizeof(_SO_ERROR);
		//WaitForSingleObject( hMutex, INFINITE );
		
		
	/*	int rc = getsockopt( sockClient, IPPROTO_TCP, SO_ERROR, (char *)&_SO_ERROR,&Size_SO_ERROR );
		if(_SO_ERROR!=0)
		{
			return 1;
		}*/
		int ret = recv(g_sockClient, (char*)temp, sizeof(phread_t), 0);
		if(ret>0)
		{
			//WaitForSingleObject(GetMessagehMutex,INFINITE);
			g_Data.push(temp);
			//ReleaseMutex( GetMessagehMutex );
			if(temp->length>0)
			{
				char *pTemp =(char*)malloc(temp->length+1);
				if(pTemp!=NULL)
				{
					memset(pTemp,0,temp->length+1);
					ret = recv(g_sockClient, pTemp, temp->length, 0);
					if(ret>0)
					{
                        //WaitForSingleObject(GetRemindContentMutex,INFINITE);
						g_Message.push(pTemp);
                        //ReleaseMutex(GetRemindContentMutex);
					}
					else
					{
						//phread_t heartbeatPack;
						//heartbeatPack.type = CodeHeartBeat;
      //                  heartbeatPack.length = 0;
						//ret = send(sockClient,(char*)&heartbeatPack,sizeof(phread_t),0);
						//if(ret<0)
						//{
						//	FlagConnectServer = false;
						//	temp->length =0;
						//	temp->type = CodeErrorConnect;
						//	//WaitForSingleObject(GetMessagehMutex,INFINITE);
						//	g_Data.push(temp);
						//	//ReleaseMutex( GetMessagehMutex );
						//	return 1;
						//}
					}
				}
			}
			//recv(sockClient,(char *)&da,sizeof(da),0);
		}
		else 
		{
			//phread_t heartbeatPack;
			//heartbeatPack.type = CodeHeartBeat;
			//ret = send(sockClient,(char*)&heartbeatPack,sizeof(phread_t),0);
			//if(ret<0)
			//{
			//	FlagConnectServer = false;
			//	temp->length =0;
			//	temp->type = CodeErrorConnect;
			//	//WaitForSingleObject(GetMessagehMutex,INFINITE);
			//	g_Data.push(temp);
			//	//ReleaseMutex( GetMessagehMutex );
			//	return 1;
			//}
			//delete temp;
		}
		//ReleaseMutex( hMutex );
		Sleep(30);
	}
	return 0;
}

unsigned int WINAPI SendThread(LPVOID arg)
{
	int rvalue =0;
	phread_t temp ;
	int cycle =0;
	int Rvalue = 0;
	while(1)
	{
		temp.length = 0;
		temp.type = 0;
		//WaitForSingleObject( SendMessageMutex, INFINITE );
        
		if(!g_SendData.empty())
		{	
			void *p = g_SendData.front();
            g_SendData.pop();
			//ReleaseMutex( SendMessageMutex);
            //WaitForSingleObject( hMutex, INFINITE );
			do 
			{
                cycle++;
                if(cycle>5)
                    Sleep(50);
                else if(cycle>1)
                    Sleep(10);
				Rvalue  = send(g_sockClient,(char*)p,sizeof(phread_t)+((phread_t*)p)->length,0);
				//ReleaseMutex( hMutex);
				
			} while (Rvalue<0&&cycle<10);
			if(cycle==10)
			{
				g_FlagConnectServer = false;
				phread_t *ptemp = new phread_t;
				ptemp->length =0;
				ptemp->type = CodeErrorConnect;
				//WaitForSingleObject(GetMessagehMutex,INFINITE);
				g_Data.push(ptemp);
				//ReleaseMutex( GetMessagehMutex);
                free(p);
				return 1;
			}
            if(p)
                free(p);
			cycle =0;
            /*
			temp.length = 0;
			temp.type = 0;
			while(1) 
			{
				//WaitForSingleObject( hMutex, INFINITE );
				Rvalue = recv(sockClient, (char*)&temp, sizeof(phread_t), 0);
			//	ReleaseMutex( hMutex );
				if(temp.type==CodeOK)
					break;
                else if(temp.type!=0&&temp.length!=0)
                {
                    int c =10;
                }
				if(Rvalue<0)
				{
					phread_t heartbeatPack;
					heartbeatPack.type = CodeHeartBeat;
					//WaitForSingleObject( hMutex, INFINITE );
					Rvalue = send(sockClient,(char*)&heartbeatPack,sizeof(phread_t),0);
					//ReleaseMutex( hMutex );
					if(Rvalue<0)
					{
						FlagConnectServer = false;
						phread_t *ptemp = new phread_t;
						ptemp->length =0;
						ptemp->type = CodeErrorConnect;
						//WaitForSingleObject(GetMessagehMutex,INFINITE);
						g_Data.push(ptemp);
						ReleaseMutex( GetMessagehMutex);
						return 1;
					}
				}
			} 
			if(Rvalue<0)
			{
				rvalue = 1;	
			}
			if(p)
				free(p);
                */
            //ReleaseMutex( hMutex );
			//WaitForSingleObject( SendMessageMutex, INFINITE );
			//g_SendData.pop();
			//ReleaseMutex( SendMessageMutex);
		}
		else
		{
			//ReleaseMutex( SendMessageMutex);
			Sleep(10);
		}
		
		
	}
	
	
	
}
DLLAPI phread_t _stdcall ReceiveMessage()
{
	//WaitForSingleObject( hMutex, INFINITE );
	phread_t temp ;
	temp.length = 0;
	temp.type = 0;
	int ret = recv(g_sockClient, (char*)&temp, sizeof(phread_t), 0);
	if(ret<0)
	{
		phread_t heartbeatPack;
		heartbeatPack.type = CodeHeartBeat;
		ret = send(g_sockClient,(char*)&heartbeatPack,sizeof(phread_t),0);
		if(ret<0)
		{
			g_FlagConnectServer = false;
			temp.length =0;
			temp.type = CodeErrorConnect;
		}
		else
		{
			temp.length = 0;
			temp.type = 0;
		}

	}
	//ReleaseMutex( hMutex );
	return temp;
	
}

DLLAPI int _stdcall InitClient()
{
	g_FlagConnectServer = false;
	/*hMutex = CreateMutex( NULL,
			FALSE, 
			NULL );
	GetMessagehMutex = CreateMutex( NULL,
		FALSE, 
		NULL );
	SendMessageMutex = CreateMutex( NULL,
		FALSE, 
		NULL );
    GetRemindContentMutex = CreateMutex( NULL,
        FALSE, 
        NULL );*/
	if (WSAStartup(MAKEWORD(2, 2), &g_wsaData) != 0)
	{
		return -1;
	}
	g_addrSrv.sin_family = AF_INET;
	g_addrSrv.sin_port = htons(8081);
	g_addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	g_sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == g_sockClient)
	{
		//printf("Socket() error:%d", WSAGetLastError());
		return -1;
	}
	int nNetTimeout = 5000;
	setsockopt(g_sockClient,SOL_SOCKET,SO_RCVTIMEO, (char *)&nNetTimeout,sizeof(int));
	int nSendTimeOut = 10000;
	setsockopt(g_sockClient,SOL_SOCKET,SO_SNDTIMEO, (char *)&nSendTimeOut,sizeof(int));

	
	// HANDLE DLLHandle =GetCurrentThread();
	//bool RE = SetThreadPriority(DLLHandle,THREAD_PRIORITY_TIME_CRITICAL);
	//if(!RE)
		//return -1;
	//ConnectServer();
	
	return 0;
	
}
DLLAPI int _stdcall ConnectServer()
{
		
	if (connect(g_sockClient, (struct  sockaddr*)&g_addrSrv, sizeof(g_addrSrv)) == INVALID_SOCKET)
	{
		//printf("Connect failed:%d", WSAGetLastError());
		return -1;
	}
	g_FlagConnectServer = true;
    //StartSendThread();
	return 0;
}

DLLAPI int _stdcall Disconnect()
{
	int revalue = 0;
	if(g_FlagConnectServer=true)
	{
		closesocket(g_sockClient);
		WSACleanup();
		g_FlagConnectServer = false;
	}
	return revalue;
}
DLLAPI int _stdcall StartReadThread()
{
	return 0;
    if(!g_FlagConnectServer)
        return 1;
	hThread = (HANDLE)::_beginthreadex(NULL, 0, ReadThread, NULL, 0, NULL);
	//bool Flag = SetThreadPriority(hThread,THREAD_PRIORITY_IDLE);
	//if(!Flag)
	//{
		//return -1;
	//}
	return 0;
}
DLLAPI int _stdcall StartSendThread()
{
    if(!g_FlagConnectServer)
        return 1;
    if(SendDataHandle!=NULL)
        //TerminateThread(SendDataHandle,0);
	//SendDataHandle = (HANDLE)::_beginthreadex(NULL, 0, SendThread, NULL, 0, NULL);
	return 0;
}
DLLAPI phread_t _stdcall GetMMessage()
{
	phread_t temp ;
	temp.length = 0;
	temp.type = 0;
	
	int ret = recv(g_sockClient, (char*)(&temp), sizeof(phread_t), 0);

	if(temp.length > 0)
	{
		msgStr = (char*)malloc(temp.length+1);
		memset(msgStr,0,temp.length+1);
		ret = recv(g_sockClient, msgStr, temp.length, 0);
	}
	return temp;
}
char*  Gpoint = NULL;
DLLAPI  int _stdcall GetRemindContent(char *pContent)
{
	int rvalue = 0;
    if(pContent!=NULL && msgStr != NULL)
    {
		memset(pContent,0,sizeof(pContent));
		memcpy(pContent,msgStr,strlen(msgStr));
		free(msgStr);
		msgStr = NULL;
    }
	
	return rvalue;
}
void DeleteMesssage()
{
    
	while(!g_Data.empty())
	{
		delete g_Data.front();
		g_Data.pop();
	}
    
    
}

void DeleteSendQueue()
{
	while(!g_SendData.empty())
	{
		free(g_SendData.front()) ;
		g_SendData.pop();
	}
}
DLLAPI  int _stdcall ClearRemindQueue()
{
    //WaitForSingleObject(GetRemindContentMutex,INFINITE);
    while(!g_Message.empty())
    {
        free( g_Message.front());
        g_Message.pop();
    }
    //ReleaseMutex(GetRemindContentMutex);
    ////WaitForSingleObject(GetMessagehMutex,INFINITE);
    while(!g_Data.empty())
    {
        delete g_Data.front();
        g_Data.pop();
    }
    //ReleaseMutex(GetMessagehMutex);
    return 0;
}
DLLAPI  int _stdcall SentMessage(int type,int length, void* arg)
{
	void *pack =NULL;
    if(!g_FlagConnectServer)
        return 1;
	pack = PackageData(type,length,arg);
	//WaitForSingleObject( SendMessageMutex, INFINITE );
	//g_SendData.push(pack);
	int sendcount  = send(g_sockClient,(char*)pack,sizeof(phread_t)+((phread_t*)pack)->length,0);
	free(pack) ;
	//ReleaseMutex( SendMessageMutex );
	if(sendcount<=0&&bDebug)
	{
		char TempLogBuf[1024] = {0};
		sprintf(TempLogBuf,"Send ReValue:%d\r\n",sendcount);
		WriteLog(TempLogBuf);
	}
	if(sendcount<=0)
		return 1;
	return 0;
}
DLLAPI  int _stdcall SendString(char* Pstr,int StrType)
{
    int revalue =0;
    if(!g_FlagConnectServer)
        return 1;
    if(Pstr!=NULL)
    { 
        phread_t head;
        //head.length = length;
        head.type = StrType;
        char* Pdata = NULL;
        head.length = strlen(Pstr);
        Pdata = (char*)malloc(sizeof(phread_t)+head.length);
        if(Pdata!=NULL)
        {
            memcpy(Pdata,&head,sizeof(head));
        }
        else
            revalue =1;
        if(Pstr!=NULL&&Pdata!=NULL)
        {
            memcpy(Pdata+sizeof(head),Pstr,head.length);
        }
        else
            revalue = 1;
        //WaitForSingleObject( SendMessageMutex, INFINITE );
        //g_SendData.push((void *)Pdata);
        //ReleaseMutex( SendMessageMutex );
		int sendcount  = send(g_sockClient,(char*)Pdata,sizeof(phread_t)+((phread_t*)Pdata)->length,0);
    }
    else
        revalue =1;
    return revalue;
}
/*
DLLAPI  int _stdcall SentMessage(int type,int length, void* arg)
{
	if(!FlagConnectServer)
		return 1;
	int rvalue =0;
	void *pack =NULL;
	int flag = 0;
	WaitForSingleObject( hMutex, INFINITE );
	pack = PackageData(type,length,arg);
	int  tmp  = send(sockClient,(char*)pack,sizeof(phread_t)+length,0);
	if(tmp<0)
		rvalue = 1;	
	if(pack)
		free(pack);
	ReleaseMutex( hMutex );
	return rvalue;
}
*/
unsigned int _stdcall _SendFile(void *p)
{
	char *PfilePath = (char*)p;
	char buffer[1024];
	//char *PfilePath = NULL;
	/*if(PathLength>0)
	{
		PfilePath= (char*)malloc(PathLength);
		memcpy(PfilePath,_PfilePath,PathLength);
	}*/
	FILE  *m_fp=NULL;
	void * pack;
	int result =0;
	int bytelen =0;
	int size =0;
	//WaitForSingleObject( hMutex, INFINITE );
	if(PfilePath)
	{
		result = fopen_s(&m_fp,PfilePath,"rb");
		if(result)
			return 1;
		bytelen = fread(buffer,1,1024,m_fp);
		if(bytelen>0)
		{
			int filesize = filelength(fileno(m_fp));
			pack = PackageData(FileType,filesize,NULL);
			if(pack)
			{
				result = send(g_sockClient,(char*)pack,sizeof(phread_t),0);
				if(result<0)
				{
					free(pack);
					return 1;	
				}
			}
			
		}
		while(bytelen>0)
		{
			result = send(g_sockClient,buffer,bytelen,0);
			size +=result;
			if(result<1024)
			{
				int c = 10;
			}
			phread_t temp;
			/*do 
			{
				if(result<0)
				{
					return 1;
				}
				memset(&temp,0,sizeof(temp));
				int ret = recv(sockClient, (char*)&temp, sizeof(phread_t), 0);
				Sleep(10);
			} while (temp.type!=CodeOK);*/
			memset(buffer,0,sizeof(buffer));
			bytelen = fread(buffer,1,1024,m_fp);
			if(bytelen<1024)
			{
				int a = 10;
			}
		}
	}
	//ReleaseMutex( hMutex );
	if(m_fp)
		fclose(m_fp);
	if(PfilePath)
		free(PfilePath);
	return 0;
}
DLLAPI  int _stdcall SendFile(char *PfilePath,int Type)
{
	HANDLE threadSendFIle;
	if(!g_FlagConnectServer)
		return 1;
	char *path = (char*)malloc(strlen(PfilePath)+1);
	strcpy(path,PfilePath);
	FileType = Type;
	//threadSendFIle = (HANDLE)::_beginthreadex(NULL, 0, _SendFile, (void* )path, 0, NULL);
	int ret = _SendFile(path);
	
	return 0;
}

void WriteLog(char *logContent)
{
	char  logFilePath[MAX_DATA_LENGTH]= {0}; 
	char  logPathName[MAX_DATA_LENGTH]="C:"; 
    //_getcwd(logFilePath, MAX_DATA_LENGTH);
    sprintf_s(logPathName, MAX_DATA_LENGTH, "%s\\ClientLog.txt", logFilePath);
    //printf("%s\n", logContent);
    ofstream ofs(logPathName, ios::app);
    ofs.write(logContent, strlen(logContent));
    ofs<<endl;
    ofs.close();
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
//多客户端
///////////////////////////////////////////////////////////////////////////////////////////////////////
std::map<int,mtsClient> g_ClientMap;
map<int,mtsClient>::iterator g_ServerIterator; 
int g_Session = 1000;//用于同一线程调用DLL连接不同服务端

int mtsClient::InnerInitClient()
{
	int revalue =0;
	return revalue;
}

int mtsClient::InnerConnectServer()
{
	int revalue =0,tempRevalue =0,len = 0,error = 0;
	fd_set tempSet ;
	FD_ZERO(&tempSet);
	FD_SET(sockClient,&tempSet);
	timeval tm = {2,0};
	unsigned long mode = 0;
	
	if (connect(sockClient, (struct  sockaddr*)&addrSrv, sizeof(addrSrv)) == INVALID_SOCKET)
	{
		tempRevalue = select(sockClient+1,0,&tempSet,0,&tm);
		if(tempRevalue>0)
		{
			getsockopt(sockClient,SOL_SOCKET,SO_ERROR,(char *)&error,&len) ;
			if(error!=0)
			{
				flagConnected = false;
				revalue = -1;
			}
			else 
			{
				int result = ioctlsocket(sockClient, FIONBIO, &mode);
				flagConnected = true;
			}
		}
		else 
		{
			revalue = -1;
			flagConnected = false;
		}
	}
	else 
	{
		int result = ioctlsocket(sockClient, FIONBIO, &mode);
		flagConnected = true;
	}
	return revalue;
}
int mtsClient::InnerSendMessage(int type,int length,void *arg)
{
	int revalue =0;
	void *pack =NULL;
    if(!flagConnected)
        revalue = 1;
	if(revalue==0)
	{
		pack = PackageData(type,length,arg);
		int sendcount  = send(sockClient,(char*)pack,sizeof(phread_t)+((phread_t*)pack)->length,0);
		free(pack) ;
		if(sendcount<=0&&bDebug)
		{
			char TempLogBuf[1024] = {0};
			sprintf(TempLogBuf,"Send ReValue:%d\r\n",sendcount);
			WriteLog(TempLogBuf);
		}
		if(sendcount<=0)
			revalue = -1;
	}
	return revalue;
}

phread_t mtsClient::InnerGetMessage()
{
	phread_t temp ;
	temp.length = 0;
	temp.type = 0;
	int ret = recv(sockClient, (char*)(&temp), sizeof(phread_t), 0);
	if(temp.length > 0)
	{
		msgStr = (char*)malloc(temp.length+1);
		memset(pTempContent,0,temp.length+1);
		ret = recv(sockClient, pTempContent, temp.length, 0);
	}
	return temp;
}
int mtsClient::InnerSendFile(char *PfilePath,int Type)
{
	int revalue =0;
	char buffer[1024];
	FILE  *m_fp=NULL;
	void * pack;
	int result =0;
	int bytelen =0;
	int size =0;
	if(PfilePath==NULL)
		revalue = -1;
	if(revalue==0)
	{
		result = fopen_s(&m_fp,PfilePath,"rb");
		if(result)
			return 1;
		bytelen = fread(buffer,1,1024,m_fp);
		if(bytelen>0)
		{
			int filesize = filelength(fileno(m_fp));
			pack = PackageData(FileType,filesize,NULL);
			if(pack)
			{
				result = send(sockClient,(char*)pack,sizeof(phread_t),0);
				if(result<0)
				{
					free(pack);
					revalue = -1;	
				}
			}
		}
		while(bytelen>0)
		{
			result = send(sockClient,buffer,bytelen,0);
			size +=result;
			phread_t temp;
			memset(buffer,0,sizeof(buffer));
			bytelen = fread(buffer,1,1024,m_fp);
		}
	}
	if(m_fp)
		fclose(m_fp);
	if(PfilePath)
		free(PfilePath);
	return revalue;
}
int mtsClient::InnerGetRemindContent(char *pContent)
{
	int revalue =0;
	if(pContent!=NULL && pTempContent != NULL)
    {
		memset(pContent,0,strlen(pContent));
		memcpy(pContent,pTempContent,strlen(pTempContent));
		free(pTempContent);
		pTempContent = NULL;
    }
	else 
		revalue = -1;
	return revalue;
}
int mtsClient::InnerSendString(char *Pstr,int StrType)
{
	int revalue =0;
    if(!flagConnected)
        revalue =  1;
	if(Pstr==NULL)
		revalue = -1;
    if(revalue==0)
    { 
        phread_t head;
        head.type = StrType;
        char* Pdata = NULL;
        head.length = strlen(Pstr);
        Pdata = (char*)malloc(sizeof(phread_t)+head.length);
        if(Pdata!=NULL)
        {
            memcpy(Pdata,&head,sizeof(head));
        }
        else
            revalue =1;
        if(Pstr!=NULL&&Pdata!=NULL)
        {
            memcpy(Pdata+sizeof(head),Pstr,head.length);
        }
        else
            revalue = 1;
		int sendcount  = send(sockClient,(char*)Pdata,sizeof(phread_t)+((phread_t*)Pdata)->length,0);
		if(Pdata!=NULL)
			free(Pdata);
    }
	return revalue;
}
int mtsClient::InnerGetString(char *Pstr,int StrSize)
{
	int revalue = 0;
	if(!flagConnected)
        revalue =  1;
	if(StrSize>0&&Pstr!=NULL&&revalue == 0)
	{
		revalue = InnerGetDataContent(StrSize,Pstr);
	}
	else 
		revalue = -1;
	return revalue;
}

int mtsClient::InnerGetDataHead(phread_t &ArgPhread)
{
	int revalue = 0,ret=0,ReTryTimes = 0,RevLength = 0;
	int TargetLength = sizeof(ArgPhread);
	if(!flagConnected)
        revalue =  1;
	if(revalue==0)
	{
		do
		{
			RevLength = recv(sockClient, (char*)(&ArgPhread), sizeof(ArgPhread), 0);
			if(RevLength<0)
			{
				ReTryTimes++;
				if(ReTryTimes>2)
				{
					revalue = -1;
					break;
				}
			}
			else if(RevLength==0)
			{
				ReTryTimes++;
				if(ReTryTimes>5)
				{
					revalue = -1;
					break;
				}
				else 
					Sleep(5);
			}
			if(RevLength>0)
				ret+=RevLength;
		}
		while(ret<TargetLength);
	}
	return revalue;
}

int mtsClient::InnerGetDataContent(int _ArgLength,void *_ArgPoint)
{
	int ret=0,RevLength=0,ReTryTimes=0,revalue = 0;
	
	if(!flagConnected)
        revalue =  1;
	if(_ArgLength<=0||(_ArgPoint==NULL))
		revalue = -1;
	if(revalue==0)
	{
		memset(_ArgPoint,0,_ArgLength);
		do
		{
			RevLength = recv(sockClient,(char*) _ArgPoint,_ArgLength, 0);
			ret+=RevLength;
			if(RevLength<0)
			{
				revalue = -1;
				break;
			}
			else if(RevLength==0)
			{
				ReTryTimes++;
				if(ReTryTimes>5)
				{
					revalue = -1;
					break;
				}
				else 
					Sleep(5);
			}
			
		}while(ret<_ArgLength);
	}
	return revalue;
}
int mtsClient::InnerSentDataHead(phread_t &ArgPhread)
{
	int revalue = 0,sendCount= 0;
	if(!flagConnected)
        revalue =  1;
	if(revalue==0)
	{
		sendCount = send(sockClient,(char*)&ArgPhread,sizeof(ArgPhread),0);
		if(sendCount<=0)
			revalue = -1;
	}
	return revalue ;
}
////////////////////////////////////Inner V1////////////////////////////////////////////////////////////////////////
int mtsClient::InnerSendMessageV1(packheadV1 ArgPackHead,void *Arg)
{
	int revalue = 0,totalCount = 0,totalSend = 0;
	void *pack =NULL;
	totalCount = sizeof(ArgPackHead)+ArgPackHead.Head.length;
	pack = PackageDataV1(ArgPackHead,Arg);
	while(totalCount!=totalSend)
	{
		int sendcount  = send(sockClient,(char*)pack,totalCount,0);
		if(sendcount<0)
		{
			revalue = -1;
			break;
		}
		else 
		{
			totalSend +=sendcount;
		}
	}
	return revalue;
}
int mtsClient::InnerGetDataHeadV1(packheadV1 &ArgPhread)
{
	int revalue = 0,ret=0,ReTryTimes = 0,RevLength = 0;
	int TargetLength = sizeof(ArgPhread);
	if(!flagConnected)
        revalue =  1;
	if(revalue==0)
	{
		do
		{
			RevLength = recv(sockClient, (char*)(&ArgPhread), sizeof(ArgPhread), 0);
			if(RevLength<0)
			{
				ReTryTimes++;
				if(ReTryTimes>2)
				{
					revalue = -1;
					break;
				}
			}
			else if(RevLength==0)
			{
				ReTryTimes++;
				if(ReTryTimes>5)
				{
					revalue = -1;
					break;
				}
				else 
					Sleep(5);
			}
			if(RevLength>0)
				ret+=RevLength;
		}
		while(ret<TargetLength);
	}
	return revalue;
}


int mtsClient::InnerSendStringV1(packheadV1 ArgPackHead,char* Pstr)
{
	int revalue =0,TotalByte = 0;
	char *pData = NULL;
	if(Pstr==NULL||(ArgPackHead.Head.length <=0))
		revalue = -1;
	else 
	{
		TotalByte = sizeof(ArgPackHead)+ArgPackHead.Head.length;
	}
    if(revalue==0)
    { 
        pData = (char*)malloc(TotalByte);
        if(pData!=NULL)
        {
            memcpy(pData,&ArgPackHead,sizeof(ArgPackHead));
        }
        else
            revalue =1;
        if(Pstr!=NULL&&pData!=NULL)
        {
            memcpy(pData+sizeof(ArgPackHead),Pstr,ArgPackHead.Head.length);
			revalue = InnerSend(TotalByte,pData);
        }
        else
            revalue = 1;
    }
	return revalue;
}
int mtsClient::InnerGetStringV1(char *Pstr,int StrSize)
{
	int revalue = 0;
	if(!flagConnected)
        revalue =  1;
	if(StrSize>0&&Pstr!=NULL&&revalue == 0)
	{
		revalue = InnerGetDataContent(StrSize,Pstr);
	}
	else 
		revalue = -1;
	return revalue;
}

int mtsClient::InnerSend(int ArgTotalByte,void *pData)
{
	int revalue = 0,SendTotal=0;
	if(pData!=NULL&&ArgTotalByte>0)
	{	
		while(ArgTotalByte!=SendTotal)
		{
			int sendcount  = send(sockClient,(char*)pData,ArgTotalByte,0);
			if(sendcount<0)
			{
				revalue = -1;
				break;
			}
			else 
			{
				SendTotal +=sendcount;
			}
		}
	}
	else 
		revalue = -1;
	return revalue;
}
int mtsClient::InnerSentDataHeadV1(packheadV1 &ArgPhread)
{
	int revalue = 0,sendCount= 0;
	revalue = InnerSend(sizeof(ArgPhread),(void *)&ArgPhread);
	return revalue ;
}
///////////////////////////////////Exprot V0///////////////////////////////////////////////////////////////

DLLAPI int _stdcall InitClientMul(const char* ipAddress,USHORT port,int RecTimeOut,int SenTimeOut,int &ReSession)
{
	int revalue = 0;
	mtsClient TempClient;
	TempClient.flagConnected = false;
	if (WSAStartup(MAKEWORD(2, 2), &g_wsaData) != 0)
		revalue =  -1;
	if(ipAddress==NULL)
		revalue = -1;
	if(revalue==0)
	{
		TempClient.addrSrv.sin_family = AF_INET;
		TempClient.addrSrv.sin_port = htons(port);
		TempClient.addrSrv.sin_addr.S_un.S_addr = inet_addr(ipAddress);
		TempClient.sockClient = socket(AF_INET, SOCK_STREAM, 0);
		if (SOCKET_ERROR == TempClient.sockClient)
			revalue = -1;
	}
	unsigned long mode = 1;
	int result = ioctlsocket(TempClient.sockClient, FIONBIO, &mode);
	if(revalue ==0)
	{
		const char chOpt = 1;
		setsockopt(TempClient.sockClient, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));//no delay send
		if(RecTimeOut>0)
			setsockopt(TempClient.sockClient,SOL_SOCKET,SO_RCVTIMEO, (char *)&RecTimeOut,sizeof(int));
		if(SenTimeOut>0)
			setsockopt(TempClient.sockClient,SOL_SOCKET,SO_SNDTIMEO, (char *)&SenTimeOut,sizeof(int));
		while(g_ClientMap.find(g_Session)!=g_ClientMap.end())
		{
			g_Session++;
		}
		g_ClientMap.insert(std::pair<int,mtsClient>(g_Session,TempClient));
		ReSession = g_Session;
		g_Session++;
	}
	return revalue;
}
DLLAPI int _stdcall ConnectServerMul(int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerConnectServer();
	return revalue ;
}
DLLAPI  int _stdcall SentMessageMul(int type,int length, void* arg,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerSendMessage(type,length,arg);
	return revalue ;
}
DLLAPI phread_t _stdcall GetMessageMul(int ClientSession)
{
	int revalue = 0;
	phread_t tempHead;
	tempHead.length = 0;
	tempHead.type = -1;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		return tempHead;
	if(revalue ==0)
		 return g_ServerIterator->second.InnerGetMessage();
}
DLLAPI  int _stdcall SendFileMul(char *PfilePath,int Type,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerSendFile(PfilePath,Type);
	return revalue;
}
DLLAPI  int  _stdcall GetRemindContentMul(char *pContent,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerGetRemindContent(pContent);
	return revalue;
}
DLLAPI  int _stdcall SendStringMul(char* Pstr,int StrType,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerSendString(Pstr,StrType);
	return revalue;
}
DLLAPI int _stdcall GetStringMul(char *Pstr,int StrSize,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerGetString(Pstr,StrSize);
	return revalue;
}
DLLAPI int _stdcall CloseClientMul(int ClientSession)
{
	int revalue = 0 ;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
	{
		closesocket(g_ServerIterator->second.sockClient);
		WSACleanup();
	}	
	return revalue;
		
}
DLLAPI int _stdcall GetDataHead(int ClientSession,phread_t & ArgDataHead)
{
	int revalue = 0;
	ArgDataHead.length = 0;
	ArgDataHead.type = -1;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	else 
	{
		revalue = g_ServerIterator->second.InnerGetDataHead(ArgDataHead);
	}
	return revalue;
}

DLLAPI int _stdcall GetDataContent(int ClientSession,int _ArgLength,void *_ArgPoint)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	else 
	{
		revalue = g_ServerIterator->second.InnerGetDataContent(_ArgLength,_ArgPoint);
	}
	return revalue;
}

DLLAPI int SentDataHeadMul(int ClientSession,phread_t &ArgPhread)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue == 0)
	{
		revalue = g_ServerIterator->second.InnerSentDataHead(ArgPhread);
	}
	return revalue;
}
/////////////////////////////////Export V1//////////////////////////////////////////////////////////////////////
DLLAPI  int _stdcall SentMessageMulV1(packheadV1 & ArgPackHead, void* arg,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerSendMessageV1(ArgPackHead,arg);
	return revalue ;
}
DLLAPI  int _stdcall SendStringMulV1(packheadV1 &ArgPackHead,char* Pstr,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerSendStringV1(ArgPackHead,Pstr);
	return revalue;
}
DLLAPI int _stdcall GetStringMulV1(char *Pstr,int StrSize,int ClientSession)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
		revalue = g_ServerIterator->second.InnerGetString(Pstr,StrSize);
	return revalue;
}
DLLAPI int _stdcall GetDataHeadV1(int ClientSession,packheadV1 & ArgDataHead)
{
	int revalue = 0;
	ArgDataHead.Head.length=0;
	ArgDataHead.Head.type = -1;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	else 
	{
		revalue = g_ServerIterator->second.InnerGetDataHeadV1(ArgDataHead);
	}
	return revalue;
}

DLLAPI int _stdcall GetDataContentV1(int ClientSession,int _ArgLength,void *_ArgPoint)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	else 
	{
		revalue = g_ServerIterator->second.InnerGetDataContent(_ArgLength,_ArgPoint);
	}
	return revalue;
}
DLLAPI int _stdcall SentDataHeadMulV1(int ClientSession,packheadV1 &ArgPhread)
{
	int revalue = 0;
	g_ServerIterator = g_ClientMap.find(ClientSession);
	if(g_ServerIterator==g_ClientMap.end())
		revalue = -1;
	if(revalue == 0)
	{
		revalue = g_ServerIterator->second.InnerSentDataHeadV1(ArgPhread);
	}
	return revalue;
}
DLLAPI int _stdcall ClientListIsHaveMsgV1(int ArgClientSession,int &IsHaveData)
{
	int revalue = 0,tempRevalue=0,len = 0,error = 0;
	map<int,mtsClient>::iterator IClient;
	IClient = g_ClientMap.find(ArgClientSession);
	fd_set tempSet ;
	timeval tm = {0,10};
	IsHaveData = 0;
	if(IClient==g_ClientMap.end())
		revalue = -1;
	if(revalue ==0)
	{
		FD_ZERO(&tempSet);
		FD_SET(IClient->second.sockClient,&tempSet);
		tempRevalue = select(IClient->second.sockClient+1,&tempSet,0,0,&tm);
		if(tempRevalue>0)
		{
			getsockopt(IClient->second.sockClient,SOL_SOCKET,SO_ERROR,(char *)&error,&len) ;
			if(error==0)
				IsHaveData= 1;
		}
	}
	return revalue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

DLLAPI int _stdcall IsHaveMsg(int &IsHaveData)
{
	int revalue = 0,tempRevalue=0,len = 0,error = 0;
	fd_set tempSet ;
	timeval tm = {0,10};
	IsHaveData = 0;
	if(revalue ==0)
	{
		FD_ZERO(&tempSet);
		FD_SET(g_sockClient,&tempSet);
		tempRevalue = select(g_sockClient+1,&tempSet,0,0,&tm);
		if(tempRevalue>0)
		{
			getsockopt(g_sockClient,SOL_SOCKET,SO_ERROR,(char *)&error,&len) ;
			if(error==0)
				IsHaveData= 1;
		}
	}
	return revalue;
}