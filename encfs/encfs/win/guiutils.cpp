#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <shlobj.h>
#include "guiutils.h"
#include "resource.h"

static int CALLBACK BrowseProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM /* lpData */)
{
	if (uMsg == BFFM_VALIDATEFAILED)
		return 1;

	if (uMsg == BFFM_SELCHANGED) {
		TCHAR path[MAX_PATH];
		BOOL res = SHGetPathFromIDList((LPITEMIDLIST) lParam, path);
		SendMessage(hwnd, BFFM_ENABLEOK, 0, res);
	}
	return 0;
}

std::string GetExistingDirectory(HWND hwnd)
{
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));

	bi.hwndOwner = hwnd;
	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_USENEWUI;
	bi.lpfn = BrowseProc;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	TCHAR path[MAX_PATH];
	if (!pidl || !SHGetPathFromIDList(pidl, path))
		return "";
	return path;
}

static char driveSelected;

static LRESULT CALLBACK
DriveDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /* lParam*/)
{
	int i;
	DWORD drives;
	char buf[16];

	switch (message) {
	case WM_INITDIALOG:
		drives = GetLogicalDrives();
		for (i = 2; i < 26; ++i) {
			if (drives & (1 << i))
				continue;
			sprintf(buf, "%c:", 'A' + i);
			SendDlgItemMessage(hDlg, IDC_CMBDRIVE, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) buf);
		}
		SendDlgItemMessage(hDlg, IDC_CMBDRIVE, CB_SETCURSEL, 0, 0);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			driveSelected = 0;
			i = SendDlgItemMessage(hDlg, IDC_CMBDRIVE, CB_GETCURSEL, 0, 0);
			if (i != CB_ERR) {
				buf[0] = 0;
				SendDlgItemMessage(hDlg, IDC_CMBDRIVE, CB_GETLBTEXT, i, (LPARAM) (LPTSTR) buf);
				driveSelected = buf[0];
			}
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

char SelectFreeDrive(HWND hwnd)
{
	DWORD drives = (GetLogicalDrives() ^ 0x3ffffffu) & ~3;
	if (!drives)
		return 0;

	if (DialogBox(hInst, (LPCTSTR) IDD_DRIVE, hwnd, (DLGPROC) DriveDlgProc) != IDOK)
		return 0;

	return driveSelected;
}
