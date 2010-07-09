# Alien Arena top level Makefile.am
# Process this file with automake to produce Makefile.in
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

SUBDIRS = source 

ACLOCAL_AMFLAGS = -I m4

if ALTERNATE_INSTALL
AM_MAKEFLAGS = DESTDIR=$(top_srcdir)
endif

# TODO: create "official" INSTALL, README, etc.
EXTRA_DIST = \
	alienarena.png \
	docs/license.txt \
	docs/README.txt \
	m4 \
	arena \
	botinfo \
	data1

# remove files that do not belong in Linux distribution
# "svn export" into a distribution build directory to avoid having to deal
#   with hidden svn subdirectories and other unwanted files 
dist-hook:
	rm -rf $(distdir)/data1/maps/mapsrc

# remove hidden svn directories 	
#	rm -rf `find $(distdir) -name .svn`


#
# Alternate Install: 
#
install-alternate:
if ALTERNATE_INSTALL
	cp $(srcdir)/source/crx $(srcdir)/crx
else
	@echo "make install-alternate requires configuring with --enable-alternate-install"
endif		

#
# install/uninstall added rules for the game read-only data
#
# TODO: when installing in standard way, may need to rm .svn directories
#   from $(pkgdatadir) if they exist.
#
install-data-hook:
if ALTERNATE_INSTALL
	@echo "Using --enable-alternate-install. Data install disabled."
else
	mkdir -p $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/docs $(DESTDIR)$(docdir)
	cp --preserve=mode $(srcdir)/alienarena.png $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/arena $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/botinfo $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/data1 $(DESTDIR)$(pkgdatadir)	
	chmod -R u+w $(DESTDIR)$(docdir)
	chmod -R u+w $(DESTDIR)$(pkgdatadir)
endif	

# remove what install-hook installed
uninstall-hook:
if ALTERNATE_INSTALL
	@echo "Using --enable-alternate-install. Data uninstall disabled."
else
	rm -rf $(DESTDIR)$(docdir)
	rm -rf $(DESTDIR)$(pkgdatadir)
	rm -rf $(DESTDIR)$(pkglibexecdir)
endif
	
	
