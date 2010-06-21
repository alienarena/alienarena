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

EXTRA_DIST = \
	alienarena.png \
	docs/license.txt \
	docs/README.txt \
	arena \
	botinfo \
	data1

## remove subversion files from distribution directory before creating archive
## also remove Win32 dll, and mapsrc subdir
## TODO: see if it is possible to svn export to the distribution directory
## dist-hook:
## 	rm -rf `find $(distdir) -name .svn`
## 	rm -f $(distdir)/data1/gamex86.dll
## 	rm -rf $(distdir)/data1/maps/mapsrc

##
## install/uninstall added rules for the game read-only data
##
## install-data-hook:
## 	mkdir -p $(DESTDIR)$(docdir)
## 	mkdir -p $(DESTDIR)$(pkgdatadir)
## 	cp -r --preserve=mode $(srcdir)/docs $(DESTDIR)$(docdir)
## 	cp --preserve=mode $(srcdir)/alienarena.png $(DESTDIR)$(pkgdatadir)
## 	cp -r --preserve=mode $(srcdir)/arena $(DESTDIR)$(pkgdatadir)
## 	cp -r --preserve=mode $(srcdir)/botinfo $(DESTDIR)$(pkgdatadir)
## 	cp -r --preserve=mode $(srcdir)/data1 $(DESTDIR)$(pkgdatadir)	
## 	chmod -R u+w $(DESTDIR)$(docdir)
## 	chmod -R u+w $(DESTDIR)$(pkgdatadir)

	
##	
## the main program looks in $LIBDIR/arena/ and $LIBDIR/data1/ subdirectories
##  for game.so. arena/ is for mods, data1/ is the main game module
## so, until a better way is found, just create data1 and move the game.so,la
##	
## TODO: static link to game library eliminates need for this
##install-exec-hook:
##	mkdir -p $(DESTDIR)$(pkglibexecdir)/data1
##	mv -f $(DESTDIR)$(pkglibexecdir)/game.so $(DESTDIR)$(pkglibexecdir)/data1
##	mv -f $(DESTDIR)$(pkglibexecdir)/game.la $(DESTDIR)$(pkglibexecdir)/data1

## remove what install-hook installed
##uninstall-hook:
##	rm -rf $(DESTDIR)$(docdir)
##	rm -rf $(DESTDIR)$(pkgdatadir)
##	rm -rf $(DESTDIR)$(pkglibexecdir)


