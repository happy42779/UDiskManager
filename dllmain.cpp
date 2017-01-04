// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <stdio.h>
#include <tchar.h>

HWND hSASWnd = NULL;
FARPROC FOldPorc = NULL;
HANDLE hThread = NULL;
HANDLE hThread1 = NULL;
DWORD dwThreadId = 0;
HHOOK hHook = NULL;
char szOutput[36] = { 0 };
HMODULE hMod = NULL;

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);
LRESULT CALLBACK KeyboardProc(int, WPARAM, LPARAM);
LRESULT CALLBACK SASWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI ThreadFunc(LPARAM lParam);
DWORD WINAPI tskmgrDetect(LPARAM lParam);
void Hook();
void UnHook();

VOID PRINT_INFO(PCHAR szOutput)
{
#ifdef _DEBUG
	OutputDebugStringA(szOutput);
//#else _DEBUG
	OutputDebugStringA(szOutput);
#endif
}

extern "C" _declspec(dllexport) void StartHook()
{
	Hook();
}

extern "C" _declspec(dllexport) void StopHook()
{
	UnHook();
}

void Hook()
{
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunc, NULL, 0, &dwThreadId);
	hThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)tskmgrDetect, NULL, 0, NULL);
}

void UnHook()
{
	if (!UnhookWindowsHookEx(hHook))
	{
		PRINT_INFO("Unhook failed...");
	}
	PRINT_INFO("Unhook keyboard hook successfully");
	TerminateThread(hThread, 0);
	TerminateThread(hThread1, 0);
	CloseHandle(hThread);
	CloseHandle(hThread1);
}




BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		PRINT_INFO("dll load successfully!\n");
		hMod = hModule;
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		PRINT_INFO("dll unload successfully!\n");
		break;
	}
	return TRUE;
}

DWORD WINAPI ThreadFunc(LPARAM lParam)
{
	//PRINT_INFO("Thread Func is running\n");
	HDESK hDesk = OpenDesktop(L"Winlogon", 0, FALSE, MAXIMUM_ALLOWED);

	EnumDesktopWindows(hDesk, (WNDENUMPROC)EnumWindowProc, 0);

	if (NULL != hSASWnd)
	{
		FOldPorc = (FARPROC)SetWindowLongPtr(hSASWnd, GWLP_WNDPROC, (LONG)SASWindowProc);
	}
	CloseHandle(hDesk);

	hDesk = OpenDesktop(L"Default", 0, FALSE, MAXIMUM_ALLOWED);
	SetThreadDesktop(hDesk);
	CloseHandle(hDesk);

	//MessageBox(NULL, L"im hooking", NULL, NULL);
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hMod, 0);
	if (NULL == hHook)
	{
		PRINT_INFO("Set KeyBoard hook failed...");
		return 1;
	}
	PRINT_INFO("Set keyBoard hook successfully...");

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 1;
}

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam)
{
	char ClassBuf[128] = { 0 };
	GetWindowTextA(hWnd, ClassBuf, sizeof(ClassBuf));
	if (NULL != strstr(ClassBuf, "SAS window"))
	{
		hSASWnd = hWnd;
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK SASWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_HOTKEY == uMsg)
	{
		PRINT_INFO("All SAS window's hotkeys are disabled");
		return 1;
		WORD wKey = HIWORD(lParam);
		WORD wModifier = LOWORD(lParam);
		bool IsCtrlDown = ((wModifier & VK_CONTROL) != 0);
		bool IsAltDown = ((wModifier & VK_MENU) != 0);
		bool IsShiftDown = ((wModifier & VK_SHIFT) != 0);

		if (IsCtrlDown && IsAltDown && (wKey == VK_DELETE))
		{
			PRINT_INFO("Is Ctrl + alt + delete\n");
			return 1;
		}
		else if (IsCtrlDown && IsShiftDown && (wKey == VK_ESCAPE))
		{
			PRINT_INFO("Is Ctrl + alt + escape\n");
			return 1;
		}
	}
	return CallWindowProc((WNDPROC)FOldPorc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (HC_ACTION == nCode)
	{
		switch (wParam)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
			if (LLKHF_ALTDOWN & p->flags)
			{
				PRINT_INFO("test alt\n");
				return 1;
			}
			if (VK_LWIN == p->vkCode || VK_RWIN == p->vkCode)
			{
				PRINT_INFO("win is down\n");
				return 1;
			}
			if ((VK_TAB == p->vkCode) && (p->flags & LLKHF_ALTDOWN) != 0)
			{
				PRINT_INFO("alt is down\n");
				return 1;
			}
			else if ((VK_ESCAPE == p->vkCode) && (p->flags & LLKHF_ALTDOWN) != 0)
			{
				PRINT_INFO("esc is down\n");
				return 1;
			}
				
			else if ((VK_ESCAPE == p->vkCode) && ((GetKeyState(VK_CONTROL) & 0x8000) != 0))
			{
				PRINT_INFO("ctrl is down\n");
				return 1;
			}	
			else if (((GetKeyState(VK_CONTROL) & 0x8000) != 0) && ((GetKeyState(VK_MENU) & 0x8000) != 0))
			{
				PRINT_INFO("menu is down\n");
				return 1;
			}	
			break;
		}
		return CallNextHookEx(hHook, nCode, wParam, lParam);
	}
}

DWORD WINAPI tskmgrDetect(LPARAM lParam)
{
	HWND hWnd; 
	TCHAR szClassName[128] = { 0 };
	while (1)
	{	
		hWnd = GetForegroundWindow();
		GetClassName(hWnd, szClassName, sizeof(szClassName) >> 1);
		if ((0 == _tcscmp(L"TaskManagerWindow", szClassName)) || (0 == _tcscmp(L"#32770", szClassName)))
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			PRINT_INFO("found task manager\n");
		}
		Sleep(500);
	}
	return 1;
}