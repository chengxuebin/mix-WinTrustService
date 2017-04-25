#include "WinTrustServiceMain.h"
#include "mix-mutex.h"
int _tmain (int argc, TCHAR *argv[])
{
    WriteToLog(("My Sample Service: Main: Entry\n"));

    SERVICE_TABLE_ENTRY ServiceTable[] = 
    {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher (ServiceTable) == FALSE)
    {
       WriteToLog(("My Sample Service: Main: StartServiceCtrlDispatcher returned error\n"));
       return GetLastError ();
    }

    WriteToLog(("My Sample Service: Main: Exit\n"));
    return 0;
}

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv)
{
    DWORD Status = E_FAIL;
	HANDLE hThread = NULL;
	HANDLE hDeviceEventThread= NULL;

    WriteToLog("My Sample Service: ServiceMain: Entry\n");

    g_StatusHandle = RegisterServiceCtrlHandlerEx (SERVICE_NAME, ServiceCtrlHandlerEx, NULL);
    if (g_StatusHandle == NULL) 
    {
        WriteToLog(("My Sample Service: ServiceMain: RegisterServiceCtrlHandler returned error\n"));
        return;
    }

	//xiaxu 2016/12/14 { 
	//注册设备通知
	if( DoRegisterDeviceInterface(GUID_DEVCLASS_USB, &g_hDevNotify) != TRUE)
	{
		return;
	}

	GUID   hid_guid={0x4D1E55B2,0xF16F,0x11CF,0x88,0xCB,0x00,0x11,0x11,0x00,0x00,0x30};
	if( DoRegisterDeviceInterface(hid_guid, &g_hid_hDevNotify)!= TRUE)
	{
		return;
	}

	GUID disk_guid = 	{0x53F56307, 0xB6BF, 0x11D0, 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B};
	if( DoRegisterDeviceInterface(disk_guid, &g_disk_hDevNotify)!= TRUE)
	{
		return;
	}

	GUID cdrom_guid = {0x53F56308, 0xB6BF, 0x11D0, 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B};
	if( DoRegisterDeviceInterface(cdrom_guid, &g_cdrom_hDevNotify)!= TRUE)
	{
		return;
	}
	//xiaxu 2016/12/14 }

    // Tell the service controller we are starting
    ZeroMemory (&g_ServiceStatus, sizeof (g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;//启动
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) 
    {
        WriteToLog(("My Sample Service: ServiceMain: SetServiceStatus returned error\n"));
    }
     // Perform tasks neccesary to start the service here    
    WriteToLog(("My Sample Service: ServiceMain: Performing Service Start Operations\n"));

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);// 安全:null， 手动：true， 初始：未触发
    if (g_ServiceStopEvent == NULL) 
    {
        WriteToLog(("My Sample Service: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error\n"));

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
	    {
		    WriteToLog(("My Sample Service: ServiceMain: SetServiceStatus returned error\n"));
	    }
        return;
    }    

	//devices event

	// Create stop event to wait on later.  g_ServiceDevicesEvent
	g_ServiceDevicesEvent = CreateEvent (NULL, TRUE, FALSE, NULL);// 安全:null， 手动：true， 初始：未触发
	if (g_ServiceStopEvent == NULL) 
	{
		WriteToLog(("My Sample Service: ServiceMain: CreateEvent(g_ServiceDevicesEvent) returned error\n"));

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			WriteToLog(("My Sample Service: ServiceMain: SetServiceStatus returned error\n"));
		}
		return;
	} 
	
    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
	    WriteToLog(("My Sample Service: ServiceMain: SetServiceStatus returned error\n"));
    }

    // Start the thread that will perform the main task of the service
    hThread = CreateThread (NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	if (NULL == hThread)
	{
		WriteToLog("CreateThread error\n");
		return;
	}

    WriteToLog(("My Sample Service: ServiceMain: Waiting for Worker Thread to complete\n"));


	// Start the thread that will perform the main task of the service
	hDeviceEventThread = CreateThread (NULL, 0, ServiceDeviesEvent, NULL, 0, NULL);
	if (NULL == hDeviceEventThread)
	{
		WriteToLog("CreateThread for  ServiceDeviesEvent error\n");
		return;
	}

	WriteToLog(("My Sample Service: ServiceMain: Waiting for DeviesEvent Thread to complete\n"));

	// Wait until our DeviesEvent thread exits effectively signaling that the service needs to stop
	//WaitForSingleObject (hDeviceEventThread, INFINITE); 
	//CloseHandle(hDeviceEventThread);

    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject (hThread, INFINITE);
	if( WAIT_OBJECT_0 != WaitForSingleObject(hDeviceEventThread,200) )
	{
		DWORD dwExitCode = 0;
		TerminateThread(hDeviceEventThread, dwExitCode); 
		CloseHandle(hDeviceEventThread);
	}
	
	CloseHandle(hThread);
	CloseHandle (g_ServiceStopEvent);
	CloseHandle(g_ServiceDevicesEvent);

    
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
	    WriteToLog(("My Sample Service: ServiceMain: SetServiceStatus returned error\n"));
    }

	WriteToLog(("My Sample Service: ServiceMain: Exit\n"));
}

int  __stdcall ThreadFunc_DeviceChange(void* param)  
{   
	BOOL bPopupBrowser = FALSE;

	if (!LaunchAppIntoDifferentSession(bPopupBrowser) )
	{
		goto EndOP;
	}
	return TRUE;
EndOP:
	return FALSE;
}

DWORD WINAPI ServiceCtrlHandlerEx(
	DWORD    dwControl,
	DWORD    dwEventType,
	LPVOID    lpEventData,
	LPVOID    lpContext
	)
{
    WriteToLog(("My Sample Service: ServiceCtrlHandler: Entry"));

    switch (dwControl) 
	{
     case SERVICE_CONTROL_STOP :

        WriteToLog(("My Sample Service: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
           break;

		//xiaxu 2016/12/14 { 
		// 卸载设备通知
		//AfxMessageBox("quit");
		UnregisterDeviceNotification(g_hDevNotify);
		UnregisterDeviceNotification(g_hid_hDevNotify);
		UnregisterDeviceNotification(g_cdrom_hDevNotify);
		UnregisterDeviceNotification(g_disk_hDevNotify  );  
		//xiaxu 2016/12/14 }
        /* 
         * Perform tasks neccesary to stop the service here 
         */

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			WriteToLog("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error");
		}
		// This will signal the worker thread to start shutting down
		SetEvent (g_ServiceStopEvent);
        break;
	 case SERVICE_CONTROL_DEVICEEVENT:
    	OnDeviceChange(dwEventType, (DWORD_PTR)lpEventData);
		 break;
     default:
         break;
    }

    WriteToLog(("My Sample Service: ServiceCtrlHandler: Exit"));
	return NO_ERROR;
}

void getCurTime()
{
	SYSTEMTIME sys_time;  
	GetLocalTime(&sys_time);  
	char msg[100];  
	sprintf(msg,"\n\n%4d/%02d/%02d %02d:%02d:%02d.%03d %s%s",sys_time.wYear,sys_time.wMonth,sys_time.wDay,sys_time.wHour,sys_time.wMinute,sys_time.wSecond,sys_time.wMilliseconds,"发起","创建"); 
	WriteToLog(msg);
}

BOOL __stdcall LaunchAppIntoDifferentSession(BOOL bPopupBrowser)
{
	//{ xiaxu 20170224
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	HANDLE hToken = NULL;
	HANDLE hTokenDup = NULL;
	LPVOID  pEnv = NULL;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD dwCreationFlag = NORMAL_PRIORITY_CLASS|CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_CONSOLE;
	char* pLastCharPostion = NULL;
	//获取当前活动的SessionId
	//获取用户Token
	if(!WTSQueryUserToken(dwSessionId, &hToken))
	{
		WriteToLog(" WTSQueryUserToken\n");
		goto EndOP;
	}
	//复制Token
	if(!DuplicateTokenEx(hToken,MAXIMUM_ALLOWED,NULL,SecurityIdentification,TokenPrimary,&hTokenDup))
	{
		WriteToLog(" DuplicateTokenEx\n");
		goto EndOP;
	}
	//获取环境信息
	if(!CreateEnvironmentBlock(&pEnv,hTokenDup,FALSE))
	{
		WriteToLog("CreateEnvironmentBlock \n");
		goto EndOP;
	}
	//设置启动参数信息
	ZeroMemory( &si, sizeof( STARTUPINFO ) );
	si.cb = sizeof( STARTUPINFO );
	si.lpDesktop = "winsta0\\default";
	ZeroMemory( &pi, sizeof(pi) );
	//以当前用户启动记事本
	//执行当前路径下的 suloong_sm.exe, 其中，suloong_sm需要用宏或者常量来定义
	char _szPathTemp[512];
	GetModuleFileName(NULL, _szPathTemp, 512);
	pLastCharPostion = strrchr(_szPathTemp, '\\');
	if (pLastCharPostion)
	{
		*pLastCharPostion = '\0';
	}
	strcat_s(_szPathTemp, WRITECERTS_NAME);

	if(!CreateProcessAsUser(hTokenDup, _szPathTemp,NULL,NULL,NULL,FALSE,dwCreationFlag,pEnv,NULL,&si,&pi))
	{
		WriteToLog("CreateProcessAsUser \n");
		goto EndOP;
	}
	//等待启动的进程结束
	WaitForSingleObject(pi.hProcess, INFINITE);
/*	if (bPopupBrowser)
	{
		//启动速龙，速龙的位置需要判断
		//执行当前路径下的 suloong_sm.exe, 其中，suloong_sm需要用宏或者常量来定义
		char szPathTemp[512];
		GetModuleFileName(NULL, szPathTemp, 512);
		//取出文件路径   strrchr   “123”   找‘2’  返回 “23”
		pLastCharPostion = NULL;
		pLastCharPostion = strrchr(szPathTemp, '\\');
		if (pLastCharPostion)
		{
			*pLastCharPostion = '\0';
		}
		
		strcat_s(szPathTemp, SULOONGNAME);
		strcat_s(szPathTemp, "--ssl-version-max=tls1");
		if(!CreateProcessAsUser(hTokenDup, NULL, szPathTemp, NULL, NULL,FALSE,dwCreationFlag,pEnv,NULL,&si,&pi))
		{
			goto EndOP;
		}
	}*/
EndOP:
	//清理工作
	if (pEnv != NULL)
	{
		DestroyEnvironmentBlock(pEnv);
	}

	if (hTokenDup != NULL)
	{
		CloseHandle(hTokenDup);
	}

	if (hToken != NULL)
	{
		CloseHandle(hToken);
	}
	return TRUE;
	// xiaxu 20170224}
}

BOOL __stdcall OnDeviceChange(UINT eventType, DWORD_PTR eventData)
{   	
	int ulRet = 0;
	char szTmp[1024] = {0};
	char szDevsName[1024] = {0};
	unsigned int ulDevsNameLen = 0;

	char strLog[50] = {0};                              //log
	unsigned char *data_value = NULL;
	unsigned int data_len;

	char szDev[1024] = {0};
	unsigned long ulDev = sizeof(szDev);
	DWORD timeCurrent_Remove = 0;
	DWORD timeCurrent_Insert = 0;
	DWORD dwExitCode = 0;

	switch (eventType)
	{    
	case DBT_DEVICEREMOVECOMPLETE:
	case DBT_DEVICEARRIVAL:
		timeCurrent_Insert=GetTickCount();
		if((timeCurrent_Insert-g_InsertTime)>100)
		{
			WriteToLog(" Set Event \n");
			SetEvent(g_ServiceDevicesEvent);
		}
		g_InsertTime=timeCurrent_Insert;
	default:    
		break;    
	} 
	return TRUE;
}

DWORD WINAPI ServiceWorkerThread (LPVOID lpParam)
{
    WriteToLog(("My Sample Service: ServiceWorkerThread: Entry"));

    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {        
       Sleep(3000);
    }
    WriteToLog(("My Sample Service: ServiceWorkerThread: Exit"));

    return ERROR_SUCCESS;
}


DWORD WINAPI ServiceDeviesEvent (LPVOID lpParam)
{
    WriteToLog(("My Sample Service: ServiceDeviesEvent: Entry"));

	while(1)
	{
		while (WaitForSingleObject(g_ServiceDevicesEvent, 500) != WAIT_OBJECT_0)
		{        
			Sleep(100);Sleep(100);Sleep(100);Sleep(100);Sleep(100);
		}
		{
			Sleep(100);Sleep(100);Sleep(100);Sleep(100);Sleep(100);
			Sleep(100);Sleep(100);Sleep(100);Sleep(100);Sleep(100);
			Sleep(100);Sleep(100);Sleep(100);Sleep(100);Sleep(100);
			WriteToLog("got the event\n");
			CloseHandle((HANDLE)_beginthreadex(0, 0, (unsigned int (__stdcall *)(void *))ThreadFunc_DeviceChange,  NULL, 0, 0));
			//reset event
			ResetEvent(g_ServiceDevicesEvent);
		}
	}
    WriteToLog(("My Sample Service: ServiceDeviesEvent: Exit"));
    return ERROR_SUCCESS;
}

//xiaxu 2016/12/14 { 
/*
Routine Description:
    Registers for notification of changes in the device interfaces for
    the specified interface class GUID. 

Parameters:
    InterfaceClassGuid - The interface class GUID for the device 
        interfaces. 

    hDevNotify - Receives the device notification handle. On failure, 
        this value is NULL.

Return Value:
    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.
*/
BOOL DoRegisterDeviceInterface(GUID InterfaceClassGuid , HDEVNOTIFY *hDevNotify)
{
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = InterfaceClassGuid;

    *hDevNotify = RegisterDeviceNotification( g_StatusHandle, 
        &NotificationFilter,
		DEVICE_NOTIFY_SERVICE_HANDLE
    );

    if(!*hDevNotify) 
    {
        return FALSE;
    }

    return TRUE;
}

void WriteToLog(char* str)
{
	UseMixMutex mutex("WriteToLog");
#ifdef __WINTRUSTSERVICE_LOG__
	FILE* log = fopen("D:\\1.txt", "a+");
	if (log == NULL)
		return;
	fprintf(log, "%s\n", str);
	fclose(log);
#endif
}