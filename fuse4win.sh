#!/bin/bash
set -e
NAME=fuse4win-070d3a2e43de

# check we are not working
if test -d $NAME; then
	echo "Please delete directory manually... work in progress" >&2
	exit 1
fi

OUT=$PWD/out
rm -rf $NAME
tar jxvf $NAME.tar.bz2
cp dokan.h $NAME/src/dokan.h
cd $NAME
rm -rf patches
tar zxvf ../fuse_patches.tar.gz
quilt push -a
mkdir build
cd build
CC=i586-mingw32msvc-gcc CXX=i586-mingw32msvc-g++ cmake ..
make
i586-mingw32msvc-ranlib libfuse_static.a
cp libfuse_static.a $OUT/lib/libfuse.a
cd ../include
cp *.h $OUT/include
cd ../..
rm -rf $NAME
echo fuse4win ok
