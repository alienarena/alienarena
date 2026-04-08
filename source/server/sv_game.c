/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_game.c -- interface to the game module

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "server.h"



// Minimal IQM structs needed for reading model bounds server-side.
// (The full IQM header lives in ref_gl/r_iqm.h which the server doesn't include.)
typedef struct {
	char         id[16];
	unsigned int version;
	unsigned int filesize;
	unsigned int flags;
	unsigned int num_text, ofs_text;
	unsigned int num_meshes, ofs_meshes;
	unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
	unsigned int num_triangles, ofs_triangles, ofs_neighbors;
	unsigned int num_joints, ofs_joints;
	unsigned int num_poses, ofs_poses;
	unsigned int num_anims, ofs_anims;
	unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
	unsigned int num_comment, ofs_comment;
	unsigned int num_extensions, ofs_extensions;
} sv_iqmheader_t;

typedef struct {
	float mins[3], maxs[3];
	float xyradius, radius;
} sv_iqmbounds_t;

game_export_t	*ge;


/*
===============
PF_Unicast

Sends the contents of the mutlicast buffer to a single client
===============
*/
void PF_Unicast (edict_t *ent, qboolean reliable)
{
	int		p;
	client_t	*client;

	if (!ent)
		return;

	p = NUM_FOR_EDICT(ent);
	if (p < 1 || p > maxclients->value)
		return;

	client = svs.clients + (p-1);

	if (reliable)
		SZ_Write (&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
	else
		SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);

	SZ_Clear (&sv.multicast);
}


/*
===============
PF_dprintf

Debug print to server console
===============
*/
void PF_dprintf (char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start (argptr,fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Printf ("%s", msg);
}


/*
===============
PF_cprintf

Print to a single client
===============
*/
void PF_cprintf (edict_t *ent, int level, char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n;

	if (ent)
	{
		n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > maxclients->value)
			Com_Error (ERR_DROP, "cprintf to a non-client");
	}

	va_start (argptr,fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if (ent)
		SV_ClientPrintf (svs.clients+(n-1), level, "%s", msg);
	else
		Com_Printf ("%s", msg);
}


/*
===============
PF_centerprintf

centerprint to a single client
===============
*/
void PF_centerprintf (edict_t *ent, char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n;

	n = NUM_FOR_EDICT(ent);
	if (n < 1 || n > maxclients->value)
		return;	// Com_Error (ERR_DROP, "centerprintf to a non-client");

	va_start (argptr,fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multicast,svc_centerprint);
	MSG_WriteString (&sv.multicast,msg);
	PF_Unicast (ent, true);
}


/*
===============
PF_error

Abort the server with a game error
===============
*/
void PF_error (char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start (argptr,fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Com_Error (ERR_DROP, "Game Error: %s", msg);
}


/*
=================
SV_ReadModelBounds

Reads the bounding box of a model file into mins/maxs.
Mirrors the renderer's IQM-over-MD2 preference: tries a .iqm file first
(replacing extension if needed), then falls back to .md2, then to ±16.
=================
*/
static void SV_ReadModelBounds (const char *name, vec3_t mins, vec3_t maxs)
{
	int			loaded = 0;
	int			i;
	const char	*ext = strrchr(name, '.');

	// --- Try IQM (either directly or by swapping .md2 -> .iqm) ---
	if (ext && (!Q_strcasecmp(ext, ".md2") || !Q_strcasecmp(ext, ".iqm"))) {
		char	iqmname[MAX_QPATH];
		void	*buf = NULL;
		int		len;

		if (!Q_strcasecmp(ext, ".md2")) {
			COM_StripExtension(name, iqmname);
			strcat(iqmname, ".iqm");
		} else {
			strncpy(iqmname, name, sizeof(iqmname) - 1);
			iqmname[sizeof(iqmname) - 1] = '\0';
		}

		len = FS_LoadFile(iqmname, &buf);
		if (buf && len > (int)sizeof(sv_iqmheader_t)) {
			sv_iqmheader_t *hdr = (sv_iqmheader_t *)buf;
			unsigned int ofs_b = LittleLong(hdr->ofs_bounds);
			unsigned int num_f = LittleLong(hdr->num_frames);
			if (!memcmp(hdr->id, "INTERQUAKEMODEL", 16) &&
			    (LittleLong(hdr->version) == 1 || LittleLong(hdr->version) == 2) &&
			    num_f > 0 && ofs_b != 0 &&
			    ofs_b + (int)sizeof(sv_iqmbounds_t) <= (unsigned int)len) {
				sv_iqmbounds_t *bounds = (sv_iqmbounds_t *)((byte *)buf + ofs_b);
				for (i = 0; i < 3; i++) {
					mins[i] = LittleFloat(bounds->mins[i]);
					maxs[i] = LittleFloat(bounds->maxs[i]);
				}
				loaded = 1;
			}
		}
		if (buf)
			FS_FreeFile(buf);
	}

	// --- Try MD2 (only when original name was .md2 and IQM failed) ---
	if (!loaded && ext && !Q_strcasecmp(ext, ".md2")) {
		void	*buf = NULL;
		int		len = FS_LoadFile(name, &buf);
		if (buf && len > (int)sizeof(dmdl_t)) {
			dmdl_t	*hdr = (dmdl_t *)buf;
			if (LittleLong(hdr->ident) == IDALIASHEADER &&
			    LittleLong(hdr->version) == ALIAS_VERSION &&
			    LittleLong(hdr->ofs_frames) + (int)sizeof(daliasframe_t) <= len) {
				daliasframe_t *frame = (daliasframe_t *)((byte *)buf + LittleLong(hdr->ofs_frames));
				for (i = 0; i < 3; i++) {
					mins[i] = LittleFloat(frame->translate[i]);
					maxs[i] = mins[i] + LittleFloat(frame->scale[i]) * 255.0f;
				}
				loaded = 1;
			}
		}
		if (buf)
			FS_FreeFile(buf);
	}

	if (!loaded) {
		// Fallback for unknown formats or IQM without frame bounds
		VectorSet(mins, -16, -16, -16);
		VectorSet(maxs,  16,  16,  16);
	}
}

/*
=================
PF_setmodel

Also sets mins and maxs for inline bmodels
For regular models, sets default mins/maxs to prevent culling issues
=================
*/
extern float sv_model_bounds[MAX_MODELS];
void PF_setmodel (edict_t *ent, char *name)
{
	int		i;
	cmodel_t	*mod;

	if (!name)
		return;
		//Com_Error (ERR_DROP, "PF_setmodel: NULL");

	i = SV_ModelIndex (name);

//	ent->model = name;
	ent->s.modelindex = i;

// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		mod = CM_InlineModel (name);
		VectorCopy (mod->mins, ent->mins);
		VectorCopy (mod->maxs, ent->maxs);
		SV_LinkEdict (ent);
	}
	else if (name[0] != 0)
	{
		// Cache model bounds for angular-size culling (side-array, not ent->mins/maxs,
		// so we don't change the entity's PVS leaf placement).
		if (i > 0 && i < MAX_MODELS && sv_model_bounds[i] == 0.0f) {
			vec3_t bmins, bmaxs;
			SV_ReadModelBounds(name, bmins, bmaxs);
			float sx = bmaxs[0] - bmins[0];
			float sy = bmaxs[1] - bmins[1];
			float sz = bmaxs[2] - bmins[2];
			float ms = sx > sy ? sx : sy;
			if (sz > ms) ms = sz;
			if (ms < 1.0f) ms = 1.0f;
			sv_model_bounds[i] = ms;
		}

		SV_LinkEdict (ent);
	}

}

/*
===============
PF_Configstring

===============
*/
void PF_Configstring (int index, char *val)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error (ERR_DROP, "configstring: bad index %i\n", index);

	if (!val)
		val = "";

	// change the string in sv
	strcpy (sv.configstrings[index], val);


	if (sv.state != ss_loading)
	{	// send the update to everyone
		SZ_Clear (&sv.multicast);
		MSG_WriteChar (&sv.multicast, svc_configstring);
		MSG_WriteShort (&sv.multicast, index);
		MSG_WriteString (&sv.multicast, val);

		SV_Multicast (vec3_origin, MULTICAST_ALL_R);
	}
}



void PF_WriteChar (int c) {MSG_WriteChar (&sv.multicast, c);}
void PF_WriteByte (int c) {MSG_WriteByte (&sv.multicast, c);}
void PF_WriteShort (int c) {MSG_WriteShort (&sv.multicast, c);}
void PF_WriteLong (int c) {MSG_WriteLong (&sv.multicast, c);}
void PF_WriteFloat (float f) {MSG_WriteFloat (&sv.multicast, f);}
void PF_WriteString (char *s) {MSG_WriteString (&sv.multicast, s);}
void PF_WritePos (vec3_t pos) {MSG_WritePos (&sv.multicast, pos);}
void PF_WriteDir (vec3_t dir) {MSG_WriteDir (&sv.multicast, dir);}
void PF_WriteAngle (float f) {MSG_WriteAngle (&sv.multicast, f);}


void PF_StartSound (edict_t *entity, int channel, int sound_num, float volume,
    float attenuation, float timeofs)
{
	if (!entity)
		return;
	SV_StartSound (NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}

//==============================================

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs (void)
{
	if (!ge)
		return;
	ge->Shutdown ();
	Sys_UnloadGame ();
	ge = NULL;
}

/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/
void SCR_DebugGraph (float value, const float color[]);

void SV_InitGameProgs (void)
{
	game_import_t	import;

	// unload anything we have now
	if (ge)
		SV_ShutdownGameProgs ();


	// load a new game dll
	import.multicast = SV_Multicast;
	import.unicast = PF_Unicast;
	import.bprintf = SV_BroadcastPrintf;
	import.dprintf = PF_dprintf;
	import.cprintf = PF_cprintf;
	import.centerprintf = PF_centerprintf;
	import.error = PF_error;

	import.linkentity = SV_LinkEdict;
	import.unlinkentity = SV_UnlinkEdict;
	import.BoxEdicts = SV_AreaEdicts;
	import.trace = SV_Trace;
	import.pointcontents = SV_PointContents;
	import.setmodel = PF_setmodel;
	import.inPVS = CM_inPVS;
	import.inPHS = CM_inPHS;
	import.Pmove = Pmove;

	import.modelindex = SV_ModelIndex;
	import.soundindex = SV_SoundIndex;
	import.imageindex = SV_ImageIndex;
	
	import.checkmodelindex = SV_CheckModelIndex;
	import.checksoundindex = SV_CheckSoundIndex;
	import.checkimageindex = SV_CheckImageIndex;

	import.configstring = PF_Configstring;
	import.sound = PF_StartSound;
	import.positioned_sound = SV_StartSound;

	import.WriteChar = PF_WriteChar;
	import.WriteByte = PF_WriteByte;
	import.WriteShort = PF_WriteShort;
	import.WriteLong = PF_WriteLong;
	import.WriteFloat = PF_WriteFloat;
	import.WriteString = PF_WriteString;
	import.WritePosition = PF_WritePos;
	import.WriteDir = PF_WriteDir;
	import.WriteAngle = PF_WriteAngle;

	import.TagMalloc = Z_TagMalloc;
	import.TagFree = Z_Free;
	import.FreeTags = Z_FreeTags;

	import.cvar = Cvar_Get;
	import.cvar_set = Cvar_Set;
	import.cvar_forceset = Cvar_ForceSet;
	import.cvar_describe = Cvar_Describe;

	import.argc = Cmd_Argc;
	import.argv = Cmd_Argv;
	import.args = Cmd_Args;
	import.AddCommandString = Cbuf_AddText;

	import.DebugGraph = SCR_DebugGraph;
	import.SetAreaPortalState = CM_SetAreaPortalState;
	import.AreasConnected = CM_AreasConnected;

	import.Sys_Milliseconds = Sys_Milliseconds;

	import.FullPath = FS_FullPath;
	import.FullWritePath = FS_FullWritePath;

	ge = (game_export_t *)Sys_GetGameAPI (&import);

	if (!ge)
		Com_Error (ERR_DROP, "failed to load game module");

	if (ge->apiversion != GAME_API_VERSION)
		Com_Error (ERR_DROP, "game is version %i, not %i", ge->apiversion,
		GAME_API_VERSION);

	ge->Init ();
}

