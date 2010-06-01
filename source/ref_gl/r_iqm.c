#include "r_local.h"
#include "r_iqm.h"
#include "r_matrixlib.h"

extern  void Q_strncpyz( char *dest, const char *src, size_t size );
qboolean testflag;

extern 
void R_LoadIQMVertexArrays(model_t *iqmmodel, float *vposition)
{
	int i;

	Com_Printf("numverts: %i\n", iqmmodel->numvertexes);

	if(iqmmodel->numvertexes > 16384)
		return;

	iqmmodel->vertexes = (mvertex_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mvertex_t));

	for(i=0; i<iqmmodel->numvertexes; i++){
		VectorSet(iqmmodel->vertexes[i].position,
					LittleFloat(vposition[0]),
					LittleFloat(vposition[1]),
					LittleFloat(vposition[2]));		
		vposition += 3;
	}
	
}

qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer)
{
	const char *text;
	iqmheader_t *header;
	int i, j, k;
	const int *inelements;
	float *vposition = NULL, *vtexcoord = NULL, *vnormal = NULL, *vtangent = NULL;
	unsigned char *vblendindexes = NULL, *vblendweights = NULL;
	unsigned char *pbase;
	iqmjoint_t *joint;
	iqmpose_t *pose;
	iqmbounds_t *bounds;
	iqmvertexarray_t *va;
	unsigned short *framedata;
	float biggestorigin;

	pbase = (unsigned char *)buffer;
	header = (iqmheader_t *)buffer;
	if (memcmp(header->id, "INTERQUAKEMODEL", 16)) {
		Com_Printf ("Mod_INTERQUAKEMODEL_Load: %s is not an Inter-Quake Model", mod->name);
		return false;
	}

	if (LittleLong(header->version) != 1) {
		Com_Printf ("Mod_INTERQUAKEMODEL_Load: only version 1 models are currently supported (name = %s)", mod->name);
		return false;
	}

	mod->type = mod_iqm;

	// byteswap header
	header->version = LittleLong(header->version);
	header->filesize = LittleLong(header->filesize);
	header->flags = LittleLong(header->flags);
	header->num_text = LittleLong(header->num_text);
	header->ofs_text = LittleLong(header->ofs_text);
	header->num_meshes = LittleLong(header->num_meshes);
	header->ofs_meshes = LittleLong(header->ofs_meshes);
	header->num_vertexarrays = LittleLong(header->num_vertexarrays);
	header->num_vertexes = LittleLong(header->num_vertexes);
	header->ofs_vertexarrays = LittleLong(header->ofs_vertexarrays);
	header->num_triangles = LittleLong(header->num_triangles);
	header->ofs_triangles = LittleLong(header->ofs_triangles);
	header->ofs_neighbors = LittleLong(header->ofs_neighbors);
	header->num_joints = LittleLong(header->num_joints);
	header->ofs_joints = LittleLong(header->ofs_joints);
	header->num_poses = LittleLong(header->num_poses);
	header->ofs_poses = LittleLong(header->ofs_poses);
	header->num_anims = LittleLong(header->num_anims);
	header->ofs_anims = LittleLong(header->ofs_anims);
	header->num_frames = LittleLong(header->num_frames);
	header->num_framechannels = LittleLong(header->num_framechannels);
	header->ofs_frames = LittleLong(header->ofs_frames);
	header->num_comment = LittleLong(header->num_comment);
	header->ofs_comment = LittleLong(header->ofs_comment);
	header->num_extensions = LittleLong(header->num_extensions);
	header->ofs_extensions = LittleLong(header->ofs_extensions);

	if (header->num_triangles < 1 || header->num_vertexes < 3 || header->num_vertexarrays < 1 || header->num_meshes < 1)
	{
		Com_Printf("%s has no geometry\n", mod->name);
		return false;
	}
	if (header->num_frames < 1 || header->num_anims < 1)
	{
		Com_Printf("%s has no animations\n", mod->name);
		return false;
	}

	va = (iqmvertexarray_t *)(pbase + header->ofs_vertexarrays);
	for (i = 0;i < (int)header->num_vertexarrays;i++)
	{
		va[i].type = LittleLong(va[i].type);
		va[i].flags = LittleLong(va[i].flags);
		va[i].format = LittleLong(va[i].format);
		va[i].size = LittleLong(va[i].size);
		va[i].offset = LittleLong(va[i].offset);
		switch (va[i].type)
		{
		case IQM_POSITION:
			if (va[i].format == IQM_FLOAT && va[i].size == 3)
				vposition = (float *)(pbase + va[i].offset);
			break;
		case IQM_TEXCOORD:
			if (va[i].format == IQM_FLOAT && va[i].size == 2)
				vtexcoord = (float *)(pbase + va[i].offset);
			break;
		case IQM_NORMAL:
			if (va[i].format == IQM_FLOAT && va[i].size == 3)
				vnormal = (float *)(pbase + va[i].offset);
			break;
		case IQM_TANGENT:
			if (va[i].format == IQM_FLOAT && va[i].size == 4)
				vtangent = (float *)(pbase + va[i].offset);
			break;
		case IQM_BLENDINDEXES:
			if (va[i].format == IQM_UBYTE && va[i].size == 4)
				vblendindexes = (unsigned char *)(pbase + va[i].offset);
			break;
		case IQM_BLENDWEIGHTS:
			if (va[i].format == IQM_UBYTE && va[i].size == 4)
				vblendweights = (unsigned char *)(pbase + va[i].offset);
			break;
		}
	}
	if (!vposition || !vtexcoord || !vblendindexes || !vblendweights)
	{
		Com_Printf("%s is missing vertex array data\n", mod->name);
		return false;
	}

	text = header->num_text && header->ofs_text ? (const char *)(pbase + header->ofs_text) : "";
	
	mod->num_frames = header->num_anims;
	mod->num_bones = header->num_joints;
	mod->num_poses = header->num_frames;
	mod->numvertexes = header->num_vertexes;
	mod->num_triangles = header->num_triangles;

	mod->extradata = Hunk_Begin (0x300000); 

	// load the bone info
	joint = (iqmjoint_t *) (pbase + header->ofs_joints);
	mod->bones = (aliasbone_t*)Hunk_Alloc (header->num_joints * sizeof(aliasbone_t));
	mod->baseboneposeinverse = (float*)Hunk_Alloc (header->num_joints * sizeof(float));
	for (i = 0;i < mod->num_bones;i++)
	{
		matrix4x4_t relbase, relinvbase, pinvbase, invbase;
		joint[i].name = LittleLong(joint[i].name);
		joint[i].parent = LittleLong(joint[i].parent);
		for (j = 0;j < 3;j++)
		{
			joint[i].origin[j] = LittleFloat(joint[i].origin[j]);
			joint[i].rotation[j] = LittleFloat(joint[i].rotation[j]);
			joint[i].scale[j] = LittleFloat(joint[i].scale[j]);
		}
		Q_strncpyz(mod->bones[i].name, &text[joint[i].name], sizeof(mod->bones[i].name));
		mod->bones[i].parent = joint[i].parent;
		if (mod->bones[i].parent >= i)
			Com_Printf("%s bone[%i].parent >= %i", mod->name, i, i);
		Matrix4x4_FromDoom3Joint(&relbase, joint[i].origin[0], joint[i].origin[1], joint[i].origin[2], joint[i].rotation[0], joint[i].rotation[1], joint[i].rotation[2]);
		Matrix4x4_Invert_Simple(&relinvbase, &relbase);
		if (mod->bones[i].parent >= 0)
		{
			Matrix4x4_FromArray12FloatD3D(&pinvbase, mod->baseboneposeinverse + 12*mod->bones[i].parent);
			Matrix4x4_Concat(&invbase, &relinvbase, &pinvbase);
			Matrix4x4_ToArray12FloatD3D(&invbase, mod->baseboneposeinverse + 12*i);
		}	
		else Matrix4x4_ToArray12FloatD3D(&relinvbase, mod->baseboneposeinverse + 12*i);
	}

	pose = (iqmpose_t *) (pbase + header->ofs_poses);
	biggestorigin = 0;
	for (i = 0;i < (int)header->num_poses;i++)
	{
		float f;
		pose[i].parent = LittleLong(pose[i].parent);
		pose[i].channelmask = LittleLong(pose[i].channelmask);
		pose[i].channeloffset[0] = LittleFloat(pose[i].channeloffset[0]);
		pose[i].channeloffset[1] = LittleFloat(pose[i].channeloffset[1]);
		pose[i].channeloffset[2] = LittleFloat(pose[i].channeloffset[2]);	
		pose[i].channeloffset[3] = LittleFloat(pose[i].channeloffset[3]);
		pose[i].channeloffset[4] = LittleFloat(pose[i].channeloffset[4]);
		pose[i].channeloffset[5] = LittleFloat(pose[i].channeloffset[5]);
		pose[i].channeloffset[6] = LittleFloat(pose[i].channeloffset[6]);
		pose[i].channeloffset[7] = LittleFloat(pose[i].channeloffset[7]);
		pose[i].channeloffset[8] = LittleFloat(pose[i].channeloffset[8]);
		pose[i].channelscale[0] = LittleFloat(pose[i].channelscale[0]);
		pose[i].channelscale[1] = LittleFloat(pose[i].channelscale[1]);
		pose[i].channelscale[2] = LittleFloat(pose[i].channelscale[2]);
		pose[i].channelscale[3] = LittleFloat(pose[i].channelscale[3]);
		pose[i].channelscale[4] = LittleFloat(pose[i].channelscale[4]);
		pose[i].channelscale[5] = LittleFloat(pose[i].channelscale[5]);
		pose[i].channelscale[6] = LittleFloat(pose[i].channelscale[6]);
		pose[i].channelscale[7] = LittleFloat(pose[i].channelscale[7]);
		pose[i].channelscale[8] = LittleFloat(pose[i].channelscale[8]);
		f = fabs(pose[i].channeloffset[0]); biggestorigin = max(biggestorigin, f);
		f = fabs(pose[i].channeloffset[1]); biggestorigin = max(biggestorigin, f);
		f = fabs(pose[i].channeloffset[2]); biggestorigin = max(biggestorigin, f);
		f = fabs(pose[i].channeloffset[0] + 0xFFFF*pose[i].channelscale[0]); biggestorigin = max(biggestorigin, f);
		f = fabs(pose[i].channeloffset[1] + 0xFFFF*pose[i].channelscale[1]); biggestorigin = max(biggestorigin, f);
		f = fabs(pose[i].channeloffset[2] + 0xFFFF*pose[i].channelscale[2]); biggestorigin = max(biggestorigin, f);
	}
	mod->num_posescale = biggestorigin / 32767.0f;
	mod->num_poseinvscale = 1.0f / mod->num_posescale;

	// load the pose data - fix this
	framedata = (unsigned short *) (pbase + header->ofs_frames);
	mod->poses = (short*)Hunk_Alloc (header->num_frames * 7 * sizeof(short));
	for (i = 0, k = 0;i < (int)header->num_frames;i++)	
	{
		for (j = 0;j < (int)header->num_poses;j++, k++)
		{
			mod->poses[k*6 + 0] = mod->num_poseinvscale * (pose[j].channeloffset[0] + (pose[j].channelmask&1 ? (unsigned short)LittleShort(*framedata++) * pose[j].channelscale[0] : 0));
			mod->poses[k*6 + 1] = mod->num_poseinvscale * (pose[j].channeloffset[1] + (pose[j].channelmask&2 ? (unsigned short)LittleShort(*framedata++) * pose[j].channelscale[1] : 0));
			mod->poses[k*6 + 2] = mod->num_poseinvscale * (pose[j].channeloffset[2] + (pose[j].channelmask&4 ? (unsigned short)LittleShort(*framedata++) * pose[j].channelscale[2] : 0));
			mod->poses[k*6 + 3] = 32767.0f * (pose[j].channeloffset[3] + (pose[j].channelmask&8 ? (unsigned short)LittleShort(*framedata++) * pose[j].channelscale[3] : 0));
			mod->poses[k*6 + 4] = 32767.0f * (pose[j].channeloffset[4] + (pose[j].channelmask&16 ? (unsigned short)LittleShort(*framedata++) * pose[j].channelscale[4] : 0));
			mod->poses[k*6 + 5] = 32767.0f * (pose[j].channeloffset[5] + (pose[j].channelmask&32 ? (unsigned short)LittleShort(*framedata++) * pose[j].channelscale[5] : 0));
			// skip scale data for now
			if(pose[j].channelmask&64) framedata++;
			if(pose[j].channelmask&128) framedata++;
			if(pose[j].channelmask&256) framedata++;
		}
	}

	// load bounding box data(still need to set mod->bbox)
	if (header->ofs_bounds)
	{
		float xyradius = 0, radius = 0;
		bounds = (iqmbounds_t *) (pbase + header->ofs_bounds);
		VectorClear(mod->mins);
		VectorClear(mod->maxs);
		for (i = 0; i < (int)header->num_frames;i++)
		{
			bounds[i].mins[0] = LittleFloat(bounds[i].mins[0]);
			bounds[i].mins[1] = LittleFloat(bounds[i].mins[1]);
			bounds[i].mins[2] = LittleFloat(bounds[i].mins[2]);
			bounds[i].maxs[0] = LittleFloat(bounds[i].maxs[0]);			
			bounds[i].maxs[1] = LittleFloat(bounds[i].maxs[1]);	
			bounds[i].maxs[2] = LittleFloat(bounds[i].maxs[2]);	
			bounds[i].xyradius = LittleFloat(bounds[i].xyradius);
			bounds[i].radius = LittleFloat(bounds[i].radius);
			if (!i)
			{
				VectorCopy(bounds[i].mins, mod->mins);
				VectorCopy(bounds[i].maxs, mod->maxs);
			}
			else
			{
				if (mod->mins[0] > bounds[i].mins[0]) mod->mins[0] = bounds[i].mins[0];
				if (mod->mins[1] > bounds[i].mins[1]) mod->mins[1] = bounds[i].mins[1];
				if (mod->mins[2] > bounds[i].mins[2]) mod->mins[2] = bounds[i].mins[2];
				if (mod->maxs[0] < bounds[i].maxs[0]) mod->maxs[0] = bounds[i].maxs[0];
				if (mod->maxs[1] < bounds[i].maxs[1]) mod->maxs[1] = bounds[i].maxs[1];
				if (mod->maxs[2] < bounds[i].maxs[2]) mod->maxs[2] = bounds[i].maxs[2];
			}
			if (bounds[i].xyradius > xyradius)
				xyradius = bounds[i].xyradius;
			if (bounds[i].radius > radius)
				radius = bounds[i].radius;
		}
		
		mod->radius = radius;
	}

	// load triangle data
	inelements = (const int *) (pbase + header->ofs_triangles);

	mod->tris = (iqmtriangle_t*)Hunk_Alloc(header->num_triangles * sizeof(iqmtriangle_t));

	for (i = 0;i < (int)header->num_triangles;i++)
	{
		mod->tris[i].vertex[0] = LittleLong(inelements[0]);		
		mod->tris[i].vertex[1] = LittleLong(inelements[1]);
		mod->tris[i].vertex[2] = LittleLong(inelements[2]);
		inelements += 3;
	}
	
	//find triangle neighbors(note - this model format has these defined(save for later, work on basic stuff right now)

	// load vertex data
	R_LoadIQMVertexArrays(mod, vposition);
	
	//fix this next section

	// load texture coodinates
    mod->st = (fstvert_t*)Hunk_Alloc (header->num_vertexes * sizeof(fstvert_t));	

	for (i = 0;i < (int)header->num_vertexes;i++)
	{
		mod->st[i].s = vtexcoord[0];
		mod->st[i].t = vtexcoord[1];

		vtexcoord+=2;
	}
/*
	if(vnormal)
	{
		mod->normal = (mnormal_t*)malloc(header->num_vertexes * sizeof(mnormal_t));
		for (i = 0;i < (int)header->num_vertexes;i++)
		{
			mod->normal->dir[0] = LittleFloat(vnormal[0]);
			mod->normal->dir[1] = LittleFloat(vnormal[1]);
			mod->normal->dir[2] = LittleFloat(vnormal[2]);
			vnormal += 3;
			mod->normal ++;
		}
	}

	if(vtangent)
	{
		mod->tangent = (mtangent_t*)malloc(header->num_vertexes * sizeof(mtangent_t));
		for (i = 0;i < (int)header->num_vertexes;i++)
		{
			mod->tangent->dir[0] = LittleFloat(vtangent[0]);
			mod->tangent->dir[1] = LittleFloat(vtangent[1]);
			mod->tangent->dir[2] = LittleFloat(vtangent[2]);
			vtangent += 3;
			mod->tangent ++;
		}
	}
*/
	Com_Printf("Successfully loaded %s\n", mod->name);
	testflag = true;
	return true;
}


void GL_AnimateIqmFrame(int posenum)
{
	//do the animation of vertexes, I'm thinking this should be a blend between currentenity->frame and currententity->oldframe
}

extern void R_DrawNullModel (void);
extern vec3_t shadelight;
void GL_DrawIqmFrame()
{
	int		i, j;
	int		index_xyz, index_st;
	int		va;

	//render the model
	
	//just need a basic test render to get me started, once I have this, I can implement rscript and GLSL items among other things
	va=0;

	R_InitVArrays (VERT_SINGLE_TEXTURED);
	//I know this section is inherently wrong, the vertexes should be coming from another array created by the animation routine
	//Just wanted the overall structure of rendering outlined here

	GL_SelectTexture( GL_TEXTURE0);
	qglBindTexture (GL_TEXTURE_2D, r_cowtest->texnum);

	//Note - we will build this section in the animation of the vertexes eventually
	for (i=0; i<currentmodel->num_triangles; i++)
	{	
		for (j=0; j<3; j++)
		{			
			index_xyz = index_st = currentmodel->tris[i].vertex[j];
			
			VArray[0] = currentmodel->vertexes[index_xyz].position[0];
			VArray[1] = currentmodel->vertexes[index_xyz].position[1];
			VArray[2] = currentmodel->vertexes[index_xyz].position[2];

			VArray[3] = currentmodel->st[index_st].s;
			VArray[4] = currentmodel->st[index_st].t;
			
			// increment pointer and counter
			VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			va++;			
		}		
	}

	if(qglLockArraysEXT)						
		qglLockArraysEXT(0, va);

	qglDrawArrays(GL_TRIANGLES,0,va);
				
	if(qglUnlockArraysEXT)						
		qglUnlockArraysEXT();

	R_KillVArrays ();

	testflag = false;
}
		
/*
=================
R_DrawINTERQUAKEMODEL - initially this will only deal with player models, so much is stripped out.  Not worrying about shadows yet either
=================
*/

//much of this will need work, but for now for the basic render, this suffices
void R_DrawINTERQUAKEMODEL (entity_t *e)
{
	int			i;
	//vec3_t		bbox[8];
	image_t		*skin;

	if((r_newrefdef.rdflags & RDF_NOWORLDMODEL ) && !(e->flags & RF_MENUMODEL))
		return;
	
	//do culling after we know this renders ok!
	//if ( R_CullAliasModel( bbox, e ) )
	//	return;
	//bug - currentmodel is not retaining the verts at all
	if(0) {
		for (i=0; i<currentmodel->numvertexes; i++)
			Com_Printf("vertex: %4.2f %4.2f %4.2f\n", currentmodel->vertexes[i].position[0], currentmodel->vertexes[i].position[1], currentmodel->vertexes[i].position[2]);
	}
	//will have to deal with shells at some point, for now just leave this here
	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE) )
	{

		VectorClear (shadelight);
		if (currententity->flags & RF_SHELL_HALF_DAM)
		{
				shadelight[0] = 0.56;
				shadelight[1] = 0.59;
				shadelight[2] = 0.45;
		}
		if ( currententity->flags & RF_SHELL_DOUBLE )
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}
		if ( currententity->flags & RF_SHELL_RED )
			shadelight[0] = 1.0;
		if ( currententity->flags & RF_SHELL_GREEN )
		{
			shadelight[1] = 1.0;
			shadelight[2] = 0.6;
		}
		if ( currententity->flags & RF_SHELL_BLUE )
		{
			shadelight[2] = 1.0;
			shadelight[0] = 0.6;
		}

	}
	else if (currententity->flags & RF_FULLBRIGHT)
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint (currententity->origin, shadelight, true);
	}
	if ( currententity->flags & RF_MINLIGHT )
	{
		float minlight;

		if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value) 
			minlight = 0.1;
		else
			minlight = 0.2;
		for (i=0 ; i<3 ; i++)
			if (shadelight[i] > minlight)
				break;
		if (i == 3)
		{
			shadelight[0] = minlight;
			shadelight[1] = minlight;
			shadelight[2] = minlight;
		}
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;
		float	minlight;

		scale = 0.2 * sin(r_newrefdef.time*7);
		if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value) 
			minlight = 0.1;
		else
			minlight = 0.2;
		for (i=0 ; i<3 ; i++)
		{			
			shadelight[i] += scale;
			if (shadelight[i] < minlight)
				shadelight[i] = minlight;
		}
	}

    qglPushMatrix ();
	e->angles[PITCH] = -e->angles[PITCH];
	R_RotateForEntity (e);
	e->angles[PITCH] = -e->angles[PITCH];

	// select skin
	if (currententity->skin) {
		skin = currententity->skin;
	}
	if (!skin)
		skin = r_notexture;	// fallback...
	GL_Bind(skin->texnum);

	// draw it

	qglShadeModel (GL_SMOOTH);

	GL_TexEnv( GL_MODULATE );

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglEnable (GL_BLEND);
		qglBlendFunc (GL_ONE, GL_ONE);
	}

	//frame/oldframe stuff for blending?
	GL_AnimateIqmFrame(currententity->frame);
	GL_DrawIqmFrame();

	GL_TexEnv( GL_REPLACE );
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglDisable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	qglColor4f (1,1,1,1);
}	