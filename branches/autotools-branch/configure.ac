# Alien Arena auto configuration source
# Process this file with autoconf to produce a configure script.
#

AC_PREREQ(2.65)
AC_INIT([alienarena],[7.41.x0],[alienrace@comcast.net],[alienarena])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

AM_MAINTAINER_MODE

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_CC_C_O

# Debug mode option
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],[enable debug code (default is no)])],
	[debug=${enableval}], [debug=no])
AM_CONDITIONAL([DEBUG],[test "x${debug}" = xyes])
AC_MSG_RESULT( checking for debug build...${debug} )

# Unix/Linux host dedicated server only build option
no_client=no
AC_ARG_ENABLE( 
	[no_client],
	[AS_HELP_STRING([--no_client],
		[build dedicated server only, not the client (default:no)])],
	[no_client=${enableval}],
	[no_client=no])
AC_MSG_RESULT(checking for dedicated server only build...${no_client} )	

# check of host environment
# assumption: if it is not MinGW, then it must be some kind of Unix/Linux
#
cond_build_win32=no
cond_build_unix=no
AS_CASE([ ${host} ],
	[ *-*-mingw* ],[ cond_build_win32=yes ],
	[ cond_build_unix=yes ])

AC_MSG_RESULT( checking host...${host} )
AC_MSG_RESULT( checking whether to build win32 version...${cond_build_win32} )
AC_MSG_RESULT( checking whether to build unix version...${cond_build_unix} )

cond_build_client=yes
AS_IF(
	[ test "x${cond_build_unix}" = xyes ],
		[AS_IF([ test "x${no_client}" != xno ],[cond_build_client=no])], 
	[AC_MSG_WARN( "--no_client option does not apply to MinGW build" )])
	
AC_MSG_RESULT(checking whether to build client...${cond_build_client} );	

AM_CONDITIONAL( BUILD_WIN32, test "x${cond_build_win32}" = xyes )
AM_CONDITIONAL( BUILD_UNIX,  test "x${cond_build_unix}" = xyes )
AM_CONDITIONAL( BUILD_CLIENT, test "x${cond_build_client}" = xyes )

dnl seeing how it shows up in config
AS_IF([ test "x${cond_build_unix}" = xyes ],
	[AC_DEFINE([UNIX_VARIANT],1,[unix specific conditional compile])])

AS_IF([ test "x${cond_build_win32}" = xyes ],
	[AC_DEFINE([WIN32_VARIANT],1,[win32 specific conditional compile])])

#
# Checks for libraries.
#
AC_SEARCH_LIBS(acos,[m])
AC_SEARCH_LIBS(pthread_mutex_init,[pthread])
AC_SEARCH_LIBS(dlopen,[dl])

AS_IF( [ test "x${cond_build_client}" = xyes ],
[
AC_SEARCH_LIBS(jpeg_read_header,[jpeg],,
	[AC_MSG_ERROR(["Required jpeg library not found."])])

AC_SEARCH_LIBS([ogg_stream_init],[ogg],,
	[AC_MSG_ERROR(["Required ogg library not found."])])
		
AC_SEARCH_LIBS([vorbis_block_init],[vorbis],,
	[AC_MSG_ERROR(["Required vorbis library not found."])])

AC_SEARCH_LIBS([ov_open_callbacks],[vorbisfile],,
	[AC_MSG_ERROR(["Required vorbisfile library not found."])],
	[ -lvorbis -logg ])

AC_SEARCH_LIBS([alGenBuffers],[openal],,
	[AC_MSG_ERROR(["Required openal library not found."])])

AC_SEARCH_LIBS([curl_easy_reset],[curl],,
	[AC_MSG_ERROR(["Required curl library not found."])])

AC_PATH_X

AC_SEARCH_LIBS([XF86VidModeQueryVersion],[Xxf86vm])

have_Xxf86dga=no
AC_SEARCH_LIBS([XF86DGAQueryVersion],[Xxf86dga],[have_Xxf86dga=yes])

AC_SEARCH_LIBS([glBegin],[GL],,
	[AC_MSG_ERROR(["Required gl library not found."])])

AC_SEARCH_LIBS([gluBuild2DMipmaps],[GLU],,
	[AC_MSG_ERROR(["Required glu library not found."])],
	[ -lGL ])

]) dnl end build client conditional for libraries

AC_CHECK_HEADERS(
		[arpa/inet.h fcntl.h float.h limits.h malloc.h netdb.h netinet/in.h \
	 		stddef.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h \
	 		sys/time.h sys/vt.h termios.h unistd.h])
	
AS_IF( [ test "x${cond_build_client}" = xyes ],
[
AC_CHECK_HEADERS([jpeglib.h],,
	[AC_MSG_ERROR("Required JPEG header files not found.")])

AC_CHECK_HEADERS([vorbis/vorbisfile.h],,
	[AC_MSG_ERROR("Required Ogg-Vorbis header file not found.")])

AC_CHECK_HEADERS([AL/al.h AL/alc.h],,
	[AC_MSG_ERROR("Required OpenAL header files not found.")])

AC_CHECK_HEADERS([curl/curl.h],,
	[AC_MSG_ERROR("Required cURL header file not found.")])
	 		
AC_CHECK_HEADERS([X11/Xlib.h X11/Xatom.h X11/keysym.h X11/cursorfont.h],,
	[AC_MSG_ERROR("Required X11 files not found")])

AC_CHECK_HEADERS([X11/extensions/xf86vmode.h],
	,
	[AC_MSG_ERROR("Required X11 x86 extension header file not found")],
	[#include <X11/Xlib.h>])

AS_IF([test "x${have_Xxf86dga}" = xyes],
	[AC_CHECK_HEADERS(
		[X11/extensions/Xxf86dga.h],
		[found=yes],
			[AC_CHECK_HEADERS([X11/extensions/xf86dga.h],
				[found=yes],
				[have_Xxf86dga=no],
				[#include <X11/Xlib.h #include <X11/Xproto.h> #include <X11/Xfuncproto.h>])],
		[#include <X11/Xlib.h>  #include <X11/Xproto.h>])])
	
AC_CHECK_HEADERS([GL/gl.h GL/glu.h GL/glx.h],,
	[AC_MSG_ERROR("Required OpenGL header files not found.")])

]) dnl end build client conditional for headers

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
#  note: redefines __inline or __inline__ to inline if compiler wants that
AC_C_INLINE
#  note: defines a suitable type for size_t if system headers do not
AC_TYPE_SIZE_T
#  note: defines a suitable type uid_t and gid_t if system headers do not 
AC_TYPE_UID_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC

AC_CHECK_FUNCS([ floor gethostbyname gettimeofday memmove memset mkdir dnl
		munmap mremap putenv select socket sqrt strchr strdup strerror dnl
		stricmp _stricmp strcasecmp strncasecmp strrchr strdup strstr ]) dnl

# outputs
AC_CONFIG_FILES([Makefile source/Makefile])
AC_OUTPUT
