#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "FileUtils.h"
#include "FSConfig.h"
#include "guiutils.h"
#include "drives.h"
#include "resource.h"

// TODO preference, start at login

NOTIFYICONDATA niData;

static ULONGLONG GetDllVersion(LPCTSTR lpszDllName);
static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
static INT_PTR CALLBACK MainDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK OptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#define TRAYICONID	1
#define SWM_TRAYMSG	WM_APP+100

extern "C" int main_gui(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */ , LPSTR /* lpCmdLine */ ,int nCmdShow)
{
	MSG msg;

	// TODO check Dokan version

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

	HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC) MainDlgProc);
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

struct OptionsData
{
	OptionsData():paranoia(false) { password[0] = 0; }
	std::string rootDir;
	bool paranoia;
	char password[128];
};

static BOOL
OnInitDialog(HWND hWnd, OptionsData& data)
{
	FillFreeDrive(GetDlgItem(hWnd, IDC_CMBDRIVE));

	SetDlgItemText(hWnd, IDC_DIR, data.rootDir.c_str());

	if (data.paranoia)
		SendDlgItemMessage(hWnd, IDC_CHKPARANOIA, BM_SETCHECK, BST_CHECKED, 0);

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
	Drives::AddMenus(hMenu);
	AppendMenu(hMenu, 0, IDM_ABOUT, _T("About"));
	AppendMenu(hMenu, 0, IDM_EXIT, _T("Exit"));
	SetMenuDefaultItem(hMenu, IDM_OPEN, FALSE);

	// avoid disappear
	SetForegroundWindow(hWnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
}

static
std::string slashTerminate(const std::string &src)
{
	std::string result = src;
	if (result[ result.length()-1 ] != '/')
		result.append( "/" );
	return result;
}

static void
OpenOrCreate(HWND hwnd)
{
	static bool openAlreadyOpened = false;

	if (openAlreadyOpened)
		return;
	openAlreadyOpened = true;
	for (;;) {
		std::string dir = GetExistingDirectory(hwnd, "Select a folder which contains or will contain encrypted data.", "Select Crypt Folder");
		if (dir.empty())
			break;

		// if directory is already configured add and try to mount
		boost::shared_ptr<EncFSConfig> config(new EncFSConfig);
		if (readConfig(slashTerminate(dir), config) != Config_None) {
			char drive = SelectFreeDrive(hwnd);
			if (drive) {
				Drives::drive_t dr(Drives::Add(dir, drive));
				if (dr)
					dr->Mount(hwnd);
			}
			break;
		}

		// TODO check directory is empty, warning if continue
		// "You are initializing a crypted directory with a no-empty directory. Is this expected?"
		OptionsData data;
		data.rootDir = dir;
		if (DialogBoxParam(hInst, (LPCTSTR) IDD_OPTIONS, hwnd, (DLGPROC) OptionsDlgProc, (LPARAM) &data) != IDOK)
			break;

		memset(data.password, 0, sizeof(data.password));
		// TODO add configuration and add new drive
		break;
	}
	openAlreadyOpened = false;
}

// Message handler for the app
static INT_PTR CALLBACK
OptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	OptionsData *pData = (OptionsData*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	char buf1[sizeof(pData->password)];
	char buf2[sizeof(pData->password)];
	bool diff;

	switch (message) {
	case WM_INITDIALOG:
		pData = (OptionsData*) lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
		return OnInitDialog(hWnd, *pData);
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		case IDOK:
			pData->paranoia = (IsDlgButtonChecked(hWnd, IDC_CHKPARANOIA) == BST_CHECKED);
			GetDlgItemText(hWnd, IDC_PWD1, buf1, sizeof(buf1));
			GetDlgItemText(hWnd, IDC_PWD2, buf2, sizeof(buf2));
			diff = (strcmp(buf1, buf2) != 0);
			memset(buf1, 0, sizeof(buf1));
			memset(buf2, 0, sizeof(buf2));
			if (diff) {
				MessageBox(hWnd, "Passwords don't match", "EncFS", MB_ICONERROR);
				return TRUE;
			}
			GetDlgItemText(hWnd, IDC_PWD1, pData->password, sizeof(pData->password));
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		// TODO add support for options
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
			return 1;
		break;
	case WM_CLOSE:
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}
	return 0;
}

static INT_PTR CALLBACK
MainDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int id;

	switch (message) {
	case WM_INITDIALOG:
		Drives::Load();
		return TRUE;
	case SWM_TRAYMSG:
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
			OpenOrCreate(hWnd);
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_COMMAND:
		id = LOWORD(wParam);
		switch (id) {
		case IDM_OPEN:
			OpenOrCreate(hWnd);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR) IDD_ABOUTBOX, hWnd, (DLGPROC) AboutDlgProc);
			break;
		default:
			if (id >= IDM_MOUNT_START && id < IDM_MOUNT_END) {
				boost::shared_ptr<Drive> drive = Drives::GetDrive(IDM_MOUNT_N(id));
				if (!drive)
					return 1;

				switch (IDM_MOUNT_TYPE(id)) {
				case IDM_TYPE_MOUNT:
					drive->Mount(hWnd);
					break;
				case IDM_TYPE_UMOUNT:
					drive->Umount(hWnd);
					break;
				case IDM_TYPE_SHOW:
					drive->Show(hWnd);
					break;
				}
			}
			break;
		}
		return 1;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

static INT_PTR CALLBACK
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

