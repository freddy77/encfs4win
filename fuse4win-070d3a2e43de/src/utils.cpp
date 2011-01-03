#include <windows.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"

char* wchar_to_utf8(const wchar_t* str)
{
	if (str==NULL)
		return NULL;
	
	//Determine required length
	int ln=WideCharToMultiByte(CP_ACP,0,str,-1,NULL,0,NULL,NULL);
	char *res=(char *)malloc(sizeof(char)*ln);

	//Convert to Unicode
	WideCharToMultiByte(CP_ACP,0,str,-1,res,ln,NULL,NULL);
	return res;
}

wchar_t* utf8_to_wchar(const char *str)
{
	if (str==NULL)
		return NULL;
	//Determine required length
	int ln=MultiByteToWideChar(CP_ACP,0,str,-1,NULL,0)/* | raise_w32_error()*/;
	wchar_t *res=(wchar_t *)malloc(sizeof(wchar_t)*ln);

	//Convert to Unicode
	MultiByteToWideChar(CP_ACP,0,str,-1,res,(int)(strlen(str)+1))/* | raise_w32_error()*/;
	return res;
}

void utf8_to_wchar_buf(const char *src, wchar_t *res, int maxlen)
{
	if (res==NULL || maxlen==0) return;

	int ln=MultiByteToWideChar(CP_ACP,0,src,-1,NULL,0)/* | raise_w32_error()*/;
	if (ln>=maxlen)
	{
		*res=L'\0';
		return;
	}
	MultiByteToWideChar(CP_ACP,0,src,-1,res,(int)(strlen(src)+1))/* | raise_w32_error()*/;
}

std::string wchar_to_utf8_cstr(const wchar_t *str)
{
	char *utf=wchar_to_utf8(str);
	std::string res(utf);
	free(utf);
	return res;
}

std::string unixify(const std::string &str)
{
	//Replace slashes
	std::string res=str;
	for(size_t f=0;f<res.size();++f)
	{
		char ch=res[f];
		if (ch=='\\')
			res[f]='/';
	}
	//Remove the trailing slash
	if (res.size()>1 && res[res.size()-1]=='/')
		res.resize(res.size()-1);

	return res;
}

FILETIME unixTimeToFiletime(time_t t)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000LL;
	FILETIME res;
	res.dwLowDateTime = (DWORD)ll;
	res.dwHighDateTime = (DWORD)(ll >> 32);
	return res;
}

bool is_filetime_set(const FILETIME *ft)
{
	if (ft==0 || (ft->dwHighDateTime==0 && ft->dwLowDateTime==0)) 
		return false;
	return true;
}

time_t filetimeToUnixTime(const FILETIME *ft)
{
	if (!is_filetime_set(ft)) return 0;

	ULONGLONG ll={0};
	ll=(ULONGLONG(ft->dwHighDateTime)<<32) + ft->dwLowDateTime;
	return time_t((ll-116444736000000000LL)/10000000LL);
}

///////////////////////////////////////////////////////////////////////////////////////
////// Error mapping and arguments conversion
///////////////////////////////////////////////////////////////////////////////////////
struct errentry 
{
	unsigned long oscode;           /* OS return value */
	int errnocode;  /* System V error code */
};

static struct errentry errtable[] = {
	{  ERROR_INVALID_FUNCTION,       EINVAL    },  /* 1 */
	{  ERROR_FILE_NOT_FOUND,         ENOENT    },  /* 2 */
	{  ERROR_PATH_NOT_FOUND,         ENOENT    },  /* 3 */
	{  ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  /* 4 */
	{  ERROR_ACCESS_DENIED,          EACCES    },  /* 5 */
	{  ERROR_INVALID_HANDLE,         EBADF     },  /* 6 */
	{  ERROR_ARENA_TRASHED,          ENOMEM    },  /* 7 */
	{  ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  /* 8 */
	{  ERROR_INVALID_BLOCK,          ENOMEM    },  /* 9 */
	{  ERROR_BAD_ENVIRONMENT,        E2BIG     },  /* 10 */
	{  ERROR_BAD_FORMAT,             ENOEXEC   },  /* 11 */
	{  ERROR_INVALID_ACCESS,         EINVAL    },  /* 12 */
	{  ERROR_INVALID_DATA,           EINVAL    },  /* 13 */
	{  ERROR_INVALID_DRIVE,          ENOENT    },  /* 15 */
	{  ERROR_CURRENT_DIRECTORY,      EACCES    },  /* 16 */
	{  ERROR_NOT_SAME_DEVICE,        EXDEV     },  /* 17 */
	{  ERROR_NO_MORE_FILES,          ENOENT    },  /* 18 */
	{  ERROR_LOCK_VIOLATION,         EACCES    },  /* 33 */
	{  ERROR_BAD_NETPATH,            ENOENT    },  /* 53 */
	{  ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  /* 65 */
	{  ERROR_BAD_NET_NAME,           ENOENT    },  /* 67 */
	{  ERROR_FILE_EXISTS,            EEXIST    },  /* 80 */
	{  ERROR_CANNOT_MAKE,            EACCES    },  /* 82 */
	{  ERROR_FAIL_I24,               EACCES    },  /* 83 */
	{  ERROR_INVALID_PARAMETER,      EINVAL    },  /* 87 */
	{  ERROR_NO_PROC_SLOTS,          EAGAIN    },  /* 89 */
	{  ERROR_DRIVE_LOCKED,           EACCES    },  /* 108 */
	{  ERROR_BROKEN_PIPE,            EPIPE     },  /* 109 */
	{  ERROR_DISK_FULL,              ENOSPC    },  /* 112 */
	{  ERROR_INVALID_TARGET_HANDLE,  EBADF     },  /* 114 */
	{  ERROR_INVALID_HANDLE,         EINVAL    },  /* 124 */
	{  ERROR_WAIT_NO_CHILDREN,       ECHILD    },  /* 128 */
	{  ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  /* 129 */
	{  ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  /* 130 */
	{  ERROR_NEGATIVE_SEEK,          EINVAL    },  /* 131 */
	{  ERROR_SEEK_ON_DEVICE,         EACCES    },  /* 132 */
	{  ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  /* 145 */
	{  ERROR_NOT_LOCKED,             EACCES    },  /* 158 */
	{  ERROR_BAD_PATHNAME,           ENOENT    },  /* 161 */
	{  ERROR_MAX_THRDS_REACHED,      EAGAIN    },  /* 164 */
	{  ERROR_LOCK_FAILED,            EACCES    },  /* 167 */
	{  ERROR_ALREADY_EXISTS,         EEXIST    },  /* 183 */
	{  ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  /* 206 */
	{  ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  /* 215 */
	{  ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }    /* 1816 */
};
const int errtable_size=sizeof(errtable)/sizeof(errtable[0]);

extern "C" int win32_error_to_errno(int win_res)
{
	if (win_res==0) return 0; //No error

	if (win_res<0) win_res=-win_res;
	for (int f=0;f<errtable_size;++f)
		if (errtable[f].oscode==win_res) return errtable[f].errnocode;
	return EINVAL;
}

extern "C" int errno_to_win32_error(int err)
{
	if (err==0) return 0; //No error

	if (err<0) err=-err;
	for (int f=0;f<errtable_size;++f)
		if (errtable[f].errnocode==err) return errtable[f].oscode;
	return ERROR_INVALID_FUNCTION;
}

extern "C" char** convert_args(int argc, wchar_t* argv[])
{
	char **arr=(char**)malloc(sizeof(char*)*(argc+1));
	for(int f=0;f<argc;++f)
		arr[f]=wchar_to_utf8(argv[f]);
	arr[argc]=NULL;
	return arr;
}

extern "C" void free_converted_args(int argc, char **argv)
{
	for(int f=0;f<argc;++f)
		free(argv[f]);
	free(argv);
}

std::string extract_file_name(const std::string &str)
{
	std::string extracted;

	for(std::string::const_reverse_iterator f=str.rbegin(),en=str.rend();f!=en;++f)
		if (*f=='/')
		{
			extracted=str.substr(en-f-1);
			break;
		}

	if (extracted.empty())
		return str;
	if (extracted[0]=='/')
		return extracted.substr(1);
	return extracted;
}

std::string extract_dir_name(const std::string &str)
{
	for(std::string::const_reverse_iterator f=str.rbegin(),en=str.rend();f!=en;++f)
		if (*f=='/')
			return str.substr(0,en-f);			
	return str;
}
