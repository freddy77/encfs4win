#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

static void
check(int line, bool test, const char *chk, const char *fmt, ...)
{
	if (test) return;

	DWORD err = GetLastError();
	va_list ap;

	fprintf(stderr, "error at line %d (last err %u): ", line, (unsigned) err);
	va_start(ap, fmt);
	if (fmt)
		vfprintf(stderr, fmt, ap);
	else
		fprintf(stderr, "test failed was %s", chk);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

#define CHECK(test, arg...) do { SetLastError(0); check(__LINE__, test, #test, arg); } while(0)
#define CHECK_NORESET(test, arg...) do { check(__LINE__, test, #test, arg); } while(0)

static const char fn[] = "testfile.txt";
static const char fn2[] = "testfile2.txt";
static const char long_fn[] = "Test very long File Name.Txt";
static const char test_dir[] = "test_DIR";

static FILETIME *
Time(FILETIME &t, int y, int m, int d, int h=12)
{
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));
	st.wYear  = y;
	st.wMonth = m;
	st.wDay   = d;
	st.wHour  = h;
	SystemTimeToFileTime(&st, &t);
	return &t;
}

static bool
SameTime(const FILETIME &t, int y, int m, int d)
{
	SYSTEMTIME st;
	FileTimeToSystemTime(&t, &st);
	if (st.wYear != y || st.wMonth != m || st.wDay != d)
		return false;
	return true;
}

int main()
{
	HANDLE h1, h2;

	// cleanup
	DeleteFile(fn);

	// create simple test file
	FILE *f = fopen(fn, "w");
	CHECK(f != NULL, "error creating test file");
	fputs("test 123\n", f);
	fclose(f);

	printf("test share\n");
	h1 = CreateFile(fn, FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	h2 = CreateFile(fn, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK(h2 != INVALID_HANDLE_VALUE, "second handle should open successfully");
	CloseHandle(h1);
	CloseHandle(h2);

	printf("test share (copy&paste on same directory create this sequence)\n");
	h1 = CreateFile(fn, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	h2 = CreateFile(fn, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK(h2 == INVALID_HANDLE_VALUE, "second handle should fail");
	CloseHandle(h1);
	CloseHandle(h2);

	// excel need locking to open a file
	printf("locking should succeed\n");
	h1 = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	CHECK(LockFile(h1, 0, 0, 10, 0), "lock failed");
	CHECK(!LockFile(h1, 0, 0, 10, 0), "double lock succeeded");
	CHECK(UnlockFile(h1, 0, 0, 10, 0), "unlock failed");
	// TODO currently Dokan return always success :(
//	CHECK(!UnlockFile(h1, 0, 0, 20, 0), "unlock succeeded");

	// check overlapping locks
	CHECK(LockFile(h1, 10, 0, 10, 0), "lock failed");
	CHECK(LockFile(h1, 20, 0, 10, 0), "lock failed");
	CHECK(LockFile(h1, 40, 0, 10, 0), "lock failed");

	// (un)lock that overlap (inside, middle, side, two contiguos)
	CHECK(!LockFile(h1, 15, 0, 3,  0), "lock succeeded inside");
	CHECK(!LockFile(h1, 10, 0, 5,  0), "lock succeeded on left side");
	CHECK(!LockFile(h1, 15, 0, 5,  0), "lock succeeded on right side");
	CHECK(!LockFile(h1, 10, 0, 20, 0), "lock succeeded of 2 contiguos");
	CHECK(!LockFile(h1, 30, 0, 20, 0), "lock succeeded 1+free piece");

//	CHECK(!UnlockFile(h1, 15, 0, 3,  0), "unlock succeeded inside");
//	CHECK(!UnlockFile(h1, 10, 0, 5,  0), "unlock succeeded on left side");
//	CHECK(!UnlockFile(h1, 15, 0, 5,  0), "unlock succeeded on right side");
//	CHECK(!UnlockFile(h1, 10, 0, 20, 0), "unlock succeeded of 2 contiguos");
//	CHECK(!UnlockFile(h1, 30, 0, 20, 0), "unlock succeeded 1+free piece");

	// test some read/write
	h2 = CreateFile(fn, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h2 != INVALID_HANDLE_VALUE, NULL);
	char buf[256];
	DWORD readed;
	readed = 123;
	CHECK(SetFilePointer(h2, 0, NULL, FILE_BEGIN) == 0, NULL);
	CHECK(ReadFile(h2, buf, 10, &readed, NULL), NULL);
	CHECK(readed == 10, NULL);
	readed = 123;
	CHECK(SetFilePointer(h2, 0, NULL, FILE_BEGIN) == 0, NULL);
	CHECK(!ReadFile(h2, buf, 20, &readed, NULL), NULL);
	// TODO
//	CHECK_NORESET(GetLastError() == ERROR_LOCK_VIOLATION, NULL);
	readed = 123;
	CHECK(SetFilePointer(h1, 0, NULL, FILE_BEGIN) == 0, NULL);
	CHECK(ReadFile(h1, buf, 20, &readed, NULL), NULL);
	CloseHandle(h2);

	CHECK(UnlockFile(h1, 20, 0, 10, 0), "unlock failed");
	CHECK(UnlockFile(h1, 10, 0, 10, 0), "unlock failed");
	CHECK(UnlockFile(h1, 40, 0, 10, 0), "unlock failed");
	CloseHandle(h1);

	printf("check a bug with CREATE_ALWAYS\n");
	h1 = CreateFile(fn, FILE_GENERIC_WRITE|FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, "create failed");
	CloseHandle(h1);

	printf("rename to an open file\n");
	h1 = CreateFile(fn2, SYNCHRONIZE|FILE_WRITE_DATA, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, "create failed");
	CHECK(!MoveFile(fn, fn2) && GetLastError() == ERROR_ALREADY_EXISTS, "move succeeded");
	CloseHandle(h1);
	CHECK(!MoveFile(fn, fn2) && GetLastError() == ERROR_ALREADY_EXISTS, "move succeeded");
	CHECK(MoveFileEx(fn, fn2, MOVEFILE_REPLACE_EXISTING), "move failed");
	CHECK(MoveFile(fn2, fn), NULL);
	DeleteFile(fn2);

	printf("time check and attributes\n");
	CHECK(SetFileAttributes(fn, FILE_ATTRIBUTE_HIDDEN), NULL);
	CHECK((GetFileAttributes(fn) & (~FILE_ATTRIBUTE_SPARSE_FILE)) == FILE_ATTRIBUTE_HIDDEN, NULL);
	h1 = CreateFile(fn, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, "create failed");
	CHECK((GetFileAttributes(fn) & (~FILE_ATTRIBUTE_SPARSE_FILE)) == FILE_ATTRIBUTE_HIDDEN, NULL);
	FILETIME c,a,m;
	CHECK(SetFileTime(h1, Time(c,2000,1,2), Time(a,2001,3,5), Time(m,2010,12,24)), NULL);
	// this is required only for Dokan cause SetFileInformation is called with BasicInformation
	CHECK(SetFileAttributes(fn, FILE_ATTRIBUTE_HIDDEN), NULL);
	CloseHandle(h1);

	WIN32_FIND_DATA wfd;
	h1 = FindFirstFile(fn, &wfd);
	CHECK(h1 != INVALID_HANDLE_VALUE, "find failed");
	CHECK((GetFileAttributes(fn) & (~FILE_ATTRIBUTE_SPARSE_FILE)) == FILE_ATTRIBUTE_HIDDEN, NULL);
	CHECK(SameTime(wfd.ftCreationTime,2000,1,2), NULL);
	CHECK(SameTime(wfd.ftLastAccessTime,2001,3,5), NULL);
	CHECK(SameTime(wfd.ftLastWriteTime,2010,12,24), NULL);
	FindClose(h1);
	CHECK((GetFileAttributes(fn) & (~FILE_ATTRIBUTE_SPARSE_FILE)) == FILE_ATTRIBUTE_HIDDEN, NULL);
	CHECK(SetFileAttributes(fn, FILE_ATTRIBUTE_NORMAL), NULL);

	printf("deleting while open\n");
	h1 = CreateFile(fn, FILE_READ_ATTRIBUTES, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	CHECK(DeleteFile(fn), "failed deleting file");
	h2 = FindFirstFile(fn, &wfd);
	CHECK(h2 != INVALID_HANDLE_VALUE, NULL);
	FindClose(h2);
	CloseHandle(h1);
	CHECK(FindFirstFile(fn, &wfd) == INVALID_HANDLE_VALUE, NULL);

	f = fopen(long_fn, "w");
	CHECK(f != NULL, "error creating test file");
	fputs("long test\n", f);
	fclose(f);

	h1 = FindFirstFile(long_fn, &wfd);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	CHECK(strcmp(wfd.cFileName, long_fn) == 0, NULL);
	FindClose(h1);

	// check directory
	RemoveDirectory(test_dir);
	CHECK(CreateDirectory(test_dir, NULL), NULL);
	h1 = CreateFile(test_dir, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, "create failed");
	CHECK(SetFileTime(h1, Time(c,2000,1,2), Time(a,2001,3,5), Time(m,2010,12,24)), NULL);
	CloseHandle(h1);

	h1 = FindFirstFile(test_dir, &wfd);
	CHECK(h1 != INVALID_HANDLE_VALUE, "find failed");
	CHECK(SameTime(wfd.ftCreationTime,2000,1,2), NULL);
	CHECK(SameTime(wfd.ftLastAccessTime,2001,3,5), NULL);
	CHECK(SameTime(wfd.ftLastWriteTime,2010,12,24), NULL);
	FindClose(h1);

	RemoveDirectory(test_dir);

	printf("all done!\n");

	return 0;
}

