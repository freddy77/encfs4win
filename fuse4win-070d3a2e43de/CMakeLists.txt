PROJECT(fuse4win)

cmake_minimum_required(VERSION 2.4)

include_directories(include src)

ADD_DEFINITIONS(-DFUSE_USE_VERSION=27 -D_FILE_OFFSET_BITS=64)

SET(fuse_SRCS 
src/fuse_helpers.c
src/fuse_opt.c
src/docanfuse.cpp
src/fusemain.cpp
src/utils.cpp
src/FUSE4Win.def
src/FUSE4Win.rc
)

# Add include files to VisualStudio project
IF(WIN32)
	SET(fuse_SRCS ${fuse_SRCS}
	include/fuse.h
	include/fuse_common.h
	include/fuse_opt.h
	include/fuse_sem_fix.h
	include/fuse_win.h
	src/docanfuse.h
	src/dokan.h
	src/FUSE4Win.h
	src/fusemain.h
	src/ScopeGuard.h
	src/utils.h)
ENDIF(WIN32)

# SET(CMAKE_BUILD_TYPE Debug)

# library to be built
ADD_LIBRARY(fuse SHARED ${fuse_SRCS})
ADD_LIBRARY(fuse_static STATIC ${fuse_SRCS})


IF(CYGWIN)

	INSTALL(
		FILES 
	include/fuse.h
	include/fuse_common.h
	include/fuse_opt.h
	include/fuse_sem_fix.h
	include/fuse_win.h
		DESTINATION /usr/include/fuse)

	INSTALL(TARGETS fuse
  		RUNTIME DESTINATION /bin
  		LIBRARY DESTINATION /lib
  		ARCHIVE DESTINATION /lib
	)

	INSTALL(FILES package/fuse.pc DESTINATION /lib/pkgconfig)

ENDIF(CYGWIN)

IF(MINGW AND CMAKE_CROSSCOMPILING)
	INSTALL(
		FILES
	include/fuse.h
	include/fuse_common.h
	include/fuse_opt.h
	include/fuse_sem_fix.h
	include/fuse_win.h
		DESTINATION include)

	INSTALL(TARGETS fuse_static ARCHIVE DESTINATION lib)

ENDIF(MINGW AND CMAKE_CROSSCOMPILING)

