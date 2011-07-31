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

if USE_SYSTEM_LIBODE

noinst_LIBRARIES = libgame.a

else

if BUILD_CLIENT
noinst_LIBRARIES = libgame.a libode.a
else
noinst_LIBRARIES = libgame.a
endif

endif

#
# Setup the integrated ODE library.
# note: the -isystem option is for making #include <ode/ode.h> look in
#  the build directory before the system directory.
#
if USE_SYSTEM_LIBODE
AM_CPPFLAGS = -I $(top_srcdir)/source
else
AM_CPPFLAGS = -I $(top_srcdir)/source -isystem $(top_srcdir)/source/unix
endif

# CPP flags for standard installation
if !ALTERNATE_INSTALL
AM_CPPFLAGS += -DDATADIR='"$(pkgdatadir)"'
endif

# Dedicated Server libraries and flags
crx_ded_LDADD = libgame.a
crx_ded_CPPFLAGS = $(AM_CPPFLAGS) -DDEDICATED_ONLY

# Client libraries and flags
crx_CPPFLAGS = $(AM_CPPFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(X11_CLAGS) \
	$(DEPS_CFLAGS) \
	$(ODE_CFLAGS) \
	$(XXF86VM_CFLAGS) \
	$(XXF86DGA_CFLAGS)

if USE_SYSTEM_LIBODE
#
# crx with system libode
#
crx_LDADD = libgame.a \
	$(PTHREAD_LIBS) \
	$(X11_LIBS) \
	$(DEPS_LIBS) \
	$(ODE_LIBS) \
	$(XXF86VM_LIBS) \
	$(XXF86DGA_LIBS)

else
#
# crx with integrated libode
#
crx_LDADD = libgame.a \
	libode.a \
	$(PTHREAD_LIBS) \
	$(X11_LIBS) \
	$(DEPS_LIBS) \
	$(XXF86VM_LIBS) \
	$(XXF86DGA_LIBS) \
	-lstdc++
	
libode_a_CPPFLAGS = -I$(top_srcdir)/source/unix/odesrc/OPCODE \
	-isystem $(top_srcdir)/source/unix \
	-DdDOUBLE \
	-DdTRIMESH_ENABLED \
	-DdTRIMESH_OPCODE
	
if ODE_NODEBUG
libode_a_CPPFLAGS += -DNDEBUG -DdNODEBUG
endif
	
endif

if BUILD_WIN32

# On Win32, always build the client
# TODO: maybe fixup libs, cppflags hacks
bin_PROGRAMS = crx
crx_LDADD += $(WIN32_LIBS)
crx_CPPFLAGS += -DCURL_STATICLIB -DNDEBUG

else

# On Unix, always build the server, and the client if BUILD_CLIENT is enabled
bin_PROGRAMS = crx-ded
if BUILD_CLIENT
bin_PROGRAMS += crx
endif

endif

# common client sources
crx_SOURCES = \
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
	client/vid_menu.c \
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
	qcommon/mdfour.c \
	qcommon/sha2.c \
	qcommon/hmac_sha2.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	ref_gl/anorms.h \
	ref_gl/anormtab.h \
	ref_gl/glext.h \
	ref_gl/qgl.h \
	ref_gl/r_bloom.c \
	ref_gl/r_bmfont.c \
	ref_gl/r_bmfont.h \
	ref_gl/r_draw.c \
	ref_gl/r_image.c \
	ref_gl/r_image.h \
	ref_gl/r_iqm.h \
	ref_gl/r_iqm.c \
	ref_gl/r_light.c \
	ref_gl/r_local.h \
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
crx_SOURCES += \
	unix/gl_glx.c \
	unix/glw_unix.h \
	unix/net_udp.c \
	unix/qal_unix.c \
	unix/qasm.h \
	unix/qgl_unix.c \
	unix/q_shunix.c \
	unix/rw_unix.c \
	unix/rw_unix.h \
	unix/sys_unix.c \
	unix/vid_so.c
endif	

# Win32 MinGW Client sources
if BUILD_WIN32
crx_SOURCES += \
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
crx_ded_SOURCES = \
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
	
#
# Integrated Open Dynamic Engine (ODE) Library
#
libode_a_SOURCES = \
	unix/ode/collision.h \
	unix/ode/collision_space.h \
	unix/ode/collision_trimesh.h \
	unix/ode/common.h \
	unix/ode/compatibility.h \
	unix/ode/contact.h \
	unix/ode/error.h \
	unix/ode/export-dif.h \
	unix/ode/mass.h \
	unix/ode/matrix.h \
	unix/ode/memory.h \
	unix/ode/misc.h \
	unix/ode/objects.h \
	unix/ode/odeconfig.h \
	unix/ode/odecpp_collision.h \
	unix/ode/odecpp.h \
	unix/ode/ode.h \
	unix/ode/odeinit.h \
	unix/ode/odemath.h \
	unix/ode/rotation.h \
	unix/ode/timer.h
	
libode_a_SOURCES += \
	unix/odesrc/array.cpp \
	unix/odesrc/array.h \
	unix/odesrc/box.cpp \
	unix/odesrc/capsule.cpp \
	unix/odesrc/collision_cylinder_box.cpp \
	unix/odesrc/collision_cylinder_plane.cpp \
	unix/odesrc/collision_cylinder_sphere.cpp \
	unix/odesrc/collision_kernel.cpp \
	unix/odesrc/collision_kernel.h \
	unix/odesrc/collision_quadtreespace.cpp \
	unix/odesrc/collision_sapspace.cpp \
	unix/odesrc/collision_space.cpp \
	unix/odesrc/collision_space_internal.h \
	unix/odesrc/collision_std.h \
	unix/odesrc/collision_transform.cpp \
	unix/odesrc/collision_transform.h \
	unix/odesrc/collision_trimesh_colliders.h \
	unix/odesrc/collision_trimesh_disabled.cpp \
	unix/odesrc/collision_trimesh_internal.h \
	unix/odesrc/collision_util.cpp \
	unix/odesrc/collision_util.h \
	unix/odesrc/convex.cpp \
	unix/odesrc/cylinder.cpp \
	unix/odesrc/error.cpp \
	unix/odesrc/export-dif.cpp \
	unix/odesrc/heightfield.cpp \
	unix/odesrc/heightfield.h \
	unix/odesrc/lcp.cpp \
	unix/odesrc/lcp.h \
	unix/odesrc/mass.cpp \
	unix/odesrc/mat.cpp \
	unix/odesrc/mat.h \
	unix/odesrc/matrix.cpp \
	unix/odesrc/memory.cpp \
	unix/odesrc/misc.cpp \
	unix/odesrc/objects.h \
	unix/odesrc/obstack.cpp \
	unix/odesrc/obstack.h \
	unix/odesrc/ode.cpp \
	unix/odesrc/odeinit.cpp \
	unix/odesrc/odemath.cpp \
	unix/odesrc/odeou.h \
	unix/odesrc/odetls.h \
	unix/odesrc/plane.cpp \
	unix/odesrc/quickstep.cpp \
	unix/odesrc/quickstep.h \
	unix/odesrc/ray.cpp \
	unix/odesrc/rotation.cpp \
	unix/odesrc/sphere.cpp \
	unix/odesrc/step.cpp \
	unix/odesrc/stepfast.cpp \
	unix/odesrc/step.h \
	unix/odesrc/testing.cpp \
	unix/odesrc/testing.h \
	unix/odesrc/timer.cpp \
	unix/odesrc/util.cpp \
	unix/odesrc/util.h

libode_a_SOURCES += \
	unix/odesrc/fastldlt.c \
	unix/odesrc/fastltsolve.c \
	unix/odesrc/fastdot.c \
	unix/odesrc/fastlsolve.c

libode_a_SOURCES += \
	unix/odesrc/collision_cylinder_trimesh.cpp \
	unix/odesrc/collision_trimesh_box.cpp \
	unix/odesrc/collision_trimesh_ccylinder.cpp \
	unix/odesrc/collision_trimesh_distance.cpp \
	unix/odesrc/collision_trimesh_internal.h \
	unix/odesrc/collision_trimesh_opcode.cpp \
	unix/odesrc/collision_trimesh_plane.cpp \
	unix/odesrc/collision_trimesh_ray.cpp \
	unix/odesrc/collision_trimesh_sphere.cpp \
	unix/odesrc/collision_trimesh_trimesh.cpp \
	unix/odesrc/collision_trimesh_trimesh_new.cpp
	
libode_a_SOURCES += \
	unix/odesrc/joints/amotor.cpp \
	unix/odesrc/joints/amotor.h \
	unix/odesrc/joints/ball.cpp \
	unix/odesrc/joints/ball.h \
	unix/odesrc/joints/contact.cpp \
	unix/odesrc/joints/contact.h \
	unix/odesrc/joints/fixed.cpp \
	unix/odesrc/joints/fixed.h \
	unix/odesrc/joints/hinge2.cpp \
	unix/odesrc/joints/hinge2.h \
	unix/odesrc/joints/hinge.cpp \
	unix/odesrc/joints/hinge.h \
	unix/odesrc/joints/joint.cpp \
	unix/odesrc/joints/joint.h \
	unix/odesrc/joints/joint_internal.h \
	unix/odesrc/joints/joints.h \
	unix/odesrc/joints/lmotor.cpp \
	unix/odesrc/joints/lmotor.h \
	unix/odesrc/joints/null.cpp \
	unix/odesrc/joints/null.h \
	unix/odesrc/joints/piston.cpp \
	unix/odesrc/joints/piston.h \
	unix/odesrc/joints/plane2d.cpp \
	unix/odesrc/joints/plane2d.h \
	unix/odesrc/joints/pr.cpp \
	unix/odesrc/joints/pr.h \
	unix/odesrc/joints/pu.cpp \
	unix/odesrc/joints/pu.h \
	unix/odesrc/joints/slider.cpp \
	unix/odesrc/joints/slider.h \
	unix/odesrc/joints/universal.cpp \
	unix/odesrc/joints/universal.h
	
libode_a_SOURCES += \
	unix/odesrc/OPCODE/OPC_AABBCollider.cpp \
	unix/odesrc/OPCODE/OPC_AABBCollider.h \
	unix/odesrc/OPCODE/OPC_AABBTree.cpp \
	unix/odesrc/OPCODE/OPC_AABBTree.h \
	unix/odesrc/OPCODE/OPC_BaseModel.cpp \
	unix/odesrc/OPCODE/OPC_BaseModel.h \
	unix/odesrc/OPCODE/OPC_BoxBoxOverlap.h \
	unix/odesrc/OPCODE/OPC_Collider.cpp \
	unix/odesrc/OPCODE/OPC_Collider.h \
	unix/odesrc/OPCODE/OPC_Common.cpp \
	unix/odesrc/OPCODE/OPC_Common.h \
	unix/odesrc/OPCODE/OPC_HybridModel.cpp \
	unix/odesrc/OPCODE/OPC_HybridModel.h \
	unix/odesrc/OPCODE/OPC_IceHook.h \
	unix/odesrc/OPCODE/OPC_LSSAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_LSSCollider.cpp \
	unix/odesrc/OPCODE/OPC_LSSCollider.h \
	unix/odesrc/OPCODE/OPC_LSSTriOverlap.h \
	unix/odesrc/OPCODE/OPC_MeshInterface.cpp  \
	unix/odesrc/OPCODE/OPC_MeshInterface.h \
	unix/odesrc/OPCODE/OPC_Model.cpp  \
	unix/odesrc/OPCODE/OPC_Model.h \
	unix/odesrc/OPCODE/OPC_OBBCollider.cpp \
	unix/odesrc/OPCODE/OPC_OBBCollider.h \
	unix/odesrc/OPCODE/Opcode.cpp  \
	unix/odesrc/OPCODE/Opcode.h \
	unix/odesrc/OPCODE/OPC_OptimizedTree.cpp \
	unix/odesrc/OPCODE/OPC_OptimizedTree.h \
	unix/odesrc/OPCODE/OPC_Picking.cpp \
	unix/odesrc/OPCODE/OPC_Picking.h \
	unix/odesrc/OPCODE/OPC_PlanesAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_PlanesCollider.cpp \
	unix/odesrc/OPCODE/OPC_PlanesCollider.h \
	unix/odesrc/OPCODE/OPC_PlanesTriOverlap.h \
	unix/odesrc/OPCODE/OPC_RayAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_RayCollider.cpp \
	unix/odesrc/OPCODE/OPC_RayCollider.h \
	unix/odesrc/OPCODE/OPC_RayTriOverlap.h \
	unix/odesrc/OPCODE/OPC_Settings.h \
	unix/odesrc/OPCODE/OPC_SphereAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_SphereCollider.cpp \
	unix/odesrc/OPCODE/OPC_SphereCollider.h \
	unix/odesrc/OPCODE/OPC_SphereTriOverlap.h \
	unix/odesrc/OPCODE/OPC_TreeBuilders.cpp \
	unix/odesrc/OPCODE/OPC_TreeBuilders.h \
	unix/odesrc/OPCODE/OPC_TreeCollider.cpp \
	unix/odesrc/OPCODE/OPC_TreeCollider.h \
	unix/odesrc/OPCODE/OPC_TriBoxOverlap.h \
	unix/odesrc/OPCODE/OPC_TriTriOverlap.h \
	unix/odesrc/OPCODE/OPC_VolumeCollider.cpp \
	unix/odesrc/OPCODE/OPC_VolumeCollider.h \
	unix/odesrc/OPCODE/Stdafx.h
	
	
libode_a_SOURCES += \
	unix/odesrc/OPCODE/Ice/IceAABB.cpp \
	unix/odesrc/OPCODE/Ice/IceAABB.h \
	unix/odesrc/OPCODE/Ice/IceAxes.h \
	unix/odesrc/OPCODE/Ice/IceBoundingSphere.h \
	unix/odesrc/OPCODE/Ice/IceContainer.cpp \
	unix/odesrc/OPCODE/Ice/IceContainer.h \
	unix/odesrc/OPCODE/Ice/IceFPU.h \
	unix/odesrc/OPCODE/Ice/IceHPoint.cpp \
	unix/odesrc/OPCODE/Ice/IceHPoint.h \
	unix/odesrc/OPCODE/Ice/IceIndexedTriangle.cpp \
	unix/odesrc/OPCODE/Ice/IceIndexedTriangle.h \
	unix/odesrc/OPCODE/Ice/IceLSS.h \
	unix/odesrc/OPCODE/Ice/IceMatrix3x3.cpp \
	unix/odesrc/OPCODE/Ice/IceMatrix3x3.h \
	unix/odesrc/OPCODE/Ice/IceMatrix4x4.cpp \
	unix/odesrc/OPCODE/Ice/IceMatrix4x4.h \
	unix/odesrc/OPCODE/Ice/IceMemoryMacros.h \
	unix/odesrc/OPCODE/Ice/IceOBB.cpp \
	unix/odesrc/OPCODE/Ice/IceOBB.h \
	unix/odesrc/OPCODE/Ice/IcePairs.h \
	unix/odesrc/OPCODE/Ice/IcePlane.cpp \
	unix/odesrc/OPCODE/Ice/IcePlane.h \
	unix/odesrc/OPCODE/Ice/IcePoint.cpp \
	unix/odesrc/OPCODE/Ice/IcePoint.h \
	unix/odesrc/OPCODE/Ice/IcePreprocessor.h \
	unix/odesrc/OPCODE/Ice/IceRandom.cpp \
	unix/odesrc/OPCODE/Ice/IceRandom.h \
	unix/odesrc/OPCODE/Ice/IceRay.cpp \
	unix/odesrc/OPCODE/Ice/IceRay.h \
	unix/odesrc/OPCODE/Ice/IceRevisitedRadix.cpp \
	unix/odesrc/OPCODE/Ice/IceRevisitedRadix.h \
	unix/odesrc/OPCODE/Ice/IceSegment.cpp \
	unix/odesrc/OPCODE/Ice/IceSegment.h \
	unix/odesrc/OPCODE/Ice/IceTriangle.cpp \
	unix/odesrc/OPCODE/Ice/IceTriangle.h \
	unix/odesrc/OPCODE/Ice/IceTriList.h \
	unix/odesrc/OPCODE/Ice/IceTypes.h \
	unix/odesrc/OPCODE/Ice/IceUtils.cpp \
	unix/odesrc/OPCODE/Ice/IceUtils.h
	
