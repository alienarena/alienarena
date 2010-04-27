## Process this file with automake to produce Makefile.in

SUBDIRS = source 

ACLOCAL_AMFLAGS = -I m4

##
EXTRA_DIST = \
	docs/license.txt \
	docs/README.txt \
	arena \
	botinfo \
	data1

## remove subversion files from distribution directory before creating archive
## also remove Win32 dll, and mapsrc subdir
dist-hook:
	rm -rf `find $(distdir) -name .svn`
	rm -f $(distdir)/data1/gamex86.dll
	rm -rf $(distdir)/data1/maps/mapsrc

#
# may should check for existence of libraries before doing this
# force overwrite of existing files that match, but keep what is there
# note: docdir contains package name
#
install-data-hook:
	rm -rf $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(docdir)
	rm -rf $(DESTDIR)$(pkgdatadir)
	mkdir -p $(DESTDIR)$(pkgdatadir)
	cp -r -p $(srcdir)/docs $(DESTDIR)$(docdir)
	cp -r -p $(srcdir)/arena $(DESTDIR)$(pkgdatadir)
	cp -r -p $(srcdir)/botinfo $(DESTDIR)$(pkgdatadir)
	cp -r -p $(srcdir)/data1 $(DESTDIR)$(pkgdatadir)	
	chmod -R u+w $(DESTDIR)$(docdir)
	chmod -R u+w $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -rf $(DESTDIR)$(docdir)
	rm -rf $(DESTDIR)$(pkgdatadir)

