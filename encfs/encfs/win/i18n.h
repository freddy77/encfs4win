#ifndef ENCFS4WIN_I18N_H
#define ENCFS4WIN_I18N_H

#include <tchar.h>
#include <stdexcept>

#define LENGTH(v) (sizeof(v)/sizeof(v[0]))
std::string wchar_to_utf8_cstr(const wchar_t *str);
void utf8_to_wchar_buf(const char *src, wchar_t *res, int maxlen);

class wruntime_error
{
public:
	wruntime_error(const std::wstring& _err):err(_err) {}
	virtual ~wruntime_error() {}
	virtual const wchar_t *what() const { return err.c_str(); }
private:
	std::wstring err;
};

#ifdef UNICODE
# define tstring wstring
typedef wruntime_error truntime_error;
#else
# define tstring string
typedef std::runtime_error truntime_error;
#endif

#endif // ENCFS4WIN_I18N_H

