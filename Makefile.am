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
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

SUBDIRS = source

ACLOCAL_AMFLAGS = -I m4 

# Alien Arena documents to be installed in $docdir
if INSTALL_DOCS
dist_doc_DATA = \
	docs/license.txt \
	docs/README.txt \
	README
endif

# FUSE game server browser and Server management scripts
# COPYING and INSTALL are GNU/FSF-supplied documents.
EXTRA_DIST = \
	Tools/fuse.tar.gz \
	Tools/LinuxScripts \
	COPYING \
	INSTALL

# remove svn directories from distribution staging area after creating
#  distribution in staging area and before running archiver
dist-hook:
	rm -rf `find $(distdir) -name .svn`

#
# Alternate Install
# This non-standard target is used for traditional single directory, 
# in-place install. Good for map makers, frequent SVN updates.
# Also, maybe the preferable installation for Game Server Providers doing
#   dedicated server only builds
#
# Use: 
#  ./configure --enable-alternate-install && make && make install-alternate
#
# Note: this does not support program name translation and other autoconf
#   features.
#
install-alternate:
if ALTERNATE_INSTALL
if BUILD_CLIENT
	cp $(top_builddir)/source/crx@EXEEXT@ $(srcdir)/crx@EXEEXT@
if REMOVE_SYMBOLS
	strip $(srcdir)/crx@EXEEXT@
endif
endif	
if BUILD_UNIX	
	cp $(top_builddir)/source/crx-ded $(srcdir)/crx-ded
if REMOVE_SYMBOLS
	strip $(srcdir)/crx-ded
endif
endif
else
	@echo "make install-alternate requires configuring with --enable-alternate-install"
endif		

#
# install/uninstall added rules for alternate-install to prevent "undefined" behaviour. 
#
install-data-local:
if ALTERNATE_INSTALL
	@echo "Alternate install: Use "make install-alternate"!!!
	exit 1
endif

uninstall-local:
if ALTERNATE_INSTALL
	@echo "Alternate install. Manual uninstall required!!!"
	exit 1
endif

#
# game data for distribution package
# 
include game_data.am

