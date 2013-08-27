# Process this file with autoconf to produce a configure script.
#
# Alien Arena auto configuration source
#
# Copyright (C) 2010 COR Entertainment, LLC.
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
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# configure.ac for
#  GNU/Linux, Unix
#  Windows (32-bit) using MinGW - Experimental
#  Mac OS X / Darwin using X11 - Experimental

AC_PREREQ([2.69])
AC_INIT([alienarena],[7.66],[alienrace@comcast.net])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

dnl insure default prefix
AX_EXPAND_PREFIX

dnl 1.14 = require automake 1.14 or greater
dnl foreign = do not apply GNU doc requirements
dnl subdir-objects = compile objects into their sub directories
dnl silent-rules = less output (--disable-silent-rules for more output)
dnl add dist-zip, dist-bzip2 for other distributions besides tar.gz
AM_INIT_AUTOMAKE([ 1.14 foreign subdir-objects silent-rules ])
AM_SILENT_RULES([yes])

dnl use --enable-maintainer-mode to get full set of make targets.
dnl autotools generated files will not be updated otherwise.
dnl without --enable-maintainer-mode, autotools programs are not needed
dnl to build the program, which is better for non-developers.   
AM_MAINTAINER_MODE

AC_CANONICAL_HOST

#
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_AWK
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
# check host environment
#
cond_build_win32=no
cond_build_unix=no
unix_host=generic
AS_CASE([ ${host} ],
	[ *-*-mingw* ],[ cond_build_win32=yes ],
	[ *-*-darwin*],
		[
		cond_build_unix=yes
		unix_host=darwin
		],
	[ *-*-linux* ],
		[
		cond_build_unix=yes
		unix_host=linux
		],
	[ cond_build_unix=yes ])

AM_CONDITIONAL([BUILD_WIN32],[ test "x${cond_build_win32}" = xyes ])
AM_CONDITIONAL([BUILD_UNIX],[ test "x${cond_build_unix}" = xyes ])

#
# Environment variable for specifying libGL.dylib path for Darwin when it
# is not in the dlopen() search path. see dlopen (3) for Mac OS X.
# Either export or in command line: GL_LIBDIR=/usr/Xll/lib, for example.
# If this GL_LIBDIR is not given, use the shell variable x_libraries from
# the AC_PATH_X macro (which may be empty)
#
gl_dlopen_path=default
AS_IF([ test x"${unix_host}" = xdarwin ],
	[
	AC_PATH_X
	AC_MSG_CHECKING([for GL_LIBDIR (location of libGL.dylib)])
	AC_ARG_VAR( GL_LIBDIR, [directory containing libGL.dylib])
	AC_MSG_RESULT([${GL_LIBDIR}])
	AS_IF([ test x"${GL_LIBDIR}" = x ],
		[gl_dlopen_path=${x_libraries}],
		[gl_dlopen_path=${GL_LIBDIR}])
])

#
# Unix/Linux/Darwin dedicated server only build option
#
AC_MSG_CHECKING([whether to build the client])
AM_COND_IF([BUILD_UNIX],
	[
	AC_ARG_ENABLE(
		[client],
		[AS_HELP_STRING([--disable-client],
			[build dedicated server only, not the client (default:no)])],
		[build_client=${enableval}],
		[build_client=yes])
	],
	[build_client=yes])
AC_MSG_RESULT([${build_client}])
AM_CONDITIONAL([BUILD_CLIENT],[ test "x${build_client}" = xyes ])

#
# Alternate Install. Install in the traditional single directory way.
# Might be required for map making tools to work. Also, good when updating
# from SVN regularly. Affects how Makefile sets DATADIR and does make-install.
# for now, win32/mingw is always alternate install
#
AM_COND_IF([BUILD_WIN32],
	[alternate_install=yes],
	[
	AC_MSG_CHECKING([for traditional single directory, in place installation])
	AC_ARG_ENABLE(
		[alternate_install],
		[AS_HELP_STRING([--enable-alternate-install],
			[traditional single directory, in-place installation (default:no)])],
		[alternate_install=${enableval}],
		[alternate_install=no])
	AC_MSG_RESULT([${alternate_install}])
])

AM_CONDITIONAL([ALTERNATE_INSTALL],[ test "x${alternate_install}" = xyes ])

dnl
dnl symbols for config.h
dnl
AM_COND_IF([BUILD_UNIX],
	[
	AC_DEFINE([UNIX_VARIANT],1,[unix-specific conditional compile])

	AS_CASE([ ${unix_host} ],
		[ darwin ],
			[
			AC_DEFINE([DARWIN_SPECIAL_CASE],1,[darwin-specific conditional compile])
			dnl
			dnl possible error from select() in unix/net_udp.c::Net_Sleep()
			dnl
			AC_DEFINE([_DARWIN_UNLIMITED_SELECT],1,[see select (2) man page for Mac OS X])
			],
		[ linux ],
			[AC_DEFINE([LINUX_SPECIAL_CASE],1,[linux-specific conditional compile])],
		[ generic ]
			[AC_DEFINE([GENERIC_UNIX],1,[non-specific unix conditional compile])])

	AC_ARG_ENABLE([ansi-color],
	[AS_HELP_STRING([--enable-ansi-color],[ANSI terminal color (default: no)])],
	[ansi_color=${enableval}],[ansi_color=no])
	AS_IF([ test "x$ansi_color" = "xyes" ],[
		AC_DEFINE([ANSI_COLOR],1,[Enable ANSI terminal color codes in stdout])
		])
])

AM_COND_IF([BUILD_WIN32],
	[AC_DEFINE([WIN32_VARIANT],1,[win32-specific conditional compile])])

AC_DEFINE_UNQUOTED(CPUSTRING, "${host_cpu}", [Canonical CPU identification])
AC_DEFINE_UNQUOTED(BUILDSTRING, "${host_os}", [Canonical OS identification])

#
# ODE (Open Dynamic Engine) Library
#
# if no option or "--without-system-libode" is specified, use the recommended
#  integrated ODE.
# if "--with-system-libode" is specified, check. look under BUILD_CLIENT
#  for details.
#
AC_ARG_WITH([system-libode],
	[AS_HELP_STRING([--with(out)-system-libode],
		[use system-supplied libode (default:without)])],
	[system_ode=${withval}],
	[system_ode=no])

#
# Option for disabling installation of documents to allow for custom
# documentation installation. Normally only useful for distro packagers.
# Do not put in status message, because  documents probably are being 
# installed in a custom way.
#
AC_ARG_ENABLE([documents],
	[AS_HELP_STRING([--disable-documents],
			[Disable document installation (default: no)])],
	[no_docs=${documents}],
	[no_docs=no])
AM_CONDITIONAL([INSTALL_DOCS],[ test "x${no_docs}" = "xno" ])

#
# Option for specifying where icon is installed.
#
AC_ARG_WITH(icondir,
	[AS_HELP_STRING([--with-icondir=DIR],
		[icon install directory (default:DATADIR/icons)])],
	[ with_icondir=${withval} ], [ with_icondir=${datadir}/icons ])
AC_SUBST([icondir], [${with_icondir}])

#
# User writable data and configuration subdirectory in $HOME
#  normally this is a hidden subdirectory
#  default is .codered. but distros modify this
#  can be overridden by environment variable, COR_GAME
#
# (These also are #defined in qcommon.h, but changing them is not a good idea
# BASE_GAMEDATA  "data1", GAME_GAMEDATA  "arena", BOT_GAMEDATA   "botinfo")
#
# AC_ARG_VAR creates ALIENARENA_HOMEDIR entry for ./configure --help
# AC_DEFINE_UNQUOTED adds the setting to config.h,
#  overriding the define in qcommon.h
#   and
#  using the default, if not specified in the commandline
#  (Note: could also be specified in enviroment. That is not recommended
#   but is the reason for the package specific variable name.)
#
alienarena_homedir=".codered"
AC_ARG_VAR([ALIENARENA_HOMEDIR],[User data and cfg subdirectory in $HOME])
AS_IF([ test "x${ALIENARENA_HOMEDIR}" = "x" ],
	[ALIENARENA_HOMEDIR=${alienarena_homedir}],
	[alienarena_homedir=${ALIENARENA_HOMEDIR}])
AC_DEFINE_UNQUOTED([USER_GAMEDATA],["${alienarena_homedir}"],
	[User data and cfg subdirectory in $HOME])

#
# Checks for libraries.
#
AC_SEARCH_LIBS([acos],[m])
AC_SEARCH_LIBS([dlopen],[dl])
AC_SEARCH_LIBS([clock_gettime],[rt])

dnl
dnl start of client-only library requirements
dnl
AM_COND_IF([BUILD_CLIENT],
	[

	AX_PTHREAD([],[AC_MSG_ERROR(["Required pthreads support not found."])])
#
# This might be a workaround for DSO missing pthread library error
#
	AS_IF([ test "x$PTHREAD_LIBS" = "x" ],[PTHREAD_LIBS="-pthread"])

	dnl
	dnl zlib support
	dnl
	dnl If no option is specified, then check for the library and act accordingly.
	dnl If "--without-zlib" is specified, do not check, and disable the library.
	dnl If "--with-zlib" is specified, check and bail out if it cannot be found.
	dnl
	AC_ARG_WITH([zlib],
		[AS_HELP_STRING([--with(out)-zlib],[include Zlib compression support (default: check)])],
		[],
		[with_zlib=check])
	have_zlib=no
	AS_IF([ test "x$with_zlib" != "xno" ],[
		PKG_CHECK_MODULES([ZLIB],[zlib],[
			have_zlib=yes
			AC_DEFINE([HAVE_ZLIB],1,[Enable ZLib support])
		],[
			have_zlib=no
			AS_IF([ test "x$with_zlib" = "xyes" ],[
				AC_MSG_FAILURE([--with-zlib specified, but the library or headers could not be found])
			])
		])
	])


	dnl IJG libjpeg required  
	AC_SEARCH_LIBS([jpeg_read_header],[jpeg],,
		[AC_MSG_ERROR(["Required jpeg library not found."])])


	AM_COND_IF([BUILD_UNIX],
	[
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
			[AS_HELP_STRING([--with(out)-xf86vm],[include XF86 VidMode support (default: check)])],
			[],
			[with_xf86vm=check])
		have_Xxf86vm=no
		AS_IF([ test "x$with_xf86vm" != "xno" ],[
			PKG_CHECK_MODULES([XXF86VM],[xxf86vm],[
				have_Xxf86vm=yes
				AC_DEFINE([HAVE_XXF86VM],1,[Enable X86 VidMode support])
			],[
				have_Xxf86vm=no
				AS_IF([ test "x$with_xf86vm" = "xyes" ],[
					AC_MSG_FAILURE([--with-xf86vm specified, but the library or headers could not be found])
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
			[AS_HELP_STRING([--with(out)-xf86dga],[include XF86 DGA support (default: without)])],
			[],
			[with_xf86dga=no])
		have_Xxf86dga=no
		AS_IF([ test "x$with_xf86dga" != "xno" ],[
			PKG_CHECK_MODULES([XXF86DGA],[xxf86dga],[
				have_Xxf86dga=yes
				AC_DEFINE([HAVE_XXF86DGA],1,[Enable X86 DGA support])
				AC_CHECK_HEADERS([X11/extensions/Xxf86dga.h],[],[],[
#include <X11/Xlib.h>
#include <X11/Xatom.h>
])
			],[
				have_Xxf86dga=no
				AS_IF([ test "x$with_xf86dga" = "xyes" ],[
					AC_MSG_FAILURE([--with-xf86dga specified, but the library or headers could not be found])
				])
			])
		])
	])

#
# if option to use system libode, rather than integrated ODE, is selected
#  use pkg-config to get cflags and libs. do not default to
#  integrated ODE on failure, as that is probably not what is wanted.
#
# for recommended integrated ODE build, setup client dependencies,
#  and configuration. Double precision math is "hardwired" here and
#  is not considered to be a user-specifiable option.
#
	AS_IF([ test "x$system_ode" = "xyes" ],[
		PKG_CHECK_MODULES([ODE],[ode],,[
			AC_MSG_FAILURE(
				[--with-system-libode specified, but the library or headers could not be found])
			])
		],[
		AC_SUBST([ODE_CFLAGS],[-DdDOUBLE])
		AC_DEFINE([dEpsilon],[DBL_EPSILON],[Define to DBL_EPSILON for ODE])
	])

	dnl
	dnl Check common client dependencies:
	dnl Curl, Ogg/Vorbis and vorbisfile
	dnl
	PKG_CHECK_MODULES([DEPS],[libcurl ogg vorbis vorbisfile freetype2])

	dnl Hack in the extras for Win32
	dnl TODO: this may not be the best way to do this
	win32_syslibs=""
	AM_COND_IF([BUILD_WIN32],
		[ win32_syslibs="-lmingw32 -lwinmm -lwsock32 -lws2_32 -lgdi32 -luser32"])
	AC_SUBST([WIN32_LIBS],[$win32_syslibs])


])

AM_CONDITIONAL([USE_SYSTEM_LIBODE],[ test "x$system_ode" = "xyes" ])

dnl
dnl end of client-only library requirements
dnl

#
# Checks for header files
#

AC_CHECK_HEADERS(
	[inttypes.h stdint.h stddef.h stdlib.h unistd.h],
	[],[],[])

AC_CHECK_HEADERS(
	[float.h limits.h],
	[],[],[])

AC_CHECK_HEADERS(
	[arpa/inet.h netinet/in.h sys/ioctl.h sys/socket.h netdb.h termios.h sys/select.h]
	[],[],[])

AC_CHECK_HEADERS(
	[dlfcn.h fcntl.h malloc.h string.h sys/param.h sys/stat.h]
	[],[],[])

AC_CHECK_HEADERS(
	[time.h sys/time.h]
	[],[],[])

AC_CHECK_HEADERS([windows.h winsock.h winsock2.h])

dnl
dnl start of client-only header requirements
dnl
AM_COND_IF([BUILD_CLIENT],[

	jpeglib_h_found=no
	AC_CHECK_HEADERS([jpeglib.h jpeg/jpeglib.h],[ jpeglib_h_found=yes ])
	AS_IF([ test "x${jpeglib_h_found}" = xno ],
		[AC_MSG_ERROR("Required JPEG header file not found.")])

	openal_h_found=no
	AC_CHECK_HEADERS([AL/al.h al.h OpenAL/al.h],[ openal_h_found=yes ])
	AS_IF([ test "x${openal_h_found}" = xno ],
		[AC_MSG_ERROR("Required OpenAL header file not found.")])

	openalc_h_found=no
	AC_CHECK_HEADERS([AL/alc.h alc.h OpenAL/alc.h],[ openalc_h_found=yes ])
	AS_IF([ test "x${openalc_h_found}" = xno ],
		[AC_MSG_ERROR("Required OpenAL header file not found.")])

	AC_CHECK_HEADERS([GL/gl.h],,
		[AC_MSG_ERROR("Required OpenGL header files not found.")])
	AM_COND_IF([BUILD_UNIX],
		[AC_CHECK_HEADERS([GL/glx.h],,
			[AC_MSG_ERROR("Required OpenGL header files not found.")
		])
	])
])
dnl
dnl end of client-only header requirements
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


#
# Checks for library functions.
#
AC_FUNC_ERROR_AT_LINE
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_OBSTACK

dnl network functions
AC_CHECK_FUNCS([closesocket socket gethostbyname select])

dnl thread functions
AC_CHECK_FUNCS([pthread_create _begin_thread])

dnl string functions
AC_CHECK_FUNCS([strchr strerror strrchr strstr strlcpy isascii])
AC_CHECK_FUNCS([strcasecmp strncasecmp ])
AC_CHECK_FUNCS([strdup _strdup stricmp _stricmp strnicmp _strnicmp])

dnl file system functions
AC_CHECK_FUNCS([filelength stat fstat getcwd _getcwd unlink _unlink mkdir _mkdir])

dnl math functions
AC_CHECK_FUNCS([pow sqrt floor])

dnl mem mgmt functions
AC_CHECK_FUNCS([memmove memset munmap mremap])

dnl time functions
AC_CHECK_FUNCS([gettimeofday clock_gettime])

dnl other functions
AC_CHECK_FUNCS([ putenv _putenv])

dnl
dnl Set names of dlopened libraries for config.h
dnl
AM_COND_IF([BUILD_UNIX],
	[AS_CASE([ ${unix_host} ],
		[ darwin ],
			[AS_IF([ test x"${gl_dlopen_path}" = xdefault ],
				[AC_DEFINE([OPENGL_DRIVER],
					["libGL.dylib"],
					[OpenAL dynamic lib name])
				],
				[AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
					["${gl_dlopen_path}/libGL.dylib"],
					[OpenGL dynamic lib path])
			])

			AC_DEFINE([OPENAL_DRIVER],
				["/System/Library/Frameworks/OpenAL.framework/OpenAL"],
				[OpenAl dynamic lib path])
			],

		[
		AC_DEFINE([OPENGL_DRIVER],
			["libGL.so.1"],[OpenGL shared lib name])

		AC_DEFINE([OPENAL_DRIVER],
			["libopenal.so.1"],[OpenAL shared lib name])
	])
])

AM_COND_IF([BUILD_WIN32],
	[
	AC_DEFINE([OPENGL_DRIVER],["opengl32"],[OpenGL DLL name])

	AC_DEFINE([OPENAL_DRIVER],["OpenAL32.dll"],[OpenAL dll name])
])

#
# required gcc/g++ options.
#  fast-math because strict adherence to IEEE standard is not needed.
#  no-strict-aliasing because strict aliasing is not adhered to in places.
#
AX_APPEND_FLAG([-ffast-math],[CFLAGS])
AX_APPEND_FLAG([-ffast-math],[CXXFLAGS])
AX_APPEND_FLAG([-fno-strict-aliasing],[CFLAGS])
AX_APPEND_FLAG([-fno-strict-aliasing],[CXXFLAGS])

# outputs
AC_CONFIG_FILES([ Makefile source/Makefile ])

AC_OUTPUT

dnl Option to disable this message.
AC_ARG_ENABLE([build-status],[AS_HELP_STRING([--disable-build-status],
			[hide the status message at the end of the configuration script (default:no)])],
			[hide_status=${enableval}],[hide_status=no])

AS_IF([test "x$hide_status" = "xno"],[
	AS_ECHO()
	AS_ECHO(["-------------------------------------------"])
	AS_ECHO(["Package: ${PACKAGE_NAME} Version: ${PACKAGE_VERSION}"])
	AS_ECHO([" OS: .................... ${host_os}"])
	AS_ECHO([" CPU: ................... ${host_cpu}"])
	AM_COND_IF([BUILD_WIN32],,[
		AS_IF([ test "x$build_client" = "xyes" ],[dedicated_only=no],[dedicated_only=yes])
		AS_ECHO([" Dedicated Only: ........ ${dedicated_only}"])
		AS_ECHO([" Server terminal color: . ${ansi_color}"])
	])
	AS_ECHO([" Alternate install: ..... ${alternate_install}"])

	AM_COND_IF([ALTERNATE_INSTALL],[
		AM_COND_IF([BUILD_CLIENT],[
			AS_ECHO([" System ODE library ..... ${system_ode}"])
			AS_ECHO([" XF86 DGA support: ...... ${have_Xxf86dga}"])
			AS_ECHO([" XF86 VidMode support: .. ${have_Xxf86vm}"])
			AS_ECHO([" Zlib support: .......... ${have_zlib}"])
			])
		AS_ECHO([" Shared Data ............ ${srcdir}"])
		AS_ECHO(["   note: with alternate install, data is located relative to CWD."])
		AS_ECHO([" User Home R/W Data ..... ${alienarena_homedir}"])
		],[
dnl
dnl try to generate the full path for automake pkgdatadir
dnl  which determines DATADIR that locates data1, arena, and botinfo
dnl
		game_data=${datadir}/${PACKAGE_NAME}
		game_data_a=`eval "echo ${game_data}"`
		game_data=`eval "echo ${game_data_a}"`
		AM_COND_IF([BUILD_CLIENT],[
			AS_ECHO([" System ODE library ..... ${system_ode}"])
			AS_ECHO([" XF86 DGA support: ...... ${have_Xxf86dga}"])
			AS_ECHO([" XF86 VidMode support: .. ${have_Xxf86vm}"])
			AS_ECHO([" Zlib support: .......... ${have_zlib}"])
			AS_IF([ test x"${unix_host}" = xdarwin ],[
				AS_ECHO([" GL_LIBDIR: ............. ${gl_dlopen_path}"])
			])
		])
		AS_ECHO([" Prefix ................. ${prefix}"])
		AS_ECHO([" Shared RO Data: ........ ${game_data}"])
		AS_ECHO([" User Home R/W Data ..... ${alienarena_homedir}"])
	])

	AS_ECHO()		

	AM_COND_IF([ALTERNATE_INSTALL],
		[AS_ECHO(["Run \"make\" to build, then \"make install-alternate\" to install."])],
		[AS_ECHO(["Run \"make\" to build, then \"sudo make install\" to install."])])
		
	AS_ECHO()		
	AS_ECHO(["Advanced Build Information:"])
	AS_ECHO([" See README for information and recommendations."])
	AS_ECHO([" Run ./configure --help for configure options."])
	AS_ECHO([" Compiler options:"])
	AS_ECHO(["  CFLAGS:.... ${CFLAGS}"])
	AS_ECHO(["  CXXFLAGS:.. ${CXXFLAGS}"])
	AS_ECHO()		

	AS_ECHO(["--------------------------------------------"])
])
