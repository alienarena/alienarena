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

# TODO: create "official" INSTALL, README, etc.
# TODO: maybe create a subdirectory Makefile.am for install of arena/
#   and doc/ files
EXTRA_DIST = \
	alienarena.png \
	arena/motd.txt \
	arena/server.cfg \
	docs/license.txt \
	docs/README.txt \
	m4 \
	botinfo \
	data1

# Remove files that do not belong in distribution
# Probably good idea to "svn export" into a another directory to avoid 
#  having to deal with hidden svn subdirectories and other unwanted files 
dist-hook:
	rm -rf $(distdir)/data1/maps/mapsrc
	rm -rf `find $(distdir) -name .svn`

#
# Alternate Install
# This non-standard target is used for traditional single directory, 
# in-place install. Good for map makers, frequent SVN updates.
# Use: 
#  ./configure --enable-alternate-install && make && make install-alternate
#
# TODO: this does not support program name translation and other fancy stuff
#   (can install.sh be used here?)
#
install-alternate:
if ALTERNATE_INSTALL
	cp $(top_builddir)/source/crx $(srcdir)/crx
	cp $(top_builddir)/source/crx-ded $(srcdir)/crx-ded
	strip $(srcdir)/crx
	strip $(srcdir)/crx-ded
else
	@echo "make install-alternate requires configuring with --enable-alternate-install"
endif		

#
# install/uninstall added rules for the game read-only data
# 
# - Normally requires root privelages.
#
# For $(DESTDIR)$(pkgdatadir) and
# - existing botinfo/ and data1/ directories are removed on install.
# - any game.so in arena/ is removed, but otherwise existing arena 3rd party
#    data should remain. It is possible that assets from data1/ used by 
#    3rd party maps may not exist after a new install.
# - any .svn subdirectories are removed.
#
# TODO: Maybe move some of this to an arena/Makefile.am and a doc/Makefile.am 
#
#
install-data-hook:
if ALTERNATE_INSTALL
	@echo "Using --enable-alternate-install. Data install not needed."
else
	mkdir -p $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(pkgdatadir)
	mkdir -p $(DESTDIR)$(pkgdatadir)/arena
	rm -rf $(DESTDIR)$(pkgdatadir)/botinfo
	rm -rf $(DESTDIR)$(pkgdatadir)/data1
	rm -f $(DESTDIR)$(pkgdatadir)/arena/game.so
	cp --preserve=mode $(srcdir)/alienarena.png $(DESTDIR)$(pkgdatadir)
	cp --preserve=mode $(srcdir)/arena/motd.txt $(DESTDIR)$(pkgdatadir)/arena
	cp --preserve=mode $(srcdir)/arena/server.cfg $(DESTDIR)$(pkgdatadir)/arena
	cp --preserve=mode $(srcdir)/docs/license.txt $(DESTDIR)$(docdir)
	cp --preserve=mode $(srcdir)/docs/README.txt $(DESTDIR)$(docdir)
	cp -r --preserve=mode $(srcdir)/botinfo $(DESTDIR)$(pkgdatadir)
	@echo "copying game data, please be patient."
	cp -r --preserve=mode $(srcdir)/data1 $(DESTDIR)$(pkgdatadir)
	rm -rf `find $(DESTDIR)$(pkgdatadir) -name .svn`
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
endif
	
	
