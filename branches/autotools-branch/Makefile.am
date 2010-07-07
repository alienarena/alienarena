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

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I source

# these cflags are required 
AM_CFLAGS = -fno-strict-aliasing -ffast-math
# -U_FORTIFY_SOURCE or -D_FORTIFY_SOURCE


if BUILD_UNIX
bin_PROGRAMS = crx-ded
if BUILD_CLIENT
bin_PROGRAMS += crx
endif
else
bin_PROGRAMS = crx
endif

noinst_LIBRARIES = libgame.a

if BUILD_UNIX
crx_ded_CFLAGS = -DDEDICATED_ONLY -D_FORTIFY_SOURCE
crx_ded_LDADD = libgame.a
if BUILD_CLIENT
crx_CFLAGS = -D_FORTIFY_SOURCE
crx_LDADD = libgame.a
endif
else
crx_CFLAGS = -DCURL_STATICLIB -DAL_NO_PROTOTYPES -U_FORTIFY_SOURCE
crx_LDADD = libgame.a
LIBS += $(WIN32_LIBS)
endif

if BUILD_UNIX
if BUILD_CLIENT
if BUILD_DARWIN
LIBS += $(DARWIN_LIBS)
endif
endif
endif

libgame_a_CFLAGS = -D_FORTIFY_SOURCE 

if BUILD_UNIX
crx_ded_SOURCES = \
	source/client/anorms.h \
	source/game/game.h \
	source/game/q_shared.c \
	source/game/q_shared.h \
	source/game/q_shared.h \
	source/null/cl_null.c \
	source/qcommon/cmd.c \
	source/qcommon/cmodel.c \
	source/qcommon/common.c \
	source/qcommon/crc.c \
	source/qcommon/cvar.c \
	source/qcommon/files.c \
	source/qcommon/mdfour.c \
	source/qcommon/net_chan.c \
	source/qcommon/pmove.c \
	source/qcommon/qcommon.h \
	source/qcommon/qfiles.h \
	source/server/server.h \
	source/server/sv_ccmds.c \
	source/server/sv_ents.c \
	source/server/sv_game.c \
	source/server/sv_init.c \
	source/server/sv_main.c \
	source/server/sv_send.c \
	source/server/sv_user.c \
	source/server/sv_world.c \
	source/unix/glob.c \
	source/unix/glob.h \
	source/unix/glob.h \
	source/unix/net_udp.c \
	source/unix/q_shunix.c \
	source/unix/rw_unix.h \
	source/unix/sys_unix.c
endif

if BUILD_WIN32
crx_SOURCES = \
	source/win32/conproc.c \
	source/win32/glw_imp.c \
	source/win32/in_win.c \
	source/win32/net_wins.c \
	source/win32/qal_win.c \
	source/win32/qgl_win.c \
	source/win32/q_shwin.c \
	source/win32/sys_win.c \
	source/win32/vid_dll.c
else
if BUILD_CLIENT
crx_SOURCES = \
	source/unix/gl_glx.c \
	source/unix/glw_unix.h \
	source/unix/net_udp.c \
	source/unix/qal_unix.c \
	source/unix/qasm.h \
	source/unix/qgl_unix.c \
	source/unix/q_shunix.c \
	source/unix/rw_unix.c \
	source/unix/rw_unix.h \
	source/unix/sys_unix.c \
	source/unix/vid_so.c
endif	
endif	

if BUILD_CLIENT
crx_SOURCES +=  \
	source/client/anorms.h \
	source/client/cl_ents.c \
	source/client/cl_fx.c \
	source/client/cl_http.c \
	source/client/client.h \
	source/client/cl_input.c \
	source/client/cl_inv.c \
	source/client/cl_irc.c \
	source/client/cl_main.c \
	source/client/cl_parse.c \
	source/client/cl_pred.c \
	source/client/cl_scrn.c \
	source/client/cl_stats.c \
	source/client/cl_tent.c \
	source/client/cl_view.c \
	source/client/console.c \
	source/client/console.h \
	source/client/input.h \
	source/client/keys.c \
	source/client/keys.h \
	source/client/menu.c \
	source/client/qal.c \
	source/client/qal.h \
	source/client/qmenu.c \
	source/client/qmenu.h \
	source/client/ref.h \
	source/client/screen.h \
	source/client/snd_file.c \
	source/client/snd_openal.c \
	source/client/sound.h \
	source/client/vid.h \
	source/client/vid_menu.c \
	source/game/game.h \
	source/game/q_shared.c \
	source/game/q_shared.h \
	source/qcommon/cmd.c \
	source/qcommon/cmodel.c \
	source/qcommon/common.c \
	source/qcommon/crc.c \
	source/qcommon/crc.h \
	source/qcommon/cvar.c \
	source/qcommon/files.c \
	source/qcommon/mdfour.c \
	source/qcommon/net_chan.c \
	source/qcommon/pmove.c \
	source/qcommon/qcommon.h \
	source/qcommon/qfiles.h \
	source/ref_gl/anorms.h \
	source/ref_gl/anormtab.h \
	source/ref_gl/glext.h \
	source/ref_gl/qgl.h \
	source/ref_gl/r_bloom.c \
	source/ref_gl/r_draw.c \
	source/ref_gl/r_image.c \
	source/ref_gl/r_image.h \
	source/ref_gl/r_iqm.h \
	source/ref_gl/r_iqm.c \
	source/ref_gl/r_light.c \
	source/ref_gl/r_local.h \
	source/ref_gl/r_main.c \
	source/ref_gl/r_math.c \
	source/ref_gl/r_math.h \
	source/ref_gl/r_matrixlib.c \
	source/ref_gl/r_matrixlib.h \
	source/ref_gl/r_mesh.c \
	source/ref_gl/r_misc.c \
	source/ref_gl/r_model.c \
	source/ref_gl/r_model.h \
	source/ref_gl/r_particle.c \
	source/ref_gl/r_postprocess.c \
	source/ref_gl/r_program.c \
	source/ref_gl/r_script.c \
	source/ref_gl/r_script.h \
	source/ref_gl/r_shadowmaps.c \
	source/ref_gl/r_shadows.c \
	source/ref_gl/r_surf.c \
	source/ref_gl/r_varray.c \
	source/ref_gl/r_vbo.c \
	source/ref_gl/r_vlights.c \
	source/ref_gl/r_warp.c \
	source/ref_gl/warpsin.h \
	source/server/server.h \
	source/server/sv_ccmds.c \
	source/server/sv_ents.c \
	source/server/sv_game.c \
	source/server/sv_init.c \
	source/server/sv_main.c \
	source/server/sv_send.c \
	source/server/sv_user.c \
	source/server/sv_world.c \
	source/unix/glob.c \
	source/unix/glob.h
endif	

libgame_a_SOURCES =  \
	source/client/anorms.h \
	source/game/acesrc/acebot_ai.c \
	source/game/acesrc/acebot_cmds.c \
	source/game/acesrc/acebot.h \
	source/game/acesrc/acebot_items.c \
	source/game/acesrc/acebot_movement.c \
	source/game/acesrc/acebot_nodes.c \
	source/game/acesrc/acebot_spawn.c \
	source/game/c_cam.c \
	source/game/cow.h \
	source/game/g_ai.c \
	source/game/game.h \
	source/game/g_chase.c \
	source/game/g_cmds.c \
	source/game/g_combat.c \
	source/game/g_cow.c \
	source/game/g_ctf.c \
	source/game/g_deathball.c \
	source/game/g_deathray.c \
	source/game/g_deathray.h \
	source/game/g_func.c \
	source/game/g_items.c \
	source/game/g_local.h \
	source/game/g_main.c \
	source/game/g_misc.c \
	source/game/g_monster.c \
	source/game/g_phys.c \
	source/game/g_save.c \
	source/game/g_spawn.c \
	source/game/g_svcmds.c \
	source/game/g_target.c \
	source/game/g_trigger.c \
	source/game/g_unlagged.c \
	source/game/g_utils.c \
	source/game/g_vehicles.c \
	source/game/g_weapon.c \
	source/game/m_move.c \
	source/game/m_player.h \
	source/game/p_client.c \
	source/game/p_hud.c \
	source/game/p_trail.c \
	source/game/p_view.c \
	source/game/p_weapon.c \
	source/game/q_shared.c \
	source/game/q_shared.h \
	source/qcommon/qfiles.h

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
# TODO: possible to not create $(DESTDIR)/bin when $(DESTDIR) eq $(srcdir) ?
#   may need to reverse the copy: cp $(bindir) to $(srcdir) and rm $(bindir)
#
#install-exec-hook:
	

#
# install/uninstall added rules for the game read-only data
#
# TODO: detect when $(DESTDIR) is same as $(srcdir) and do not copy data
# TODO: when installing in standard way, may need to rm .svn directories
#   from $(pkgdatadir)
#
install-data-hook:
	mkdir -p $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/docs $(DESTDIR)$(docdir)
	cp --preserve=mode $(srcdir)/alienarena.png $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/arena $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/botinfo $(DESTDIR)$(pkgdatadir)
	cp -r --preserve=mode $(srcdir)/data1 $(DESTDIR)$(pkgdatadir)	
	chmod -R u+w $(DESTDIR)$(docdir)
	chmod -R u+w $(DESTDIR)$(pkgdatadir)

# remove what install-hook installed
uninstall-hook:
	rm -rf $(DESTDIR)$(docdir)
	rm -rf $(DESTDIR)$(pkgdatadir)
	rm -rf $(DESTDIR)$(pkglibexecdir)
	
