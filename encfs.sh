#!/bin/bash
set -e
OUT=$PWD/out
cd encfs
make distclean || true
autoreconf
export lt_cv_deplibs_check_method='pass_all'
LDFLAGS=-L$OUT/lib CPPFLAGS=-I$OUT/include ./configure --prefix=$OUT --with-boost=$OUT --without-libiconv-prefix --without-libintl-prefix --host=i586-mingw32msvc
cd encfs
perl -i.orig -pe 's/-lboost_serialization -lboost_filesystem/-lboost_serialization -lboost_filesystem -lboost_system -lws2_32 -luser32 -lgdi32/' Makefile
make encfs.exe
cp .libs/encfs.exe $OUT/bin
cd ../..
echo encfs ok

