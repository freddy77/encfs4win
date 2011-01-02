#!/bin/bash
set -e
NAME=openssl-0.9.8l
OUT=$PWD/../out
rm -rf $NAME
tar zxvf $NAME.tar.gz
patch -p0 < $NAME.diff
cd $NAME
CC=i586-mingw32msvc-gcc ./Configure --prefix=$OUT mingw
make CC=i586-mingw32msvc-gcc RANLIB=i585-mingw32msvc-ranlib || true
i586-mingw32msvc-ranlib libssl.a 
i586-mingw32msvc-ranlib libcrypto.a 
make CC=i586-mingw32msvc-gcc RANLIB=i585-mingw32msvc-ranlib
make install
cd ..
rm -rf $NAME
echo OpenSSL ok

