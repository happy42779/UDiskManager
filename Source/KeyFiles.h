#pragma once
#include "precompiler.h"
#include "MD5.h"
#include "ErrMsg.h"
/*
*@ Description: This is to provide a key related operations
*@ Functions:
	1. Generate key
	2. Verify key
	3. Modify key
	4. Read key
	5. Write key
	6. Update key
	7. Encrypt key file
	8. Decrypt key file
*@ Date: 06:02pm 11/19/2016
*@ Author: Ian
*/

class KeyFiles
{
private:
	KeyFiles()
		:hKeyFile(NULL),
		hThread(NULL),
		nWrongTrial(0),
		bCaughtWrong(FALSE)
	{	
		//memset(szPWDInput, 0, sizeof(szPWDInput));
		memset(szKeyFilePath, 0, sizeof(szKeyFilePath));
		memset(szMD5Buf, 0, sizeof(szMD5Buf));
		memset(szKeyBuffer, 0, sizeof(szKeyBuffer));
	}
public:
	~KeyFiles()
	{
		CloseHandle(hKeyFile);
	}
public:
	static KeyFiles*& GetInstance()
	{
		if (NULL == m_phInstance)
			m_phInstance = new KeyFiles();

		return m_phInstance;
	}
	static DWORD WINAPI ThreadProc(LPVOID lpParam);

	//generate key files
	BOOL GenerateKF();
	//revise keys & set keys
	BOOL SetPWD(const TCHAR*, int flag = 0);
	//verify keys
	BOOL VerifyPWD(const TCHAR*);
	//Check status
	int CheckStatus();
	//load password
	BOOL loadPWD();
	//record wrong trial times
	VOID RecordWrong();
	BOOL SendWrongRecord(HWND&);
private:
	BOOL ReadPWDBuffer();  
	BOOL CalcPos(int&, int*&);
	BOOL Encrypt();
	BOOL Decrypt();
private:
	static KeyFiles* m_phInstance;
	HANDLE hKeyFile;
	TCHAR szKeyBuffer[1023];
	TCHAR szKeyFilePath[MAX_PATH];
	HANDLE hThread;		//closed where the exit code of the thread is retrieved
	BOOL bCaughtWrong;
	char szMD5Buf[40];
	int nWrongTrial;
	 
};

/*
*@ Function: GenerateKF()
*@ How to:
	1. randomly generate a buffer with upper/lower case letters, 
		numbers and marks, 20 groups, 50 chars each group
*@ return: TRUE if succeeds, FALSE otherwise
*/
BOOL KeyFiles::GenerateKF()
{
	int groupRand;
	int charRand;
	char* pChar = NULL;
	char CharToWrite;

	char lowerCaseEnum[] = {
		'a', 'b', 'c', 'd', 'e',
		'f', 'g', 'h', 'i', 'j',
		'k', 'l', 'm', 'n', 'o',
		'p', 'q', 'r', 's', 't',
		'u', 'v', 'w', 'x', 'y',
		'z' }; //26+26+10+32
											
	char upperCaseEnum[] = {
		'A', 'B', 'C', 'D', 'E',
		'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O',
		'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y',
		'Z' }; //26

	char numberEnum[] = { '0', '1', '2', '3', '4', 
		'5', '6', '7', '8', '9' };	//10

	char specialMarkEnum[] = { ' ', '!', '"', '#', '$', '%', '\'', '(', ')',
	'*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '\\',
	'[', ']', '^', '_', '`', '{', '}', '|', '~' }; //32

	srand((unsigned)GetTickCount());
	
	//asm optimization
	int i, nBufLen;
	nBufLen = sizeof(szKeyBuffer) / sizeof(TCHAR);
	for (i = 0; i < nBufLen; ++i)
	{
		pChar = (char*)&szKeyBuffer[i];
		groupRand = rand() % 4 + 1;
		switch (groupRand)
		{
		case 1:					//小写字母
		{
			charRand = rand() % 26;
			CharToWrite = lowerCaseEnum[charRand];
		}
		break;
		case 2:					//大写字母
		{
			charRand = rand() % 26;
			CharToWrite = upperCaseEnum[charRand];
		}
		break;
		case 3:				    //数字
		{
			charRand = rand() % 9;
			CharToWrite = numberEnum[charRand];
		}
		break;
		case 4:				    //字符
		{
			charRand = rand() % 31;
			CharToWrite = specialMarkEnum[charRand];
		}
		break;
		default:break;
		}
		szKeyBuffer[i] |= (TCHAR)CharToWrite;
		*(pChar + 1) |= (CharToWrite ^ 7);
		
	}
	return TRUE;
}

/*
*@ Function: VerifyPWD(const TCHAR*)
*@ Description: verify password wrong or right
*@ How to:	write password into buffer as level 1
			encryption, then compare the hash value
			of the file containing the passcode.
*@ Return: TRUE if succeeds, otherwise FALSE
*/
BOOL KeyFiles::VerifyPWD(const TCHAR* pszPWD)
{
	char md5Buf[40] = { 0 };
	//char* pwdBuf = (char*)malloc(sizeof(szKeyBuffer));
	//memset(pwdBuf, 0, sizeof(szKeyBuffer));

	//szMD5中没有数据，或者密码输入错误之后，重新读取buffer
	if (szMD5Buf[0] == '\0' || nWrongTrial)
		if (!ReadPWDBuffer())
			return FALSE;
	//put passcode
	if (!SetPWD(pszPWD, 1))
	{
		MessageBox(NULL, L"verify password failed", L"ERROR", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	//compare hash
	MD5hashProc((char*)szKeyBuffer, sizeof(szKeyBuffer), md5Buf);
	//MD5hashProc(pwdBuf, sizeof(szKeyBuffer), md5Buf1);

	if (strcmp(md5Buf, szMD5Buf))
	{
		//free(pwdBuf);
		return FALSE;
	}
	//free(pwdBuf);
	return TRUE;
}


/*
*@ Function: CheckStatus()
*@ Description: check the key file status, whether exists
*@ Return:	TRUE if exists, otherwise FALSE
*/
int KeyFiles::CheckStatus()
{
	TCHAR szAppDir[MAX_PATH] = { 0 };
	DWORD nBytesWritten = 0;

	GetEnvironmentVariable(L"APPDATA", szAppDir, sizeof(szAppDir));
	wsprintf(szKeyFilePath, L"%s\\UDManager", szAppDir);
	wsprintf(szAppDir, L"%s\\security_", szKeyFilePath);
	m_phInstance->hKeyFile = CreateFile(szAppDir, GENERIC_READ | GENERIC_WRITE, NULL, \
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == m_phInstance->hKeyFile)
	{	
		int ret = GetLastError();
		if (ERROR_SHARING_VIOLATION == ret)
			return -1;
		else if (ERROR_FILE_NOT_FOUND == ret)
			return 0;
	}
	
	return 1;
}

BOOL KeyFiles::loadPWD()
{
	if (!ReadPWDBuffer())
		return FALSE;
	MD5hashProc((char*)szKeyBuffer, sizeof(szKeyBuffer), szMD5Buf);
	if (!ReadPWDBuffer())
		return FALSE;
	return TRUE;
}

VOID KeyFiles::RecordWrong()
{
	nWrongTrial++;
	if (!bCaughtWrong)
		bCaughtWrong = TRUE;
}

//HWND parameter is the handle to the window
//to which this message is send
BOOL KeyFiles::SendWrongRecord(HWND& hWnd)
{
	int res;
	if (!hWnd)
		return FALSE;
	if (bCaughtWrong)
	{
		res = PostMessage(hWnd, WM_WRONGTRIAL, 0, (LPARAM)nWrongTrial);
		if (0 == res)
			PRINT_ERROR(L"SendWrongRecord:PostThreadMessage");
		bCaughtWrong = FALSE;
		return FALSE;
	}
	return TRUE;
}

/*
*@ Function: WritePWD(const char*& pszPWD, int flag)
	1. const char*& pszPWD: password passed in
	2. int flag: 0 default---update and write, 1 verify 
		passcode, not used as default
*@ Description: write passcode into buffer first,
	then write to key file. Application leaves the 
	writing file part to a thread then return.
*@ Return:	TRUE if succeeds, otherwise FALSE
*/
BOOL KeyFiles::SetPWD(const TCHAR* pszPWD, int flag)	
{	
	int iPWDLen = wcslen(pszPWD);
	int a = sizeof(int) * iPWDLen;
	int* iPWDPos = (int*)malloc(a);
	memset(iPWDPos, -1, a);
	//memcpy(szPWDInput, pszPWD, sizeof(szPWDInput));
	
	if (!CalcPos(iPWDLen, iPWDPos))
		return FALSE;

	int offset = 0;
	char* p = NULL;
	//begin to put
	for (int i = 0; i < iPWDLen;)
	{
		offset += iPWDPos[i];
		p = (char*)&szKeyBuffer[offset];
		szKeyBuffer[offset] |= pszPWD[i];
		offset = (++i) * 20;
	}
	if (flag&1)
	{
		free(iPWDPos);
		return TRUE;
	}
	hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ThreadProc, NULL, NULL, NULL);
	if (NULL == hThread)
	{
		MessageBox(NULL, L"create thread failed", L"ERROR", MB_OK | MB_ICONERROR);
		free(iPWDPos);
		return FALSE;
	}
	free(iPWDPos);
	return TRUE;
}
/*
*@ Function: ReadPWDBuffer()
*@ Description: Read the whole PWD content into a buffer
				then compare the MD5 Value
*/
BOOL KeyFiles::ReadPWDBuffer()
{
	DWORD nBytesRead = 0;
	char* p = (char*)szKeyBuffer;
	SetFilePointer(hKeyFile, 0, NULL, FILE_BEGIN);		
	if (!ReadFile(hKeyFile, szKeyBuffer, sizeof(szKeyBuffer), &nBytesRead, NULL))
	{
		//what if this function failed?***
		MessageBox(NULL, L"ReadFile failed", L"ERROR", MB_OK | MB_ICONERROR);
		return FALSE;	//should not return false to indicate failure
	}

	if (szMD5Buf[0] == '\0')
	{
		for (int i = 0; i < nBytesRead; i += 2)
			*(p + i + 1) &= 0;
		return TRUE;
	}
	//clear the real pass word and move set the sugar clothes to the buffer.
	for (int i = 0; i < nBytesRead; i += 2)
	{
		*(p + i) &= 0;
		*(p + i + 1) ^= 7;
		szKeyBuffer[i >> 1] = (TCHAR)*(p + i + 1);
	}
	return TRUE;
}

/* 
*@ Function: ThreadProc
*@ Description: a thread callback function to execute sync
				buffer to a file so that the calling function
				could return to it's calling function to give
				user feedback since this function may take a 
				while to complete
*@ Return:	return value can be retrieved by calling GetExitCodeThread()
			in the firstly calling function after giving feedback to users
*@ Revise:	set another thread to monitor the state of this operation,
			if failed, try to recall this function again.
*/
DWORD WINAPI KeyFiles::ThreadProc(LPVOID lpParam)
{
	//begin writing to file
	TCHAR szAppDir[128] = { 0 };
	DWORD nBytesWritten = 0;
	char* pChar = NULL;
	
	if (!CreateDirectory(m_phInstance->szKeyFilePath, NULL))
	{
		if (ERROR_ALREADY_EXISTS != GetLastError())
		{
			MessageBox(NULL, L"failed to create directory", L"ERROR", MB_ICONERROR | MB_OK);
			return FALSE;
		}
	}
	wsprintf(m_phInstance->szKeyFilePath, L"%s\\security_", m_phInstance->szKeyFilePath);
	
	//if hKeyFile is, then the key file does not exist.
	if (INVALID_HANDLE_VALUE == m_phInstance->hKeyFile)
	{
		m_phInstance->hKeyFile = CreateFile(m_phInstance->szKeyFilePath, GENERIC_READ | GENERIC_WRITE, NULL, \
			NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == m_phInstance->hKeyFile)
		{
			MessageBox(NULL, L"Open file failed", L"ERROR", MB_OK | MB_ICONERROR);
			return FALSE;
		}
	}

	if (!WriteFile(m_phInstance->hKeyFile, m_phInstance->szKeyBuffer, sizeof(m_phInstance->szKeyBuffer), &nBytesWritten, NULL))
	{
		//MessageBox(GetForegroundWindow(), L"Write file failed", L"ERROR", MB_OK | MB_ICONERROR);
		TCHAR szErrMsg[128] = { 0 };
		DWORD dwErrCode = GetLastError();
		wsprintf(szErrMsg, L"Error code: %d\nBytes written: %d", dwErrCode, nBytesWritten);
		wsprintf(szErrMsg, L"%s\nHandle to the file: %d", szErrMsg, (DWORD)m_phInstance->hKeyFile);
		MessageBox(GetForegroundWindow(), szErrMsg, L"ERROR", MB_ICONERROR | MB_OK);
		CloseHandle(m_phInstance->hKeyFile);
		return FALSE;
	}
	//CloseHandle(m_phInstance->hKeyFile);
	//calc the md5 value
	int nBufLen = sizeof(m_phInstance->szKeyBuffer) / sizeof(TCHAR); 
	for (int i = 0; i < nBufLen; i++)
	{
		pChar = (char*)&m_phInstance->szKeyBuffer[i];
		*(pChar + 1) &= 0;
	}
	MD5hashProc((char*)m_phInstance->szKeyBuffer, sizeof(m_phInstance->szKeyBuffer), m_phInstance->szMD5Buf);
	return TRUE;
}

BOOL KeyFiles::CalcPos(int& iPWDLen, int*& iPWDPos)
{	
	//分区序列号
	char szSN[256] = { 0 };		
	DWORD dwVolSN = -1;
	
	//获取分区序列号
	if (!GetVolumeInformation(L"C:\\", NULL, NULL, &dwVolSN, 0, 0, NULL, 0))
	{
		MessageBox(NULL, L"GetVolumeInformation failed", L"ERROR", MB_OK | MB_ICONERROR);
		return FALSE;
	}	

	sprintf(szSN, "%u", dwVolSN); //转换dword 为 字符存在数组当中
	int iSNLen = strlen(szSN);	  //获取序列号长度
	//获取存储位置
	//当密码长度小于序列号的密码组合时
	int i, j;
	int iPosCnt = 0;
	for (i = 0; i < iSNLen - 1; ++i)
	{
		for (j = i + 1; iPosCnt < iPWDLen && j < iSNLen; ++j, ++iPosCnt)
		{
			iPWDPos[iPosCnt] = szSN[i] + szSN[j] - 96;
		}
		if (iPosCnt == iPWDLen)
			break;
	}
	while (iPosCnt < iPWDLen)
	{
		iPWDPos[iPosCnt] = iPWDPos[--j];
		++iPosCnt;
	}
	return TRUE;
}
