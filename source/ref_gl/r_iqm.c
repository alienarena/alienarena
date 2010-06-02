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
	matrix3x4_t	*baseframe;
	matrix3x4_t	*inversebaseframe;
	iqmpose_t *poses;
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

	// load the bone info(I think this can stay the same but check)
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

	//get this area working before moving on at all.  There is alot of math issues to resolve.
	//once this is finished, the rest will be a alot easier
	
	//these don't need to be a part of mod - remember to free them
	baseframe = (matrix3x4_t*)malloc (header->num_joints * sizeof(matrix3x4_t));
	inversebaseframe = (matrix3x4_t*)malloc (header->num_joints * sizeof(matrix3x4_t));
    for(i = 0; i < (int)header->num_joints; i++)
    {
		vec3_t rot;
		vec4_t q_rot;
        iqmjoint_t j = joint[i]; 

		//first need to make a vec4 quat from our rotation vec
		VectorSet(rot, j.rotation[0], j.rotation[1], j.rotation[2]);
		Vector4Set(q_rot, j.rotation[0], j.rotation[1], j.rotation[2], -sqrt(max(1.0 - VectorLength(rot) * VectorLength(rot), 0.0)));
		//check these vals against w^2 = 1 - (x^2 + y^2 + z^2)

		//get this from the demo
		//Matrix3x4_FromVectors(mod->baseframe[i], q_rot, j.origin, j.scale);

        //inversebaseframe[i].invert(baseframe[i]);
        if(j.parent >= 0) 
        {
			//Going to need to create matrix multiplier funcs for 3x4 matrixes.
            //baseframe[i].m = baseframe[j.parent].m * baseframe[i].m;
            //inversebaseframe[i].m *= inversebaseframe[j.parent].m;
        }
    }

	poses = (iqmpose_t *) (pbase + header->ofs_poses);
	mod->frames = (matrix3x4_t*)Hunk_Alloc (header->num_frames * header->num_poses * sizeof(matrix3x4_t));
    framedata = (unsigned short *) (pbase + header->ofs_frames);

    for(i = 0; i < header->num_frames; i++)
    {
        for(j = 0; j < header->num_poses; j++)
        {
            iqmpose_t p = poses[j];
            vec3_t translate, rotate, scale;
            translate[0] = p.channeloffset[0]; if(p.channelmask&0x01) translate[0] += *framedata++ * p.channelscale[0];
            translate[1] = p.channeloffset[1]; if(p.channelmask&0x02) translate[1] += *framedata++ * p.channelscale[1];
            translate[2] = p.channeloffset[2]; if(p.channelmask&0x04) translate[2] += *framedata++ * p.channelscale[2];
            rotate[0] = p.channeloffset[3]; if(p.channelmask&0x08) rotate[0] += *framedata++ * p.channelscale[3];
            rotate[1] = p.channeloffset[4]; if(p.channelmask&0x10) rotate[1] += *framedata++ * p.channelscale[4];
            rotate[2] = p.channeloffset[5]; if(p.channelmask&0x20) rotate[2] += *framedata++ * p.channelscale[5];
            scale[0] = p.channeloffset[6]; if(p.channelmask&0x40) scale[0] += *framedata++ * p.channelscale[6];
            scale[1] = p.channeloffset[7]; if(p.channelmask&0x80) scale[1] += *framedata++ * p.channelscale[7];
            scale[2] = p.channeloffset[8]; if(p.channelmask&0x100) scale[2] += *framedata++ * p.channelscale[8];
            // Concatenate each pose with the inverse base pose to avoid doing this at animation time.
            // If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
            // Thus it all negates at animation time like so: 
            //   (parentPose * parentInverseBasePose) * (parentBasePose * childPose * childInverseBasePose) =>
            //   parentPose * (parentInverseBasePose * parentBasePose) * childPose * childInverseBasePose =>
            //   parentPose * childPose * childInverseBasePose
       /*     Matrix3x4 m(Quat(rotate), translate, scale);
            if(p.parent >= 0) 
				mod->frames[i*header->num_poses + j] = baseframe[p.parent] * m * inversebaseframe[j];
            else 
				mod->frames[i*header->num_poses + j] = m * inversebaseframe[j];*/
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

	//free temp non hunk mem
	if(baseframe)
		free(baseframe);
	if(inversebaseframe)
		free(inversebaseframe);

	return true;
}


void GL_AnimateIqmFrame(float curframe)
{
    int frame1 = (int)floor(curframe),
        frame2 = frame1 + 1;
    float frameoffset = curframe - frame1;
	frame1 %= currentmodel->num_frames;
	frame2 %= currentmodel->num_frames;
  /*
    matrix3x4_t *mat1 = &mod->frames[frame1 * currentmodel->num_bones],
              *mat2 = &mod->frames[frame2 * currentmodel->num_bones];

    // Interpolate matrixes between the two closest frames and concatenate with parent matrix if necessary.
    // Concatenate the result with the inverse of the base pose.
    // You would normally do animation blending and inter-frame blending here in a 3D engine.
    for(int i = 0; i < currentmodel->num_bones; i++)
    {
        matrix3x4_t mat = mat1[i]*(1 - frameoffset) + mat2[i]*frameoffset;
        if(currentmodel->bones[i].parent >= 0) 
			outframe[i] = outframe[currentmodel->bones[i].parent] * mat;
        else 
			outframe[i] = mat;
    }
  
	// The actual vertex generation based on the matrixes follows...
	//John's note - the "in" vars, need to define and figure these out
	//I THINK inposition is the original vertex position, innorm, original normal, etc
	const mvertex_t *srcpos = (const mvertex_t *)currentmodel->vertexes;
	const mnormal_t *srcnorm = (const mnormal_t *)currentmodel->normal;
	const mtangent_t *srctan = (const mtangent_t *)currentmodel->tangent; 

    mvertex_t *dstpos = (mvertex_t *)outposition;
	*dstnorm = (mnormal_t *)outnormal;
	*dsttan = (mtangent_t *)outtangent;
	//*dstbitan = (vec3_t *)outbitangent; //we don't need these

	//not sure about these
    const uchar *index = inblendindex, *weight = inblendweight;

	//this next section need major translation to our codebase
	for(int i = 0; i < currentmodel->numvertexes; i++)
    {
        // Blend matrixes for this vertex according to its blend weights. 
        // the first index/weight is always present, and the weights are
        // guaranteed to add up to 255. So if only the first weight is
        // presented, you could optimize this case by skipping any weight
        // multiplies and intermediate storage of a blended matrix. 
        // There are only at most 4 weights per vertex, and they are in 
        // sorted order from highest weight to lowest weight. Weights with 
        // 0 values, which are always at the end, are unused.
        Matrix3x4 mat = outframe[index[0]] * (weight[0]/255.0f);
        for(int j = 1; j < 4 && weight[j]; j++)
            mat += outframe[index[j]] * (weight[j]/255.0f);

        // Transform attributes by the blended matrix.
        // Position uses the full 3x4 transformation matrix.
        // Normals and tangents only use the 3x3 rotation part 
        // of the transformation matrix.
        *dstpos = mat.transform(*srcpos);
        // Note that if the matrix includes non-uniform scaling, normal vectors
        // must be transformed by the inverse-transpose of the matrix to have the
        // correct relative scale. Note that invert(mat) = adjoint(mat)/determinant(mat),
        // and since the absolute scale is not important for a vector that will later
        // be renormalized, the adjoint-transpose matrix will work fine, which can be
        // cheaply generated by 3 cross-products.
        //
        // If you don't need to use joint scaling in your models, you can simply use the
        // upper 3x3 part of the position matrix instead of the adjoint-transpose shown 
        // here.
        Matrix3x3 matnorm(mat.b.cross3(mat.c), mat.c.cross3(mat.a), mat.a.cross3(mat.b));
        *dstnorm = matnorm.transform(*srcnorm);
        // Note that input tangent data has 4 coordinates, 
        // so only transform the first 3 as the tangent vector.
        *dsttan = matnorm.transform(Vec3(*srctan));
        // Note that bitangent = cross(normal, tangent) * sign, 
        // where the sign is stored in the 4th coordinate of the input tangent data.
        *dstbitan = dstnorm->cross(*dsttan) * srctan->w;

		//all of this will be using currentmodel data vars
        srcpos++;
        srcnorm++;
        srctan++;
        dstpos++;
        dstnorm++;
        dsttan++;
        dstbitan++;

        index += 4;
        weight += 4;
    }*/
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