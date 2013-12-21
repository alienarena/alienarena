# Process this file with automake to produce Makefile.in
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
# A copy of the  GNU General Public License is included;
# see the file, COPYING.
#

# 2013 December:
# - See notes in source/Makefile.am, and configure.ac
# - Recursive build for data1 and botinfo. Same effect as
#    previous game_data.am methods, but avoids problem
#    with command line overflow when running install.
# - Moved the data1 default.cfg and maps.lst to arena
#   subdir for consistency between arena and tactical.
#    

SUBDIRS = source data1 botinfo

ACLOCAL_AMFLAGS = -I m4 

#
# Alien Arena documents to be installed in $docdir
#
#if INSTALL_DOCS

#dist_doc_DATA = \
#	unix_doc/README \
#	unix_doc/COPYING \
#	unix_doc/INSTALL \
#	unix_doc/GPL2

#endif

# Alien Arena icon
# TODO: change default icon directory to share/${PACKAGE}
#  and put these in a unixicon subdir in svn, maybe
#

#dist_icons_DATA = \
#	unix_icons/alienarena.png \
#	unix_icons/aa-tactical-demo.png

# FUSE game server browser and Server management scripts
#EXTRA_DIST = \
#	Tools/fuse.tar.gz \
#	Tools/LinuxScripts

#
# Default configuration for client and server
#
arenadir = $(pkgdatadir)
nobase_dist_arena_DATA = \
	arena/motd.txt \
	arena/server.cfg \
	arena/default.cfg \
	arena/maps.lst

tacticaldir = $(pkgdatadir)
nobase_dist_tactical_DATA = \
	tactical/maps.lst \
	tactical/motd.txt \
	tactical/default.cfg \
	tactical/server.cfg


# remove svn directories from distribution staging area after creating
#  distribution in staging area and before running archiver
# but, it is probably a better idea to export the repository to the
#  staging area since export does not include the .svn parts
#
dist-hook:
	rm -rf `find $(distdir) -name .svn`

#
# Alternate Install deemed redundant and removed.
# Almost the same thing:
#  ./configure --prefix=/home/user/COR_games && make && make install
#
# TODO: if possible, detect when --prefix == top build dir and
#  avoid copying. Also, test to see what happens if this is done, install
#  may handle it.
#
