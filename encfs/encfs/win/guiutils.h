
#include <string>

extern HINSTANCE hFuseDllInstance;
#define hInst hFuseDllInstance

std::string GetExistingDirectory(HWND hwnd);
char SelectFreeDrive(HWND hwnd);
static inline bool YesNo(HWND hwnd, const char *msg) { return MessageBox(hwnd, msg, "EncFS", MB_YESNO|MB_ICONQUESTION) == IDYES; }
