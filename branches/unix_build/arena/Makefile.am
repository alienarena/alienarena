# Process this file with automake to produce Makefile.in

# Copyright (C) 2014 COR Entertainment, LLC.

# --- ALIEN ARENA ORIGINAL  ---

#
# TODO: add licensing and other docs for the package
#

#
# Default configuration for client and server
#
arenadir = $(datadir)/alienarena/arena
nobase_dist_arena_DATA = \
	motd.txt \
	server.cfg \
	default.cfg \
	maps.lst

#
# someday game source goes here
#

dist-hook:
	rm -rf `find $(distdir) -name .svn`

