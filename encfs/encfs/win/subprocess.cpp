#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "subprocess.h"

#define ErrorExit(s) do { info->errorPart = s; goto Cleanup; } while(0)

static void WINAPI CreateChildProcess(LPTSTR cmd, HANDLE in, HANDLE out, HANDLE err, SubProcessInformations* info);

BOOL WINAPI CreateSubProcess(LPTSTR cmdline, SubProcessInformations* info)
{
	HANDLE hInRd = NULL;
	HANDLE hOutWr = NULL;
	HANDLE hErrWr = NULL;

	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	info->errorPart = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&info->hOut, &hOutWr, &saAttr, 0))
		ErrorExit(TEXT("StdoutRd CreatePipe"));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(info->hOut, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(TEXT("Stdout SetHandleInformation"));

	// Create a pipe for the child process's STDIN. 
	if (!CreatePipe(&hInRd, &info->hIn, &saAttr, 0))
		ErrorExit(TEXT("Stdin CreatePipe"));

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if (!SetHandleInformation(info->hIn, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(TEXT("Stdin SetHandleInformation"));


	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	if (!DuplicateHandle(GetCurrentProcess(),hOutWr,
			     GetCurrentProcess(),&hErrWr,0,
			     TRUE,DUPLICATE_SAME_ACCESS))
		ErrorExit(TEXT("Stderr DuplicateHandle"));


	// Create the child process. On fail info->errorPart is set
	CreateChildProcess(cmdline, hInRd, hOutWr, hErrWr, info);

	// close unneeded handles passed to child
Cleanup:
	DWORD saveErr(GetLastError());
	CloseHandle(hOutWr);
	CloseHandle(hErrWr);
	CloseHandle(hInRd);

	SetLastError(saveErr);
	return info->errorPart ? FALSE : TRUE;
}

static void WINAPI
CreateChildProcess(LPTSTR cmd, HANDLE in, HANDLE out, HANDLE err, SubProcessInformations* info)
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
	PROCESS_INFORMATION piProcInfo;

	STARTUPINFO siStartInfo;

	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = err;
	siStartInfo.hStdOutput = out;
	siStartInfo.hStdInput = in;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 
	bSuccess = CreateProcess(NULL, cmd,	// command line 
				 NULL,	// process security attributes 
				 NULL,	// primary thread security attributes 
				 TRUE,	// handles are inherited
				 info->creationFlags, // creation flags 
				 NULL,	// use parent's environment 
				 NULL,	// use parent's current directory 
				 &siStartInfo,	// STARTUPINFO pointer 
				 &piProcInfo);	// receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if (!bSuccess)
		ErrorExit(TEXT("CreateProcess"));

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(piProcInfo.hThread);
	info->pid = piProcInfo.dwProcessId;
	info->hProcess = piProcInfo.hProcess;

Cleanup:
	; // do nothing
}

