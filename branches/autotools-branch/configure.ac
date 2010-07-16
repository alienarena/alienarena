# Alien Arena auto configuration source
# Process this file with autoconf to produce a configure script.
#
# Copyright (C) 2010 COR Entertainment
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# 
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

# configure.ac for
#  Linux/Unix
#  Windows using MinGW 
#  Mac OSX Darwin 

AC_PREREQ(2.61)
AC_INIT([alienarena],[7.41alpha],[alienrace@comcast.net],[alienarena])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])

AM_SILENT_RULES([yes])

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

# Unix/Linux host dedicated server only build option
AC_MSG_CHECKING([for dedicated server only build])
no_client=no
AC_ARG_ENABLE( 
	[no_client],
	[AS_HELP_STRING([--enable-no-client],
		[build dedicated server only, not the client (default:no)])],
	[no_client=${enableval}],
	[no_client=no])
AC_MSG_RESULT([${no_client}])

# Alternate Install. Install in the traditional single directory way.
# Probably required for map making tools to work. Also, good when updating
# from SVN regularly. Affects how Makefile sets DATADIR and does make-install.
#  
AC_MSG_CHECKING([for alternate_install])	
alternate_install=no
AC_ARG_ENABLE(
	[alternate_install],
	[AS_HELP_STRING([--enable-alternate-install],
		[traditional single directory, in-place installation (default:no)])],
	[alternate_install=${enableval}],
	[alternate_install=no])
AC_MSG_RESULT([${alternate_install}])
AM_CONDITIONAL(ALTERNATE_INSTALL, test "x${alternate_install}" = xyes)
 	

#
# check host environment
#
cond_build_win32=no
cond_build_unix=no
cond_build_darwin=no
AS_CASE([ ${host} ],
	[ *-*-mingw* ],[ cond_build_win32=yes ],
	[ *-*-darwin*],[ cond_build_darwin=yes cond_build_unix=yes],
	[ cond_build_unix=yes ])

AC_MSG_CHECKING(host)
AC_MSG_RESULT(${host})
AC_MSG_CHECKING(whether to build win32 version)
AC_MSG_RESULT(${cond_build_win32})
AC_MSG_CHECKING(whether to build unix version)
AC_MSG_RESULT(${cond_build_unix})
AC_MSG_CHECKING(whether to build darwin version)
AC_MSG_RESULT(${cond_build_darwin})

AC_MSG_CHECKING(whether to build client)
cond_build_client=yes
AS_IF( [ test "x${no_client}" != xno ],
	[AS_IF( [ test "x${cond_build_unix}" = xyes ],[cond_build_client=no],
		[AC_MSG_WARN( "--no_client option valid only for unix build" )])])
AC_MSG_RESULT(${cond_build_client});	

AM_CONDITIONAL( BUILD_WIN32, test "x${cond_build_win32}" = xyes )
AM_CONDITIONAL( BUILD_UNIX,  test "x${cond_build_unix}" = xyes )
AM_CONDITIONAL( BUILD_CLIENT, test "x${cond_build_client}" = xyes )
AM_CONDITIONAL( BUILD_DARWIN, test "x${cond_build_darwin}" = xyes )

dnl symbols for config.h
AS_IF([ test "x${cond_build_unix}" = xyes ],
	[AC_DEFINE([UNIX_VARIANT],1,[unix-specific conditional compile])])

AS_IF([ test "x${cond_build_win32}" = xyes ],
	[AC_DEFINE([WIN32_VARIANT],1,[win32-specific conditional compile])])
	
dnl for cases where darwin differs from unix/linux	
AS_IF([ test "x${cond_build_darwin}" = xyes ],
	[AC_DEFINE([DARWIN_SPECIAL_CASE],1,[darwin-specific conditional compile])])

AC_DEFINE_UNQUOTED(CPUSTRING, "${host_cpu}", [Canonical CPU identification])
AC_DEFINE_UNQUOTED(BUILDSTRING, "${host_os}", [Canonical OS identification])

#
# Checks for libraries.
#

# TODO: this may not be the best way to do this
win32_syslibs=""
AS_IF([ test "x${cond_build_win32}" = xyes ],
	[ win32_syslibs="-lglu32 -lmingw32 -lwinmm -lwsock32 -lws2_32 -lgdi32 -luser32"])
AC_SUBST([WIN32_LIBS],[$win32_syslibs])

AS_IF([ test "x${cond_build_win32}" = xyes ],
	[AC_DEFINE([OPENGL_DRIVER],["opengl32"],[OpenGL DLL name])])

dnl 2010-07-06 this is just a wild guess
darwin_syslibs=""
AS_IF([ test "x${cond_build_darwin}" = xyes ],
	[ darwin_syslibs="-framework OpenGL -framework OpenAL"])
AC_SUBST([DARWIN_LIBS],[$darwin_syslibs])	

AC_SEARCH_LIBS(acos,[m])
AC_SEARCH_LIBS(pthread_mutex_init,[pthread])
AC_SEARCH_LIBS(dlopen,[dl])

dnl
dnl start of client-only library requirements
dnl
AS_IF( [ test "x${cond_build_client}" = xyes ],
[
AC_SEARCH_LIBS(zlibVersion,[z zlib],,
	[AC_MSG_ERROR(["Required zlib library not found."])])

AC_SEARCH_LIBS(jpeg_read_header,[jpeg],,
	[AC_MSG_ERROR(["Required jpeg library not found."])])

AC_SEARCH_LIBS([ogg_stream_init],[ogg],,
	[AC_MSG_ERROR(["Required ogg library not found."])])
		
AC_SEARCH_LIBS([vorbis_block_init],[vorbis],,
	[AC_MSG_ERROR(["Required vorbis library not found."])])

AC_SEARCH_LIBS([ov_open_callbacks],[vorbisfile],,
	[AC_MSG_ERROR(["Required vorbisfile library not found."])],
	[ -lvorbis -logg ])

dnl theory: darwin always has openal in a framework
dnl: TODO: find out what the name of the darwin openal driver is
AS_IF([ test "x${cond_build_darwin}" = xno ],
	[AC_SEARCH_LIBS([alGenBuffers],[openal openal32 openal-soft],,
		[AC_MSG_ERROR(["Required openal library not found."])])])		
	
AS_IF([ test "x${cond_build_unix}" = xyes ],
[

dnl TODO: find real name of darwin framework dynamic lib
AS_IF( [ test "x${cond_build_darwin}" = xyes ],
	[AC_DEFINE([OPENGL_DRIVER],["libopengl.dylib"],[OpenGL dynamic lib name])],
	[AC_DEFINE([OPENGL_DRIVER],["libGL.so.1"],[OpenGL shared lib name])])
])

dnl TODO: find real name of darwin framework dynamic lib
AS_IF( [ test "x${cond_build_darwin}" = xyes ],
	[AC_DEFINE([OPENAL_DRIVER],["libopenal.dylib"],[OpenAL dynamic lib name])],
	[AC_DEFINE([OPENAL_DRIVER],["libopenal.so.1"],[OpenAL shared lib name])])

]) dnl unix
	
AS_IF([ test "x${cond_build_win32}" = xyes ],
	[AC_DEFINE([OPENAL_DRIVER],["OpenAL32.dll"],[OpenAL dll name])])

curl_xtralibs=""
AS_IF( [ test "x${cond_build_win32}" = xyes ],
	[curl_xtralibs=" -lz -lwsock32"])

AC_SEARCH_LIBS([curl_easy_reset],[curl],,
	[AC_MSG_WARN(["Recommended curl library not found."])],
	[ $curl_xtralibs ]) 

AC_PATH_X

AC_SEARCH_LIBS([XF86VidModeQueryVersion],[Xxf86vm])

have_Xxf86dga=no
AC_SEARCH_LIBS([XF86DGAQueryVersion],[Xxf86dga],[have_Xxf86dga=yes])

dnl opengl32 and glu32 handled as win32 system libs above
dnl darwin framework opengl also above
AS_IF( [ test "x${cond_build_win32}" = xno -a "x${cond_build_darwin}" = xno ],
[
AC_SEARCH_LIBS([glGetError],[GL],,
	[AC_MSG_ERROR(["Required OpenGL library not found."])])
	
AC_SEARCH_LIBS([gluGetString],[GLU],,
	[AC_MSG_ERROR(["Required GLU library not found."])])
	
])	
dnl 
dnl end of client-only library requirements
dnl

#
# Checks for header files
# TODO: verify these are used and needed
#
AC_CHECK_HEADERS(
	[arpa/inet.h netinet/in.h sys/ioctl.h sys/socket.h unistd.h netdb.h termios.h])

AC_CHECK_HEADERS(
	[ fcntl.h float.h limits.h malloc.h stddef.h stdlib.h string.h sys/param.h dnl 
	sys/time.h unistd.h sys/stat.h ])

dnl	
dnl start of client-only header requirements
dnl	
AS_IF( [ test "x${cond_build_client}" = xyes ],
[

dnl for Darwin, using macports jpeg6b. TODO: is jpeg8 available?
jpeglib_h_found=no
AC_CHECK_HEADERS([jpeglib.h jpeg/jpeglib.h jpeg6b/include/jpeglib.h],[ jpeglib_h_found=yes ])
AS_IF([ test "x${jpeglib_h_found}" = xno ],
	[AC_MSG_ERROR("Required JPEG header file not found.")])

AC_CHECK_HEADERS([vorbis/vorbisfile.h],,
	[AC_MSG_ERROR("Required Ogg-Vorbis header file not found.")])

openal_h_found=no
AC_CHECK_HEADERS([AL/al.h al.h OpenAL/al.h],[ openal_h_found=yes ])
AS_IF([ test "x${openal_h_found}" = xno ],
	[AC_MSG_ERROR("Required OpenAL header file not found.")])

openalc_h_found=no
AC_CHECK_HEADERS([AL/alc.h alc.h OpenAL/alc.h],[ openalc_h_found=yes ])
AS_IF([ test "x${openalc_h_found}" = xno ],
	[AC_MSG_ERROR("Required OpenAL header file not found.")])

AC_CHECK_HEADERS([curl/curl.h],,
	[AC_MSG_ERROR("Required cURL header file not found.")])

AS_IF([ test "x${cond_build_unix}" = xyes ],
[
 		
AC_CHECK_HEADERS([X11/Xlib.h X11/Xatom.h X11/keysym.h X11/cursorfont.h],,
	[AC_MSG_ERROR("Required X11 header files not found")])

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
])
	
AC_CHECK_HEADERS([GL/gl.h GL/glu.h],,
	[AC_MSG_ERROR("Required OpenGL header files not found.")])

dnl TODO: mingw has this, figure out the right way to do it
AS_IF([ test "x${cond_build_unix}" = xyes ],
	[AC_CHECK_HEADERS([GL/glx.h],,
		[AC_MSG_ERROR("Required OpenGL header files not found.")])])	

])
dnl 
dnl end of client-only header requirements
dnl

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
#  note: redefines __inline or __inline__ to inline if compiler wants that
AC_C_INLINE
#  note: defines a suitable type for size_t if system headers do not
AC_TYPE_SIZE_T
#  note: defines a suitable type uid_t and gid_t if system headers do not 
AC_TYPE_UID_T
AC_CHECK_TYPES([ptrdiff_t])

#
# Checks for library functions.
# note: some checks to help in manual edit of config.h for Win32/MSVC build
#
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC

# network functions
AC_CHECK_FUNCS([ closesocket socket gethostbyname ]) 

# thread functions
AC_CHECK_FUNCS([ pthread_create _begin_thread ])

# string functions
AC_CHECK_FUNCS([ strchr strdup _strdup strerror stricmp _stricmp dnl 
		strcasecmp strncasecmp strrchr strstr strnicmp strlcpy ]) dnl
		
# file system functions
AC_CHECK_FUNCS([ filelength stat fstat getcwd unlink _unlink mkdir ])
		
AC_CHECK_FUNCS([ floor gettimeofday memmove memset munmap mremap dnl
		putenv _putenv select ]) 

# outputs
AC_CONFIG_FILES([Makefile source/Makefile])

AC_OUTPUT
