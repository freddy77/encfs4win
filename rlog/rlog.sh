#!/bin/bash
set -e
NAME=rlog-1.4
OUT=$PWD/../out
rm -rf $NAME
tar zxvf $NAME.tar.gz
patch -p0 < $NAME.diff
cd $NAME
./configure --prefix=$OUT --host=i586-mingw32msvc
make
make install
cd ..
rm -rf $NAME
echo RLog ok
