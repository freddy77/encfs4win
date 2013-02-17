#!/bin/bash
set -e
NAME=rlog-1.4
OUT=$PWD/../out
rm -rf $NAME
tar zxvf $NAME.tar.gz
cd $NAME
patch -p1 < ../$NAME.diff
patch -p1 < ../rlog-1.4-win.diff
patch -p1 < ../rlog-1.4-win2.diff
HOST=unknown
for h in i386-mingw32 i586-mingw32msvc i686-w64-mingw32; do
	if $h-gcc --help &> /dev/null; then
		HOST=$h
		break
	fi
done
if [ $HOST = unknown ]; then
	echo Host not detected >&2
	exit 1
fi
./configure --prefix=$OUT --host=$HOST
make
make install
cd ..
rm -rf $NAME
echo RLog ok
