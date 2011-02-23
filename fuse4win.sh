#!/bin/bash
set -e
OUT=$PWD/out
cd fuse4win-070d3a2e43de
rm -rf build
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cross.cmake -DCMAKE_INSTALL_PREFIX=$OUT ..
#LDFLAGS=-pg CFLAGS="-DSTATISTICS -pg -fprofile" CXXFLAGS="-DSTATISTICS -pg -fprofile" cmake -DCMAKE_TOOLCHAIN_FILE=../cross.cmake -DCMAKE_INSTALL_PREFIX=$OUT ..
make
make install
cd ../..
cd out/lib
test -r libfuse_static.a
rm -f libfuse.a
ln -s libfuse_static.a libfuse.a 
echo fuse4win ok
