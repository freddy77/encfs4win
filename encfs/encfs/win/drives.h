#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "config.hpp"
#include "subprocess.h"

enum {
	IDM_MOUNT_MAX = 256,
	IDM_MAX_TYPES = 4, // must be power of two

	IDM_TYPE_MOUNT = 0,
	IDM_TYPE_UMOUNT,
	IDM_TYPE_SHOW,

	IDM_EXIT = 2000,
	IDM_ABOUT,
	IDM_OPEN,
	IDM_PREFERENCES,
	IDM_MOUNT_START,
	IDM_MOUNT_END = IDM_MOUNT_START + IDM_MOUNT_MAX*IDM_MAX_TYPES
};

#define IDM_MOUNT_N(i) (((i)-IDM_MOUNT_START)/IDM_MAX_TYPES)
#define IDM_MOUNT_TYPE(i)  (((i)-IDM_MOUNT_START)%IDM_MAX_TYPES)
#define IDM_MOUNT_ID(n,type) ((n)*IDM_MAX_TYPES+(type)+IDM_MOUNT_START)


class Drives;

class Drive: public boost::enable_shared_from_this<Drive>
{
friend class Drives;
public:
	void Show(HWND hwnd);
	void Mount(HWND hwnd);
	void Umount(HWND hwnd);
	bool Mounted() const { return mounted; }
	void CheckMounted();
private:
	std::string configName;
	std::tstring dir;
	TCHAR mnt[4];
	bool mounted;
	boost::shared_ptr<SubProcessInformations> subProcess;
	Drive(const std::string& configName, const std::tstring& _dir, char drive, DWORD pid = 0);
	void Save();
	static boost::shared_ptr<Drive> Load(const std::string& name);
};

class Drives
{
friend class Drive;
public:
	static bool autoShow;
	typedef boost::shared_ptr<Drive> drive_t;
	static drive_t GetDrive(int n);
	static void Load();
	static void AddMenus(HMENU menu);
	static drive_t Add(const std::tstring& dir, char drive);
private:
	static void NewDrive(const std::string& name, void* param);
	static void AddMenus(HMENU menu, bool mounted, unsigned count, LPCTSTR fmt, LPCTSTR title, int type);
	static void Delete(drive_t);
};

