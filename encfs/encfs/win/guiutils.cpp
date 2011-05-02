#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include "guiutils.h"

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

