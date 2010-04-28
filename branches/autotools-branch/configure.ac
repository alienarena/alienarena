# Process this file with autoconf to produce a configure script.

#
# 2010-04-25: This implementation does not fully use config.h for the first-cut 
# on 7.40 release package. Not ideal, but safer for the short term.
#

AC_PREREQ(2.65)
AC_INIT([alienarena-7.40],[7.40],[alienrace@comcast.net],[alienarena],[http://red.planetarena.org/])

dnl This may not be a good idea since /usr/local/games/bin may not be in PATH
dnl AC_PREFIX_DEFAULT([/usr/local/games])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])

dnl For building game.so, which is dlopen'd by crx and crded
LT_PREREQ([2.2.6b])
LT_INIT([disable-static dlopen])

AC_CANONICAL_HOST

dnl AS_CASE([$host],
dnl	[*-*-mingw*],
dnl		[AS_ECHO(["Win32"])],
dnl	[*-*-linux*],
dnl		[AS_ECHO(["Linux"])],
dnl	[*-*-freebsd*],
dnl		[AS_ECHO(["FreeBSD"])],
dnl	[AS_ECHO(["OS Not Known"])])

dnl AS_CASE([$host],
dnl [i686-*],
dnl		[AS_ECHO(["i686"])],
dnl 	[x86_64-*],
dnl 		[AS_ECHO(["x86_64"])],
dnl 	[AS_ECHO(["CPU Not Known"])])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_PATH_X
AC_SEARCH_LIBS(acos,[m])
AC_SEARCH_LIBS(dlopen,[dl])
AC_SEARCH_LIBS(jpeg_read_header,[jpeg])
AC_SEARCH_LIBS([ov_open_callbacks],[vorbisfile],,[vorbis ogg])
AC_SEARCH_LIBS([gluBuild2DMipmaps],[GLU])
AC_SEARCH_LIBS([XF86VidModeQueryVersion],[Xxf86vm])
AC_SEARCH_LIBS([XF86DGAQueryVersion],[Xxf86dga])
AC_SEARCH_LIBS([curl_easy_reset],[curl])

dnl Handled by AC_PATH_X (I think)
dnl AC_SEARCH_LIBS([XCreateWindow],[X11])
dnl AC_SEARCH_LIBS([],[Xext])

dnl OpenGL and OpenAL are dlopen'd, dont want -l's in link (i think)
dnl AC_SEARCH_LIBS([glXCreateContext],[GL])
dnl AC_SEARCH_LIBS([alGenBuffers],[openal])

dnl vorbisfile depends on these, but our code does not directly use them
dnl AC_SEARCH_LIBS([ogg_stream_init],[ogg])
dnl AC_SEARCH_LIBS([vorbis_block_init],[vorbis])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h])

AC_CHECK_HEADERS([X11/Xlib.h X11/Xatom.h X11/keysym.h X11/cursorfont.h],,
	[AC_MSG_ERROR("Required X11 files not found")])

dnl TODO: put support back in for deprecated headers for unix/gl_glx.c
dnl deprecated version is checked, if first choice fails
AC_CHECK_HEADER([X11/extensions/Xxf86dga.h],
	,
	[AC_CHECK_HEADER([X11/extensions/xf86dga.h],
		,
		[AC_MSG_ERROR("Required X11 x86 extension file not found")],
		[#include <X11/Xlib.h #include <X11/Xproto.h>])],
	[#include <X11/Xlib.h>  #include <X11/Xproto.h>])
	
AC_CHECK_HEADER([X11/extensions/xf86vmode.h],
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

AC_CONFIG_FILES([Makefile source/Makefile])
AC_OUTPUT

