Tools needed:
- 7zip (to extract Boost)
- patch (patch rlog)
- CMake

I created a directory c:\work with files from git.


Boost
=====
Download Boost libraries. I downloaded file boost_1_53_0.7z, extract in work directory
build (bootstrap and b2, real manual). At end of compile output should look like:

The Boost C++ Libraries were successfully built!

The following directory should be added to compiler include paths:

    C:/work/boost_1_53_0

The following directory should be added to linker library paths:

    C:\work\boost_1_53_0\stage\lib


OpenSSL
=======
OpenSSL downloaded from http://slproweb.com/products/Win32OpenSSL.html, I downloaded file
Win32OpenSSL-1_0_1e.exe (developer version, about 16MB)
installed in C:\work\OpenSSL-Win32, dll installed in bin directory


RLog
====
Decompress rlog-1.4.tar.gz apply the patches
- rlog-1.4.diff
- rlog-1.4-win.diff
- rlog-1.4-win2.diff
(all in git rlog directory)

To compiler rlog just open solution at rlog-1.4/win32/rlog.sln


fuse4win
========
Enter git directory and generate makefiles with cmake specifying source as \fuse4win-070d3a2e43de
and directory to build fuse4win-070d3a2e43de\build.
Open generated project files with Visual C++ and compile fuse and fuse_static


encfs
=====
Under git directory there is a msvc/encfs.sln file ready to compile.
Probably you will need to change some include/library directories (based on
Boost and OpenSSL versions).



You need to add rlog.dll and some dll from OpenSSL to make executables run
correctly.

Enjoy!

