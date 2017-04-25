#include "stdafx.h"
#include "service_control/service_control.h"
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

const wchar_t* const kServiceName = L"WinTrustService";
const wchar_t* const kBinPath = L"\\WinTrustService.exe";
#define UNINSTALL

std::wstring GetFilePath( const std::wstring& wsFullName )
{
	std::wstring::size_type nIndex1 = wsFullName.find_last_of(L"\\");
	std::wstring::size_type nIndex2 = wsFullName.find_last_of(L"/");
	if (std::wstring::npos == nIndex1)
	{
		nIndex1 = 0;
	}
	if (std::wstring::npos == nIndex2)
	{
		nIndex2 = 0;
	}
	std::wstring::size_type nIndex = max(nIndex1, nIndex2);
	return wsFullName.substr(0, nIndex);
}

std::wstring GetAppPath()
{
	wchar_t lpszFileName[MAX_PATH] = {0};
	::GetModuleFileName(NULL, lpszFileName, MAX_PATH);
	std::wstring strFullName = lpszFileName;

	return GetFilePath(strFullName);
}


void test_service()
{
	ServiceControl control;
#ifndef UNINSTALL
	std::wstring app_path = GetAppPath();
	//app_path += L"\\service_example.exe";
	app_path += kBinPath;

	control.InstallService(app_path, kServiceName);
	control.UpdateSvcDesc(kServiceName, L"��ȷ��ʹ�����°��������ȫ����������ͣ�û��жϴ˷���������������ȫ�����ĳЩ���ܲ���ʹ�á����������ȫ�������ж�أ���˷��������ж�ء�");
	control.StartService(kServiceName);
	control.AutoStart(kServiceName);
	//::Sleep(30000000);
#else
	control.StopService(kServiceName, FALSE);
	control.DeleteService(kServiceName);
#endif
}

int main(int argc, _TCHAR* argv[])
{
	test_service();
	//::system("pause");
	return 0;
}
