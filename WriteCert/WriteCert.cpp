#include "stdio.h"
#include "WTF_Interface.h"
#include "SMC_Interface.h"
#include "mix-mutex.h"
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#define STRLOG 50
#define _WINTRUSTSERVICE_LOG_

void WriteToLog(char* str)
{
	UseMixMutex mutex("WriteToLog");

#ifdef _WINTRUSTSERVICE_LOG_
	if (NULL == str)
	{
		return;
	}
	FILE* log = fopen("D:\\1.txt", "a+");
	if (log == NULL)
		return;
	fprintf(log, "%s.\n", str);
	fclose(log);
#endif
}

int main(int argc, char * argv[])
{
	unsigned long ulRet = 0;
	unsigned char *data_value = NULL;
	unsigned int data_len = 1024*1024;

	SK_CERT_CONTENT *pCertContent = NULL;
	unsigned int ulTmpLen = 0;

	PCCERT_CONTEXT certContext_OUT = NULL;
	HCERTSTORE hCertStore = NULL;

	char szDev[1024] = {0};
	unsigned long ulDev = sizeof(szDev);

	char strLog[STRLOG] = {0};


	data_value = (unsigned char *)malloc(data_len);
	if(data_value == NULL)
	{
		memset(strLog, 0x00, sizeof(strLog));
		sprintf(strLog, "malloc error, Line=%d\n", ulRet, __LINE__);
		goto EndOP;
	}
		
	ulRet = WTF_EnumCert(NULL, data_value, &data_len, CERT_ALG_SM2_FLAG, CERT_SIGN_FLAG|CERT_EX_FLAG, CERT_NOT_VERIFY_FLAG, 0);// 是否过滤
	if(ulRet != 0)
	{
		memset(strLog, 0x00, sizeof(strLog));
		sprintf(strLog, "WTF_EnumCert error, ulRet = 0x%x, Line=%d\n", ulRet, __LINE__);
		goto EndOP;
	}

	
EndOP:

	// clear
	WriteToLog("WTF_ClearStore\n");
	ulRet = WTF_ClearStore(DEFAULT_SMC_STORE_SM2_USER_ID);
	if (0 != ulRet)
	{
		memset(strLog, 0x00, sizeof(strLog));
		sprintf(strLog, "WTF_ClearStore error, ulRet = 0x%x, Line=%d\n", ulRet, __LINE__);
		goto EndOP;
	}

	if(!SMC_CertCreateSMCStores())
	{
		ulRet = EErr_SMC_CREATE_STORE;
		memset(strLog, 0x00, sizeof(strLog));
		sprintf(strLog, "SMC_CertCreateSMCStores error, ulRet = 0x%x, Line=%d\n", ulRet, __LINE__);
		goto EndOP;
	}

	while(ulTmpLen < data_len)
	{
		pCertContent = (SK_CERT_CONTENT*)(data_value+ulTmpLen);
		//pCertContent->pbValue;
		ulTmpLen += sizeof(SK_CERT_CONTENT) + pCertContent->nValueLen;
		//返回值为0表示失败，非0成功
		ulRet = SMC_ImportUserCert(pCertContent->pbValue, pCertContent->nValueLen, &(pCertContent->stProperty));
		if(ulRet == 0)
		{
			memset(strLog, 0x00, sizeof(strLog));
			sprintf(strLog, "SMC_ImportUserCert error, ulRet = 0x%x, Line=%d\n", ulRet, __LINE__);
			goto EndOP;
		}

	}


	if(data_value != NULL)
	{
		free(data_value);
		data_value = NULL;
	}
	WriteToLog(strLog);
	return 0;
}
