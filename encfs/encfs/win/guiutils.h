
#include <string>
#include "i18n.h"

extern HINSTANCE hFuseDllInstance;
#define hInst hFuseDllInstance

std::tstring GetExistingDirectory(HWND hwnd, LPCTSTR title = NULL, LPCTSTR caption = NULL);
bool GetPassword(HWND hwnd, TCHAR *pass, size_t len);
char SelectFreeDrive(HWND hwnd);
static inline bool YesNo(HWND hwnd, const TCHAR *msg) { return MessageBox(hwnd, msg, _T("EncFS"), MB_YESNO|MB_ICONQUESTION) == IDYES; }
bool FillFreeDrive(HWND combo);
