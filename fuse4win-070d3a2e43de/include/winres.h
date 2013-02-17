/* winres.h Missing in MinGW. Adding missing constants */

#ifdef __GNUC__
#ifndef __WINRES_H
#define __WINRES_H

#ifdef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#define HOTKEY_CLASSA "msctls_hotkey32"

#ifdef _COMMCTRL_H
#error "Resource should include winres.h first!"
#endif

/* FIXME: It seems if we include richedit.h later we got a problem with MinGW + UNICODE */
#ifdef UNICODE
#undef UNICODE
#endif

#include <winresrc.h>

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#ifndef IDCLOSE
#define IDCLOSE 8
#endif

#endif
#endif

