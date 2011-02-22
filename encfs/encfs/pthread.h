#ifndef __PTHREAD_H
#define __PTHREAD_H

#include <windows.h>
#include <stdio.h>

typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;

void pthread_mutex_init(pthread_mutex_t *mtx, int );
void pthread_mutex_destroy(pthread_mutex_t *mtx);
void pthread_mutex_lock(pthread_mutex_t *mtx);
void pthread_mutex_unlock(pthread_mutex_t *mtx);

int pthread_create(pthread_t *thread, int, void *(*start_routine)(void*), void *arg);
void pthread_join(pthread_t thread, int);

int fsync(int fd);
int fdatasync(int fd);

ssize_t pread(int fd, void *buf, size_t count, __int64 offset);
ssize_t pwrite(int fd, const void *buf, size_t count, __int64 offset);

int truncate(const char *path, __int64 length);
int utimes(const char *filename, const struct timeval times[2]);
int my_stat (const char* fn, struct FUSE_STAT* st);
#define lstat my_stat
int my_open(const char *fn, int flags);
namespace pthread {
int mkdir(const char *fn, int mode);
}
using pthread::mkdir;

#define mlock(a,b) do { } while(0)
#define munlock(a,b) do { } while(0)


typedef struct
{
  int waiters_count_;
  // Count of the number of waiters.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.

  int release_count_;
  // Number of threads to release via a <pthread_cond_broadcast> or a
  // <pthread_cond_signal>. 
  
  int wait_generation_count_;
  // Keeps track of the current "generation" so that we don't allow
  // one thread to steal all the "releases" from the broadcast.

  HANDLE event_;
  // A manual-reset event that's used to block and release waiting
  // threads. 
} pthread_cond_t;

int pthread_cond_init (pthread_cond_t *cv, int);
int pthread_cond_destroy(pthread_cond_t *cv);
int pthread_cond_wait (pthread_cond_t *cv,
	pthread_mutex_t *external_mutex);
void pthread_cond_timedwait (pthread_cond_t *cv,
	pthread_mutex_t *external_mutex, const struct timespec *abstime);
int pthread_cond_signal (pthread_cond_t *cv);
int pthread_cond_broadcast (pthread_cond_t *cv);

#endif /* __PTHREAD_H */

