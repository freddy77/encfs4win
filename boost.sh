#!/bin/bash
set -e
NAME=boost_1_40_0
OUT=$PWD/out

rm -rf $NAME
tar jxvf $NAME.tar.bz2
cd $NAME
./bootstrap.sh --prefix=$OUT
echo "using gcc : 4.4.4 : i586-mingw32msvc-g++ ;" >> tools/build/v2/user-config.jam
#./bjam install
#./bjam --layout=system variant=release threading=multi link=shared runtime-link=shared toolset=gcc target-os=windows threadapi=win32 install
./bjam --layout=system variant=release threading=multi link=static runtime-link=static toolset=gcc target-os=windows threadapi=win32 install || true

cd $OUT/lib
test -r libboost_filesystem.lib
test -r libboost_system.lib
test -r libboost_serialization.lib
for lib in libboost_*.lib; do
	i586-mingw32msvc-ranlib $lib
	rm -f ${lib%.lib}.a
	ln -s $lib ${lib%.lib}.a
done
cd ../..
rm -rf $NAME

echo Boost ok
