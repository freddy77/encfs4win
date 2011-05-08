#include <windows.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdexcept>
#include "drives.h"
#include "guiutils.h"
#include "FileUtils.h"
#include "fuse.h"

Drive::Drive(const std::string& _configName, const std::string& _dir, char drive):
	configName(_configName), dir(_dir), mounted(false)
{
	sprintf(mnt, "%c:\\", drive);
}

void Drive::Show(HWND hwnd)
{
	ShellExecute(hwnd, "open", mnt, NULL, NULL, SW_SHOWNORMAL);
}

void Drive::Mount(HWND hwnd)
{
	// check drive empty or require a new drive
	while (GetDriveType(mnt) != DRIVE_NO_ROOT_DIR) {
		char drive = SelectFreeDrive(hwnd);
		if (!drive)
			return;
		sprintf(mnt, "%c:\\", drive);
		Save();
	}

	// check directory existence
	if (!isDirectory(dir.c_str())) {
		if (YesNo(hwnd, "Directory does not exists. Remove from list?"))
			Drives::Delete(shared_from_this());
		return;
	}

	// TODO check configuration still exists ?? ... no can cause recursion problem

	// search if executable is present
	char executable[MAX_PATH];
	if (!SearchPath(NULL, "encfs.exe", NULL, sizeof(executable), executable, NULL))
		throw std::runtime_error("Unable to find encfs.exe file");

	// ask a password to mount
	char pass[128+2];
	if (!GetPassword(hwnd, pass, sizeof(pass)-2))
		return;
	strcat(pass, "\r\n");

	// mount using a sort of popen
	char cmd[2048];
	_snprintf(cmd, sizeof(cmd), "\"%s\" -S \"%s\" %c:", executable, dir.c_str(), mnt[0]);
	boost::shared_ptr<SubProcessInformations> proc(new SubProcessInformations);
	proc->creationFlags = CREATE_NO_WINDOW;
	if (!CreateSubProcess(cmd, proc.get())) {
		DWORD err = GetLastError();
		memset(pass, 0, sizeof(pass));
		_snprintf(cmd, sizeof(cmd), "Error: %s (%u)", proc->errorPart, (unsigned) err);
		throw std::runtime_error(cmd);
	}
	subProcess = proc;

	// send the password
	DWORD written;
	WriteFile(proc->hIn, pass, strlen(pass), &written, NULL);
	CloseHandle(proc->hIn);	// close input so sub process does not any more
	proc->hIn = NULL;
	memset(pass, 0, sizeof(pass));

	mounted = false;

	// wait for mount, read error and give feedback
	for (unsigned n = 0; n < 5*10; ++n) {
		// drive appeared
		if (GetDriveType(mnt) != DRIVE_NO_ROOT_DIR) {
			Show(hwnd);
			break;
		}

		// process terminated
		DWORD readed;
		switch (WaitForSingleObject(subProcess->hProcess, 200)) {
		case WAIT_OBJECT_0:
		case WAIT_ABANDONED:
			if (ReadFile(proc->hOut, cmd, sizeof(cmd)-1, &readed, NULL)) {
				cmd[readed] = 0;
			} else {
				sprintf(cmd, "Unknown error mounting drive %c:", mnt[0]);
			}
			subProcess.reset();
			throw std::runtime_error(cmd);
		}
	}
	if (subProcess)
		mounted = true;
	Save(); // save for resume
}

void Drive::Umount(HWND)
{
	// check mounted
	CheckMounted();
//	if (GetDriveType(mnt) == DRIVE_NO_ROOT_DIR)
//		mounted = false;
	if (!mounted)
		throw std::runtime_error("Cannot unmount a not mounted drive");

	// unmount
	fuse_unmount(mnt, NULL);

	if (subProcess) {
		GenerateConsoleCtrlEvent(CTRL_C_EVENT, subProcess->pid);
		GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, subProcess->pid);
		WaitForSingleObject(subProcess->hProcess, 1000);
		TerminateProcess(subProcess->hProcess, 0);
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
	Config::Save(configName, "Directory", dir);
	Config::Save(configName, "Drive", mnt);

	// save pid for resume
	char buf[16];
	buf[0] = 0;
	if (subProcess)
		sprintf(buf, "%u", (unsigned) subProcess->pid);
	Config::Save(configName, "Pid", buf);
}

boost::shared_ptr<Drive> Drive::Load(const std::string& name)
{
	boost::shared_ptr<Drive> ret;
	std::string dir, drive, pid;
	try {
		dir = Config::Load(name, "Directory");
		drive = Config::Load(name, "Drive");
		pid = Config::Load(name, "Pid");
	}
	catch(const std::runtime_error&) {
		// ignore
	}

	// check values readed
	char c = 0;
	if (!drive.empty()) c = toupper(drive[0]);
	if (c < 'C' || c > 'Z') {
		Config::Delete(name);
		return ret;
	}
	// TODO handle resume
	ret = boost::shared_ptr<Drive>(new Drive(name, dir, c));
	return ret;
}

// DRIVES

typedef std::vector<Drives::drive_t> drives_t;

static drives_t drives;

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

void Drives::AddMenus(HMENU menu, bool mounted, unsigned count, const char *fmt, const char *title, int type)
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

		char buf[512];
		_snprintf(buf, sizeof(buf), count > 2 ? "%s (%c)" : fmt, (*i)->dir.c_str(), (*i)->mnt[0]);
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

	AddMenus(menu, true, numMounted, "Open %s (%c)", "Open", IDM_TYPE_SHOW);
	AddMenus(menu, false, numUnmounted, "Mount %s (%c)", "Mount", IDM_TYPE_MOUNT);
	AddMenus(menu, true, numMounted, "Unmount %s (%c)", "Unmount", IDM_TYPE_UMOUNT);
}

void Drives::Delete(drive_t drive)
{
	drives.erase(std::remove(drives.begin(), drives.end(), drive), drives.end());
	// delete from configuration!
	Config::Delete(drive->configName);
}

Drives::drive_t Drives::Add(const std::string& dir, char drive)
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

