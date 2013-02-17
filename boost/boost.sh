#!/bin/bash
set -e
NAME=boost_1_46_0
OUT=$PWD/../out

rm -rf $NAME
tar jxvf $NAME.tar.bz2
cd $NAME
./bootstrap.sh --prefix=$OUT
echo "using gcc : 4.6.3 : i686-w64-mingw32-g++ ;" >> tools/build/v2/user-config.jam
#./bjam install
#./bjam --layout=system variant=release threading=multi link=shared runtime-link=shared toolset=gcc target-os=windows threadapi=win32 install
./bjam --layout=system variant=release threading=multi link=static runtime-link=static toolset=gcc target-os=windows threadapi=win32 install || true

cd $OUT/lib
test -r libboost_filesystem.a
test -r libboost_system.a
test -r libboost_serialization.a
i686-w64-mingw32-ranlib libboost_*.a
cd ../../boost
rm -rf $NAME

echo Boost ok
