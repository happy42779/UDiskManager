#pragma once

#include <windows.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <SetupAPI.h>
#include <string>
#include <regex>
#include <initguid.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <Dbt.h>
#include <vector>
#include <map>

#define WM_WRONGTRIAL (WM_USER + 2)
#define WM_ADDRECORD (WM_USER + 3)

typedef struct 
{
	DWORD dwDiskNum;
	TCHAR szVolMask;
	BOOL bMode;	//TRUE: read only FALSE:normal
	BOOL bStatus;	//TRUE: yes FALSE: no whether connected or disconnected
	TCHAR szVolName[30];
	DWORD dwDiskSpace; //number of MBs
	SYSTEMTIME sysTime;
	std::string szSN;
	int IndexOfRecord;
}DEVICEINFO, * PDEVICEINFO;
//device count
extern DWORD g_devCount;
//how many devs are waiting for handling
extern DWORD g_devsToHandle;
extern HMODULE g_hModule;


