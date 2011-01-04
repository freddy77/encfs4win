#!/bin/bash
set -e
OUT=$PWD/out
cd fuse4win-070d3a2e43de
rm -rf build
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cross.cmake -DCMAKE_INSTALL_PREFIX=$OUT .. 
make
make install
cd ../..
echo fuse4win ok
