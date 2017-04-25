#include <Windows.h>
#include <tchar.h>
#include <string>
#include "WTF_Interface.h"
#include <dbt.h>//xiaxu 2016/12/14
#include <devguid.h> //xiaxu 2016/12/14 
#include <atlbase.h>
#include <atlconv.h>
#include <CommCtrl.h>
#include <Shlobj.h>
#pragma comment( lib, "shell32.lib")
#include <WtsApi32.h>
#pragma comment(lib, "WtsApi32.lib")
#include <UserEnv.h>
#pragma comment(lib, "UserEnv.lib")

#define  __WINTRUSTSERVICE_LOG__
#define  SERVICE_NAME  _T("WinTrustService")
#define  WRITECERTS_NAME _T("\\WriteCert.exe")
#define SULOONGNAME "\\suloong_sm.exe"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <accctrl.h>
#include <aclapi.h>

#define SD_SIZE (65536 + SECURITY_DESCRIPTOR_MIN_LENGTH)
#define SYSTEM_PID 2
// end by xiaxu }


#define  TRY_TIMES_FRO_IN    2  //÷ÿ ‘¥Œ ˝
#define  TRY_TIMES_FOR_REMOVE  1//

SERVICE_STATUS                g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE         g_StatusHandle = NULL;
HANDLE                        g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE                        g_ServiceDevicesEvent = INVALID_HANDLE_VALUE;
DWORD                         g_InsertTime = 0;
DWORD                         g_RemoveTime = 0;

BOOL bFirstIn = TRUE;
BOOL bFirstOut = TRUE;

static char g_szDevsName[1024] = {0};
static unsigned int g_ulDevsNameLen = 0;

char szLog[1024] = {0};

HDEVNOTIFY g_hDevNotify           = NULL;
HDEVNOTIFY g_hid_hDevNotify       = NULL;
HDEVNOTIFY g_cdrom_hDevNotify    = NULL;
HDEVNOTIFY g_disk_hDevNotify      = NULL;

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv);
DWORD WINAPI ServiceCtrlHandlerEx(
	DWORD    dwControl,
	DWORD    dwEventType,
	LPVOID   lpEventData,
	LPVOID   lpContext
	);
DWORD WINAPI ServiceWorkerThread (LPVOID lpParam);
DWORD WINAPI ServiceDeviesEvent (LPVOID lpParam);
BOOL __stdcall OnDeviceChange(UINT eventType, DWORD_PTR eventData);
BOOL __stdcall LaunchAppIntoDifferentSession(BOOL bPopupBrowser);
int __stdcall ThreadFunc_DeviceChange(void* param);
BOOL DoRegisterDeviceInterface(GUID InterfaceClassGuid , HDEVNOTIFY *hDevNotify);
void WriteToLog(char* str);
void getCurTime();

