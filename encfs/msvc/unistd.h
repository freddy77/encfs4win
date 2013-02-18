#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <windows.h>
#define write fake_write
#include <io.h>
#undef write

#ifdef __cplusplus
static inline int write( int handle, const void *buffer, unsigned int count )
{
    return _write(handle, buffer, count );
}
#else
#define write _write
#endif

#define read _read
#define close _close


#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#endif //_UNISTD_H_
