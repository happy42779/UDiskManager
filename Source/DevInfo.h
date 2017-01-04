#pragma once

#include "precompiler.h"
#include "ErrMsg.h"

#pragma comment(lib, "SetupAPI.lib")
//#pragma comment(lib, "regex.lib")


//DEFINE_GUID(GUID_UDISK, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, \
	0xC0, 0x4F, 0xB9, 0x51, 0xED);	//GUID_DEVINTERFACE_USB_DEVICE
DEFINE_GUID(GUID_UDISK, 0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, \
	0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);  //USB DISK

class DevInfo
{ 
private:
	DevInfo():
		hDevice(NULL){}
public:
	~DevInfo()
	{
		devinfo.clear();
	}
public:
	static DevInfo*& GetInstance()
	{
		if (NULL == m_phInstance)
			m_phInstance = new DevInfo();
		return m_phInstance;
	}
	//register
	//BOOL Register();
	//get drive number through volume label
	BOOL GetDiskNumByVol(DWORD&);
	//set read only
	BOOL SetDiskReadOnly(TCHAR);
	//set open normally
	BOOL OpenNormal(TCHAR);
	//check is read only or not
	BOOL IsReadOnly(int diskNum);
	//query
	BOOL QueryDevInfo();
	//pass information to set the  listbox and  combobox
	PDEVICEINFO QueryInfoRequired(); 
	TCHAR QueryVolLabel();
	DWORD GetDevCount();
	//device remove
	VOID UpdateInfo_OnDeviceRemove(DWORD&);
	std::vector<PDEVICEINFO>& QueryInfoSet();
	VOID SetDevMode(BOOL);
	VOID SetDevStatus(BOOL);
	VOID w2c(const PTCHAR, char*);
	VOID c2w(const char*, PTCHAR);
	DWORD GetIndexofRecord(DWORD&);
private:
	DWORD CalcDiskNumber(PTCHAR);
	BOOL CalcIdFromPath(PTCHAR);
	DWORD CalcVolIndex(DWORD&);
private:
	static DevInfo* m_phInstance;
	HANDLE hDevice;		//handle to the device
	PDEVICEINFO pInfo;
	std::vector<PDEVICEINFO> devinfo;
	std::map<std::string, int> devRecord;//string for sn, int for index in the pInfo
};

BOOL DevInfo::GetDiskNumByVol(DWORD& dwUnitMask)
{
	DWORD dwVolNum = 0;
	TCHAR szRootPath[] = L"\\\\.\\@:";
	
	pInfo = new DEVICEINFO;

	if (1 == g_devsToHandle)
		pInfo->IndexOfRecord = 0;
	else
		pInfo->IndexOfRecord = g_devsToHandle - 1;

	//for (; dwVolNum < 26; dwVolNum++)
	//{
	//	if (dwUnitMask & (1 << dwVolNum))
	//		break;
	//}
	dwVolNum = CalcVolIndex(dwUnitMask);

	pInfo->szVolMask = (TCHAR)(dwVolNum + 'A');
	szRootPath[4] = pInfo->szVolMask;

	if (0 == (pInfo->dwDiskNum = CalcDiskNumber(szRootPath)))
		return FALSE;

	return TRUE;
}
//set the control value, if false to set readonly
//then just detach the device
BOOL DevInfo::SetDiskReadOnly(TCHAR volSelected)
{
	HANDLE hScript = NULL;
	DWORD nBytesWritten;
	char script[64] = { 0 };
	TCHAR cmdLine[] = L"/C Diskpart /s C:\\dpt";
	STARTUPINFO startupinfo = { 0 };
	startupinfo.cb = sizeof(startupinfo);
	PROCESS_INFORMATION pi;
	int index;

	//if read only already
	for (index = 0; index < devinfo.size(); index++)
	{
		if (NULL == devinfo[index])
			continue;
		if (0 == devinfo[index]->szVolMask - volSelected)
			break;
	}
	if (IsReadOnly(devinfo[index]->dwDiskNum))
		return FALSE;

	sprintf(script, "select disk %d\nattributes disk set readonly\nexit",
		devinfo[index]->dwDiskNum);
	
	hScript = CreateFileA("C:\\dpt", GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, NULL, NULL);
	if (INVALID_HANDLE_VALUE == hScript)
	{
		PRINT_ERROR(L"DevInfo:SetDiskReadOnly.CreateFile");
		return TRUE;
	}
	WriteFile(hScript, script, sizeof(script),
		&nBytesWritten, NULL);
	if (!nBytesWritten)
	{
		PRINT_ERROR(L"DevInfo:SetDiskReadOnly.WriteFile");
		return TRUE;
	}
	CloseHandle(hScript);

	CreateProcess(L"C:\\Windows\\System32\\cmd.exe",
		cmdLine, NULL, NULL, NULL, CREATE_NEW_CONSOLE, NULL, NULL,
		&startupinfo, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	DeleteFile(L"C:\\dpt");
	return TRUE;
}
//set the return value to capture the error when 
//the error is failed to set readonly just disconnect
BOOL DevInfo::OpenNormal(TCHAR volSelected)
{
	HANDLE hScript = NULL;
	DWORD nBytesWritten;
	char script[64] = { 0 };
	TCHAR cmdLine[] = L"/C Diskpart /s C:\\dpt";
	STARTUPINFO startupinfo = { 0 };
	startupinfo.cb = sizeof(startupinfo);
	PROCESS_INFORMATION pi;
	int index;

	for (index = 0; index < devinfo.size(); index++)
	{
		if (NULL == devinfo[index])
			continue;
		if (0 == (devinfo[index]->szVolMask - volSelected))
			break;
	}
	//if not read only
	if (!IsReadOnly(devinfo[index]->dwDiskNum))
		return TRUE;

	sprintf(script, "select disk %d\nattributes disk clear readonly\nexit",
		devinfo[index]->dwDiskNum);

	hScript = CreateFileA("C:\\dpt", GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, NULL, NULL);
	if (INVALID_HANDLE_VALUE == hScript)
	{
		PRINT_ERROR(L"DevInfo:SetDiskReadOnly.CreateFile");
		return FALSE;
	}
	WriteFile(hScript, script, sizeof(script),
		&nBytesWritten, NULL);
	if (!nBytesWritten)
	{
		PRINT_ERROR(L"DevInfo:SetDiskReadOnly.WriteFile");
		return FALSE;
	}
	CloseHandle(hScript);

	CreateProcess(L"C:\\Windows\\System32\\cmd.exe",
		cmdLine, NULL, NULL, NULL, CREATE_NEW_CONSOLE, NULL, NULL,
		&startupinfo, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	DeleteFile(L"C:\\dpt");
	return TRUE;
}

BOOL DevInfo::IsReadOnly(int diskNum)
{
	OVERLAPPED ovrlpd;
	memset(&ovrlpd, 0, sizeof(OVERLAPPED));
	TCHAR szDrivePath[MAX_PATH] = TEXT("\\\\.\\PhysicalDrive@");
	szDrivePath[17] = (TCHAR)('0' + diskNum);
	GET_DISK_ATTRIBUTES gda;

	HANDLE diskHandle = CreateFile(
		szDrivePath,
		GENERIC_ALL,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (INVALID_HANDLE_VALUE == diskHandle)
		return FALSE;

	if (!DeviceIoControl(diskHandle,
		IOCTL_DISK_GET_DISK_ATTRIBUTES,
		NULL,
		0,
		&gda,
		sizeof(GET_DISK_ATTRIBUTES),
		0,
		&ovrlpd))
	{
		CloseHandle(diskHandle);
		return FALSE;
	}

	if (2 == gda.Attributes)
	{
		CloseHandle(diskHandle);
		// read only
		return TRUE;
	}
	
	CloseHandle(diskHandle);
	//not read only
	return FALSE;
}

DWORD DevInfo::CalcDiskNumber(PTCHAR szRootPath)
{
	//TCHAR szRootPath[] = L"\\\\.\\PHYSICALDRIVE@";
	//szRootPath[17] = (TCHAR)(devinfo.dwDiskNum + '0');
	STORAGE_DEVICE_NUMBER sdn;
	DWORD dwBytesReturn = 0;
	hDevice = CreateFile(szRootPath, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hDevice/* && ERROR_GEN_FAILURE != GetLastError()*/)
	{
		PRINT_ERROR(L"CalcDiskNumber:CreateFile");
		return 0;
	}
	if (!DeviceIoControl(hDevice,
		IOCTL_STORAGE_GET_DEVICE_NUMBER,
		NULL, 0, &sdn,
		sizeof(STORAGE_DEVICE_NUMBER),
		&dwBytesReturn,
		NULL))
	{
		PRINT_ERROR(L"CalcDiskNumber:DeviceIoControl---STORAGE_GET_DEVICE_NUMBER");
		CloseHandle(hDevice);
		return 0;
	}
	CloseHandle(hDevice);

	return sdn.DeviceNumber;
}

BOOL DevInfo::CalcIdFromPath(PTCHAR path)
{
	bool status;
	char pathBuf[256] = { 0 };
	//regex_t reg;
	std::regex pattern("\#\\w*\\d+\\w*\&");
	//regmatch_t match;
	std::smatch match; 
	w2c(path, pathBuf);

	std::string src(pathBuf);
	status = std::regex_search(src, match, pattern);
	if (!status)
	{
		PRINT_ERROR(L"CalcIdFromPath:regex_search");
		return FALSE;
	}
	for (int i = 0; i < match.size(); i++)
	{
		pInfo->szSN.assign(match[i]);
		pInfo->szSN.erase(pInfo->szSN.begin());
		pInfo->szSN.pop_back();
	}
	
	return TRUE;
}

DWORD DevInfo::CalcVolIndex(DWORD& uMask)
{
	DWORD dwVolNum;
	for (dwVolNum = 0; dwVolNum < 26; ++dwVolNum)
	{
		if (uMask & (1 << dwVolNum))
			break;
	}
	return dwVolNum;
}

VOID DevInfo::w2c(const PTCHAR pwchar, char* pchar)
{
	int i = 0;
	while ('\0' != (pchar[i] = (char)pwchar[i]))
	{
		i++;
	}
}

VOID DevInfo::c2w(const char* pchar, PTCHAR wpchar)
{
	int i = 0;
	while ('\0' != (wpchar[i] = (TCHAR)pchar[i]))
	{
		++i;
	}
}

DWORD DevInfo::GetIndexofRecord(DWORD& uMask)
{
	DWORD dwVolNum;
	int i;
	dwVolNum = CalcVolIndex(uMask);
	int nIndexofRecord = -1;

	for (i = 0; i < devinfo.size(); i++)
	{
		if (NULL == devinfo[i])
			continue;
		if (dwVolNum == (devinfo[i]->szVolMask - 'A'))
		{
			nIndexofRecord = devinfo[i]->IndexOfRecord;
			break;
		}
	}

	for (; i < devinfo.size(); i++)
	{
		devinfo[i]->IndexOfRecord = i - 1;
	}

	return nIndexofRecord;
}

BOOL DevInfo::QueryDevInfo()
{	
	SP_DEVICE_INTERFACE_DETAIL_DATA spdidd;
	HDEVINFO hDevinfo;
	//SP_DEVINFO_DATA spdd;
	SP_DEVICE_INTERFACE_DATA spdid;
	DWORD dwIndex = 0;	//this parameter must be zero before being passed.
	PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = &spdidd;
	DWORD dwSize = 0;

	hDevinfo = SetupDiGetClassDevs(&GUID_UDISK, 
		NULL, NULL, 
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (INVALID_HANDLE_VALUE == hDevinfo)
	{
		PRINT_ERROR(L"QueryDevInfo:SetupDiGetClassDevs");
		return FALSE;
	}

	while (1)
	{
		spdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (!SetupDiEnumDeviceInterfaces(hDevinfo,
			NULL,
			&GUID_UDISK,
			dwIndex,
			&spdid))
		{
			if (ERROR_NO_MORE_ITEMS == GetLastError())
				break;
			PRINT_ERROR(L"QueryDevInfo:EnumDeviceInterfaces");
			SetupDiDestroyDeviceInfoList(hDevinfo);
			return FALSE;
		}

		pspdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		if (!SetupDiGetDeviceInterfaceDetail(hDevinfo,
			&spdid,
			pspdidd,
			sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA),
			&dwSize,
			NULL))
		{
			if (dwSize != 0 && ERROR_INSUFFICIENT_BUFFER == GetLastError())
				pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GMEM_FIXED, dwSize);
			pspdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (!SetupDiGetDeviceInterfaceDetail(hDevinfo,
				&spdid,
				pspdidd,
				dwSize,
				&dwSize,
				NULL))
			{
				PRINT_ERROR(L"QueryDevInfo:SetupDiGetDeviceInterfaceDetail");
				SetupDiDestroyDeviceInfoList(hDevinfo);
				return FALSE;
			}
		}		
		/*
		if (INVALID_HANDLE_VALUE == CreateFile(pspdidd->DevicePath,
			GENERIC_ALL, FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL))
		{
			PRINT_ERROR(L"QueryDevInfo:CreateFile");
			SetupDiDestroyDeviceInfoList(hDevinfo);
			return FALSE;
		}
		*/
		if (pInfo->dwDiskNum == CalcDiskNumber(pspdidd->DevicePath))
		{
			if(CalcIdFromPath(pspdidd->DevicePath))
				break;
			//if can not get the id, then read only as default
			SetupDiDestroyDeviceInfoList(hDevinfo);
			return  FALSE;
		}									  
		++dwIndex;
	}

	SetupDiDestroyDeviceInfoList(hDevinfo);
	return TRUE;
}

PDEVICEINFO DevInfo::QueryInfoRequired()
{
	TCHAR szPath[] = L"@:\\";
	szPath[0] = pInfo->szVolMask;
	ULARGE_INTEGER ulDiskSpace;
	std::map<std::string, int>::iterator it;
	std::pair<std::map<std::string, int>::iterator, bool> ret;

	memset(pInfo->szVolName, 0, sizeof(pInfo->szVolName));
	if (!GetVolumeInformation(szPath, pInfo->szVolName, sizeof(pInfo->szVolName) >> 1, NULL, NULL, NULL, NULL, NULL))
	{
		PRINT_ERROR(L"QueryInfoRequired:GetVolumeInformation");
		wsprintf(pInfo->szVolName, L"%s", L"Unknown device name");
	}
	
	GetDiskFreeSpaceEx(szPath, NULL, NULL, &ulDiskSpace);
	pInfo->dwDiskSpace = ulDiskSpace.QuadPart >> 20;

	ret = devRecord.insert(make_pair(pInfo->szSN, devinfo.size()));
	if (FALSE == ret.second)//如果插入失败,则证明存在
	{
		it = devRecord.find(pInfo->szSN);
		//if (it == devRecord.end())		 //不应该会找不到
		devinfo[ret.first->second] = pInfo;
	}	
	else
		devinfo.push_back(pInfo);
	return pInfo;
}

TCHAR DevInfo::QueryVolLabel()
{
	return pInfo->szVolMask;
}

DWORD DevInfo::GetDevCount()
{
	return devinfo.size();
}

VOID DevInfo::UpdateInfo_OnDeviceRemove(DWORD& uMask)
{	
	
	DWORD dwVolNum;
	PDEVICEINFO pdev;
	//for (dwVolNum = 0; dwVolNum < 26; ++dwVolNum)
	//{
	//	if (uMask & (1 << dwVolNum))
	//		break;	
	//}
	dwVolNum = CalcVolIndex(uMask);

	//here to delete the DEVINFO newed
	for (int i = 0; i < devinfo.size(); ++i)
	{
		if (NULL == devinfo[i])
			continue;
		if (dwVolNum == (devinfo[i]->szVolMask - 'A'))
		{
			pdev = devinfo[i];
			devinfo[i] = NULL;
			delete pdev;
			break;
		}
	}
}

std::vector<PDEVICEINFO>& DevInfo::QueryInfoSet()
{
	return devinfo;
}

//true: write false: read
VOID DevInfo::SetDevMode(BOOL b)
{
	//*** still need to get which device is disconnected
	pInfo->bMode = b;
}

//true: connected false: disconnected
VOID DevInfo::SetDevStatus(BOOL b)
{
	//*** still need to get which device is disconnected
	pInfo->bStatus = b;
}

