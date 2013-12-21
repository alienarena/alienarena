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
# A copy of the  GNU General Public License is included;
# see the file, COPYING.
#

# 2013 December:
# - See notes in top-level Makefile.am, and configure.ac
# - The integrated ODE library option is now unnecessary and is removed.
# - And so, libode is now a required dependency for the client.
# - 'Alternate Install' is removed. There is a better way to do the equivalent.
# - The MinGW Windows variant complicates things and is removed, for now.
# - unix/files_unix.c replaces qcommon/files.c for Unix/GNU Linux
# - aa_tactical_demo client added here, for now.

noinst_LIBRARIES = libgame.a

DATADIR_CPPFLAGS = -DDATADIR='"$(pkgdatadir)"'

# Dedicated Server libraries and flags
alienarena_ded_LDADD = libgame.a
alienarena_ded_CPPFLAGS = $(DATADIR_CPPFLAGS) -DDEDICATED_ONLY

# Client libraries and flags
alienarena_CPPFLAGS = \
	$(DATADIR_CPPFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(X11_CLAGS) \
	$(DEPS_CFLAGS) \
	$(XXF86VM_CFLAGS) \
	$(XXF86DGA_CFLAGS) \
	$(ZLIB_CFLAGS)

alienarena_LDADD = libgame.a \
	$(PTHREAD_LIBS) \
	$(X11_LIBS) \
	$(DEPS_LIBS) \
	$(XXF86VM_LIBS) \
	$(XXF86DGA_LIBS) \
	$(ZLIB_LIBS)


aa_tactical_demo_CPPFLAGS = -DTACTICAL \
	$(DATADIR_CPPFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(X11_CLAGS) \
	$(DEPS_CFLAGS) \
	$(XXF86VM_CFLAGS) \
	$(XXF86DGA_CFLAGS) \
	$(ZLIB_CFLAGS)

aa_tactical_demo_LDADD = libgame.a \
	$(PTHREAD_LIBS) \
	$(X11_LIBS) \
	$(DEPS_LIBS) \
	$(XXF86VM_LIBS) \
	$(XXF86DGA_LIBS) \
	$(ZLIB_LIBS)


# On Unix, always build the server, and the client if BUILD_CLIENT is enabled
bin_PROGRAMS = alienarena-ded
if BUILD_CLIENT
bin_PROGRAMS += alienarena  aa_tactical_demo
endif

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
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/md5.c \
	qcommon/md5.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/anorms.h \
	ref_gl/anormtab.h \
	ref_gl/glext.h \
	ref_gl/glfuncs.h \
	ref_gl/qgl.h \
	ref_gl/r_bloom.c \
	ref_gl/r_draw.c \
	ref_gl/r_image.c \
	ref_gl/r_image.h \
	ref_gl/r_iqm.h \
	ref_gl/r_iqm.c \
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
	unix/gl_glx.c \
	unix/glob.c \
	unix/glob.h \
	unix/glw_unix.h \
	unix/net_udp.c \
	unix/qal_unix.c \
	unix/qgl_unix.c \
	unix/q_shunix.c \
	unix/rw_unix.c \
	unix/rw_unix.h \
	unix/sys_unix.c \
	unix/vid_so.c \
	unix/files_unix.c


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
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
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
	unix/sys_unix.c \
	unix/files_unix.c


aa_tactical_demo_SOURCES = \
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
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/md5.c \
	qcommon/md5.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/anorms.h \
	ref_gl/anormtab.h \
	ref_gl/glext.h \
	ref_gl/glfuncs.h \
	ref_gl/qgl.h \
	ref_gl/r_bloom.c \
	ref_gl/r_draw.c \
	ref_gl/r_image.c \
	ref_gl/r_image.h \
	ref_gl/r_iqm.h \
	ref_gl/r_iqm.c \
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
	unix/gl_glx.c \
	unix/glob.c \
	unix/glob.h \
	unix/glw_unix.h \
	unix/net_udp.c \
	unix/qal_unix.c \
	unix/qgl_unix.c \
	unix/q_shunix.c \
	unix/rw_unix.c \
	unix/rw_unix.h \
	unix/sys_unix.c \
	unix/vid_so.c \
	unix/files_unix.c


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
