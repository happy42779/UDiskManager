#ifndef _MD5EXPORT
#include <windows.h>
extern "C" DWORD __stdcall MD5hashProc(char * lpBuf,DWORD dwSize,char * lpMD5Buf);
#pragma comment(lib,"MD5.Lib")
#endif