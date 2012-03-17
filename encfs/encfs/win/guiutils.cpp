#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <shlobj.h>
#include "guiutils.h"
#include "resource.h"

static int CALLBACK BrowseProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED && lpData)
		SetWindowText(hwnd, (LPCTSTR) lpData);

	if (uMsg == BFFM_VALIDATEFAILED)
		return 1;

	if (uMsg == BFFM_SELCHANGED) {
		TCHAR path[MAX_PATH];
		BOOL res = SHGetPathFromIDList((LPITEMIDLIST) lParam, path);
		SendMessage(hwnd, BFFM_ENABLEOK, 0, res);
	}
	return 0;
}

std::tstring GetExistingDirectory(HWND hwnd, LPCTSTR title, LPCTSTR caption)
{
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));

	bi.hwndOwner = hwnd;
	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_USENEWUI;
	bi.lpfn = BrowseProc;
	bi.lpszTitle = title;
	bi.lParam = (LPARAM) caption;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	TCHAR path[MAX_PATH];
	if (!pidl || !SHGetPathFromIDList(pidl, path))
		return _T("");
	return path;
}

bool FillFreeDrive(HWND combo)
{
	bool res = false;
	DWORD drives = GetLogicalDrives();
	int i;

	for (i = 2; i < 26; ++i) {
		TCHAR buf[16];

		if (drives & (1 << i))
			continue;
		_stprintf(buf, _T("%c:"), 'A' + i);
		SendMessage(combo, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) buf);
		res = true;
	}
	SendMessage(combo, CB_SETCURSEL, 0, 0);
	return res;
}

static INT_PTR CALLBACK
DriveDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;
	TCHAR buf[16];
	char *pSelectedDrive = (char*) GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch (message) {
	case WM_INITDIALOG:
		pSelectedDrive = (char*) lParam;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		*pSelectedDrive = 0;
		FillFreeDrive(GetDlgItem(hDlg, IDC_CMBDRIVE));
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			*pSelectedDrive = 0;
			i = SendDlgItemMessage(hDlg, IDC_CMBDRIVE, CB_GETCURSEL, 0, 0);
			if (i != CB_ERR) {
				buf[0] = 0;
				SendDlgItemMessage(hDlg, IDC_CMBDRIVE, CB_GETLBTEXT, i, (LPARAM) (LPTSTR) buf);
				*pSelectedDrive = buf[0];
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

	char selectedDrive;
	if (DialogBoxParam(hInst, (LPCTSTR) IDD_DRIVE, hwnd, (DLGPROC) DriveDlgProc, (LPARAM) &selectedDrive) != IDOK)
		return 0;

	return selectedDrive;
}

struct PasswordData {
	TCHAR *buf;
	size_t len;
};

static INT_PTR CALLBACK
PasswordDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	PasswordData *pData = (PasswordData*) GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch (message) {
	case WM_INITDIALOG:
		pData = (PasswordData*) lParam;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pData->buf[0] = 0;
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		case IDOK:
			GetDlgItemText(hDlg, IDC_PASSWORD, pData->buf, pData->len);
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

bool GetPassword(HWND hwnd, TCHAR *pass, size_t len)
{
	PasswordData data = { pass, len };
	
	return DialogBoxParam(hInst, (LPCTSTR) IDD_PASSWORD, hwnd, (DLGPROC) PasswordDlgProc, (LPARAM) &data) == IDOK;
}
