#!/bin/bash
set -e
CROSS=i686-w64-mingw32
OUT=$PWD/out
cd encfs
if true; then
	make distclean || true
	autoreconf
	export lt_cv_deplibs_check_method='pass_all'
	#LDFLAGS="-L$OUT/lib -pg" CXXFLAGS="-pg -fprofile" CFLAGS="-pg -fprofile" CPPFLAGS="-I$OUT/include" ./configure --prefix=$OUT --with-boost=$OUT --without-libiconv-prefix --without-libintl-prefix --host=i586-mingw32msvc
	LDFLAGS=-L$OUT/lib CPPFLAGS=-I$OUT/include ./configure --prefix=$OUT --with-boost=$OUT --without-libiconv-prefix --without-libintl-prefix --host=$CROSS
fi
cd encfs
rm -f encfs.exe encfsctl.exe libencfsall.la
make encfs.exe encfsw.exe encfsctl.exe
test -r .libs/libencfsall-1.dll
DLL=$($CROSS-objdump -p .libs/libencfsall-1.dll | grep '^Name ' | sed 's/.*encfs/encfs/')
cp .libs/encfs.exe .libs/encfsw.exe .libs/encfsctl.exe $OUT/bin
cp .libs/libencfsall-1.dll $OUT/bin/$DLL
cd ../..
echo encfs ok

