# Process this file with automake to produce Makefile.in
#
# Copyright (C) 2010 COR Entertainment, LLC
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

noinst_LIBRARIES = libgame.a 

AM_CPPFLAGS = -I $(top_srcdir)/source

# CPP flags for standard installation
if !ALTERNATE_INSTALL
AM_CPPFLAGS += -DDATADIR='"$(pkgdatadir)"'
endif

# Dedicated Server libraries and flags
alienarena_ded_LDADD = libgame.a
alienarena_ded_CPPFLAGS = $(AM_CPPFLAGS) -DDEDICATED_ONLY

# Client libraries and flags
alienarena_CPPFLAGS = $(AM_CPPFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(X11_CLAGS) \
	$(DEPS_CFLAGS) \
	$(ODE_CFLAGS) \
	$(XXF86VM_CFLAGS) \
	$(XXF86DGA_CFLAGS) \
	$(ZLIB_CFLAGS)

#
# alienarena with system libode
#
alienarena_LDADD = libgame.a \
	$(PTHREAD_LIBS) \
	$(X11_LIBS) \
	$(DEPS_LIBS) \
	$(ODE_LIBS) \
	$(XXF86VM_LIBS) \
	$(XXF86DGA_LIBS) \
	$(ZLIB_LIBS)

if BUILD_WIN32

# On Win32, always build the client
# TODO: maybe fixup libs, cppflags hacks
bin_PROGRAMS = alienarena
alienarena_LDADD += $(WIN32_LIBS)
alienarena_CPPFLAGS += -DCURL_STATICLIB -DNDEBUG

else

# On Unix, always build the server, and the client if BUILD_CLIENT is enabled
bin_PROGRAMS = alienarena-ded
if BUILD_CLIENT
bin_PROGRAMS += alienarena
endif

endif

# common client sources
alienarena_SOURCES = \
	client/anorms.h \
	client/cl_ents.c \
	client/cl_fx.c \
	client/cl_http.c \
	client/client.h \
	client/cl_input.c \
	client/cl_inv.c \
	client/cl_irc.c \
	client/cl_main.c \
	client/cl_parse.c \
	client/cl_pred.c \
	client/cl_scrn.c \
	client/cl_stats.c \
	client/cl_updates.c \
	client/cl_tent.c \
	client/cl_view.c \
	client/console.c \
	client/console.h \
	client/input.h \
	client/keys.c \
	client/keys.h \
	client/menu.c \
	client/qal.c \
	client/qal.h \
	client/qmenu.c \
	client/qmenu.h \
	client/ref.h \
	client/screen.h \
	client/snd_file.c \
	client/snd_openal.c \
	client/sound.h \
	client/vid.h \
	game/game.h \
	game/q_shared.c \
	game/q_shared.h \
	qcommon/cmd.c \
	qcommon/cmodel.c \
	qcommon/common.c \
	qcommon/crc.c \
	qcommon/crc.h \
	qcommon/cvar.c \
	qcommon/files.c \
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/md5.c \
	qcommon/md5.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	qcommon/binheap.c \
	qcommon/binheap.h \
	qcommon/image.c \
	qcommon/libgarland.c \
	qcommon/libgarland.h \
	qcommon/terrain.c \
	ref_gl/r_terrain.c \
	ref_gl/r_decals.c \
	ref_gl/glext.h \
	ref_gl/glfuncs.h \
	ref_gl/qgl.h \
	ref_gl/r_bloom.c \
	ref_gl/r_draw.c \
	ref_gl/r_image.c \
	ref_gl/r_image.h \
	ref_gl/r_iqm.h \
	ref_gl/r_iqm.c \
	ref_gl/r_md2.c \
	ref_gl/r_light.c \
	ref_gl/r_local.h \
	ref_gl/r_lodcalc.h \
	ref_gl/r_main.c \
	ref_gl/r_math.c \
	ref_gl/r_math.h \
	ref_gl/r_mesh.c \
	ref_gl/r_misc.c \
	ref_gl/r_model.c \
	ref_gl/r_model.h \
	ref_gl/r_particle.c \
	ref_gl/r_postprocess.c \
	ref_gl/r_program.c \
	ref_gl/r_ragdoll.c \
	ref_gl/r_ragdoll.h \
	ref_gl/r_script.c \
	ref_gl/r_script.h \
	ref_gl/r_shadowmaps.c \
	ref_gl/r_shadows.c \
	ref_gl/r_surf.c \
	ref_gl/r_text.c \
	ref_gl/r_text.h \
	ref_gl/r_ttf.c \
	ref_gl/r_ttf.h \
	ref_gl/r_varray.c \
	ref_gl/r_vbo.c \
	ref_gl/r_vlights.c \
	ref_gl/r_warp.c \
	ref_gl/warpsin.h \
	server/server.h \
	server/sv_ccmds.c \
	server/sv_ents.c \
	server/sv_game.c \
	server/sv_init.c \
	server/sv_main.c \
	server/sv_send.c \
	server/sv_user.c \
	server/sv_world.c \
	unix/glob.c \
	unix/glob.h


# Unix Client sources
if BUILD_UNIX
alienarena_SOURCES += \
	unix/gl_glx.c \
	unix/glw_unix.h \
	unix/net_udp.c \
	unix/qal_unix.c \
	unix/qgl_unix.c \
	unix/q_shunix.c \
	unix/rw_unix.c \
	unix/rw_unix.h \
	unix/sys_unix.c \
	unix/vid_so.c
endif	

# Win32 MinGW Client sources
if BUILD_WIN32
alienarena_SOURCES += \
	win32/conproc.c \
	win32/conproc.h \
	win32/glw_imp.c \
	win32/glw_win.h \
	win32/in_win.c \
	win32/net_wins.c \
	win32/qal_win.c \
	win32/qgl_win.c \
	win32/q_shwin.c \
	win32/sys_win.c \
	win32/vid_dll.c \
	win32/winquake.h \
	win32/resource.h
endif	

# Dedicated Server sources (Unix only)
alienarena_ded_SOURCES = \
	client/anorms.h \
	game/game.h \
	game/q_shared.c \
	game/q_shared.h \
	null/cl_null.c \
	qcommon/cmd.c \
	qcommon/cmodel.c \
	qcommon/common.c \
	qcommon/crc.c \
	qcommon/cvar.c \
	qcommon/files.c \
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	qcommon/binheap.c \
	qcommon/binheap.h \
	qcommon/image.c \
	qcommon/libgarland.c \
	qcommon/libgarland.h \
	qcommon/terrain.c \
	server/server.h \
	server/sv_ccmds.c \
	server/sv_ents.c \
	server/sv_game.c \
	server/sv_init.c \
	server/sv_main.c \
	server/sv_send.c \
	server/sv_user.c \
	server/sv_world.c \
	unix/glob.c \
	unix/glob.h \
	unix/net_udp.c \
	unix/q_shunix.c \
	unix/rw_unix.h \
	unix/sys_unix.c


# Game module sources
libgame_a_SOURCES =  \
	client/anorms.h \
	game/acesrc/acebot_ai.c \
	game/acesrc/acebot_cmds.c \
	game/acesrc/acebot.h \
	game/acesrc/acebot_items.c \
	game/acesrc/acebot_movement.c \
	game/acesrc/acebot_nodes.c \
	game/acesrc/acebot_spawn.c \
	game/c_cam.c \
	game/cow.h \
	game/g_ai.c \
	game/game.h \
	game/g_chase.c \
	game/g_cmds.c \
	game/g_combat.c \
	game/g_cow.c \
	game/g_ctf.c \
	game/g_deathball.c \
	game/g_deathray.c \
	game/g_deathray.h \
	game/g_func.c \
	game/g_items.c \
	game/g_local.h \
	game/g_main.c \
	game/g_misc.c \
	game/g_monster.c \
	game/g_phys.c \
	game/g_save.c \
	game/g_spawn.c \
	game/g_svcmds.c \
	game/g_target.c \
	game/g_trigger.c \
	game/g_unlagged.c \
	game/g_utils.c \
	game/g_vehicles.c \
	game/g_spider.c \
	game/g_spider.h \
	game/g_weapon.c \
	game/m_move.c \
	game/m_player.h \
	game/p_client.c \
	game/p_hud.c \
	game/p_trail.c \
	game/p_view.c \
	game/p_weapon.c \
	game/q_shared.c \
	game/q_shared.h \
	qcommon/qfiles.h
