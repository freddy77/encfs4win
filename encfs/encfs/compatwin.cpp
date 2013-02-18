//#include <windows.h>
//#include <errno.h>
//#include <stdio.h>
//#include <io.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <fuse.h>

#include "encfs.h"
#include "pthread.h"

#include <errno.h>
#include <stdio.h>
#include <io.h>
#include <unistd.h>
#include <fcntl.h>
#include <fuse.h>
#include <winioctl.h>
#include <direct.h>
#include <boost/scoped_array.hpp>

time_t filetimeToUnixTime(const FILETIME *ft);

void pthread_mutex_init(pthread_mutex_t *mtx, int )
{
	InitializeCriticalSection(mtx);
}

void pthread_mutex_destroy(pthread_mutex_t *mtx)
{
	DeleteCriticalSection(mtx);
}

void pthread_mutex_lock(pthread_mutex_t *mtx)
{
	EnterCriticalSection(mtx);
}

void pthread_mutex_unlock(pthread_mutex_t *mtx)
{
	LeaveCriticalSection(mtx);
}

int pthread_create(pthread_t *thread, int, void *(*start_routine)(void*), void *arg)
{
	errno = 0;
	DWORD dwId;
	HANDLE res = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) start_routine, arg, 0, &dwId);
	if (!res)
		return ENOMEM;
	*thread = res;
	return 0;
}

void pthread_join(pthread_t thread, int)
{
	WaitForSingleObject(thread, INFINITE);
}

int unix::fsync(int fd)
{
	FlushFileBuffers((HANDLE) _get_osfhandle(fd));
	return 0;
}

int unix::fdatasync(int fd)
{
	FlushFileBuffers((HANDLE) _get_osfhandle(fd));
	return 0;
}

ssize_t unix::pread(int fd, void *buf, size_t count, __int64 offset)
{
	HANDLE h = (HANDLE) _get_osfhandle(fd);
	if (h == INVALID_HANDLE_VALUE) {
		errno = EINVAL;
		return -1;
	}
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	DWORD len;
	ov.Offset = (DWORD) offset;
	ov.OffsetHigh = (DWORD) (offset>>32);
	if (!ReadFile(h, buf, count, &len, &ov)) {
		if (GetLastError() == ERROR_HANDLE_EOF)
			return 0;
		errno = EIO;
		return -1;
	}
	return len;
}

ssize_t unix::pwrite(int fd, const void *buf, size_t count, __int64 offset)
{
	HANDLE h = (HANDLE) _get_osfhandle(fd);
	if (h == INVALID_HANDLE_VALUE) {
		errno = EINVAL;
		return -1;
	}
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	DWORD len;
	ov.Offset = (DWORD) offset;
	ov.OffsetHigh = (DWORD) (offset>>32);
	if (!WriteFile(h, buf, count, &len, &ov)) {
		errno = EIO;
		return -1;
	}
	return len;
}

int unix::ftruncate(int fd, __int64 length) {
	return ::ftruncate64(fd, length);
}

int unix::truncate(const char *path, __int64 length)
{
	std::wstring fn(utf8_to_wfn(path));
	HANDLE fd = CreateFileW(fn.c_str(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

	if (fd == INVALID_HANDLE_VALUE)
		fd = CreateFileW(fn.c_str(), GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (fd == INVALID_HANDLE_VALUE) {
		errno = win32_error_to_errno(GetLastError());
		return -1;
	}

	LONG high = length >> 32;
	if (!SetFilePointer(fd, length, &high, FILE_BEGIN)
	    || !SetEndOfFile(fd) ) {
		int save_errno = win32_error_to_errno(GetLastError());
		CloseHandle(fd);
		errno = save_errno;
		return -1;
	}
	CloseHandle(fd);
	return 0;
}

int 
pthread_cond_init (pthread_cond_t *cv, int)
{
  cv->waiters_count_ = 0;
  cv->wait_generation_count_ = 0;
  cv->release_count_ = 0;

  InitializeCriticalSection(&cv->waiters_count_lock_);
  // Create a manual-reset event.
  cv->event_ = CreateEvent (NULL,  // no security
                            TRUE,  // manual-reset
                            FALSE, // non-signaled initially
                            NULL); // unnamed
  return 0;
}

int
pthread_cond_destroy(pthread_cond_t *cv)
{
	if (cv) {
		CloseHandle(cv->event_);
		cv->event_ = NULL;
		DeleteCriticalSection(&cv->waiters_count_lock_);
	}
	return 0;
}

int
pthread_cond_wait (pthread_cond_t *cv,
                   pthread_mutex_t *external_mutex)
{
  // Avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);

  // Increment count of waiters.
  cv->waiters_count_++;

  // Store current generation in our activation record.
  int my_generation = cv->wait_generation_count_;

  LeaveCriticalSection (&cv->waiters_count_lock_);
  LeaveCriticalSection (external_mutex);

  for (;;) {
    // Wait until the event is signaled.
    WaitForSingleObject (cv->event_, INFINITE);

    EnterCriticalSection (&cv->waiters_count_lock_);
    // Exit the loop when the <cv->event_> is signaled and
    // there are still waiting threads from this <wait_generation>
    // that haven't been released from this wait yet.
    int wait_done = cv->release_count_ > 0
                    && cv->wait_generation_count_ != my_generation;
    LeaveCriticalSection (&cv->waiters_count_lock_);

    if (wait_done)
      break;
  }

  EnterCriticalSection (external_mutex);
  EnterCriticalSection (&cv->waiters_count_lock_);
  cv->waiters_count_--;
  cv->release_count_--;
  int last_waiter = cv->release_count_ == 0;
  LeaveCriticalSection (&cv->waiters_count_lock_);

  if (last_waiter)
    // We're the last waiter to be notified, so reset the manual event.
    ResetEvent (cv->event_);
  return 0;
}

void
pthread_cond_timedwait (pthread_cond_t *cv,
                   pthread_mutex_t *external_mutex, const struct timespec *abstime)
{
  // Avoid race conditions.
  EnterCriticalSection (&cv->waiters_count_lock_);

  // Increment count of waiters.
  cv->waiters_count_++;

  // Store current generation in our activation record.
  int my_generation = cv->wait_generation_count_;

  LeaveCriticalSection (&cv->waiters_count_lock_);
  LeaveCriticalSection (external_mutex);

  DWORD start = GetTickCount();
  DWORD timeout = 0;
  if (abstime)
    timeout = abstime->tv_sec * 1000 + abstime->tv_nsec / 1000000;

  for (;;) {
    // Wait until the event is signaled.
    WaitForSingleObject (cv->event_, INFINITE);

    EnterCriticalSection (&cv->waiters_count_lock_);
    // Exit the loop when the <cv->event_> is signaled and
    // there are still waiting threads from this <wait_generation>
    // that haven't been released from this wait yet.
    int wait_done = cv->release_count_ > 0
                    && cv->wait_generation_count_ != my_generation;
    LeaveCriticalSection (&cv->waiters_count_lock_);

    if (wait_done)
      break;
    if (abstime && ((DWORD) (GetTickCount() - start)) > timeout)
      break;
  }

  EnterCriticalSection (external_mutex);
  EnterCriticalSection (&cv->waiters_count_lock_);
  cv->waiters_count_--;
  cv->release_count_--;
  int last_waiter = cv->release_count_ == 0;
  LeaveCriticalSection (&cv->waiters_count_lock_);

  if (last_waiter)
    // We're the last waiter to be notified, so reset the manual event.
    ResetEvent (cv->event_);
}

int
pthread_cond_signal (pthread_cond_t *cv)
{
  EnterCriticalSection (&cv->waiters_count_lock_);
  if (cv->waiters_count_ > cv->release_count_) {
    SetEvent (cv->event_); // Signal the manual-reset event.
    cv->release_count_++;
    cv->wait_generation_count_++;
  }
  LeaveCriticalSection (&cv->waiters_count_lock_);
  return 0;
}

int
pthread_cond_broadcast (pthread_cond_t *cv)
{
  EnterCriticalSection (&cv->waiters_count_lock_);
  if (cv->waiters_count_ > 0) {  
    SetEvent (cv->event_);
    // Release all the threads in this generation.
    cv->release_count_ = cv->waiters_count_;

    // Start a new generation.
    cv->wait_generation_count_++;
  }
  LeaveCriticalSection (&cv->waiters_count_lock_);
  return 0;
}

#include <sys/utime.h>

static FILETIME
timevalToFiletime(struct timeval t)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t.tv_sec, 10000000) + 116444736000000000LL + 10u * t.tv_usec;
	FILETIME res;
	res.dwLowDateTime = (DWORD)ll;
	res.dwHighDateTime = (DWORD)(ll >> 32);
	return res;
}

int
unix::utimes(const char *filename, const struct timeval times[2])
{
	std::wstring fn(utf8_to_wfn(filename));
	HANDLE h = CreateFileW(fn.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
		h = CreateFileW(fn.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		errno = win32_error_to_errno(GetLastError());
		return -1;
	}
	FILETIME fta = timevalToFiletime(times[0]);
	FILETIME ftm = timevalToFiletime(times[1]);
	BOOL res = SetFileTime(h, NULL, &fta, &ftm);
	DWORD win_err = GetLastError();
	CloseHandle(h);
	if (!res) {
		errno = win32_error_to_errno(win_err);
		return -1;
	}
	return 0;
}

int
unix::statvfs(const char *path, struct statvfs *fs)
{
	fs->f_bsize = 4096;
	fs->f_frsize = 4096;
	fs->f_fsid = 0;
	fs->f_flag = 0;
	fs->f_namemax = 255;
	fs->f_files = -1;
	fs->f_ffree = -1;
	fs->f_favail = -1;

	ULARGE_INTEGER avail, free_bytes, bytes;
	if (!GetDiskFreeSpaceExA(path, &avail, &bytes, &free_bytes)) {
		errno = win32_error_to_errno(GetLastError());
		return -1;
	}

	fs->f_bavail = avail.QuadPart / fs->f_bsize;
	fs->f_bfree  = free_bytes.QuadPart / fs->f_bsize;
	fs->f_blocks = bytes.QuadPart / fs->f_bsize;

	errno = 0;
	return 0;
}

static int
set_sparse(HANDLE fd)
{
	DWORD returned;
	return (int) DeviceIoControl(fd, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &returned, NULL);
}

int
my_open(const char *fn_utf8, int flags)
{
	std::wstring fn = utf8_to_wfn(fn_utf8);
	HANDLE f = CreateFileW(fn.c_str(), flags == O_RDONLY ? GENERIC_WRITE : GENERIC_WRITE|GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		int save_errno = win32_error_to_errno(GetLastError());
		f = CreateFileW(fn.c_str(), flags == O_RDONLY ? GENERIC_WRITE : GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (f == INVALID_HANDLE_VALUE) {
			errno = save_errno;
			return -1;
		}
	}
	set_sparse(f);

	int fd = _open_osfhandle((intptr_t) f, flags);
	if (fd < 0) {
		errno = ENOENT;
		CloseHandle(f);
		return -1;
	}
	return fd;
}

int
unix::open(const char *fn, int flags, ...)
{
	int mode = 0;
	va_list ap;
	va_start(ap, flags);
	if (flags & O_CREAT)
		mode = va_arg(ap, int);
	va_end(ap);
	return _wopen(utf8_to_wfn(fn).c_str(), flags, mode);
}

int
unix::utime(const char *filename, struct utimbuf *times)
{
	if (!times)
		return unix::utimes(filename, NULL);
	
	struct timeval tm[2];
	tm[0].tv_sec = times->actime;
	tm[0].tv_usec = 0;
	tm[1].tv_sec = times->modtime;
	tm[1].tv_usec = 0;
	return unix::utimes(filename, tm);
}

int
unix::mkdir(const char *fn, int mode)
{
	if (CreateDirectoryW(utf8_to_wfn(fn).c_str(), NULL))
		return 0;
	errno = win32_error_to_errno(GetLastError());
	return -1;
}

int
unix::rename(const char *oldpath, const char *newpath)
{
	if (MoveFileW(utf8_to_wfn(oldpath).c_str(), utf8_to_wfn(newpath).c_str()))
		return 0;
	errno = win32_error_to_errno(GetLastError());
	return -1;
}

int
unix::unlink(const char *path)
{
	if (DeleteFileW(utf8_to_wfn(path).c_str()))
		return 0;
	errno = win32_error_to_errno(GetLastError());
	return -1;
}

int
unix::rmdir(const char *path)
{
	if (RemoveDirectoryW(utf8_to_wfn(path).c_str()))
		return 0;
	errno = win32_error_to_errno(GetLastError());
	return -1;
}

int
unix::stat(const char *path, struct _stati64 *buffer)
{
	std::wstring fn = utf8_to_wfn(path).c_str();
	if (fn.length() && fn[fn.length()-1] == L'\\')
		fn.resize(fn.length()-1);
	if (strpbrk(path, "?*") != NULL) {
		errno = ENOENT;
		return -1;
	}

	WIN32_FIND_DATAW wfd;
	HANDLE hff = FindFirstFileW(fn.c_str(), &wfd);
	if (hff == INVALID_HANDLE_VALUE) {
		errno = win32_error_to_errno(GetLastError());
		return -1;
	}
	FindClose(hff);

	int drive;
	if (path[1] == ':')
		drive = tolower(path[0]) - 'a';
	else
		drive = _getdrive() - 1;
	
	unsigned mode;
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		mode = _S_IFDIR|0777;
	else
		mode = _S_IFREG|0666;
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		mode &= ~0222;
	// TODO executable ?? link ??

	buffer->st_dev = buffer->st_rdev = drive;
	buffer->st_ino = 0;
	buffer->st_mode = mode;
	buffer->st_nlink = 1;
	buffer->st_uid = 0;
	buffer->st_gid = 0;
	buffer->st_size = wfd.nFileSizeHigh * (((uint64_t)1)<<32) + wfd.nFileSizeLow;
	buffer->st_atime = filetimeToUnixTime(&wfd.ftLastAccessTime);
	buffer->st_mtime = filetimeToUnixTime(&wfd.ftLastWriteTime);
	buffer->st_ctime = filetimeToUnixTime(&wfd.ftCreationTime);

	return 0;
}

int
unix::chmod(const char* path, int mode)
{
	return _wchmod(utf8_to_wfn(path).c_str(), mode);
}

struct unix::DIR
{
	HANDLE hff;
	struct dirent ent;
	WIN32_FIND_DATAW wfd;
	int pos;
};

unix::DIR*
unix::opendir(const char *name)
{
	unix::DIR *dir = (unix::DIR*) malloc(sizeof(unix::DIR));
	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}
	memset(dir, 0, sizeof(*dir));
	std::wstring path = utf8_to_wfn(name);
	if(path.length() > 0 && path[path.length()-1] == L'\\')
		path += L"*";
	else
		path += L"\\*";
	dir->hff = FindFirstFileW(path.c_str(), &dir->wfd);
	if (dir->hff == INVALID_HANDLE_VALUE) {
		errno = win32_error_to_errno(GetLastError());
		free(dir);
		return NULL;
	}
	return dir;
}

int
unix::closedir(unix::DIR* dir)
{
	errno = 0;
	if (dir && dir->hff != INVALID_HANDLE_VALUE)
		FindClose(dir->hff);
	free(dir);
	return 0;
}

void utf8_to_wchar_buf(const char *src, wchar_t *res, int maxlen);
std::string wchar_to_utf8_cstr(const wchar_t *str);

struct dirent*
unix::readdir(unix::DIR* dir)
{
	errno = EBADF;
	if (!dir) return NULL;
	errno = 0;
	if (dir->pos < 0) return NULL;
	if (dir->pos == 0) {
		++dir->pos;
	} else if (!FindNextFileW(dir->hff, &dir->wfd)) {
		errno = GetLastError() == ERROR_NO_MORE_FILES ? 0 : win32_error_to_errno(GetLastError());
		return NULL;
	}
	std::string path = wchar_to_utf8_cstr(dir->wfd.cFileName);
	strncpy(dir->ent.d_name, path.c_str(), sizeof(dir->ent.d_name));
	dir->ent.d_name[sizeof(dir->ent.d_name)-1] = 0;
	dir->ent.d_namlen = strlen(dir->ent.d_name);
	return &dir->ent;
}

std::wstring
utf8_to_wfn(const std::string& src)
{
	int len = src.length()+1;
	const size_t addSpace = 6;
	boost::scoped_array<wchar_t> buf(new wchar_t[len+addSpace]);
	utf8_to_wchar_buf(src.c_str(), buf.get()+addSpace, len);
	for (wchar_t *p = buf.get()+addSpace; *p; ++p)
		if (*p == L'/')
			*p = L'\\';
	char drive = tolower(buf[addSpace]);
	if (drive >= 'a' && drive <= 'z' && buf[addSpace+1] == ':') {
		memcpy(buf.get()+(addSpace-4), L"\\\\?\\", 4*sizeof(wchar_t));
		return buf.get()+(addSpace-4);
	} else if (buf[addSpace] == L'\\' && buf[addSpace+1] == L'\\') {
		memcpy(buf.get()+(addSpace-6), L"\\\\?\\UNC", 7*sizeof(wchar_t));
		return buf.get()+(addSpace-6);
	}
	return buf.get()+addSpace;
}

