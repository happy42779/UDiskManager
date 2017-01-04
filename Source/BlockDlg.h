#pragma once
#include "precompiler.h"

#include "PwdDlg.h"
#include "DevInfo.h"

class BlockDlg
{														
private:
	BlockDlg()
	{
		xScreen = GetSystemMetrics(SM_CXSCREEN);
		yScreen = GetSystemMetrics(SM_CYSCREEN);
	}
public:
	static BlockDlg*& GetInstance()
	{
		if(NULL == m_phInstance)
			m_phInstance = new BlockDlg();
		return m_phInstance;
	}
	//show window
	void Show()
	{
		//display information
		UpdateWindow(m_blockDlg);
		ShowWindow(m_blockDlg,SW_SHOW);
		DisableHotKeys();
	}
	void Hide()
	{
		ShowWindow(m_blockDlg,SW_HIDE);
		RestoreHotKeys();
	}
	//write infomation
	BOOL DisplayInfo(HWND);
	//clear information
	BOOL ClearInfo(int);

	HWND InitDlg(HINSTANCE&, HWND&);
	static LRESULT CALLBACK WndProcBlock(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	VOID ShowInExplorer();
	VOID DisableHotKeys();
	VOID RestoreHotKeys();
private:
	static BlockDlg* m_phInstance;
	HWND hOwner;	//the owner of this window
	HWND m_blockDlg;	//handle to the blocking dialog	  
	HWND m_listBox;		//handle to the listbox that shows info						
	HWND m_comboBox;    //handle to the comboBox that shows the label of disk		
	HWND m_staticText;	//handle to the static text that gives hints			
	HWND m_btnAuth;		//handle to the button that authorize the disk				1
	HWND m_btnReadOnly;	//handle to the button that open the disk with read only	2
	int xScreen, yScreen;		//screen width and height
	std::vector<TCHAR> partitionToShow;	//how many partitions to show
};

//window call back procedure
LRESULT CALLBACK BlockDlg::WndProcBlock(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nIndexSelected = -1;
	TCHAR volSelected[3] = { 0 };
	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		return TRUE;
	case WM_NCLBUTTONDOWN:
		if (wParam == HTCAPTION)
			return 0;
		break;
	case WM_COMMAND:
		if (m_phInstance->m_btnReadOnly == (HWND)lParam)
		{
			//to do
			//get which device selected
			nIndexSelected = SendMessage(m_phInstance->m_comboBox, CB_GETCURSEL, 0, 0);
			if (CB_ERR == nIndexSelected)
			{
				MessageBox(GetForegroundWindow(), L"请选择要操作的设备", L"选择无效", MB_OK | MB_ICONERROR);
				return TRUE;
			}
			memset(volSelected, 0, sizeof(volSelected) >> 1);
			SendMessage(m_phInstance->m_comboBox, WM_GETTEXT, 2, (LPARAM)volSelected);
			//1. set device read only
			if (DevInfo::GetInstance()->SetDiskReadOnly(volSelected[0]))
				 DevInfo::GetInstance()->SetDevMode(FALSE);
			//2. hide the blocking dlg
			//if there's still other devices to handle
			if(--g_devsToHandle);
			else
				m_phInstance->Hide();
			
			m_phInstance->ShowInExplorer();
			
			m_phInstance->ClearInfo(nIndexSelected);
			
			SendMessage(m_phInstance->hOwner, WM_ADDRECORD, 0, 0);
			
			return TRUE;
		}
		else if (m_phInstance->m_btnAuth == (HWND)lParam)
		{
			//whether device selected
			nIndexSelected = SendMessage(m_phInstance->m_comboBox, CB_GETCURSEL, 0, 0);
			if (CB_ERR == nIndexSelected)
			{
				MessageBox(GetForegroundWindow(), L"请选择要操作的设备", L"选择无效", MB_OK | MB_ICONERROR);
				return TRUE;
			}
			//verify passcode
			PwdDlg::GetInstance()->ShowModal(hWnd);

			//if not verified, then to do
			if (FALSE == (BOOL)GetWindowLong(hWnd, GWL_USERDATA))
				SetWindowPos(hWnd, GetForegroundWindow(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			//if verified, 
			else
			{
				memset(volSelected, 0, sizeof(volSelected) >> 1);
				SendMessage(m_phInstance->m_comboBox, WM_GETTEXT, 2, (LPARAM)volSelected);
				DevInfo::GetInstance()->OpenNormal(volSelected[0]);
			
				//set number of devs waiting for handling
				if (--g_devsToHandle);
				else
					//Hide this blocking dlg
					m_phInstance->Hide();
				//open the device in the explorer whether verified
				m_phInstance->ShowInExplorer();
				
				m_phInstance->ClearInfo(nIndexSelected);

				DevInfo::GetInstance()->SetDevMode(TRUE);

				SendMessage(m_phInstance->hOwner, WM_ADDRECORD, 0, 0);
				return TRUE;
			}
		}
		break;
	case WM_CLOSE:
		return TRUE;
	default:break;
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}


VOID BlockDlg::ShowInExplorer()
{
	TCHAR buf[] = L"@:\\";
	TCHAR t[6] = { 0 };
	
	if (0 == SendMessage(m_comboBox, WM_GETTEXT, sizeof(t), (LPARAM)t))
		partitionToShow.push_back('\0');
	partitionToShow.push_back(t[0]); 
	if (g_devsToHandle)
		return;
	for (int i = 0; i < partitionToShow.size(); ++i)
	{
		buf[0] = partitionToShow.at(i);
		ShellExecute(NULL, L"open", L"explorer.exe", buf, NULL, SW_SHOW);
	}
	partitionToShow.clear();
}

VOID BlockDlg::DisableHotKeys()
{
	FARPROC func =GetProcAddress(g_hModule, "StartHook");
	if (NULL == func)
	{
		PRINT_ERROR(L"DisableHotkeys-GetProcAddress");
	}
	func();
}

VOID BlockDlg::RestoreHotKeys()
{	
	FARPROC func = GetProcAddress(g_hModule, "StopHook");
	if (NULL == func)
		PRINT_ERROR(L"RestoreHotkeys-GetProcAddress");
	func();
}

BOOL BlockDlg::DisplayInfo(HWND hWnd)
{
	TCHAR szBuf[128];
	TCHAR szSN[64];
	int i = 0;
	PDEVICEINFO pDev = NULL;
	pDev = DevInfo::GetInstance()->QueryInfoRequired();
	
	//show information in list box and combo box
	//设备名称， 设备挂载，设备
	memset(szBuf, 0, sizeof(szBuf));
	SendMessage(m_listBox, LB_ADDSTRING, 0, (LPARAM)L"已检测到新的USB设备接入");
		
	wsprintf(szBuf, L"设备名称：%s", pDev->szVolName);
	SendMessage(m_listBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	SendMessage(hWnd, LB_INSERTSTRING, 0, (LPARAM)szBuf);

	memset(szBuf, 0, sizeof(szBuf));
	wsprintf(szBuf, L"挂载盘符：%c", pDev->szVolMask);
	SendMessage(m_listBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	SendMessage(hWnd, LB_INSERTSTRING, 0, (LPARAM)szBuf);
		
	memset(szBuf, 0, sizeof(szBuf));
	memset(szSN, 0, sizeof(szSN));
	DevInfo::GetInstance()->c2w(pDev->szSN.c_str(), szSN);
	wsprintf(szBuf, L"设备编号：%s", szSN);
	SendMessage(m_listBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	SendMessage(hWnd, LB_INSERTSTRING, 0, (LPARAM)szBuf);
		
	memset(szBuf, 0, sizeof(szBuf));
	GetLocalTime(&pDev->sysTime);
	wsprintf(szBuf, L"接入时间：%d.%2d.%2d %2d:%2d:%2d", pDev->sysTime.wYear,
		pDev->sysTime.wMonth, pDev->sysTime.wDay,
		pDev->sysTime.wHour, pDev->sysTime.wMinute, pDev->sysTime.wSecond);
	SendMessage(m_listBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	SendMessage(hWnd, LB_INSERTSTRING, 0, (LPARAM)szBuf);

	memset(szBuf, 0, sizeof(szBuf));
	wsprintf(szBuf, L"%c", pDev->szVolMask);
	SendMessage(m_comboBox, CB_ADDSTRING, 0, (LPARAM)szBuf);
	
	
	return TRUE;
}

BOOL BlockDlg::ClearInfo(int nIndex)
{
	SendMessage(m_comboBox, CB_DELETESTRING, nIndex, 0);
	for (int i = 0; i < 5; i++)
		SendMessage(m_listBox, LB_DELETESTRING, nIndex*5, 0);
	return TRUE;
}

//init and create a transparent window
HWND BlockDlg::InitDlg(HINSTANCE& hInstance, HWND& hParent)
{
	TCHAR szAppName[] = L"Block";
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpszClassName = szAppName;
	wcex.cbClsExtra = NULL;
	wcex.cbWndExtra = NULL;
	wcex.hIcon = NULL;/*LoadIcon(NULL, MAKEINTRESOURCE(IDI_TRAYICON));*/
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.hIconSm = NULL;/*wcex.hIcon;*/
	wcex.lpfnWndProc = WndProcBlock;
	wcex.lpszMenuName = NULL;
	wcex.hInstance = hInstance;
	wcex.style = CS_HREDRAW | CS_VREDRAW;

	hOwner = hParent;

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Register Window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	//main blocking window
	m_blockDlg = CreateWindow(szAppName, NULL, WS_DLGFRAME, 0, 0, xScreen, yScreen, hParent, NULL, hInstance, NULL);
	if (!m_blockDlg)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}

	//listbox control
	m_listBox = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL \
		 | LBS_HASSTRINGS | LBS_NOSEL, (xScreen - 400)/2, (yScreen - 480)/2, 400, 250, m_blockDlg, NULL, hInstance, NULL);
	if (!m_listBox)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create list box failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	SendMessage(m_listBox, LB_SETCOLUMNWIDTH, 400, 0);

	//static text
	m_staticText = CreateWindow(L"STATIC", NULL, WS_CHILD | WS_VISIBLE \
		| SS_LEFT, (xScreen - 400)/2 + 30, (yScreen - 480)/2 + 280, 100, 25, m_blockDlg, NULL, hInstance, NULL);
	if (!m_staticText)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create list box failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	//set text
	SendMessage(m_staticText, WM_SETTEXT, NULL, (LPARAM)L"请选择设备：");

	//combo box control
	m_comboBox = CreateWindow(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE \
		| CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_DISABLENOSCROLL, (xScreen - 400)/2 + 150, (yScreen - 480)/2 + 280, 180, 60, m_blockDlg, NULL, hInstance, NULL);
	if (!m_comboBox)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create list box failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	//authorize button
	m_btnAuth = CreateWindow(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE \
		| BS_TEXT | BS_CENTER | BS_PUSHBUTTON | BS_PUSHLIKE, (xScreen - 400)/2, (yScreen - 480)/2 + 370, 100, 50, m_blockDlg, NULL, hInstance, NULL);
	if (!m_btnAuth)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create list box failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	SendMessage(m_btnAuth, WM_SETTEXT, NULL, (LPARAM)L"正常打开");

	//read only button
	m_btnReadOnly = CreateWindow(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE \
		| BS_TEXT | BS_CENTER | BS_PUSHBUTTON | BS_PUSHLIKE, xScreen/2 + 100, (yScreen - 480)/2 + 370, 100, 50, m_blockDlg, NULL, hInstance, NULL);
	if (!m_btnReadOnly)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create list box failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	SendMessage(m_btnReadOnly, WM_SETTEXT, NULL, (LPARAM)L"只读打开");

	//set transparent
	SetWindowLong(m_blockDlg, GWL_EXSTYLE, GetWindowLong(m_blockDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_blockDlg, 0, 255 * 70 / 100, LWA_ALPHA);

	//Show();

	return m_blockDlg;
}
