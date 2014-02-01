# Process this file with autoconf to produce a configure script.
#
# Alien Arena auto configuration source
#
# Copyright (C) 2010,2014 COR Entertainment, LLC.
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
# A copy of the GNU General Public License is included;
# see the file, GPL2
#
# TODO: COPYING for COR licenses,
#  separate data pkg with COR data license

AC_PREREQ([2.68])
AC_INIT([alienarena],[7.66.1],[alienrace@comcast.net])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

dnl insure default prefix. TODO: when/where would this help
dnl AX_EXPAND_PREFIX

dnl 1.11.1 TODO: verify that 1.13.x or 1.14.x is not required
dnl foreign = do not apply GNU doc requirements
dnl subdir-objects = compile objects into their sub directories
dnl silent-rules = less output, default off
dnl one might add dist-zip, dist-bzip2 for other distributions besides tar.gz
AM_INIT_AUTOMAKE([1.11.1 foreign subdir-objects silent-rules])
AM_SILENT_RULES([no])

dnl use --enable-maintainer-mode to get full set of make targets.
dnl autotools generated files will not be updated otherwise.
dnl without --enable-maintainer-mode, autotools programs are
dnl not needed to build the program, which is better for
dnl non-developers who build from repository sources.
AM_MAINTAINER_MODE

dnl
dnl Options for specifying whether to include "nested"  data1 package
dnl  for make install and make distcheck.
dnl
AC_MSG_CHECKING([whether to include game resources package])
AC_ARG_WITH([data1pkg],
	[AS_HELP_STRING([--with(out)-data1pkg],
			[include game resources from data1 directory (default:with)])
		],[
		with_data1pkg=${withval}],[
		with_data1pkg=yes
])
AS_IF([ test "x${with_data1pkg}" != xno ],[
	AC_CONFIG_SUBDIRS([data1])],[
	])
AC_MSG_RESULT([${with_data1pkg}])
AM_CONDITIONAL([WITH_DATA1PKG],[ test "x${with_data1pkg}" = xyes ])

#
# Checks for programs.
#
AC_PROG_AWK
AC_PROG_SED
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_MKDIR_P
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

#
# Check host environment
#
AC_CANONICAL_HOST

unix_host=generic
AS_CASE([ ${host} ],
	[ *-*-darwin*],
		[
		unix_host=darwin
		],
	[ *-*-linux* ],
		[
		unix_host=linux
		])


dnl
dnl host-dependent conditional compile and version string settings for config.h
dnl
dnl TODO: consider adding FreeBSD
dnl
AC_DEFINE([UNIX_VARIANT],1,[unix-specific conditional compile])

AS_CASE([${unix_host}],
	[ darwin ],
		[
		AC_DEFINE([DARWIN_SPECIAL_CASE],1,[darwin-specific conditional compile])
		dnl
		dnl possible error from select() in unix/net_udp.c::Net_Sleep()
		dnl
		AC_DEFINE([_DARWIN_UNLIMITED_SELECT],1,
				[see select (2) man page for Mac OS X])
		],
	[ linux ],
		[AC_DEFINE([LINUX_SPECIAL_CASE],1,
				[linux-specific conditional compile])],
	[ generic ]
		[AC_DEFINE([GENERIC_UNIX],1,[non-specific unix conditional compile])
])

AC_DEFINE_UNQUOTED([CPUSTRING], "${host_cpu}", [Canonical CPU identification])
AC_DEFINE_UNQUOTED([BUILDSTRING], "${host_os}", [Canonical OS identification])

dnl
dnl Set names of dlopened libraries for config.h
dnl The Darwin configuration attempts to conform to MacPorts.
dnl   As of 2013-12 consult with ryandesign@macports
dnl   The dependency checks are below, under the BUILD_CLIENT conditional
dnl
AS_CASE([ ${unix_host} ],
	[ darwin ],
	  [AS_IF([ test "x${gl_dlopen_path}" = xdefault ],
			[AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
				["libGL.dylib"],
				[OpenGL dynamic lib name])
			],
			[AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
				["${gl_dlopen_path}/libGL.dylib"],
				[OpenGL dynamic lib path])
		])
		AC_DEFINE_UNQUOTED([OPENAL_DRIVER],
			["@PREFIX@/lib/libopenal.dylib"],
			[OpenAl dynamic lib path])
		],[
		AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
			["libGL.so.1"],[OpenGL shared lib name])
		AC_DEFINE_UNQUOTED([OPENAL_DRIVER],
			["libopenal.so.1"],[OpenAL shared lib name])
	])

#
# Check for MAKE options
#

dnl
dnl Dedicated server only build option.
dnl So dedicated server may be built without video/audio dependencies.
dnl
AC_MSG_CHECKING([whether to build the client])
AC_ARG_ENABLE([client],
	[AS_HELP_STRING([--disable-client],
		[build dedicated server only, not the client (default:no)])],
	[build_client=${enableval}],
	[build_client=yes])
AC_MSG_RESULT([${build_client}])
AM_CONDITIONAL([BUILD_CLIENT],[ test "x${build_client}" = xyes ])

dnl
dnl Option for documentation installation
dnl Note: distros have custom document install methods
dnl
AC_MSG_CHECKING([whether to install documents])
AC_ARG_ENABLE([doc_install],
	[AS_HELP_STRING([--enable-doc-install],
		[Enable document installation (default: yes)])],
	[doc_install=${enableval}],
	[doc_install=yes])
AC_MSG_RESULT([${doc_install}])
AM_CONDITIONAL([DOC_INSTALL],[ test "x${doc_install}" = xyes ])

dnl
dnl Option for icon installation
dnl
AC_MSG_CHECKING([whether to install icons])
AC_ARG_ENABLE([icons_install],
	[AS_HELP_STRING([--enable-icons-install],
		[Enable icons installation (default: yes)])],
	[icons_install=${enableval}],
	[icons_install=yes])
AC_MSG_RESULT([${icons_install}])
AM_CONDITIONAL([ICONS_INSTALL],[ test "x${icons_install}" = xyes ])

#
# "Precious" Environment Variables
#

#
# TO BE CONVERTED TO XDG CONVENTION
# $HOME/.local/share/cor_games
# ALWAYS cor_games, to be hard-wired in code
# For testing modify the code, or configure.ac
#
cor_games_homedir="cor_games"
AC_ARG_VAR([COR_GAMES_HOMEDIR],[
		(User configuration and data directory (default:$XDG_?/cor_games))
])

AS_IF([ test "x${COR_GAMES_HOMEDIR}" = x ],[
	COR_GAMES_HOMEDIR=${cor_games_homedir}],[
	cor_games_homedir=${COR_GAMES_HOMEDIR}
	])

AC_DEFINE_UNQUOTED([USER_GAMEDATA],["${cor_games_homedir}"],
	[Define to default path name for user configuration and downloaded data])

#
# Checks for libraries.
#

dnl ---- math
AC_SEARCH_LIBS([acos],[m])

dnl ---- dynamic 
AC_SEARCH_LIBS([dlopen],[dl])

dnl ---- real time
AC_SEARCH_LIBS([clock_gettime],[rt])

dnl
dnl start of client-only library requirements
dnl
AM_COND_IF([BUILD_CLIENT],[

dnl ---- pthread

	AX_PTHREAD([],
		[AC_MSG_ERROR(
				["Required pthreads support not found (or invalid CFLAGS)."])])
	dnl This might be a workaround for DSO missing pthread library error:
	AS_IF([ test "x${PTHREAD_LIBS}" = x ],[PTHREAD_LIBS="-pthread"])

dnl ----  X11 Libraries

	AC_PATH_X

dnl ?this probably does not need to be darwin specific?
dnl Darwin search for OpenGL library
dnl  (ref: dlopen(3) for Mac OS X.)
dnl  see above. as of 2013-12, consult with ryandesign@macports
	gl_dlopen_path=default
	AS_IF([ test "x${unix_host}" = xdarwin ],[
		AC_MSG_CHECKING([for GL_LIBDIR (location of libGL.dylib)])
		AC_ARG_VAR( GL_LIBDIR, [directory containing libGL.dylib])
		AC_MSG_RESULT([${GL_LIBDIR}])
		AS_IF([ test "x${GL_LIBDIR}" = x ],[
			gl_dlopen_path=${x_libraries}],[
			gl_dlopen_path=${GL_LIBDIR}])
	])

dnl
dnl XF86
dnl
	PKG_CHECK_MODULES([X11],[x11])

dnl
dnl XF86 VidMode extension
dnl  Note: supports full-screen display. if it is not installed, the
dnl   build will succeed but full-screen will not work.
dnl
dnl If no option is specified, then check for the library and act accordingly.
dnl If "--without-xf86vm" is specified, do not check, and disable the library.
dnl If "--with-xf86vm" is specified, check and bail out if it cannot be found.
dnl
	AC_ARG_WITH([xf86vm],
		[AS_HELP_STRING([--with(out)-xf86vm],
				[include XF86 VidMode support (default: check)])],
		[],
		[with_xf86vm=check])
	have_Xxf86vm=no
	AS_IF([ test "x${with_xf86vm}" != xno ],[
		PKG_CHECK_MODULES([XXF86VM],[xxf86vm],[
			have_Xxf86vm=yes
			AC_DEFINE([HAVE_XXF86VM],1,[Enable X86 VidMode support])
		],[
			have_Xxf86vm=no
			AS_IF([ test "x${with_xf86vm}" = xyes ],[
				AC_MSG_FAILURE(
					[--with-xf86vm specified, library or headers found])
			])
		])
	])

dnl
dnl XF86 DGA extension
dnl  Note: this is changed to default to "without",
dnl    so "--with-xf86dga" must be specified to use this library
dnl
dnl If no option is specified, disable the library.
dnl If "--without-xf86dga" is specified, do not check, and disable the library.
dnl If "--with-xf86dga" is specified, check and bail out if it cannot be found.
dnl
	AC_ARG_WITH([xf86dga],
		[AS_HELP_STRING([--with(out)-xf86dga],
			[include XF86 DGA support (default: without)])],
		[],
		[with_xf86dga=no])
	have_Xxf86dga=no
	AS_IF([ test "x$with_xf86dga" != xno ],[
		PKG_CHECK_MODULES([XXF86DGA],[xxf86dga],[
			have_Xxf86dga=yes
			AC_DEFINE([HAVE_XXF86DGA],1,[Enable X86 DGA support])
			AC_CHECK_HEADERS([X11/extensions/Xxf86dga.h],[],[],[
#include <X11/Xlib.h>
#include <X11/Xatom.h>
				])
			])
		],[
		have_Xxf86dga=no
		AS_IF([ test "x${with_xf86dga}" = xyes ],[
			AC_MSG_FAILURE(
				[--with-xf86dga specified, library or headers not found])
		])
	])


dnl ---- zlib
dnl
dnl If no option is specified, then check for the library and act accordingly.
dnl If "--without-zlib" is specified, do not check, and disable the library.
dnl If "--with-zlib" is specified, check and bail out if it cannot be found.
dnl
	AC_ARG_WITH([zlib],
		[AS_HELP_STRING([--with(out)-zlib],
				[include Zlib compression support (default: check)])],
		[],
		[with_zlib=check])
	have_zlib=no
	AS_IF([ test "x${with_zlib}" != xno ],[
		PKG_CHECK_MODULES([ZLIB],[zlib],[
			have_zlib=yes
			AC_DEFINE([HAVE_ZLIB],1,[Enable ZLib support])
		],[
			have_zlib=no
			AS_IF([ test "x${with_zlib}" = xyes ],[
				AC_MSG_FAILURE(
					[--with-zlib specified, library or headers not found])
			])
		])
	])

dnl ---- jpeg
dnl
dnl IJG libjpeg required
	AC_SEARCH_LIBS([jpeg_read_header],[jpeg],,
		[AC_MSG_ERROR(["Required jpeg library not found."])
		])

dnl ---- libcurl

dnl ---- ogg, vorbis, vorbisfile

dnl ---- freetype2

dnl ---= ODE 

	AC_MSG_CHECKING([other library dependencies])
	PKG_CHECK_MODULES([DEPS],[libcurl ogg vorbis vorbisfile freetype2 ode])
	AC_MSG_RESULT([$DEPS_CFLAGS , $DEPS_LIBS])

])
dnl
dnl end of client-only library requirements
dnl

#
# Checks for header files
#
dnl TODO: organize for portability variations
dnl priority? string.h and strings.h
dnl ?w32 manual config.h
dnl TODO: time function variants
dnl TODO: separate these logically

AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h malloc.h \
 memory.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h \
 sys/ioctl.h sys/param.h sys/socket.h time.h sys/time.h termios.h unistd.h])

AC_CHECK_HEADERS([dlfcn.h dirent.h sys/mman.h stdarg.h])

AC_CHECK_HEADERS([signal.h])

dnl
dnl ------------------------ start of client-only header requirements
dnl
dnl TODO: add more
dnl
AM_COND_IF([BUILD_CLIENT],[

dnl ---- jpeg
	jpeglib_h_found=no
	AC_CHECK_HEADERS([jpeglib.h jpeg/jpeglib.h],[ jpeglib_h_found=yes ])
	AS_IF([ test "x${jpeglib_h_found}" = xno ],
		[AC_MSG_ERROR("Required JPEG header file not found.")])

dnl ---- OpenAL
	openal_h_found=no
	AC_CHECK_HEADERS([AL/al.h al.h OpenAL/al.h],[ openal_h_found=yes ])
	AS_IF([ test "x${openal_h_found}" = xno ],
		[AC_MSG_ERROR("Required OpenAL header file not found.")])

	openalc_h_found=no
	AC_CHECK_HEADERS([AL/alc.h alc.h OpenAL/alc.h],[ openalc_h_found=yes ])
	AS_IF([ test "x${openalc_h_found}" = xno ],
		[AC_MSG_ERROR("Required OpenAL header file not found.")])

dnl ---- OpenGL
	AC_CHECK_HEADERS([GL/gl.h],,
		[AC_MSG_ERROR("Required OpenGL header files not found.")])
	AC_CHECK_HEADERS([GL/glx.h],,
		[AC_MSG_ERROR("Required OpenGL header files not found.")])

dnl ---- ODE
	AC_CHECK_HEADERS([ode/ode.h],,
		[AC_MSG_ERROR("Required ODE header file not found.")]
		[],
		[
#define dDOUBLE
		])
])
dnl
dnl ------------------------ end of client-only header requirements
dnl

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_TYPES([uintptr_t])

dnl TODO: AC_CHECK_TYPES([]) for Linux only sig typedef

#
# Checks for library functions.
#
# TODO: verify use, separate logically

AC_FUNC_ERROR_AT_LINE
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_OBSTACK

AC_CHECK_FUNCS([clock_gettime floor getcwd gethostbyname gettimeofday \
  isascii memmove memset mkdir munmap pow putenv select socket sqrt \
  strchr strdup strerror strrchr strstr strtoul])

dnl maybe remove w32 specials, just in manual w32 config.h
AC_CHECK_FUNCS([ _begin_thread _getcwd _mkdir _putenv _strdup _stricmp \
  _strnicmp unlink _unlink ])

dnl
dnl required gcc options.
dnl  fast-math because strict adherence to IEEE standard is not needed.
dnl  no-strict-aliasing because strict aliasing is not adhered to in places.
dnl
dnl TODO: eliminate these
# AX_APPEND_FLAG([-ffast-math],[CFLAGS])
# AX_APPEND_FLAG([-fno-strict-aliasing],[CFLAGS])

dnl
dnl common definitions. TODO: incorporate in sources and eliminate
dnl
AH_BOTTOM([
#if defined UNIX_VARIANT
# include "unix/custom.h"
#endif
])

#
# Outputs
#
AC_CONFIG_FILES([
		Makefile
		source/Makefile
])

AC_OUTPUT

#
# Status message
#
dnl Option to disable this message.
AC_ARG_ENABLE([build-status],[AS_HELP_STRING([--disable-build-status],
			[hide the status message at the end of the configuration script (defdault:no)])],
			[hide_status=${enableval}],[hide_status=no])

AS_IF([ test "x${hide_status}" = xno ],[
	AS_ECHO()
	AS_ECHO(["-------------------------------------------"])
	AS_ECHO(["Package: ${PACKAGE_NAME} Version: ${PACKAGE_VERSION}"])
	AS_ECHO([" OS: .................... ${host_os}"])
	AS_ECHO([" CPU: ................... ${host_cpu}"])
	AS_IF([ test "x${build_client}" = xyes ],[
			dedicated_only=no],[
			dedicated_only=yes])
	AS_ECHO([" Dedicated Only: ........ ${dedicated_only}"])

	AM_COND_IF([BUILD_CLIENT],[
		AS_ECHO([" XF86 DGA support: ...... ${have_Xxf86dga}"])
		AS_ECHO([" XF86 VidMode support: .. ${have_Xxf86vm}"])
		AS_ECHO([" Zlib support: .......... ${have_zlib}"])
		AS_IF([ test "x${unix_host}" = xdarwin ],[
			AS_ECHO([" GL_LIBDIR: ............. ${gl_dlopen_path}"])
			])
	])
	AS_ECHO([" prefix ................ ${prefix}"])
	AS_ECHO([" datarootdir:........... ${datarootdir}"])
	AS_ECHO([" datadir: .............. ${datadir}"])
	AS_ECHO([" bindir: ............... ${bindir}"])
	AS_ECHO([" doc-install ........... ${doc_install}"])
	AS_ECHO([" docdir: ............... ${docdir}"])
	AS_ECHO([" icons-install: ........ ${icons_install}"])
	AS_ECHO([" cor_iconsdir: ......... ${cor_iconsdir}"])
	AS_ECHO([" User Home R/W Data:.... ~/${cor_games_homedir}"])
	AS_ECHO()
	AS_ECHO(["Advanced Build Information:"])
	AS_ECHO([" See README for information and recommendations."])
	AS_ECHO([" Run ./configure --help for configure options."])
	AS_ECHO([" Compiler options:"])
	AS_ECHO(["  CFLAGS:.... ${CFLAGS}"])
	AS_ECHO()
])
