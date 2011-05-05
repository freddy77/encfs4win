
#include <string>

extern HINSTANCE hFuseDllInstance;
#define hInst hFuseDllInstance

std::string GetExistingDirectory(HWND hwnd, LPCTSTR title = NULL, LPCTSTR caption = NULL);
bool GetPassword(HWND hwnd, char *pass, size_t len);
char SelectFreeDrive(HWND hwnd);
static inline bool YesNo(HWND hwnd, const char *msg) { return MessageBox(hwnd, msg, "EncFS", MB_YESNO|MB_ICONQUESTION) == IDYES; }
bool FillFreeDrive(HWND combo);
