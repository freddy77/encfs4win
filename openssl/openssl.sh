#!/bin/bash
set -e
CROSS=i686-w64-mingw32
NAME=openssl-1.0.0e
OUT=$PWD/../out
rm -rf $NAME
tar zxvf $NAME.tar.gz
cd $NAME
CC=$CROSS-gcc ./Configure --prefix=$OUT mingw
make CC=$CROSS-gcc RANLIB=$CROSS-ranlib || true
$CROSS-ranlib libssl.a
$CROSS-ranlib libcrypto.a
make CC=$CROSS-gcc RANLIB=$CROSS-ranlib
make install
cd ..
rm -rf $NAME
echo OpenSSL ok

