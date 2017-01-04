#pragma once

#include "precompiler.h"


inline void PRINT_ERROR(PTCHAR pszFunc)
{
#ifdef _DEBUG
	TCHAR szErrMsg[128] = { 0 };
	wsprintf(szErrMsg, L"%s has failed! --- Error code is: %d\n", pszFunc, GetLastError());

	OutputDebugString(szErrMsg);
#endif // _DEBUG

}																					
