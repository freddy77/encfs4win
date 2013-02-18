#include <windows.h>
#include <time.h>
#include <errno.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS) || defined(__WATCOMC__)
# define DELTA_EPOCH_IN_USEC  11644473600000000Ui64
#else
# define DELTA_EPOCH_IN_USEC  11644473600000000ULL
#endif

int gettimeofday (struct timeval *tv, void *tz)
{
        FILETIME  ft;
        unsigned __int64 tim;
        unsigned long r;

        if (!tv) {
                errno = EINVAL;
                return -1;
        }
        /*
         * Although this function returns 10^-7 precision the real
         * precision is less than milliseconds on Windows XP
         */
        GetSystemTimeAsFileTime (&ft);
        tim = ((((unsigned __int64) ft.dwHighDateTime) << 32) | ft.dwLowDateTime) -
              (DELTA_EPOCH_IN_USEC * 10U);
        /*
         * here we use same division to compute quotient
         * and remainder at the same time (gcc)
         */
        tv->tv_sec  = (long) (tim / 10000000UL);
        r = tim % 10000000UL;
        tv->tv_usec = (long) (r / 10L);
        return 0;
}
