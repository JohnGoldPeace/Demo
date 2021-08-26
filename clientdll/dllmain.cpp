// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "Package.h"
#include "Func.h"


//extern HANDLE hMutex = NULL;
extern HANDLE  m_handleThreadSem;
extern HANDLE hThread;
extern SOCKET g_sockClient;
extern queue<const phread_t*>g_Data;
extern WSADATA g_wsaData;
extern SOCKADDR_IN g_addrSrv;
//extern HANDLE hMutex;
//extern HANDLE GetMessagehMutex;
extern bool FlagConnectServer;
extern HANDLE SendDataHandle;
//extern HANDLE SendMessageMutex;
//extern HANDLE GetRemindContentMutex;
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		m_handleThreadSem = CreateSemaphore(NULL, 0, 1, NULL);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DeleteMesssage();
		DeleteSendQueue();
		closesocket(g_sockClient);
		WSACleanup();
		TerminateThread(hThread, 0);
		TerminateThread(SendDataHandle,0);
		CloseHandle(hThread);
		//CloseHandle(hMutex);
		//CloseHandle(GetMessagehMutex);
		//CloseHandle(SendMessageMutex);
        //CloseHandle(GetRemindContentMutex);
		break;
	}
	return TRUE;
}

