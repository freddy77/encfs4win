
struct SubProcessInformations
{
	SubProcessInformations():
		creationFlags(0),
		pid(0), hProcess(NULL), hIn(NULL), hOut(NULL), hErr(NULL),
		errorPart(NULL)
	{ }
	~SubProcessInformations()
	{
		CloseHandle(hIn);
		CloseHandle(hOut);
		CloseHandle(hErr);
		CloseHandle(hProcess);
	}
	DWORD creationFlags;
	DWORD pid;
	HANDLE hProcess;
	HANDLE hIn, hOut, hErr;
	LPCTSTR errorPart;
};

extern "C" BOOL WINAPI 
CreateSubProcess(LPTSTR cmdline, SubProcessInformations* info);

