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
#  Mac OS X / Darwin using X11

AC_PREREQ(2.61)
AC_INIT([alienarena],[7.46.alpha2],[alienrace@comcast.net],[alienarena])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

dnl 1.11 = require automake 1.11 or greater
dnl foreign = do not apply GNU doc requirements
dnl subdir-objects = compile objects into their sub directories
dnl silent-rules = less output (--disable-silent-rules for more output)
dnl dist-zip = build .zip distribution in addition to .tar.gz 
AM_INIT_AUTOMAKE([ 1.11 foreign subdir-objects silent-rules dist-zip ])
AM_SILENT_RULES([yes])

dnl use --enable-maintainer-mode to get full set of make targets
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


#
# OpenAL package options
#
AC_MSG_CHECKING([whether to use Apple OpenAL])
AC_ARG_WITH([apple-openal],
	[AS_HELP_STRING(
		[--with-apple-openal],
		[use Apple OpenAL on OS X (default: OS X:yes, otherwise:no)])
	],
	[apple_openal=${withval}],
	[AS_IF([ test x${unix_host} = xdarwin ],
		[apple_openal=yes],
		[apple_openal=no])
	])
AC_MSG_RESULT([${apple_openal}])	

dnl
dnl symbols for config.h
dnl

dnl The special case symbols may not be needed. Might be handy for debug, though.
AM_COND_IF([BUILD_UNIX], 
	[
	AC_DEFINE([UNIX_VARIANT],1,[unix-specific conditional compile])
	
	AS_CASE([ ${unix_host} ],
		[ darwin ],
			[AC_DEFINE([DARWIN_SPECIAL_CASE],1,[darwin-specific conditional compile])],
		[ linux ],
			[AC_DEFINE([LINUX_SPECIAL_CASE],1,[linux-specific conditional compile])],
		[ generic ]
			[AC_DEFINE([GENERIC_UNIX],1,[non-specific unix conditional compile])])
	])

AM_COND_IF([BUILD_WIN32],
	[AC_DEFINE([WIN32_VARIANT],1,[win32-specific conditional compile])])

AC_DEFINE_UNQUOTED(CPUSTRING, "${host_cpu}", [Canonical CPU identification])
AC_DEFINE_UNQUOTED(BUILDSTRING, "${host_os}", [Canonical OS identification])

#
# Checks for libraries.
#

AC_SEARCH_LIBS([acos],[m])
AC_SEARCH_LIBS([dlopen],[dl])

dnl TODO: possible to have a variable here for server_CFLAGS, server_LIBS ???

dnl
dnl start of client-only library requirements
dnl
AM_COND_IF([BUILD_CLIENT],
[
	dnl could be AC_SEARCH_LIBS([pthread_mutex_init],[pthread])
	dnl but this is probably more portable, see m4/ax_pthread.m4
	AX_PTHREAD(,[AC_MSG_ERROR(["Required pthreads support not found."])])
	
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
		dnl
		dnl If no option is specified, then check for the library and act accordingly.
		dnl If "--without-xf86dga" is specified, do not check, and disable the library.
		dnl If "--with-xf86dga" is specified, check and bail out if it cannot be found.
		dnl
		AC_ARG_WITH([xf86dga],
			[AS_HELP_STRING([--with(out)-xf86dga],[include XF86 DGA support (default: check)])],
			[],
			[with_xf86dga=check])
		have_Xxf86dga=no
		AS_IF([ test "x$with_xf86dga" != "xno" ],[
			PKG_CHECK_MODULES([XXF86DGA],[xxf86dga],[
				have_Xxf86dga=yes
				AC_DEFINE([HAVE_XXF86DGA],1,[Enable X86 DGA support])
			],[
				have_Xxf86dga=no
				AS_IF([ test "x$with_xf86dga" = "xyes" ],[
					AC_MSG_FAILURE([--with-xf86dga specified, but the library or headers could not be found])
				])
			])
		])
	])

	dnl
	dnl Check common client dependencies:
	dnl ODE, Curl, Ogg/Vorbis and vorbisfile
	dnl
	PKG_CHECK_MODULES([DEPS],[glu ode libcurl ogg vorbis vorbisfile])

	dnl Hack in the extras for Win32
	dnl TODO: this may not be the best way to do this
	win32_syslibs=""
	AM_COND_IF([BUILD_WIN32],
		[ win32_syslibs="-lmingw32 -lwinmm -lwsock32 -lws2_32 -lgdi32 -luser32"])
	AC_SUBST([WIN32_LIBS],[$win32_syslibs])

	dnl darwin openal option
	darwin_frameworks=""
	AS_IF([ test "x${unix_host}" = xdarwin ],
		[AS_IF([ test "x${apple_openal}" = xyes ],
			[ darwin_frameworks="-framework OpenAL"])])
	AC_SUBST([DARWIN_FRAMEWORKS],[$darwin_frameworks])

])
dnl 
dnl end of client-only library requirements
dnl

#
# Checks for header files
#

dnl TODO: verify these are used and needed
AC_CHECK_HEADERS(
	[arpa/inet.h netinet/in.h sys/ioctl.h sys/socket.h unistd.h netdb.h termios.h])

AC_CHECK_HEADERS(
	[ dlfcn.h fcntl.h float.h limits.h malloc.h stddef.h stdlib.h string.h sys/param.h sys/time.h unistd.h sys/stat.h ])
	
dnl for config.h generation, maybe others?	
AC_CHECK_HEADERS([ windows.h winsock.h winsock2.h ])	

dnl	
dnl start of client-only header requirements
dnl	
AM_COND_IF([BUILD_CLIENT],
[
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
#

AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC

# network functions
AC_CHECK_FUNCS([closesocket socket gethostbyname])

# thread functions
AC_CHECK_FUNCS([pthread_create _begin_thread])

# string functions
AC_CHECK_FUNCS([strchr strerror strrchr strstr strlcpy])
AC_CHECK_FUNCS([strcasecmp strncasecmp ])
AC_CHECK_FUNCS([strdup _strdup stricmp _stricmp strnicmp _strnicmp])

# file system functions
AC_CHECK_FUNCS([filelength stat fstat getcwd _getcwd unlink _unlink mkdir _mkdir])

# math functions
AC_CHECK_FUNCS([pow sqrt floor])

# mem mgmt functions
AC_CHECK_FUNCS([memmove memset munmap mremap])

# other functions
AC_CHECK_FUNCS([gettimeofday putenv _putenv select])

#
# Set names of dlopened libraries for config.h
#

dnl linux_gnu and generic get the default.
dnl TODO: darwin needs full path to libraries, does not have default search path (?)
dnl TODO: get correct darwin .dylib name for non-apple
AM_COND_IF([BUILD_UNIX],
	[AS_CASE([ ${unix_host} ],
		[ darwin ],
			[
			AC_DEFINE([OPENGL_DRIVER],
				["libGL.dylib"],[OpenGL dynamic lib name])

			AS_IF([ test "x${apple_openal}" = xyes ],
				[AC_DEFINE([OPENAL_DRIVER],
					["/System/Library/Frameworks/OpenAL.framework/OpenAL"],
					[OpenAL dynamic lib name])],
				[AC_DEFINE([OPENAL_DRIVER],
					["libOpenAL.dylib"],
					[OpenAL dynamic lib name])])
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

# 2 required gcc options, see m4/ax_cflags_gcc_option.m4
AX_CFLAGS_GCC_OPTION([-ffast-math])
AX_CFLAGS_GCC_OPTION([-fno-strict-aliasing])


#########################################################################################################
# DEBUGGING OPTIONS
#########################################################################################################

debug=no

# Debugging support + symbols removal
AC_ARG_ENABLE([debugging-symbols],
	[AS_HELP_STRING([--enable-debugging-symbols],[Compile Alien Arena with full debugging information (default:no)])],
	[debug_syms=${enableval}],[debug_syms=no])
AS_IF([ test "x$debug_syms" = "xyes" ], [
	AX_CFLAGS_GCC_OPTION([-g3])
	debug=yes
])
AM_CONDITIONAL(REMOVE_SYMBOLS,[ test "x$debug_syms" = "xno" ])

# Compiler warnings
AC_ARG_ENABLE([full-warnings],
	[AS_HELP_STRING([--enable-full-warnings],[Display all compiler warnings (default:no)])],
	[gcc_whining=${enableval}],[gcc_whining=no])
AS_IF([ test "x$gcc_whining" = "xyes" ], [
	AX_CFLAGS_GCC_OPTION([-Wall])
	debug=yes
])

# IRC client debugging
AC_ARG_ENABLE([irc-debugging],
	[AS_HELP_STRING([--enable-irc-debugging],[Debug IRC messages (default: no)])],
	[irc_debug=${enableval}],[irc_debug=no])
AS_IF([ test "x$irc_debug" = "xyes" ],[
	AC_DEFINE([DEBUG_DUMP_IRC],1,[Enable IRC message debugging])
	debug=yes
])

# Buffer size debugging
AC_ARG_ENABLE([buffer-debugging],
	[AS_HELP_STRING([--enable-buffer-debugging],[Debug buffers (default: no)])],
	[buffer_debug=${enableval}],[buffer_debug=no])
AS_IF([ test "x$buffer_debug" = "xyes" ],[
	AC_DEFINE([BUFFER_DEBUG],1,[Enable buffer debugging])
	debug=yes
])

# Paranoid message checking
AC_ARG_ENABLE([paranoid-io],
	[AS_HELP_STRING([--enable-paranoid-io],
		[Activate paranoid, speed-sapping message checking (default: no)])
	],[paranoid_io=${enableval}],[paranoid_io=no])
AS_IF([ test "x$paranoid_io" = "xyes" ],[
	AC_DEFINE([PARANOID],1,[Enable paranoid, speed-sapping message checking])
	debug=yes
])

#########################################################################################################
# END OF DEBUGGING OPTIONS
#########################################################################################################



# outputs
AC_CONFIG_FILES([ Makefile 	source/Makefile ])

AC_OUTPUT

dnl Option to disable this message.
AC_ARG_ENABLE([build-status],[AS_HELP_STRING([--disable-build-status],
			[hide the status message at the end of the configuration script (default:no)])],
			[hide_status=${enableval}],[hide_status=no])

AS_IF([test "x$hide_status" = "xno"],[
	AS_ECHO(["-------------------------------------------"])
	AS_ECHO(["Package: ${PACKAGE_NAME} Version: ${PACKAGE_VERSION}"])
	AS_ECHO([" OS: .................... ${host_os}"])
	AS_ECHO([" CPU: ................... ${host_cpu}"])
	AM_COND_IF([BUILD_WIN32],,[
		AS_IF([ test "x$build_client" = "xyes" ],[dedicated_only=no],[dedicated_only=yes])
		AS_ECHO([" Dedicated Only: ........ ${dedicated_only}"])
	])
	AS_ECHO([" XF86 DGA support: ...... ${have_Xxf86dga}"])
	AS_ECHO([" XF86 VidMode support: .. ${have_Xxf86vm}"])
	AS_ECHO([" Alternate install: ..... ${alternate_install}"])

	AM_COND_IF([ALTERNATE_INSTALL],,
		[AS_ECHO([" Prefix: ................ '${prefix}'"])])

	AS_IF([ test "x$debug" = "xyes" ],[
		AS_ECHO(["- DEBUGGING OPTIONS ------------------------"])
		AS_ECHO(["   All compiler warnings: .... ${gcc_whining}"])
		AS_ECHO(["   Debugging symbols: ........ ${debug_syms}"])
		AS_ECHO(["   IRC protocol dump: ........ ${irc_debug}"])
		AS_ECHO(["   Buffer debugging: ......... ${buffer_debug}"])
		AS_ECHO(["   Paranoid message checks: .. ${buffer_debug}"])
	])

	AM_COND_IF([ALTERNATE_INSTALL],
		[AS_ECHO(["Run \"make\" to build, then \"make install-alternate\" to install."])],
		[AS_ECHO(["Run \"make\" to build, then \"sudo make install\" to install."])])

	AS_ECHO(["--------------------------------------------"])
])
