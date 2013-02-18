/*	$OpenBSD: readpassphrase.c,v 1.12 2001/12/15 05:41:00 millert Exp $	*/

/*
 * Copyright (c) 2000 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static const char rcsid[] = "$OpenBSD: readpassphrase.c,v 1.12 2001/12/15 05:41:00 millert Exp $";
#endif /* LIBC_SCCS and not lint */

//#include "includes.h"

#ifndef HAVE_READPASSPHRASE

#include <csignal>
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cctype>
#include <conio.h>
#include <windows.h>
#include <wincon.h>
#include <tchar.h>
#include <string>
#include <readpassphrase.h>

std::string wchar_to_utf8_cstr(const wchar_t *str);

char *
readpassphrase(const char *prompt, char *buf, size_t bufsiz, int flags)
{
        int save_errno, ch;
        char *p, *end;

        /* I suppose we could alloc on demand in this case (XXX). */
        if (bufsiz == 0) {
                errno = EINVAL;
                return NULL ;
        }

        printf("%s", prompt);
        fflush(stdout);

	/* try to get a real console */
	HANDLE in = CreateFile(_T("CONIN$"),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	while (in != INVALID_HANDLE_VALUE) {
		DWORD mode, readed;
		WCHAR wbuf[256];
		if (!GetConsoleMode(in, &mode)) break;
		SetConsoleMode(in, ENABLE_LINE_INPUT|ENABLE_PROCESSED_INPUT);
		if (!ReadConsoleW(in, wbuf, 255, &readed, NULL)) break;
		while (readed > 0 && (wbuf[readed-1] == '\r' || wbuf[readed-1] == '\n'))
			--readed;
		wbuf[readed] = 0;
		SetConsoleMode(in, mode);
		CloseHandle(in);
		strncpy(buf, wchar_to_utf8_cstr(wbuf).c_str(), bufsiz);
		buf[bufsiz-1] = 0;
		printf("\n");
		return buf;
	}
	CloseHandle(in);

        end = buf + bufsiz - 1;
        for (p = buf; (ch = _getch()) != EOF && ch != '\n' && ch != '\r';) {
                if (p < end) {
                        ch &= 0xff;
                        if ((flags & RPP_SEVENBIT))
                                ch &= 0x7f;
                        if (isalpha(ch)) {
                                if ((flags & RPP_FORCELOWER))
                                        ch = tolower(ch);
                                if ((flags & RPP_FORCEUPPER))
                                        ch = toupper(ch);
                        }
                        *p++ = ch;
                }
        }
        *p = '\0';
        save_errno = errno;
        printf("\n");
        errno = save_errno;
        return (ch == EOF ? NULL : buf);
}

#endif

