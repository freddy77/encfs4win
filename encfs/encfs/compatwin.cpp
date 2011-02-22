//#include <windows.h>
//#include <errno.h>
//#include <stdio.h>
//#include <io.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <fuse.h>

#include "pthread.h"

#include <errno.h>
#include <stdio.h>
#include <io.h>
#include <unistd.h>
#include <fcntl.h>
#include <fuse.h>

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

int fsync(int fd)
{
	FlushFileBuffers((HANDLE) _get_osfhandle(fd));
	return 0;
}

int fdatasync(int fd)
{
	FlushFileBuffers((HANDLE) _get_osfhandle(fd));
	return 0;
}

ssize_t pread(int fd, void *buf, size_t count, __int64 offset)
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
		errno = EIO;
		return -1;
	}
	return len;
}

ssize_t pwrite(int fd, const void *buf, size_t count, __int64 offset)
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

int truncate(const char *path, __int64 length)
{
	int fd = open(path, O_RDWR);

	if (fd < 0) return -1;

	LONG high = length >> 32;
	HANDLE h = (HANDLE) _get_osfhandle(fd);
	if (!SetFilePointer(h, length, &high, FILE_BEGIN)
	    || !SetEndOfFile(h) ) {
		int save_errno = win32_error_to_errno(GetLastError());
		close(fd);
		errno = save_errno;
		return -1;
	}
	close(fd);
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

int
utimes(const char *filename, const struct timeval times[2])
{
	struct _utimbuf tm;
	tm.actime  = times[0].tv_sec;
	tm.modtime = times[1].tv_sec;
	return _utime(filename, &tm);
}

int
my_stat (const char* fn, struct FUSE_STAT* st)
{
	char buf[512];
	_snprintf(buf, 512, "%s", fn);
	buf[511] = 0;

	for (char *p = buf; *p; ++p)
		if (*p == '/')
			*p = '\\';

	size_t l = strlen(buf);
	if (buf[l-1] == '\\')
		buf[l-1] = 0;

	return ::_stati64(buf, st);
}

int
my_open(const char *fn, int flags)
{
	HANDLE f = CreateFile(fn, flags == O_RDONLY ? GENERIC_WRITE : GENERIC_WRITE|GENERIC_READ, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		int save_errno = win32_error_to_errno(GetLastError());
		f = CreateFile(fn, flags == O_RDONLY ? GENERIC_WRITE : GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (f == INVALID_HANDLE_VALUE) {
			errno = save_errno;
			return -1;
		}
	}
	int fd = _open_osfhandle((intptr_t) f, flags);
	if (fd < 0) {
		errno = ENOENT;
		CloseHandle(f);
		return -1;
	}
	return fd;
}

namespace pthread {

int mkdir(const char *fn, int mode)
{
	return ::mkdir(fn); 
}

}

