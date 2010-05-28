# Alien Arena auto configuration source
# Process this file with autoconf to produce a configure script.
#

#
# Existing Symbols to be incorporated
# DEDICATED_ONLY
# GAME_HARD_LINKED
# Possible New Symbols
# replace __unix__ with feature specific symbols
#   - something to enable or disable console output ?
#   - case sensitive file names

AC_PREREQ(2.65)
AC_INIT([alienarena],[7.41.0],[alienrace@comcast.net],[alienarena])

dnl
AC_PREFIX_DEFAULT([/usr/local/games])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

dnl
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])

dnl For building game.so, which is dlopened by crx and crded
dnl TODO: explore game as hard linked static library
LT_PREREQ([2.2.6b])
LT_INIT([disable-static dlopen])

dnl 
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

dnl
dnl TODO: put host customization here, set variables for different requirements
dnl
dnl AS_CASE([$host],
dnl    [*-*-mingw*],[ action ]
dnl    [*-linux*],[ action ]
dnl    [*-darwin*],[ action ]
dnl    []
dnl
dnl AS_CASE([$host],
dnl 	[i686-*],
dnl		[AS_ECHO(["i686"])],
dnl 	[x86_64-*],
dnl 		[AS_ECHO(["x86_64"])],
dnl 	[AS_ECHO(["CPU Not Known"])])
dnl


# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

#
# Checks for libraries.
#
AC_PATH_X
AX_PTHREAD
AX_CHECK_GL
AX_CHECK_GLU

AC_SEARCH_LIBS(acos,[m])
AC_SEARCH_LIBS(jpeg_read_header,[jpeg])
AC_SEARCH_LIBS([XF86VidModeQueryVersion],[Xxf86vm])
AC_SEARCH_LIBS([XF86DGAQueryVersion],[Xxf86dga])
AC_SEARCH_LIBS([curl_easy_reset],[curl])
AC_SEARCH_LIBS([ogg_stream_init],[ogg])
AC_SEARCH_LIBS([vorbis_block_init],[vorbis])
AC_SEARCH_LIBS([ov_open_callbacks],[vorbisfile],,[vorbis ogg])
AC_SEARCH_LIBS([alGenBuffers],[openal])

#
# Checks for header files.
#
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h])
AC_CHECK_HEADERS([X11/Xlib.h X11/Xatom.h X11/keysym.h X11/cursorfont.h],,
	[AC_MSG_ERROR("Required X11 files not found")])

dnl TODO: put support back in for deprecated headers for unix/gl_glx.c
dnl deprecated version is checked, if first choice fails
AC_CHECK_HEADERS([X11/extensions/Xxf86dga.h],
	,
	[AC_CHECK_HEADERS([X11/extensions/xf86dga.h],
		,
		[AC_MSG_ERROR("Required X11 x86 extension file not found")],
		[#include <X11/Xlib.h #include <X11/Xproto.h> #include <X11/Xfuncproto.h>] ) ],
	[#include <X11/Xlib.h>  #include <X11/Xproto.h>])
	
AC_CHECK_HEADERS([X11/extensions/xf86vmode.h],
	,
	[AC_MSG_ERROR("Required X11 x86 extension header file not found")],
	[#include <X11/Xlib.h>])

AC_CHECK_HEADERS([GL/gl.h GL/glu.h GL/glx.h],,
	[AC_MSG_ERROR("Required OpenGL header files not found.")])

AC_CHECK_HEADERS([AL/al.h AL/alc.h],,
	[AC_MSG_ERROR("Required OpenAL header files not found.")])

AC_CHECK_HEADER([jpeglib.h],,
	[AC_MSG_ERROR("Required JPEG header files not found.")])

AC_CHECK_HEADER([vorbis/vorbisfile.h],,
	[AC_MSG_ERROR("Required Ogg-Vorbis header file not found.")])
	
AC_CHECK_HEADER([curl/curl.h],,
	[AC_MSG_ERROR("Required cURL header file not found.")])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor gethostbyname gettimeofday memmove memset mkdir munmap putenv select socket sqrt strchr strdup strerror  strcasecmp strncasecmp strrchr strdup strstr])
dnl TODO: stricmp funcs renamed with -D in source/Makefile.am, like previous make
dnl AC_CHECK_FUNCS([stricmp _stricmp])  

dnl AC_ARG_ENABLE([debug],
dnl	[AS_HELP_STRING([--enable-debug],[Turn on debugging])],
dnl	[AM_CONDITIONAL([DEBUG],[])],
dnl	[AM_CONDITIONAL([DEBUG],[])] )


AC_CONFIG_FILES([Makefile source/Makefile])
AC_OUTPUT
