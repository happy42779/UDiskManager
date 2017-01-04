#pragma once
#include "KeyFiles.h"

class PwdDlg
{
private:
	PwdDlg()
		:m_hBtnCancel(NULL),
		m_hBtnOK(NULL),
		m_pwdDlg(NULL),
		m_hEdit(NULL),
		m_hConfrmEdit(NULL),
		bPWDInitiating(TRUE)
	{
		x = GetSystemMetrics(SM_CXSCREEN);
		y = GetSystemMetrics(SM_CYSCREEN);
		memset(szEdit, 0, sizeof(szEdit));
		memset(szEditConfrm, 0, sizeof(szEditConfrm));
	}
public:
	//get the class instance
	static PwdDlg*& GetInstance()
	{
		if (NULL == m_phInstance)
			m_phInstance = new PwdDlg();
		return m_phInstance;
	}
	//the callback msg
	static LRESULT CALLBACK WndProcPwd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//Init the pwd dlg
	BOOL InitDlg(HINSTANCE&, HWND&);

	//whether this dlg is to set pwd or to verify pwd
	void SetPWDStatus(BOOL bIsSet)	 //if the pwd is set, then the param passed in should be true;
	{
		if (bIsSet)
			bPWDInitiating = FALSE;
	}
	//provided to decide whether pwd is setting.
	BOOL IsPWDSetting()
	{
		if (bPWDInitiating)
			return TRUE;
		return FALSE;
	}
	BOOL Show();
	void ShowModal(HWND& hWnd);
private:
	inline VOID SetBuffer()
	{
    memset(szEdit, 0, sizeof(szEdit));
    memset(szEditConfrm, 0, sizeof(szEditConfrm));
		szEdit[0] = sizeof(szEdit) / sizeof(TCHAR);
		szEditConfrm[0] = sizeof(szEditConfrm) / sizeof(TCHAR);
	}
	inline VOID ClearInput()
	{	
		SendMessage(m_hEdit, WM_SETTEXT, NULL, (LPARAM)L"");
		SendMessage(m_hConfrmEdit, WM_SETTEXT, NULL, (LPARAM)L"");
	}
	inline HWND GetCallingWND()
	{
		return (HWND)GetWindowLong(m_pwdDlg, GWL_USERDATA);
	}
	inline VOID SetInputFocus()
	{
		SetFocus(m_hEdit);
	}
private:
	static PwdDlg* m_phInstance;
	HWND hOwner;
	HWND m_pwdDlg;
	HWND m_hBtnOK;
	HWND m_hBtnCancel;
	HWND m_hEdit;
	HWND m_hConfrmEdit;
	TCHAR szEdit[31];
	TCHAR szEditConfrm[31];
	int x, y;
	bool bPWDInitiating;
};

//call back for
LRESULT CALLBACK PwdDlg::WndProcPwd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		return TRUE;
	case WM_NCLBUTTONDOWN:
		if (wParam == HTCAPTION)
			return TRUE;
	case WM_COMMAND:
		if (m_phInstance->m_hBtnCancel == (HWND)lParam)	   //if clicked cancel hide the pwd and enable the blocking window again
		{
			if (m_phInstance->bPWDInitiating)	//if setting the pwd  
			{	
				//if user canceled to set the password exit the program
				PostQuitMessage(0);
			}
			else   //if verifying 1. exit the msg loop, 2. hide pwddlg, 3. enable blocking dlg again
			{
				
				ShowWindow(hWnd, SW_HIDE);
				PostQuitMessage(0);
				EnableWindow(m_phInstance->GetCallingWND(), TRUE);
				UpdateWindow(m_phInstance->GetCallingWND());
				SetForegroundWindow(m_phInstance->GetCallingWND());
				SetWindowLong(m_phInstance->GetCallingWND(), GWL_USERDATA, (LONG)FALSE);//false here means verifying password failed
				////最终为了获取traydlg的句柄，callingWND可能返回blockingdlg或者traydlg的句柄，如果返回错误尝试另外一个
				KeyFiles::GetInstance()->SendWrongRecord(m_phInstance->hOwner);
			}
		}
		//1. if setting passcode, set the bInitiatingPWD status to FALSE, then exit msg loop
		//2. if verifying passcode, verify 
		else if (m_phInstance->m_hBtnOK == (HWND)lParam)
		{
			//1. setting: 
			if (m_phInstance->bPWDInitiating)
			{
				// to do
				//1. set passcode
				if (0 == SendMessage(m_phInstance->m_hEdit, EM_LINELENGTH, 0, 0))
				{
					MessageBox(GetForegroundWindow(), L"Invalid password\nPlease try again", L"WARNING", MB_OK | MB_ICONWARNING);
					m_phInstance->ClearInput();
					m_phInstance->SetInputFocus();
					return TRUE;
				}
				m_phInstance->SetBuffer();
				SendMessage(m_phInstance->m_hEdit, EM_GETLINE, NULL, (LPARAM)m_phInstance->szEdit);

				SendMessage(m_phInstance->m_hConfrmEdit, EM_GETLINE, NULL, (LPARAM)m_phInstance->szEditConfrm);
				if (_wcsicmp(m_phInstance->szEdit, m_phInstance->szEditConfrm))
				{
					MessageBox(GetForegroundWindow(), L"You input two different passwords\nPlease try again", L"WARNING", MB_OK | MB_ICONWARNING);
					m_phInstance->ClearInput();
					m_phInstance->SetInputFocus();
					return TRUE;
				}
	
				if(KeyFiles::GetInstance()->SetPWD(m_phInstance->szEdit))
					MessageBox(NULL, L"password set successfully", L"Congratulations", MB_OK);
				else   //failed to set password,  then retry
				{
					m_phInstance->ClearInput();
					//goto
				}
				//2. set passcode status
				m_phInstance->bPWDInitiating = FALSE;
				memset(m_phInstance->szEdit, 0, sizeof(m_phInstance->szEdit));
				PostQuitMessage(0);
			}
			//2. verifying
			else
			{  
				if (0 == SendMessage(m_phInstance->m_hEdit, EM_LINELENGTH, NULL, NULL))
				{
					MessageBox(GetForegroundWindow(), L"Invalid password\n", L"WARNING", MB_OK | MB_ICONWARNING);
					m_phInstance->SetInputFocus();
					return TRUE;
				}
				//get input from editbox
				m_phInstance->SetBuffer();
				SendMessage(m_phInstance->m_hEdit, EM_GETLINE, NULL, (LPARAM)m_phInstance->szEdit);
				if (!KeyFiles::GetInstance()->VerifyPWD(m_phInstance->szEdit))
				{
					MessageBox(GetForegroundWindow(), L"密码错误！\n请重新输入...", L"ERROR", MB_OK | MB_ICONERROR);
					//clear input in the editbox
					KeyFiles::GetInstance()->RecordWrong();
					m_phInstance->ClearInput();
					m_phInstance->SetInputFocus();
					return TRUE;
				}
				
				//access permitted
				/***do something here***/
				
				//1. hide pwddlg
				//2. exit the msg loop
				//3. Enable the Window again
				//4. imply whether password is verified or not
				ShowWindow(hWnd, SW_HIDE);
				PostQuitMessage(0);
				EnableWindow(m_phInstance->GetCallingWND(), TRUE);
				EnableWindow(GetParent(hWnd), TRUE);	//enable the main msg loop again
				SetWindowLong(m_phInstance->GetCallingWND(), GWL_USERDATA, (LONG)TRUE);
				//try to send if there's any wrong password trying
				KeyFiles::GetInstance()->SendWrongRecord(m_phInstance->hOwner);		
			}
		}
		return TRUE;
	case WM_CLOSE:
		return TRUE;
	default:break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL PwdDlg::InitDlg(HINSTANCE& hInstance, HWND& hParent)
{
	WNDCLASSEX wcex;
	TCHAR szAppName[] = L"PWD";

	wcex.cbSize = sizeof(wcex);
	wcex.lpfnWndProc = WndProcPwd;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.hInstance = hInstance;	 
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon = NULL;
	wcex.hIconSm = wcex.hCursor;
	wcex.cbClsExtra = NULL;
	wcex.cbWndExtra = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szAppName;

	hOwner = hParent;

	//register
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, L"register window class failed...", L"ERROR", MB_OK | MB_ICONERROR);
		return NULL;
	}

	//create dlg
	m_pwdDlg = CreateWindow(szAppName, L"请输入管理员密码：",
		WS_DLGFRAME| WS_CAPTION, (x-250)/2, (y-120)/2, 250, 150, hParent, NULL, hInstance, NULL);
	if (NULL == m_pwdDlg)
	{
		MessageBox(NULL, L"create window failed...", L"ERROR", MB_OK | MB_ICONERROR);
		return NULL;
	}
	
	//create edit box
	m_hEdit = CreateWindow(L"EDIT", NULL, WS_CHILD | WS_VISIBLE \
		| ES_LEFT | ES_PASSWORD, 
		(250 - 100) / 2, 10, 100, 20, m_pwdDlg, NULL, hInstance, NULL);
	if (NULL == m_hEdit)
	{
		MessageBox(NULL, L"create window failed...", L"ERROR", MB_OK | MB_ICONERROR);
		return NULL;
	}
	m_hConfrmEdit = CreateWindow(L"EDIT", NULL, WS_CHILD \
		| ES_LEFT | ES_PASSWORD,
		(250 - 100) / 2, 40, 100, 20, m_pwdDlg, NULL, hInstance, NULL);
	if (NULL == m_hConfrmEdit)
	{
		MessageBox(NULL, L"create window failed...", L"ERROR", MB_OK | MB_ICONERROR);
		return NULL;
	}
	//set limit text in the edit control
	SendMessage(m_hEdit, EM_SETLIMITTEXT, 18, NULL);
	
	//create Button ok
	m_hBtnOK = CreateWindow(L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE \
		| BS_CENTER | BS_PUSHBUTTON | BS_PUSHLIKE, 
		(250 - 100) / 2 - 10, 10 + 30 + 10 + 30, 40, 25, m_pwdDlg, NULL, hInstance, NULL);
	if (NULL == m_hBtnOK)
	{
		MessageBox(NULL, L"create window failed...", L"ERROR", MB_OK | MB_ICONERROR);
		return NULL;
	}
	
	m_hBtnCancel = CreateWindow(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE \
		| BS_CENTER | BS_PUSHBUTTON | BS_PUSHLIKE,
		(250 - 100) / 2 + 30 + 40, 10 + 30 + 10 + 30, 40, 25, m_pwdDlg, NULL, hInstance, NULL);
	if (NULL == m_hBtnCancel)
	{
		MessageBox(NULL, L"create window failed...", L"ERROR", MB_OK | MB_ICONERROR);
		return NULL;
	}

	return TRUE;

}

//This is for verifying a password
void PwdDlg::ShowModal(HWND& hWnd)
{
	MSG msg;

	//disable the main dlg
	EnableWindow(hWnd, FALSE);	
	//pass the handle to blocking dlg through pwd dlg in order to enable the window in callback
	EnableWindow(GetParent(m_pwdDlg), FALSE);	//disable the main msg loop also
	SetWindowLong(m_pwdDlg, GWL_USERDATA, (LONG)hWnd); 
	//clear input
	ClearInput();
	SetInputFocus();

	if (ShowWindow(m_pwdDlg, SW_SHOW))	//如果当前密码框显示中，此函数再次被调用，会导致程序中有残留的消息循环，最后程序并不会真正退出
		return;

	while (GetMessage(&msg, 0, 0, 0))	//第二个参数必须为0，不然主消息循环被disable后，对窗口操作无反应，系统会以为是窗口无法抢占资源从而进行接管
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	//ShowWindow(m_pwdDlg, SW_HIDE); //cause pwddlg must be hide before the main msg loop is enabled again, so the hide goes in the msg callback
}

//This is for creating a password
BOOL PwdDlg::Show()
{
	MSG msg;
	
	//set the window title to setting password.
	SetWindowText(m_pwdDlg, L"请设置超级密码：");
	//SetWindowLong(m_hConfrmEdit, GWL_STYLE, WS_VISIBLE&GetWindowLong(m_hConfrmEdit, GWL_STYLE));
	ShowWindow(m_hConfrmEdit, SW_SHOW);
	ShowWindow(m_pwdDlg, SW_SHOW);
	ClearInput();
	SetInputFocus();

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
 	if (!bPWDInitiating)	//if password set successfully
	{
		ShowWindow(m_pwdDlg, SW_HIDE);
		SendMessage(m_pwdDlg, WM_SETTEXT, NULL, (LPARAM)L"请输入超级密码：");
		ShowWindow(m_hConfrmEdit, SW_HIDE);
		return TRUE;
	}
	else //setting password failed
	{
		//1. Destroy pwd window
		//2. return fail msg, the whole program exits after receiving this
		DestroyWindow(m_pwdDlg);
		return FALSE;
	}
}
