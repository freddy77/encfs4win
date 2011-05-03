
#include <string>

extern HINSTANCE hFuseDllInstance;
#define hInst hFuseDllInstance

std::string GetExistingDirectory(HWND hwnd);
char SelectFreeDrive(HWND hwnd);
