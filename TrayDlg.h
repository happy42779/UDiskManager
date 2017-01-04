#pragma once
#include "precompiler.h"

#include "BlockDlg.h"
#include "DevInfo.h"
#include "resource.h"

#define WM_SHOWTASK (WM_USER + 1)
#define IDM_EXIT 10001
#define IDM_ADDEXCLU 10002
#define IDM_RMEXCLU 10003
#define IDM_CLEAR 10004

class TrayDlg
{
private:
	TrayDlg()
	{
		width = 750;	
		height = 500;
		bFlag = FALSE;
	}	
public:
	static TrayDlg*& GetInstance()
	{
		if(NULL == m_phInstance)
			m_phInstance = new TrayDlg();
		
		return m_phInstance;
	}
	class StatusIcon
	{
		friend TrayDlg;
		StatusIcon()
		{
			memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
		}
		BOOL InitStatusIcon(HINSTANCE& hInstance, HWND& hWnd);
		BOOL PopMenu(HWND& hWnd);
	private:
		NOTIFYICONDATA m_nid;  //notify icon data
		HMENU hPopupMenu;	//menu handle			
		POINT point;	//where is the icon
	}m_statusIcon;
	
	HWND InitDlg(HINSTANCE& hInstance);
	static LRESULT CALLBACK WndProcTray(HWND, UINT, WPARAM, LPARAM);

	//show window
	void Show()
	{
		if (bFlag=~bFlag)
		{
			ShowWindow(m_trayDlg, SW_SHOW);
			UpdateWindow(m_trayDlg);
		}	
		else
			ShowWindow(m_trayDlg, SW_HIDE);
	}
	BOOL OptionMenu(HWND& hWnd);
	BOOL InserColumn();
	//add exclusion
	BOOL AddExclusion();
	//remove exclusion
	BOOL RemoveExclusion();
	//clear all record
	BOOL ClearRecord();
	//update LV info
	BOOL UpdateLVInfo();
private:
	VOID AddRecord();
	VOID AddWrongRecod(int);
	VOID SetRecordValidity();
	VOID AutoResize();	//auto resize, when the window is resized
	VOID GetDlgSize();	//get the size of the window
private:
	HWND m_trayDlg;		 //handle to the tray
	HWND m_listView;	//handle to the listView control
	HWND m_edit;		//handle
	HWND m_btnAdd;		//handle to the add exclusion button
	HWND m_btnRemove;	//handle to the remove button
	HWND m_btnRefresh;	//handle to the refresh button
	HWND m_listBox;		//handle to the listbox control
	static TrayDlg* m_phInstance;  //only instance of TrayDlg class
	int width, height;	//dialog attrib
	RECT rect;
	BOOL bFlag;	//to show or to hide
	LPNMITEMACTIVATE lpnmitem;	//point to an notify message item
	HMENU hOptionMenu;
};

//tray dialog callback
LRESULT CALLBACK TrayDlg::WndProcTray(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	PDEV_BROADCAST_HDR pdbr;
	switch(uMsg)
	{
	//case WM_SYSCOMMAND:
	//	return TRUE; 
	case WM_CREATE:
		m_phInstance->GetDlgSize();
		return TRUE;
	case WM_SIZE:	//this message is received before the dlg is displayed
		m_phInstance->AutoResize();
		return TRUE;
	case WM_SHOWTASK:  //wParam holds the uID, lParam holds the mouse message
		if (WM_LBUTTONDBLCLK == lParam)
			m_phInstance->Show();
		else if (WM_RBUTTONUP == lParam)
			m_phInstance->m_statusIcon.PopMenu(hWnd);
		return TRUE;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_RCLICK:
			m_phInstance->lpnmitem = (LPNMITEMACTIVATE)lParam;
			if (-1 == m_phInstance->lpnmitem->iItem)
				return TRUE;
			if (ListView_GetHeader(m_phInstance->m_listView) == m_phInstance->lpnmitem->hdr.hwndFrom)
				return TRUE;
			ListView_SetItemState(m_phInstance->m_listView, m_phInstance->lpnmitem->iItem, LVIS_SELECTED, LVIS_SELECTED);
			m_phInstance->OptionMenu(m_phInstance->m_listView);
			return TRUE;
		default:
			break;
		}
		break;
	case WM_COMMAND:
		if (wParam == IDM_EXIT)
		{
			//if setting pwd
			if (PwdDlg::GetInstance()->IsPWDSetting())
				return TRUE;
			//1.verify password
			//2.send quit message to the
			PwdDlg::GetInstance()->ShowModal(hWnd);
			if((BOOL)GetWindowLong(hWnd, GWL_USERDATA))
				DestroyWindow(hWnd);
			//ShowWindow(hWnd, FALSE);
			return TRUE;
		}	
		break;
	case WM_WRONGTRIAL:
		m_phInstance->AddWrongRecod((int)lParam);
		return TRUE;
	case WM_ADDRECORD:
		m_phInstance->AddRecord();
		return TRUE;
	case WM_DEVICECHANGE: 
		pdbr = (PDEV_BROADCAST_HDR)lParam;
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
			//MessageBox(GetForegroundWindow(), L"device arrived", L"NOTIFICATION", MB_OK);
			if (DBT_DEVTYP_VOLUME == pdbr->dbch_devicetype)
			{
				PDEV_BROADCAST_VOLUME pdbv = (PDEV_BROADCAST_VOLUME)lParam;
				if (pdbv->dbcv_flags & DBTF_MEDIA)
					break;
				++g_devCount;
				++g_devsToHandle;
				DevInfo::GetInstance()->GetDiskNumByVol(pdbv->dbcv_unitmask);
				if (!DevInfo::GetInstance()->QueryDevInfo())
				{
					//cannot get device info. Set to read only mode
					MessageBox(NULL, NULL, NULL, MB_OK);
					return TRUE;
				}
				DevInfo::GetInstance()->SetDevStatus(TRUE);
				//Write information to the listbox
				BlockDlg::GetInstance()->DisplayInfo(m_phInstance->m_listBox);
				BlockDlg::GetInstance()->Show();
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:// how to find which one is deleted
			
			if (DBT_DEVTYP_VOLUME == pdbr->dbch_devicetype)
			{
				PDEV_BROADCAST_VOLUME pdbv = (PDEV_BROADCAST_VOLUME)lParam;
				if (pdbv->dbcv_flags & DBTF_MEDIA)
					break;

				if (1 == g_devsToHandle)
				{
					BlockDlg::GetInstance()->Hide();
					BlockDlg::GetInstance()->ClearInfo(0);
					g_devsToHandle--;
				}
				else if (g_devsToHandle > 1)
				{
					BlockDlg::GetInstance()->ClearInfo(DevInfo::GetInstance()->GetIndexofRecord(pdbv->dbcv_unitmask));
					g_devsToHandle--;
				}

				if (g_devCount)
				{
					g_devCount--;
					DevInfo::GetInstance()->UpdateInfo_OnDeviceRemove(pdbv->dbcv_unitmask);
					m_phInstance->UpdateLVInfo();
				}
					
			}
			break;
		default:break;
		}
		return TRUE;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		m_phInstance->bFlag = ~m_phInstance->bFlag;
		return TRUE;
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &(m_phInstance->m_statusIcon.m_nid));
		PostQuitMessage(0);
		return TRUE;
	default:break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL TrayDlg::InserColumn()
{	
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;

	/*1. serial number*/
	lvc.pszText = L"设备编号";
	lvc.cx = 230;
	lvc.iSubItem = 1;
	lvc.fmt = LVCFMT_RIGHT;
	if (-1 == ListView_InsertColumn(m_listView, 1, &lvc))
	{
		MessageBox(NULL, L"insert item failed", L"ERROR", MB_OK);
		return FALSE;
	}
	

	/*2. validity	*/
	lvc.pszText = L"设备状态";
	lvc.cx = 70;
	lvc.iSubItem = 2;
	lvc.fmt = LVCFMT_RIGHT;
	if (-1 == ListView_InsertColumn(m_listView, 2, &lvc))
	{
		MessageBox(NULL, L"insert item failed", L"ERROR", MB_OK);
		return FALSE;
	}
	/*3. access	privilege*/
	lvc.pszText = L"使用权限";
	lvc.cx = 70;
	lvc.iSubItem = 3;                            
	lvc.fmt = LVCFMT_RIGHT;
	if (-1 == ListView_InsertColumn(m_listView, 3, &lvc))
	{
		MessageBox(NULL, L"insert item failed", L"ERROR", MB_OK);
		return FALSE;
	}
	/*4. name of the disk	*/
	lvc.pszText = L"设备名称";
	lvc.cx = 120;
	lvc.iSubItem = 0;
	lvc.fmt = LVCFMT_LEFT;
	if (-1 == ListView_InsertColumn(m_listView, 0, &lvc))
	{
		MessageBox(NULL, L"insert item failed", L"ERROR", MB_OK);
		return FALSE;
	}
	/*5. maximum capacity	*/
	lvc.pszText = L"设备容量";
	lvc.cx = 70;
	lvc.iSubItem = 4;
	lvc.fmt = LVCFMT_RIGHT;
	if (-1 == ListView_InsertColumn(m_listView, 4, &lvc))
	{
		MessageBox(NULL, L"insert item failed", L"ERROR", MB_OK);
		return FALSE;
	}
	/*6. time of being plugged*/
	lvc.pszText = L"连接时间";
	lvc.cx = 150;
	lvc.iSubItem = 5;
	lvc.fmt = LVCFMT_RIGHT;
	if (-1 == ListView_InsertColumn(m_listView, 5, &lvc))
	{
		int iErrCode = GetLastError();
		TCHAR szErrMsg[24] = { 0 };
		wsprintf(szErrMsg, L"insert column failed...\\n\\rerror code returned is:%d", iErrCode);
		MessageBox(NULL, szErrMsg, L"ERROR", MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL TrayDlg::UpdateLVInfo()
{
	LVITEM lvi;
	std::vector<PDEVICEINFO> devinfo = DevInfo::GetInstance()->QueryInfoSet();
	int itemCount = SendMessage(m_listView, LVM_GETITEMCOUNT, 0, 0);

	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 2;
	lvi.pszText = L"断开";

	for (int i = 0; i < itemCount; ++i)
	{
		if (NULL == devinfo[i])
		{
			lvi.iItem = i;
			SendMessage(m_listView, LVM_SETITEM, 0,(LPARAM)&lvi);
		}
	}
	return TRUE;
}

//? 这里调用多线程来写入
VOID TrayDlg::AddRecord()
{
	PDEVICEINFO pDev = DevInfo::GetInstance()->QueryInfoRequired();
	BOOL b;
	LVITEM lvi;
	memset(&lvi, 0, sizeof(lvi));
	TCHAR szBuf[64] = { 0 };
	int itemIndex = DevInfo::GetInstance()->GetDevCount() - 1;

	lvi.iSubItem = 0;
	lvi.iItem = itemIndex;
	lvi.mask = LVIF_TEXT;
	lvi.cchTextMax = 64;
	lvi.pszText = pDev->szVolName;
	ListView_InsertItem(m_listView, (LPARAM)&lvi);

	lvi.iSubItem = 1;
	DevInfo::GetInstance()->c2w(pDev->szSN.c_str(), szBuf);
	lvi.pszText = szBuf;
	b = ListView_SetItem(m_listView, &lvi);
	if (!b)
		PRINT_ERROR(L"TrayDlg:AddRecord.ListView_SetItem");
	
	lvi.iSubItem = 2;
	if (pDev->bStatus)
		lvi.pszText = L"连接";
	else
		lvi.pszText = L"断开";
	b = ListView_SetItem(m_listView, &lvi);
	
	lvi.iSubItem = 3;
	if (pDev->bMode)
		lvi.pszText = L"读写";
	else
		lvi.pszText = L"只读";
	b = ListView_SetItem(m_listView, &lvi);

	lvi.iSubItem = 4;
	memset(szBuf, 0, sizeof(szBuf));
	wsprintf(szBuf, L"%d MB", pDev->dwDiskSpace);
	lvi.pszText = szBuf;
	b = ListView_SetItem(m_listView, &lvi);	
	
	lvi.iSubItem = 5;
	memset(szBuf, 0, sizeof(szBuf));
	wsprintf(szBuf, L"%d.%2d.%2d %2d:%2d:%2d",
		pDev->sysTime.wYear, pDev->sysTime.wMonth,
		pDev->sysTime.wDay, pDev->sysTime.wHour,
		pDev->sysTime.wMinute, pDev->sysTime.wSecond);
	lvi.pszText = szBuf;
	b = ListView_SetItem(m_listView, &lvi);	
	if (!b)
		PRINT_ERROR(L"TrayDlg:AddRecord.ListView_SetItem");
}

VOID TrayDlg::AddWrongRecod(int nWrongCount)
{
	TCHAR szBuf[64] = { 0 };
	SYSTEMTIME st;
	//RECT itemRect;
	//HDC hdc;

	//hdc = GetDC(m_listBox);
	GetLocalTime(&st);

	memset(szBuf, 0, sizeof(szBuf));
	wsprintf(szBuf, L"%d.%2d.%2d %2d:%2d:%2d 用户尝试错误密码！ 当前总错误次数：%d",
		st.wYear, st.wMonth, st.wDay, st.wHour,
		st.wMinute, st.wSecond, nWrongCount);
	SendMessage(m_listBox, LB_INSERTSTRING, 0, (LPARAM)szBuf);
	
	//set text color, reference to windows programming PDFs
	//SendMessage(m_listBox, LB_GETITEMRECT, SendMessage(m_listBox, LB_GETCOUNT, 0, 0), (LPARAM)&itemRect);
	//SetTextColor(hdc, 0xff0000);
	//SetBkMode(hdc, TRANSPARENT);
	//SetBkColor(hdc, 0xffffff);
	//if (0 == DrawText(hdc, szBuf, sizeof(szBuf) / sizeof(TCHAR), &itemRect, DT_INTERNAL))
	//	PRINT_ERROR(L"AddWrongRecod:DrawText");
	//ReleaseDC(m_listBox, hdc);

	/*write log file*/
}

VOID TrayDlg::AutoResize()
{
	GetClientRect(m_trayDlg, &m_phInstance->rect);
	m_phInstance->width = m_phInstance->rect.right - m_phInstance->rect.left;
	m_phInstance->height = m_phInstance->rect.bottom - m_phInstance->rect.top;
	SetWindowPos(m_phInstance->m_listView, NULL, 0, 0, m_phInstance->width, m_phInstance->height - 165, SWP_NOMOVE);
	SetWindowPos(m_phInstance->m_listBox, NULL, 0, m_phInstance->height - 164, m_phInstance->width, 100, NULL);
	SetWindowPos(m_phInstance->m_btnRefresh, NULL, m_phInstance->width - 80, m_phInstance->height - 50, 0, 0, SWP_NOSIZE);
	SendMessage(m_phInstance->m_listBox, LB_SETCOLUMNWIDTH, width, 0);
	UpdateWindow(m_trayDlg);
}

VOID TrayDlg::GetDlgSize()
{
	GetClientRect(m_trayDlg, &m_phInstance->rect);
	m_phInstance->width = m_phInstance->rect.right - m_phInstance->rect.left;
	m_phInstance->height = m_phInstance->rect.bottom - m_phInstance->rect.top;
}

//init the tray icon
BOOL TrayDlg::StatusIcon::InitStatusIcon(HINSTANCE& hInstance, HWND& hWnd)
{
	//notify icon data
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.uID = IDI_TRAYICON;
	m_nid.uCallbackMessage = WM_SHOWTASK;
	m_nid.hWnd = hWnd;
	m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE;

	Shell_NotifyIcon(NIM_ADD, &m_nid);

	return TRUE;
}

//Create and show a popup menu when right button is clicked
BOOL TrayDlg::StatusIcon::PopMenu(HWND& hWnd)
{
	GetCursorPos(&point);
	hPopupMenu = CreatePopupMenu();
	AppendMenu(hPopupMenu, MF_STRING, IDM_EXIT, L"退    出");
	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, point.x, point.y, 0, hWnd, NULL);
	//DestroyMenu(hPopupMenu);
	return TRUE;
}

BOOL TrayDlg::OptionMenu(HWND& hWnd)
{
	GetCursorPos(&m_statusIcon.point);
	hOptionMenu = CreatePopupMenu();
	AppendMenu(hOptionMenu, MF_STRING, IDM_ADDEXCLU, L"添加权限");
	AppendMenu(hOptionMenu, MF_STRING, IDM_RMEXCLU, L"移除权限");
	AppendMenu(hOptionMenu, MF_STRING, IDM_CLEAR, L"清除记录");
	TrackPopupMenu(hOptionMenu, TPM_LEFTALIGN, m_statusIcon.point.x,
		m_statusIcon.point.y, 0, hWnd, NULL);
	DestroyMenu(hOptionMenu);
	return TRUE;
}

HWND TrayDlg::InitDlg(HINSTANCE& hInstance)
{
	TCHAR szAppName[] = L"Tray";
	WNDCLASSEX wcex;

	//window class
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpszClassName = szAppName;
	wcex.cbClsExtra = NULL;
	wcex.cbWndExtra = NULL;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.hIconSm = wcex.hIcon;
	wcex.lpfnWndProc = WndProcTray;
	wcex.lpszMenuName = NULL;
	wcex.hInstance = hInstance;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	//register window
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Register Window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	//create window	no resizible
	m_trayDlg = CreateWindow(szAppName, L"UDiskManager", WS_OVERLAPPEDWINDOW & \
		~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, wcex.hInstance, NULL);
	if (!m_trayDlg)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	//create a list view
	m_listView = CreateWindow(L"SysListView32", NULL, WS_CHILD | WS_VISIBLE \
		| LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL, \
		0, 0, width, height - 165, m_trayDlg, NULL, hInstance, NULL);
	if (!m_listView)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	ListView_SetExtendedListViewStyle(m_listView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	//create a listBox
	m_listBox = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE \
		| LBS_HASSTRINGS | LBS_DISABLENOSCROLL, 0, height - 164, width, 100, m_trayDlg, NULL, hInstance, NULL);
	if (!m_listBox)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	
	//create refresh button
	m_btnRefresh = CreateWindow(L"BUTTON", L"刷新", WS_CHILD | WS_VISIBLE \
		| BS_TEXT | BS_PUSHLIKE | BS_PUSHBUTTON, width - 80, height - 50, 60, 30, m_trayDlg, NULL, hInstance, NULL);
	if (!m_btnRefresh)
	{
		MessageBox(GetForegroundWindow(), L"ERROR! Create window failed!", L"ERROR", MB_ICONERROR);
		return NULL;
	}
	
	/*add the items to listView control*/
	if (!InserColumn())
	{
		MessageBox(GetForegroundWindow(), L"Insert column failed!", L"ERROR", MB_OK);
		return NULL;
	}
	
	//InsertItem(); //testing...

	//Init tray status icon
	m_phInstance->m_statusIcon.InitStatusIcon(hInstance, m_trayDlg);
	//Show();
	return m_trayDlg;
}