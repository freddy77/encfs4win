#!/bin/bash
set -e
NAME=encfs-1.5-2
OUT=$PWD/out
rm -rf encfs-1.5
tar zxvf $NAME.tgz
patch -p0 < $NAME.diff
cd encfs-1.5
cp ../pthread.h ../vasprintf.c ../compatwin.cpp encfs
autoreconf
export lt_cv_deplibs_check_method='pass_all'
LDFLAGS=-L$OUT/lib CPPFLAGS=-I$OUT/include ./configure --prefix=$OUT --with-boost=$OUT --without-libiconv-prefix --without-libintl-prefix --host=i586-mingw32msvc
cd encfs
perl -i.orig -pe 's/-lboost_serialization -lboost_filesystem/-lboost_serialization -lboost_filesystem -lboost_system -lws2_32 -luser32 -lgdi32/' Makefile
make encfs.exe
cp .libs/encfs.exe $OUT/bin
cd ../..
rm -rf encfs-1.5
echo encfs ok

