#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdexcept>
#include "drives.h"
#include "guiutils.h"
#include "FileUtils.h"
#include "fuse.h"

static HANDLE GetOldSubProject(DWORD pid);

Drive::Drive(const std::string& _configName, const std::tstring& _dir, char drive, DWORD pid):
	configName(_configName), dir(_dir), mounted(false)
{
	_stprintf(mnt, _T("%c:\\"), drive);

	HANDLE hProcess = GetOldSubProject(pid);
	if (hProcess) {
		subProcess.reset(new SubProcessInformations);
		subProcess->pid = pid;
		subProcess->hProcess = hProcess;
		mounted = true;
	}
}

void Drive::Show(HWND hwnd)
{
	ShellExecute(hwnd, _T("open"), mnt, NULL, NULL, SW_SHOWNORMAL);
}

void Drive::Mount(HWND hwnd)
{
	// check drive empty or require a new drive
	while (GetDriveType(mnt) != DRIVE_NO_ROOT_DIR) {
		char drive = SelectFreeDrive(hwnd);
		if (!drive)
			return;
		_stprintf(mnt, _T("%c:\\"), drive);
		Save();
	}

	// check directory existence
	if (!isDirectory(wchar_to_utf8_cstr(dir.c_str()).c_str())) {
		if (YesNo(hwnd, _T("Directory does not exists. Remove from list?")))
			Drives::Delete(shared_from_this());
		return;
	}

	// TODO check configuration still exists ?? ... no can cause recursion problem

	// search if executable is present
	TCHAR executable[MAX_PATH];
	if (!SearchPath(NULL, _T("encfs.exe"), NULL, LENGTH(executable), executable, NULL))
		throw truntime_error(_T("Unable to find encfs.exe file"));

	// ask a password to mount
	TCHAR pass[128+2];
	if (!GetPassword(hwnd, pass, LENGTH(pass)-2))
		return;
	_tcscat(pass, _T("\r\n"));

	// mount using a sort of popen
	TCHAR cmd[2048];
	_sntprintf(cmd, LENGTH(cmd), _T("\"%s\" -S \"%s\" %c:"), executable, dir.c_str(), mnt[0]);
	boost::shared_ptr<SubProcessInformations> proc(new SubProcessInformations);
	proc->creationFlags = CREATE_NEW_PROCESS_GROUP|CREATE_NO_WINDOW;
	if (!CreateSubProcess(cmd, proc.get())) {
		DWORD err = GetLastError();
		memset(pass, 0, sizeof(pass));
		_sntprintf(cmd, LENGTH(cmd), _T("Error: %s (%u)"), proc->errorPart, (unsigned) err);
		throw truntime_error(cmd);
	}
	subProcess = proc;

	// send the password
	std::string pwd = wchar_to_utf8_cstr(pass);
	DWORD written;
	WriteFile(proc->hIn, pwd.c_str(), pwd.length(), &written, NULL);
	CloseHandle(proc->hIn);	// close input so sub process does not any more
	proc->hIn = NULL;
	memset(pass, 0, sizeof(pass));
	memset((char*) pwd.c_str(), 0, pwd.length());

	mounted = false;

	// wait for mount, read error and give feedback
	for (unsigned n = 0; n < 5*10; ++n) {
		// drive appeared
		if (GetDriveType(mnt) != DRIVE_NO_ROOT_DIR) {
			if (Drives::autoShow)
				Show(hwnd);
			break;
		}

		// process terminated
		DWORD readed;
		char output[2048];
		switch (WaitForSingleObject(subProcess->hProcess, 200)) {
		case WAIT_OBJECT_0:
		case WAIT_ABANDONED:
			if (ReadFile(proc->hOut, output, sizeof(output)-1, &readed, NULL)) {
				output[readed] = 0;
				utf8_to_wchar_buf(output, cmd, LENGTH(cmd));
			} else {
				_stprintf(cmd, _T("Unknown error mounting drive %c:"), mnt[0]);
			}
			subProcess.reset();
			throw truntime_error(cmd);
		}
	}
	if (subProcess)
		mounted = true;
	Save(); // save for resume
}

extern "C" BOOL WINAPI AttachConsole(DWORD);

static BOOL WINAPI HandlerRoutine(DWORD)
{
	return TRUE;
}

void Drive::Umount(HWND)
{
	// check mounted
	CheckMounted();
//	if (GetDriveType(mnt) == DRIVE_NO_ROOT_DIR)
//		mounted = false;
	if (!mounted)
		throw truntime_error(_T("Cannot unmount a not mounted drive"));

	// unmount
	fuse_unmount(wchar_to_utf8_cstr(mnt).c_str(), NULL);

	if (subProcess) {
		// attach console to allow sending ctrl-c
		AttachConsole(subProcess->pid);

		// disable ctrl-c to not exit this process
		SetConsoleCtrlHandler(HandlerRoutine, TRUE);

		if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, subProcess->pid)
		    && !GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, subProcess->pid))
			TerminateProcess(subProcess->hProcess, 0);

		// force exit
		if (WaitForSingleObject(subProcess->hProcess, 2000) == WAIT_TIMEOUT)
			TerminateProcess(subProcess->hProcess, 0);
			
		// close the console
		FreeConsole();
		SetConsoleCtrlHandler(HandlerRoutine, FALSE);
	}
	CheckMounted();
}

void Drive::CheckMounted()
{
	if (!mounted)
		return;

	if (!subProcess) {
		mounted = false;
		return;
	}

	switch (WaitForSingleObject(subProcess->hProcess, 0)) {
	case WAIT_OBJECT_0:
	case WAIT_ABANDONED:
		subProcess.reset();
		mounted = false;
		Save();
	}
}


void Drive::Save()
{
	Config::Save(configName, _T("Directory"), dir);
	Config::Save(configName, _T("Drive"), mnt);

	// save pid for resume
	TCHAR buf[16];
	buf[0] = 0;
	if (subProcess)
		_stprintf(buf, _T("%u"), (unsigned) subProcess->pid);
	Config::Save(configName, _T("Pid"), buf);
}

static HANDLE GetOldSubProject(DWORD pid)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (!snap)
		return NULL;

	MODULEENTRY32 module;
	memset(&module, 0, sizeof(module));
	module.dwSize = sizeof(module);
	if (!Module32First(snap, &module)) {
		CloseHandle(snap);
		return NULL;
	}

	CharLower(module.szExePath);
	TCHAR *p = _tcsstr(module.szExePath, _T("encfs.exe"));
	CloseHandle(snap);
	if (p && (p == module.szExePath || p[-1] == '\\' || p[-1] == '\"'))
		return OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION|PROCESS_TERMINATE, FALSE, pid);

	return NULL;
}

boost::shared_ptr<Drive> Drive::Load(const std::string& name)
{
	boost::shared_ptr<Drive> ret;
	std::tstring dir, drive, pid;
	try {
		dir = Config::Load(name, _T("Directory"));
		drive = Config::Load(name, _T("Drive"));
		pid = Config::Load(name, _T("Pid"));
	}
	catch(const truntime_error&) {
		// ignore
	}

	// check values readed
	char c = 0;
	if (!drive.empty()) c = toupper(drive[0]);
	if (c < 'C' || c > 'Z') {
		Config::Delete(name);
		return ret;
	}

	// handle resume
	unsigned n = _tcstoul(pid.c_str(), NULL, 10);
	TCHAR mnt[4];
	_stprintf(mnt, _T("%c:\\"), c);
	if (n <= 0 || GetDriveType(mnt) == DRIVE_NO_ROOT_DIR)
		n = 0;

	ret.reset(new Drive(name, dir, c, n));
	return ret;
}

// DRIVES

typedef std::vector<Drives::drive_t> drives_t;

static drives_t drives;
bool Drives::autoShow = true;

Drives::drive_t Drives::GetDrive(int n)
{
	if (n < 0 || (unsigned) n >= drives.size())
		return boost::shared_ptr<Drive>();
	return drives[n];
}

void
Drives::NewDrive(const std::string& name, void *)
{
	drives.push_back(Drive::Load(name));
}

void Drives::Load()
{
	drives.resize(0);
	Config::Enum(NewDrive, NULL);
}

void Drives::AddMenus(HMENU menu, bool mounted, unsigned count, LPCTSTR fmt, LPCTSTR title, int type)
{
	if (!count)
		return;

	HMENU addMenu = menu;
	if (count > 2)
		addMenu = CreatePopupMenu();

	unsigned n = 0;
	for (drives_t::const_iterator i = drives.begin(); i != drives.end(); ++i, ++n) {
		if (((*i)->mounted && !mounted) || (!(*i)->mounted && mounted))
			continue;

		TCHAR buf[512];
		_sntprintf(buf, LENGTH(buf), count > 2 ? _T("%s (%c)") : fmt, (*i)->dir.c_str(), (*i)->mnt[0]);
		AppendMenu(addMenu, 0, IDM_MOUNT_ID(n, type), buf);
	}

	if (count > 2)
		AppendMenu(menu, MF_POPUP, (UINT_PTR) addMenu, title);
	else
		AppendMenu(menu, MF_MENUBREAK, -1, NULL);
}

void Drives::AddMenus(HMENU menu)
{
	// check all drives, delete closed processed and so on
	for (drives_t::iterator i = drives.begin(); i != drives.end(); ++i)
		(*i)->CheckMounted();

	// counts
	unsigned numMounted = 0, numUnmounted = 0;
	for (drives_t::const_iterator i = drives.begin(); i != drives.end(); ++i) {
		if ((*i)->mounted)
			++numMounted;
		else
			++numUnmounted;
	}

	AddMenus(menu, true, numMounted, _T("Open %s (%c)"), _T("Open"), IDM_TYPE_SHOW);
	AddMenus(menu, false, numUnmounted, _T("Mount %s (%c)"), _T("Mount"), IDM_TYPE_MOUNT);
	AddMenus(menu, true, numMounted, _T("Unmount %s (%c)"), _T("Unmount"), IDM_TYPE_UMOUNT);
}

void Drives::Delete(drive_t drive)
{
	drives.erase(std::remove(drives.begin(), drives.end(), drive), drives.end());
	// delete from configuration!
	Config::Delete(drive->configName);
}

Drives::drive_t Drives::Add(const std::tstring& dir, char drive)
{
	Drives::drive_t ret;
	// we do not wants duplicate!
	for (drives_t::iterator i = drives.begin(); ; ++i) {
		if (i == drives.end()) {
			ret = Drives::drive_t(new Drive(Config::NewName(), dir, drive));
			drives.push_back(ret);
			break;
		}
		if ((*i)->dir == dir) {
			// replace if same not mounted
			if (!(*i)->Mounted()) {
				ret = Drives::drive_t(new Drive((*i)->configName, dir, drive));
				*i = ret;
			}
			break;
		}
	}
	if (ret)
		ret->Save();
	return ret;
}

