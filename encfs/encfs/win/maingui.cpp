#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "resource.h"

extern HINSTANCE hFuseDllInstance;
#define hInst hFuseDllInstance

NOTIFYICONDATA niData;

static ULONGLONG GetDllVersion(LPCTSTR lpszDllName);
static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#define TRAYICONID	1
#define SWM_TRAYMSG	WM_APP+100
enum {
	IDM_EXIT = 101,
	IDM_ABOUT,
	IDM_OPEN
};


extern "C" int main_gui(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */ , LPSTR /* lpCmdLine */ ,int nCmdShow)
{
	MSG msg;

	if (!InitInstance(hInst, nCmdShow))
		return FALSE;

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!IsDialogMessage(msg.hwnd, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}

// Initialize the window and tray icon
static BOOL
InitInstance(HINSTANCE hInstance, int /* nCmdShow */)
{
	// prepare for XP style controls
	InitCommonControls();

	HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_DIALOG), NULL, (DLGPROC) DlgProc);
	if (!hWnd)
		return FALSE;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	ULONGLONG ullVersion = GetDllVersion(_T("shell32.dll"));
	if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
		niData.cbSize = sizeof(NOTIFYICONDATA);
	else
		niData.cbSize = NOTIFYICONDATA_V2_SIZE;

	// the ID number can be anything you choose
	niData.uID = TRAYICONID;

	// state which structure members are valid
	niData.uFlags = NIF_ICON | NIF_MESSAGE;

	// load the icon
	niData.hIcon =
		(HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
				  GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

	// the window to send messages to and the message to send
	//              note:   the message value should be in the
	//                              range of WM_APP through 0xBFFF
	niData.hWnd = hWnd;
	niData.uCallbackMessage = SWM_TRAYMSG;

	Shell_NotifyIcon(NIM_ADD, &niData);

	// free icon handle
	if (niData.hIcon && DestroyIcon(niData.hIcon))
		niData.hIcon = NULL;

	// call ShowWindow here to make the dialog initially visible
	return TRUE;
}

// Get dll version number
static ULONGLONG
GetDllVersion(LPCTSTR lpszDllName)
{
	DLLGETVERSIONPROC pDllGetVersion =
		(DLLGETVERSIONPROC) GetProcAddress(GetModuleHandle(lpszDllName), "DllGetVersion");
	if (!pDllGetVersion)
		return 0;

	DLLVERSIONINFO dvi;
	HRESULT hr;

	ZeroMemory(&dvi, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);
	hr = (*pDllGetVersion) (&dvi);
	if (SUCCEEDED(hr))
		return MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion, 0, 0);

	return 0;
}

static BOOL
OnInitDialog(HWND hWnd)
{
	HICON hIcon;

	hIcon = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
				GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

	hIcon = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
				GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
	return TRUE;
}

static void
ShowContextMenu(HWND hWnd)
{
	POINT pt;

	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();

	if (!hMenu)
		return;

	AppendMenu(hMenu, 0, IDM_OPEN, _T("Open/Create"));
	AppendMenu(hMenu, MF_MENUBREAK, -1, NULL);
	// TODO add mounted directories
	// TODO add possible directories
	AppendMenu(hMenu, 0, IDM_ABOUT, _T("About"));
	AppendMenu(hMenu, 0, IDM_EXIT, _T("Exit"));
	SetMenuDefaultItem(hMenu, IDM_OPEN, FALSE);

	// avoid disappear
	SetForegroundWindow(hWnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
}

// Message handler for the app
static INT_PTR CALLBACK
DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) {
	case SWM_TRAYMSG:
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE) {
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId) {
		case IDM_OPEN:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR) IDD_ABOUTBOX, hWnd, (DLGPROC) AboutDlgProc);
			break;
		}
		return 1;
	case WM_INITDIALOG:
		return OnInitDialog(hWnd);
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

static LRESULT CALLBACK
AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /* lParam*/)
{
	switch (message) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

