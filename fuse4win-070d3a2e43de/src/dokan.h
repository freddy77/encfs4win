/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2008 Hiroki Asakawa asakaw@gmail.com

  http://dokan-dev.net/en

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DOKAN_H_
#define _DOKAN_H_

#define DOKAN_DRIVER_NAME	L"dokan.sys"

#ifdef _EXPORTING
	#define DOKANAPI __declspec(dllimport) __stdcall
#else
	#define DOKANAPI __declspec(dllexport) __stdcall
#endif

#define DOKAN_CALLBACK __stdcall

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DOKAN_FILE_INFO {

	ULONG64	Context;      // FileSystem can use this variable
	ULONG64	DokanContext; // Don't touch this
	ULONG	ProcessId;    // process id for the thread that originally requested a given I/O operation
	BOOL	IsDirectory;  // requesting a directory file

} DOKAN_FILE_INFO, *PDOKAN_FILE_INFO;


// FillFileData
//   add an entry in FindFiles
//   return 1 if buffer is full, otherwise 0
//   (currently never return 1)
typedef int (WINAPI *PFillFindData) (PWIN32_FIND_DATAW, PDOKAN_FILE_INFO);

typedef struct _DOKAN_OPERATIONS {

	// When an error occurs, return negative value.
	// Usually you should return GetLastError() * -1.


	// CreateFile
	//   If file is a directory, CreateFile (not OpenDirectory) may be called.
	//   In this case, CreateFile should return 0 when that directory can be opened.
	//   You should set TRUE on DokanFileInfo->IsDirectory when file is a directory.
	//   When CreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS and a file already exists,
	//   you should return ERROR_ALREADY_EXISTS(183) (not negative value)
	int (DOKAN_CALLBACK *CreateFile) (
		LPCWSTR,      // FileName
		DWORD,        // DesiredAccess
		DWORD,        // ShareMode
		DWORD,        // CreationDisposition
		DWORD,        // FlagsAndAttributes
		//HANDLE,       // TemplateFile
		PDOKAN_FILE_INFO);

	int (DOKAN_CALLBACK *OpenDirectory) (
		LPCWSTR,				// FileName
		PDOKAN_FILE_INFO);

	int (DOKAN_CALLBACK *CreateDirectory) (
		LPCWSTR,				// FileName
		PDOKAN_FILE_INFO);

	int (DOKAN_CALLBACK *Cleanup) (
		LPCWSTR,      // FileName
		PDOKAN_FILE_INFO);

	int (DOKAN_CALLBACK *CloseFile) (
		LPCWSTR,      // FileName
		PDOKAN_FILE_INFO);

	int (DOKAN_CALLBACK *ReadFile) (
		LPCWSTR,  // FileName
		LPVOID,   // Buffer
		DWORD,    // NumberOfBytesToRead
		LPDWORD,  // NumberOfBytesRead
		LONGLONG, // Offset
		PDOKAN_FILE_INFO);
	

	int (DOKAN_CALLBACK *WriteFile) (
		LPCWSTR,  // FileName
		LPCVOID,  // Buffer
		DWORD,    // NumberOfBytesToWrite
		LPDWORD,  // NumberOfBytesWritten
		LONGLONG, // Offset
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *FlushFileBuffers) (
		LPCWSTR, // FileName
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *GetFileInformation) (
		LPCWSTR,          // FileName
		LPBY_HANDLE_FILE_INFORMATION, // Buffer
		PDOKAN_FILE_INFO);
	

	int (DOKAN_CALLBACK *FindFiles) (
		LPCWSTR,			// PathName
		PFillFindData,		// call this function with PWIN32_FIND_DATAW
		PDOKAN_FILE_INFO);  //  (see PFillFindData definition)


	// You should implement either FindFires or FindFilesWithPattern
	int (DOKAN_CALLBACK *FindFilesWithPattern) (
		LPCWSTR,			// PathName
		LPCWSTR,			// SearchPattern
		PFillFindData,		// call this function with PWIN32_FIND_DATAW
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *SetFileAttributes) (
		LPCWSTR, // FileName
		DWORD,   // FileAttributes
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *SetFileTime) (
		LPCWSTR,		// FileName
		CONST FILETIME*, // CreationTime
		CONST FILETIME*, // LastAccessTime
		CONST FILETIME*, // LastWriteTime
		PDOKAN_FILE_INFO);

	int (DOKAN_CALLBACK *DeleteFile) (
		LPCWSTR, // FileName
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *DeleteDirectory) ( 
		LPCWSTR, // FileName
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *MoveFile) (
		LPCWSTR, // ExistingFileName
		LPCWSTR, // NewFileName
		BOOL,	// ReplaceExisiting
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *SetEndOfFile) (
		LPCWSTR,  // FileName
		LONGLONG, // Length
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *LockFile) (
		LPCWSTR, // FileName
		LONGLONG, // ByteOffset
		LONGLONG, // Length
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *UnlockFile) (
		LPCWSTR, // FileName
		LONGLONG,// ByteOffset
		LONGLONG,// Length
		PDOKAN_FILE_INFO);


	// Neither GetDiskFreeSpace nor GetVolumeInformation
	// save the DokanFileContext->Context.
	// Before these methods are called, CreateFile may not be called.
	// (ditto CloseFile and Cleanup)

	// see Win32 API GetDiskFreeSpaceEx
	int (DOKAN_CALLBACK *GetDiskFreeSpace) (
		PULONGLONG, // FreeBytesAvailable
		PULONGLONG, // TotalNumberOfBytes
		PULONGLONG, // TotalNumberOfFreeBytes
		PDOKAN_FILE_INFO);


	// see Win32 API GetVolumeInformation
	int (DOKAN_CALLBACK *GetVolumeInformation) (
		LPWSTR, // VolumeNameBuffer
		DWORD,	// VolumeNameSize
		LPDWORD,// VolumeSerialNumber
		LPDWORD,// MaximumComponentLength
		LPDWORD,// FileSystemFlags
		LPWSTR,	// FileSystemNameBuffer
		DWORD,	// FileSystemNameSize
		PDOKAN_FILE_INFO);


	int (DOKAN_CALLBACK *Unmount) (
		PDOKAN_FILE_INFO);

} DOKAN_OPERATIONS, *PDOKAN_OPERATIONS;


typedef struct _DOKAN_OPTIONS {
	WCHAR	DriveLetter; // driver letter to be mounted
	ULONG	ThreadCount; // number of threads to be used
	UCHAR	DebugMode; // ouput debug message
	UCHAR	UseStdErr; // ouput debug message to stderr
	UCHAR	UseAltStream; // use alternate stream
	UCHAR	UseKeepAlive; // use auto unmount
	UCHAR	Dummy0;
	UCHAR	Dummy1;

} DOKAN_OPTIONS, *PDOKAN_OPTIONS;


/* DokanMain returns error codes */
#define DOKAN_SUCCESS				 0
#define DOKAN_ERROR					-1 /* General Error */
#define DOKAN_DRIVE_LETTER_ERROR	-2 /* Bad Drive letter */
#define DOKAN_DRIVER_INSTALL_ERROR	-3 /* Can't install driver */
#define DOKAN_START_ERROR			-4 /* Driver something wrong */
#define DOKAN_MOUNT_ERROR			-5 /* Can't assign a drive letter */


int DOKANAPI
DokanMain(
	PDOKAN_OPTIONS	DokanOptions,
	PDOKAN_OPERATIONS DokanOperations);


BOOL DOKANAPI
DokanUnmount(
	WCHAR DriveLetter);


// DokanIsNameInExpression
//   check whether Name can match Expression
//   Expression can contain wildcard characters (? and *)
BOOL DOKANAPI
DokanIsNameInExpression(
	LPCWSTR		Expression,		// matching pattern
	LPCWSTR		Name,			// file name
	BOOL		IgnoreCase);


ULONG DOKANAPI
DokanVersion();

ULONG DOKANAPI
DokanDriverVersion();


// for internal use
// don't call
BOOL DOKANAPI
DokanServiceInstall(
	LPCWSTR	ServiceName,
	DWORD	ServiceType,
	LPCWSTR ServiceFullPath);

BOOL DOKANAPI
DokanServiceDelete(
	LPCWSTR	ServiceName);


#ifdef __cplusplus
}
#endif


#endif // _DOKAN_H_
