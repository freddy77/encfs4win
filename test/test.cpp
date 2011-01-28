#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

static void
check(int line, bool test, const char *chk, const char *fmt, ...)
{
	if (test) return;

	va_list ap;

	fprintf(stderr, "error at line %d: ", line);
	va_start(ap, fmt);
	if (fmt)
		vfprintf(stderr, fmt, ap);
	else
		fprintf(stderr, "test failed was %s", chk);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

#define CHECK(test, arg...) check(__LINE__, test, #test, arg) 

static const char fn[] = "testfile.txt";

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
	h1 = CreateFile(fn, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	CHECK(LockFile(h1, 0, 0, 10, 0), "lock failed");
	CHECK(!LockFile(h1, 0, 0, 10, 0), "lock succeeded");	// double lock must fail
	CHECK(UnlockFile(h1, 0, 0, 10, 0), "unlock failed");
	CHECK(!UnlockFile(h1, 0, 0, 20, 0), "unlock succeeded");
	// TODO check overlapping locks
	CHECK(LockFile(h1, 10, 0, 10, 0), "lock failed");
	CHECK(LockFile(h1, 20, 0, 10, 0), "lock failed");
	CHECK(LockFile(h1, 40, 0, 10, 0), "lock failed");

	// (un)lock that overlap (inside, middle, side, two contiguos)
	CHECK(!LockFile(h1, 15, 0, 3,  0), "lock succeeded");	// inside
	CHECK(!LockFile(h1, 10, 0, 5,  0), "lock succeeded");	// side
	CHECK(!LockFile(h1, 15, 0, 5,  0), "lock succeeded");	// side
	CHECK(!LockFile(h1, 10, 0, 20, 0), "lock succeeded");	// two contiguos
	CHECK(!LockFile(h1, 30, 0, 20, 0), "lock succeeded");	// one full + free

	CHECK(!UnlockFile(h1, 15, 0, 3,  0), "unlock succeeded");	// inside
	CHECK(!UnlockFile(h1, 10, 0, 5,  0), "unlock succeeded");	// side
	CHECK(!UnlockFile(h1, 15, 0, 5,  0), "unlock succeeded");	// side
	CHECK(!UnlockFile(h1, 10, 0, 20, 0), "unlock succeeded");	// two contiguos
	CHECK(!UnlockFile(h1, 30, 0, 20, 0), "unlock succeeded");	// one full + free

	CHECK(UnlockFile(h1, 20, 0, 10, 0), "unlock failed");
	CHECK(UnlockFile(h1, 10, 0, 10, 0), "unlock failed");
	CHECK(UnlockFile(h1, 40, 0, 10, 0), "unlock failed");
	CloseHandle(h1);

	printf("deleting while open\n");
	h1 = CreateFile(fn, FILE_READ_ATTRIBUTES, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	CHECK(h1 != INVALID_HANDLE_VALUE, NULL);
	CHECK(DeleteFile(fn), "failed deleting file");
	CloseHandle(h1);

	printf("all done!\n");

	return 0;
}

