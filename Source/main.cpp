#include "precompiler.h"

#include "TrayDlg.h"
#include "BlockDlg.h"
#include "PwdDlg.h"
#include "KeyFiles.h"
#include "DBManage.h"
#include "DevInfo.h"

TrayDlg* TrayDlg::m_phInstance = NULL;
BlockDlg* BlockDlg::m_phInstance = NULL;
PwdDlg* PwdDlg::m_phInstance = NULL;
KeyFiles* KeyFiles::m_phInstance = NULL;
DevInfo* DevInfo::m_phInstance = NULL;

DWORD g_devCount = 0;
DWORD g_devsToHandle = 0;
HMODULE g_hModule = NULL;

void test()
{
	std::vector<int> v;
	for (int i = 0; i < 5; i++)
	{
		v.push_back(i);
	}
	v.erase(v.begin() + 2);
	MessageBox(NULL, NULL, NULL, 0);
}

						 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	/*code */
	MSG msg;
	HWND hPapa;
	//test();
	int ret;
	
	g_hModule = LoadLibrary(L"Hotkeys.dll");
	if (NULL == g_hModule)
	{
		PRINT_ERROR(L"LoadLibrary");
		return -1;
	}

	//create a tray dialog instance
	TrayDlg* tray = TrayDlg::GetInstance();
	hPapa = tray->InitDlg(hInstance);
	if (NULL == hPapa)
	{
		MessageBox(NULL, L"Oops, this program has failed\\Please restart!", L"ERROR", MB_OK | MB_ICONERROR);
		return -1;
	}
				
	//creating a pwd dialog instance
	PwdDlg* pwd = PwdDlg::GetInstance();
	if (FALSE == pwd->InitDlg(hInstance, hPapa))
	{
		MessageBox(NULL, L"Oops, this program has failed\\Please restart!", L"ERROR", MB_OK | MB_ICONERROR);
		return -1;
	}

	//create a key file instance
	KeyFiles* keyfile = KeyFiles::GetInstance();
	if (NULL == keyfile)
	{
		MessageBox(NULL, L"Oops, this program has failed\\Please restart!", L"ERROR", MB_OK | MB_ICONERROR);
	}

	//1. check whether password is set
	//2. set the flag 
	/*利用这个返回值来判断是否文件已经被一个instance打开*/
	//exist: 1, not exist:0, already open:-1
	ret = keyfile->CheckStatus();
	if (-1 == ret)
	{	
		MessageBox(GetForegroundWindow(), L"程序已经运行中\n不要重复运行！", L"错误", MB_OK | MB_ICONERROR);
		delete KeyFiles::GetInstance();
		delete PwdDlg::GetInstance();
		delete TrayDlg::GetInstance();
		return -1;
	}
	else if (1 == ret)
	{
		pwd->SetPWDStatus(TRUE);   //pwd is already set
		//load password
		if (!keyfile->loadPWD())
			return -1;
	}
	else
	{
		if (!keyfile->GenerateKF())
			return -1;
		if (!pwd->Show())	//if not set, then set the password
			return -1;	//failed to set the password.
	}

	//create a blocking dialog instance
	BlockDlg* block = BlockDlg::GetInstance();
	if (NULL == block->InitDlg(hInstance, hPapa))
	{
		MessageBox(NULL, L"Oops, this program has failed\\nPlease restart!", L"ERROR", MB_OK);
		return -1;
	}					  

	//create a devInfo instance
	DevInfo* devInfo = DevInfo::GetInstance();
	//devInfo->QueryDevInfo();
	
	//message loop
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	delete KeyFiles::GetInstance();
	delete DevInfo::GetInstance();
	delete PwdDlg::GetInstance();
	delete BlockDlg::GetInstance();
	delete TrayDlg::GetInstance();

	return 0;
}
