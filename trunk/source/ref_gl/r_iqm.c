#include "r_local.h"
#include "r_iqm.h"
#include "r_matrixlib.h"

extern  void Q_strncpyz( char *dest, const char *src, size_t size );

//these matrix functions should be moved to matrixlib.c or similar

void Matrix3x4_TransformNormal(mnormal_t *out, matrix3x4_t mat, const mnormal_t in)
{
	out->dir[0] = DotProduct(mat.a, in.dir);
	out->dir[1] = DotProduct(mat.b, in.dir);
	out->dir[2] = DotProduct(mat.c, in.dir);
}

void Matrix3x4_TransformTangent(mtangent_t *out, matrix3x4_t mat, const mtangent_t in)
{
	out->dir[0] = DotProduct(mat.a, in.dir);
	out->dir[1] = DotProduct(mat.b, in.dir);
	out->dir[2] = DotProduct(mat.c, in.dir);
}

void Matrix3x4_Invert(matrix3x4_t *out, matrix3x4_t in)
{
	vec3_t a, b, c, trans;

	VectorSet(a, in.a[0], in.b[0], in.c[0]);
	VectorSet(b, in.a[1], in.b[1], in.c[1]);
	VectorSet(c, in.a[2], in.b[2], in.c[2]);

	VectorScale(a, 1/DotProduct(a, a), a);
	VectorScale(b, 1/DotProduct(b, b), b);
	VectorScale(c, 1/DotProduct(c, c), c);

	VectorSet(trans, in.a[3], in.b[3], in.c[3]);

	Vector4Set(out->a, a[0], a[1], a[2], -DotProduct(a, trans));
	Vector4Set(out->b, b[0], b[1], b[2], -DotProduct(b, trans));
	Vector4Set(out->c, c[0], c[1], c[2], -DotProduct(c, trans));
}

void Matrix3x4_FromQuatAndVectors(matrix3x4_t *out, vec4_t rot, const float trans[3], const float scale[3])
{
	vec3_t a, b, c;

    //Convert the quat
    {
        float x = rot[0], y = rot[1], z = rot[2], w = rot[3],
              tx = 2*x, ty = 2*y, tz = 2*z,
              txx = tx*x, tyy = ty*y, tzz = tz*z,
              txy = tx*y, txz = tx*z, tyz = ty*z,
              twx = w*tx, twy = w*ty, twz = w*tz;
        VectorSet(a, 1 - (tyy + tzz), txy - twz, txz + twy);
        VectorSet(b, txy + twz, 1 - (txx + tzz), tyz - twx);
        VectorSet(c, txz - twy, tyz + twx, 1 - (txx + tyy));
    }

	Vector4Set(out->a, a[0]*scale[0], a[1]*scale[1], a[2]*scale[2], trans[0]);
	Vector4Set(out->b, b[0]*scale[0], b[1]*scale[1], b[2]*scale[2], trans[1]);
	Vector4Set(out->c, c[0]*scale[0], c[1]*scale[1], c[2]*scale[2], trans[2]);
}

void Matrix3x4_Multiply(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2)
{
	vec4_t a, b, c, d;

    Vector4Scale(mat2.a, mat1.a[0], a);
    Vector4Scale(mat2.b, mat1.a[1], b);
    Vector4Scale(mat2.c, mat1.a[2], c);
    Vector4Add(a, b, d);
    Vector4Add(d, c, d);
    Vector4Set(out->a, d[0], d[1], d[2], d[3] + mat1.a[3]);

    Vector4Scale(mat2.a, mat1.b[0], a);
    Vector4Scale(mat2.b, mat1.b[1], b);
    Vector4Scale(mat2.c, mat1.b[2], c);
    Vector4Add(a, b, d);
    Vector4Add(d, c, d);
    Vector4Set(out->b, d[0], d[1], d[2], d[3] + mat1.b[3]);

    Vector4Scale(mat2.a, mat1.c[0], a);
    Vector4Scale(mat2.b, mat1.c[1], b);
    Vector4Scale(mat2.c, mat1.c[2], c);
    Vector4Add(a, b, d);
    Vector4Add(d, c, d);
    Vector4Set(out->c, d[0], d[1], d[2], d[3] + mat1.c[3]);
}

void Matrix3x4_Scale(matrix3x4_t *out, matrix3x4_t in, float scale)
{
	Vector4Scale(in.a, scale, out->a);
	Vector4Scale(in.b, scale, out->b);
	Vector4Scale(in.c, scale, out->c);
}

void Matrix3x4_Add(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2)
{
	Vector4Add(mat1.a, mat2.a, out->a);
	Vector4Add(mat1.b, mat2.b, out->b);
	Vector4Add(mat1.c, mat2.c, out->c);
}

void Matrix3x4_Copy(matrix3x4_t *out, matrix3x4_t in)
{
	Vector4Copy(in.a, out->a);
	Vector4Copy(in.b, out->b);
	Vector4Copy(in.c, out->c);
}

void Matrix3x4_Transform(mvertex_t *out, matrix3x4_t mat, const mvertex_t in)
{
	out->position[0] = DotProduct(mat.a, in.position) + mat.a[3];
    out->position[1] = DotProduct(mat.b, in.position) + mat.b[3];
    out->position[2] = DotProduct(mat.c, in.position) + mat.c[3];
}

extern 
void R_LoadIQMVertexArrays(model_t *iqmmodel, float *vposition, float *vnormal, float *vtangent)
{
	int i;

	if(iqmmodel->numvertexes > 16384)
		return;

	iqmmodel->vertexes = (mvertex_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mvertex_t));
	iqmmodel->normal = (mnormal_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mnormal_t));
	iqmmodel->tangent = (mtangent_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mtangent_t));

	//set this now for later use
	iqmmodel->animatevertexes = (mvertex_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mvertex_t));
	iqmmodel->animatenormal = (mnormal_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mnormal_t));
	iqmmodel->animatetangent = (mtangent_t*)Hunk_Alloc(iqmmodel->numvertexes * sizeof(mtangent_t));

	for(i=0; i<iqmmodel->numvertexes; i++){
		VectorSet(iqmmodel->vertexes[i].position,
					LittleFloat(vposition[0]),
					LittleFloat(vposition[1]),
					LittleFloat(vposition[2]));		

		VectorSet(iqmmodel->normal[i].dir,
					LittleFloat(vnormal[0]),
					LittleFloat(vnormal[1]),
					LittleFloat(vnormal[2]));	

		Vector4Set(iqmmodel->tangent[i].dir,
					LittleFloat(vtangent[0]),
					LittleFloat(vtangent[1]),
					LittleFloat(vtangent[2]),
					LittleFloat(vtangent[3]));

		vposition	+=3;
		vnormal		+=3;
		vtangent	+=4;
	}
}

qboolean Mod_ReadSkinFile(char skin_file[MAX_QPATH], char *skinpath)
{
	FILE *fp;
	int length;
	char *buffer;
	char *s;

	if((fp = fopen(skin_file, "rb" )) == NULL)
	{
		return false;
	}
	else
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		buffer = malloc( length + 1 );
		fread( buffer, length, 1, fp );
		buffer[length] = 0;
	}
	s = buffer;

	strcpy( skinpath, COM_Parse( &s ) );
	skinpath[length] = 0; //clear any possible garbage

	if ( fp != 0 )
	{
		fp = 0;
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}
	return true;
}

qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer)
{
	const char *text;
	iqmheader_t *header;
	int i, j;
	const int *inelements;
	float *vposition = NULL, *vtexcoord = NULL, *vnormal = NULL, *vtangent = NULL;
	unsigned char *pbase;
	iqmjoint_t *joint;
	matrix3x4_t	*baseframe;
	matrix3x4_t	*inversebaseframe;
	iqmpose_t *poses;
	iqmbounds_t *bounds;
	iqmvertexarray_t *va;
	unsigned short *framedata;
	char skinname[MAX_QPATH], shortname[MAX_QPATH], fullname[MAX_OSPATH], *path;

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
	
	mod->extradata = Hunk_Begin (0x300000); 

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
			if (va[i].format == IQM_UBYTE && va[i].size == 4) {
				mod->blendindexes = (unsigned char *)Hunk_Alloc(header->num_vertexes * 4 * sizeof(unsigned char));
				memcpy(mod->blendindexes, (unsigned char *)(pbase + va[i].offset), header->num_vertexes * 4 * sizeof(unsigned char));
			}
			break;
		case IQM_BLENDWEIGHTS:
			if (va[i].format == IQM_UBYTE && va[i].size == 4) {
				mod->blendweights = (unsigned char *)Hunk_Alloc(header->num_vertexes * 4 * sizeof(unsigned char));
				memcpy(mod->blendweights, (unsigned char *)(pbase + va[i].offset), header->num_vertexes * 4 * sizeof(unsigned char));
			}
			break;
		}
	}
	if (!vposition || !vtexcoord || !mod->blendindexes || !mod->blendweights)
	{
		Com_Printf("%s is missing vertex array data\n", mod->name);
		return false;
	}

	text = header->num_text && header->ofs_text ? (const char *)(pbase + header->ofs_text) : "";
	
	mod->num_frames = header->num_anims;
	mod->num_joints = header->num_joints;
	mod->num_poses = header->num_frames;
	mod->numvertexes = header->num_vertexes;
	mod->num_triangles = header->num_triangles;
	
	// load the joints
	joint = (iqmjoint_t *) (pbase + header->ofs_joints);
	mod->joints = (iqmjoint_t*)Hunk_Alloc (header->num_joints * sizeof(iqmjoint_t));
	for (i = 0;i < mod->num_joints;i++)
	{
		mod->joints[i].name = LittleLong(joint[i].name);
		mod->joints[i].parent = LittleLong(joint[i].parent);
		for (j = 0;j < 3;j++)
		{
			mod->joints[i].origin[j] = LittleFloat(joint[i].origin[j]);
			mod->joints[i].rotation[j] = LittleFloat(joint[i].rotation[j]);
			mod->joints[i].scale[j] = LittleFloat(joint[i].scale[j]);
		}
	}
	
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
		Vector4Set(q_rot, j.rotation[0], j.rotation[1], j.rotation[2], -sqrt(max(1.0 - pow(VectorLength(rot),2), 0.0)));

		Matrix3x4_FromQuatAndVectors(&baseframe[i], q_rot, j.origin, j.scale);
		Matrix3x4_Invert(&inversebaseframe[i], baseframe[i]);

        if(j.parent >= 0)
        {
            matrix3x4_t temp;
            Matrix3x4_Multiply(&temp, baseframe[j.parent], baseframe[i]);
            baseframe[i] = temp;
            Matrix3x4_Multiply(&temp, inversebaseframe[i], inversebaseframe[j.parent]);
            inversebaseframe[i] = temp;
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
			vec4_t q_rot;
			matrix3x4_t m, temp;

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

			Vector4Set(q_rot, rotate[0], rotate[1], rotate[2], -sqrt(max(1.0 - pow(VectorLength(rotate),2), 0.0)));

			Matrix3x4_FromQuatAndVectors(&m, q_rot, translate, scale); 

			if(p.parent >= 0) {
				Matrix3x4_Multiply(&temp, baseframe[p.parent], m);
				Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], temp, inversebaseframe[j]);
			}
            else 
				Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], m, inversebaseframe[j]);
        }
    }

	// load bounding box data
	//this is not working correctly, vals are a bit odd
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
			
			if (mod->mins[0] > bounds[i].mins[0]) mod->mins[0] = bounds[i].mins[0];
			if (mod->mins[1] > bounds[i].mins[1]) mod->mins[1] = bounds[i].mins[1];
			if (mod->mins[2] > bounds[i].mins[2]) mod->mins[2] = bounds[i].mins[2];
			if (mod->maxs[0] < bounds[i].maxs[0]) mod->maxs[0] = bounds[i].maxs[0];
			if (mod->maxs[1] < bounds[i].maxs[1]) mod->maxs[1] = bounds[i].maxs[1];
			if (mod->maxs[2] < bounds[i].maxs[2]) mod->maxs[2] = bounds[i].maxs[2];
			
			if (bounds[i].xyradius > xyradius)
				xyradius = bounds[i].xyradius;
			if (bounds[i].radius > radius)
				radius = bounds[i].radius;
		}
		
		mod->radius = radius;

		//Com_Printf("iqm %s bounds: mins %4.2f, %4.2f, %4.2f maxs %4.2f %4.2f %4.2f\n", mod->name, mod->mins[0], mod->mins[1], mod->mins[2],
		//	mod->maxs[0], mod->maxs[1], mod->maxs[2]);
	}

	//compute a full bounding box
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mod->mins[0];
		else
			tmp[0] = mod->maxs[0];

		if ( i & 2 )
			tmp[1] = mod->mins[1];
		else
			tmp[1] = mod->maxs[1];

		if ( i & 4 )
			tmp[2] = mod->mins[2];
		else
			tmp[2] = mod->maxs[2];

		VectorCopy( tmp, mod->bbox[i] );
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
	
	//load triangle neighbors
	if (header->ofs_neighbors)
	{
		inelements = (const int *) (pbase + header->ofs_neighbors);

		mod->neighbors = Hunk_Alloc(header->num_triangles * sizeof(neighbors_t));

		for (i = 0;i < (int)header->num_triangles;i++)
		{
			mod->neighbors[i].n[0] = LittleLong(inelements[0]);		
			mod->neighbors[i].n[1] = LittleLong(inelements[1]);
			mod->neighbors[i].n[2] = LittleLong(inelements[2]);
			inelements += 3;
		}
	}

	// load vertex data
	R_LoadIQMVertexArrays(mod, vposition, vnormal, vtangent);

	// load texture coodinates
    mod->st = (fstvert_t*)Hunk_Alloc (header->num_vertexes * sizeof(fstvert_t));	

	for (i = 0;i < (int)header->num_vertexes;i++)
	{
		mod->st[i].s = vtexcoord[0];
		mod->st[i].t = vtexcoord[1];

		vtexcoord+=2;
	}

	//load skin from skin file
	COM_StripExtension(mod->name, shortname);
	strcat(shortname, ".skin");
	path = NULL;
	for(;;)
	{
		path = FS_NextPath( path );
		if( !path )
		{
			break;
		}
		if(path)
			Com_sprintf(fullname, sizeof(fullname), "%s/%s", path, shortname);

		i = 0;
		do
			fullname[i] = tolower(fullname[i]);
		while (fullname[i++]);

		if (Mod_ReadSkinFile(fullname, skinname))
		{
			mod->skins[0] = GL_FindImage (skinname, it_skin);

			//load shader for skin
			COM_StripExtension ( skinname, shortname );
	#ifdef _WINDOWS
			(struct rscript_s *)mod->script[0] = RS_FindScript(shortname);
	#else
			mod->script[0] = RS_FindScript(shortname); //make it gcc 4.1.1 compatible
	#endif
				
			if (mod->script[0])
				RS_ReadyScript((rscript_t *)mod->script[0]);
		}
	}

	//free temp non hunk mem
	if(baseframe)
		free(baseframe);
	if(inversebaseframe)
		free(inversebaseframe);

	return true;
}

matrix3x4_t outframe[5096]; //to do - find out what max frames/joints might be

void GL_AnimateIQMFrame(float curframe, int nextframe)
{
	int i, j;
    int frame1 = (int)floor(curframe),
        frame2 = nextframe;
    float frameoffset = curframe - frame1;
	frame1 %= currentmodel->num_poses;
	frame2 %= currentmodel->num_poses;
 
	{
		matrix3x4_t *mat1 = &currentmodel->frames[frame1 * currentmodel->num_joints],
			*mat2 = &currentmodel->frames[frame2 * currentmodel->num_joints];
	
		// Interpolate matrixes between the two closest frames and concatenate with parent matrix if necessary.
		// Concatenate the result with the inverse of the base pose.
		// You would normally do animation blending and inter-frame blending here in a 3D engine.

		for(i = 0; i < currentmodel->num_joints; i++)
		{
			matrix3x4_t mat, temp; 
			Matrix3x4_Scale(&mat, mat1[i], 1-frameoffset);
			Matrix3x4_Scale(&temp, mat2[i], frameoffset);
			Matrix3x4_Add(&mat, mat, temp);

			if(currentmodel->joints[i].parent >= 0) 
				Matrix3x4_Multiply(&outframe[i], outframe[currentmodel->joints[i].parent], mat);
			else 
				Matrix3x4_Copy(&outframe[i], mat);
		}
	}
	// The actual vertex generation based on the matrixes follows...
	{
		const mvertex_t *srcpos = (const mvertex_t *)currentmodel->vertexes;
		const mnormal_t *srcnorm = (const mnormal_t *)currentmodel->normal;
		const mtangent_t *srctan = (const mtangent_t *)currentmodel->tangent; 
	
		mvertex_t *dstpos = (mvertex_t *)currentmodel->animatevertexes;
		mnormal_t *dstnorm = (mnormal_t *)currentmodel->animatenormal;
		mtangent_t *dsttan = (mtangent_t *)currentmodel->animatetangent;		

		const unsigned char *index = currentmodel->blendindexes, *weight = currentmodel->blendweights;

		for(i = 0; i < currentmodel->numvertexes; i++)
		{
			matrix3x4_t mat, temp;

			// Blend matrixes for this vertex according to its blend weights. 
			// the first index/weight is always present, and the weights are
			// guaranteed to add up to 255. So if only the first weight is
			// presented, you could optimize this case by skipping any weight
			// multiplies and intermediate storage of a blended matrix. 
			// There are only at most 4 weights per vertex, and they are in 
			// sorted order from highest weight to lowest weight. Weights with 
			// 0 values, which are always at the end, are unused.

			Matrix3x4_Scale(&mat, outframe[index[0]], weight[0]/255.0f);
	
			for(j = 1; j < 4 && weight[j]; j++) {
				Matrix3x4_Scale(&temp, outframe[index[j]], weight[j]/255.0f);
				Matrix3x4_Add(&mat, mat, temp);
			}

			// Transform attributes by the blended matrix.
			// Position uses the full 3x4 transformation matrix.
			// Normals and tangents only use the 3x3 rotation part 
			// of the transformation matrix.

			Matrix3x4_Transform(dstpos, mat, *srcpos); 
			
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

			Matrix3x4_TransformNormal(dstnorm, mat, *srcnorm);

			// Note that input tangent data has 4 coordinates, 
			// so only transform the first 3 as the tangent vector.

			Matrix3x4_TransformTangent(dsttan, mat, *srctan);

		    srcpos++;
			srcnorm++;
			srctan++;
			dstpos++;
			dstnorm++;
			dsttan++;

			index += 4;
			weight += 4;
		}
	}
}

extern rscript_t *rs_glass;

void GL_VlightIQM (vec3_t baselight, mvertex_t *verts, vec3_t lightOut)
{
    //need to write routine for this
}

void GL_DrawIQMFrame(int skinnum)
{
	int		i, j;
	vec3_t	move, delta, vectors[3];
	rscript_t *rs = NULL;
	rs_stage_t *stage = NULL;
	float	shellscale;
	float	alpha, basealpha;
	vec3_t	lightcolor;
	char    shortname[MAX_QPATH];
	int		index_xyz, index_st;
	int		va;
	qboolean mirror = false;
	qboolean depthmaskrscipt = false;

	if (r_shaders->value)
			rs=(rscript_t *)currententity->script;

	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	if (currententity->flags & RF_TRANSLUCENT) {
		alpha = currententity->alpha;

		rs=(rscript_t *)rs_glass;
		if(!rs)
			GL_Bind(r_reflecttexture->texnum);
		else if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) {
			if(gl_mirror->value)
				mirror = true;
		}
	}
	else
		alpha = basealpha = 1.0;

	if (r_shaders->value)
		rs=(rscript_t *)currententity->script;

	VectorSubtract (currententity->oldorigin, currententity->origin, delta);

	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct (delta, vectors[0]);	// forward
	move[1] = -DotProduct (delta, vectors[1]);	// left
	move[2] = DotProduct (delta, vectors[2]);	// up

	//render the model
	
	va=0;

	if(0) 
	{	
		//shell render
	}
	else if(!rs || mirror) 
	{	//base render no shaders
		if(mirror && !(currententity->flags & RF_WEAPONMODEL))
			R_InitVArrays(VERT_COLOURED_MULTI_TEXTURED);
		else
			R_InitVArrays (VERT_COLOURED_TEXTURED);

		if(mirror) 
		{
			if( !(currententity->flags & RF_WEAPONMODEL)) 
			{
				GL_EnableMultitexture( true );
				GL_SelectTexture( GL_TEXTURE0);
				GL_TexEnv ( GL_COMBINE_EXT );
				qglBindTexture (GL_TEXTURE_2D, r_mirrortexture->texnum);
				qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
				qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				GL_SelectTexture( GL_TEXTURE1);
				GL_TexEnv ( GL_COMBINE_EXT );
				qglBindTexture (GL_TEXTURE_2D, r_mirrorspec->texnum);
				qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
				qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
			}
			else 
			{
				GL_SelectTexture( GL_TEXTURE0);
				qglBindTexture (GL_TEXTURE_2D, r_mirrortexture->texnum);
			}
		}
		else 
		{ 
			GL_SelectTexture( GL_TEXTURE0);
			qglBindTexture (GL_TEXTURE_2D, skinnum);
		}

		for (i=0; i<currentmodel->num_triangles; i++)
		{	
			for (j=0; j<3; j++)
			{			
				index_xyz = index_st = currentmodel->tris[i].vertex[j];
				
				VArray[0] = move[0] + currentmodel->animatevertexes[index_xyz].position[0];
				VArray[1] = move[1] + currentmodel->animatevertexes[index_xyz].position[1];
				VArray[2] = move[2] + currentmodel->animatevertexes[index_xyz].position[2];

				if(mirror) 
				{
					VArray[5] = VArray[3] = -(currentmodel->st[index_st].s - DotProduct (currentmodel->animatenormal[index_xyz].dir, vectors[1]));
					VArray[6] = VArray[4] = currentmodel->st[index_st].t + DotProduct (currentmodel->animatenormal[index_xyz].dir, vectors[2]);
				}
				else 
				{
					VArray[3] = currentmodel->st[index_st].s;
					VArray[4] = currentmodel->st[index_st].t;
				}

				GL_VlightIQM (shadelight, &currentmodel->animatevertexes[index_xyz], lightcolor);
									
				if(mirror && !(currententity->flags & RF_WEAPONMODEL) ) {
					VArray[7] = lightcolor[0];
					VArray[8] = lightcolor[1];
					VArray[9] = lightcolor[2];
					VArray[10] = alpha;
				}
				else {
					VArray[5] = lightcolor[0];
					VArray[6] = lightcolor[1];
					VArray[7] = lightcolor[2];
					VArray[8] = alpha;
				}

				// increment pointer and counter
				if(mirror && !(currententity->flags & RF_WEAPONMODEL))
					VArray += VertexSizes[VERT_COLOURED_MULTI_TEXTURED];
				else
					VArray += VertexSizes[VERT_COLOURED_TEXTURED];
				va++;			
			}		
		}
		if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) 
		{
			if(qglLockArraysEXT)						
				qglLockArraysEXT(0, va);

			qglDrawArrays(GL_TRIANGLES,0,va);
				
			if(qglUnlockArraysEXT)						
				qglUnlockArraysEXT();
		}
			
		if(mirror && !(currententity->flags & RF_WEAPONMODEL))
			GL_EnableMultitexture( false );
	}
	else 
	{	//render with shaders
		
		if (rs->stage && rs->stage->has_alpha)
		{
			depthmaskrscipt = true;
		}

		if (depthmaskrscipt)
			qglDepthMask(false);

		stage=rs->stage;

		while (stage)
		{
			va=0;
			VArray = &VArrayVerts[0];
			GLSTATE_ENABLE_ALPHATEST

			if (stage->normalmap && (!gl_normalmaps->value || !gl_glsl_shaders->value || !gl_state.glsl_shaders)) {
				if(stage->next) {
					stage = stage->next;
					continue;
				}	
				else
					goto done;
			}

			if(!stage->normalmap) 
			{
				GL_Bind (stage->texture->texnum);
	
				if (stage->blendfunc.blend)
				{
					GL_BlendFunction(stage->blendfunc.source,stage->blendfunc.dest);
					GLSTATE_ENABLE_BLEND
				}
				else if (basealpha==1.0f)
				{
					GLSTATE_DISABLE_BLEND
				}
				else
				{
					GLSTATE_ENABLE_BLEND
					GL_BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					alpha = basealpha;
				}

				if (stage->alphashift.min || stage->alphashift.speed)
				{
					if (!stage->alphashift.speed && stage->alphashift.min > 0)
					{
						alpha=basealpha*stage->alphashift.min;
					}
					else if (stage->alphashift.speed)
					{
						alpha=basealpha*sin(rs_realtime * stage->alphashift.speed);
						if (alpha < 0) alpha=-alpha*basealpha;
						if (alpha > stage->alphashift.max) alpha=basealpha*stage->alphashift.max;
						if (alpha < stage->alphashift.min) alpha=basealpha*stage->alphashift.min;
					}
				}
				else
					alpha=basealpha;

				if (!stage->alphamask)
				{
					GLSTATE_DISABLE_ALPHATEST
				}
			}

			if(stage->normalmap) 
			{
				vec3_t lightVec, lightVal;

				GL_GetLightVals();

				//send light level and color to shader, ramp up a bit
				VectorCopy(lightcolor, lightVal);
				for(i = 0; i < 3; i++) 
				{
					if(lightVal[i] < shadelight[i]/2)
						lightVal[i] = shadelight[i]/2; //never go completely black
					lightVal[i] *= 5;
					lightVal[i] += dynFactor;
					if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) {
						if(lightVal[i] > 1.5)
							lightVal[i] = 1.5;
					}
					else {
						if(lightVal[i] > 1.0+dynFactor)
							lightVal[i] = 1.0+dynFactor;
					}
				}

				if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) 
				{ 
					//fixed light source pointing down, slightly forward and to the left
					lightPosition[0] = -1.0; 
					lightPosition[1] = 4.0; 
					lightPosition[2] = 8.0; 
					R_ModelViewTransform(lightPosition, lightVec);
				}
				else 
				{ 
					//simple directional(relative light position)
					VectorSubtract(lightPosition, currententity->origin, lightVec);
					if(dynFactor == 0.0) //do for world lights only
					{
						VectorMA(lightPosition, 1.0, lightVec, lightPosition);
						R_ModelViewTransform(lightPosition, lightVec);
					}

					//brighten things slightly 
					for (i = 0; i < 3; i++ )
						lightVal[i] *= 1.25;
				}
											
				GL_EnableMultitexture( true );
			
				glUseProgramObjectARB( g_meshprogramObj );
					
				glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);
					
				GL_SelectTexture( GL_TEXTURE1);
				qglBindTexture (GL_TEXTURE_2D, skinnum);
				glUniform1iARB( g_location_baseTex, 1); 

				GL_SelectTexture( GL_TEXTURE0);
				qglBindTexture (GL_TEXTURE_2D, stage->texture->texnum);
				glUniform1iARB( g_location_normTex, 0); 

				GL_SelectTexture( GL_TEXTURE2);
				qglBindTexture (GL_TEXTURE_2D, stage->texture2->texnum);
				glUniform1iARB( g_location_fxTex, 2);
					
				GL_SelectTexture( GL_TEXTURE0);

				if(stage->fx)
					glUniform1iARB( g_location_useFX, 1);
				else
					glUniform1iARB( g_location_useFX, 0);

				if(stage->glow) 
					glUniform1iARB( g_location_useGlow, 1);
				else
					glUniform1iARB( g_location_useGlow, 0);

				glUniform3fARB( g_location_color, lightVal[0], lightVal[1], lightVal[2]);

				//if using shadowmaps, offset self shadowed areas a bit so not to get too dark
				if(gl_shadowmaps->value && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
					glUniform1fARB( g_location_minLight, 0.20);
				else
					glUniform1fARB( g_location_minLight, 0.15);

				glUniform1fARB( g_location_meshTime, rs_realtime);

				glUniform1iARB( g_location_meshFog, map_fog);
			}				

			for (i=0; i<currentmodel->num_triangles; i++)
			{	
				for (j=0; j<3; j++)
				{			
					index_xyz = index_st = currentmodel->tris[i].vertex[j];
						
					VArray[0] = move[0] + currentmodel->animatevertexes[index_xyz].position[0];
					VArray[1] = move[1] + currentmodel->animatevertexes[index_xyz].position[1];
					VArray[2] = move[2] + currentmodel->animatevertexes[index_xyz].position[2];

					VArray[3] = currentmodel->st[index_st].s;
					VArray[4] = currentmodel->st[index_st].t;
					
					if(stage->normalmap) { //send tangent to shader
						VectorCopy(currentmodel->animatenormal[index_xyz].dir, NormalsArray[va]); //shader needs normal array
						Vector4Copy(currentmodel->animatetangent[index_xyz].dir, TangentsArray[va]);
					}

					if(!stage->normalmap)
					{
						float red = 1, green = 1, blue = 1;

						if (stage->lightmap) { 
							GL_VlightIQM (shadelight, &currentmodel->animatevertexes[index_xyz], lightcolor);
							red = lightcolor[0];
							green = lightcolor[1];
							blue = lightcolor[2];						
						}
						//to do - recalc alpha vals for script
						if(mirror && !(currententity->flags & RF_WEAPONMODEL) ) {
							VArray[7] = red;
							VArray[8] = green;
							VArray[9] = blue;
							VArray[10] = alpha;	
						}
						else {
							VArray[5] = red;
							VArray[6] = green;
							VArray[7] = blue;
							VArray[8] = alpha;	
						}
					}

					// increment pointer and counter
					if(stage->normalmap) 
						VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED];
					else
						VArray += VertexSizes[VERT_COLOURED_TEXTURED];	
					va++;
				}		
			}

			if(stage->normalmap) 
			{
				R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
				qglNormalPointer(GL_FLOAT, 0, NormalsArray);
				glEnableVertexAttribArrayARB (1);
				glVertexAttribPointerARB(1, 4, GL_FLOAT,GL_FALSE, 0, TangentsArray);
				glUniform1iARB( g_location_isMD2, 0);
			}
			else 
				R_InitVArrays (VERT_COLOURED_TEXTURED);
				
			if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) 
			{
				if(qglLockArraysEXT)						
					qglLockArraysEXT(0, va);

				qglDrawArrays(GL_TRIANGLES,0,va);
					
				if(qglUnlockArraysEXT)						
					qglUnlockArraysEXT();
			}
				
			qglColor4f(1,1,1,1);

			if(stage->normalmap) 
			{
				glUseProgramObjectARB( 0 );
				GL_EnableMultitexture( false );
			}
			
			stage=stage->next;
		}
	}

done:
	if (depthmaskrscipt)
		qglDepthMask(true);

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN

	qglDisableClientState( GL_NORMAL_ARRAY);
	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_KillVArrays ();

	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
		qglEnable( GL_TEXTURE_2D );
}

extern qboolean have_stencil;
extern	vec3_t			lightspot;

/*
=============
R_DrawIqmShadow
=============
*/
void R_DrawIQMShadow()
{
	vec3_t	point;
	float	height, lheight;
	int		i, j;
	int		index_xyz, index_st;
	int		va = 0;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;

	height = -lheight + 0.1f;

	// if above entity's origin, skip
	if ((currententity->origin[2]+height) > currententity->origin[2])
		return;

	if (r_newrefdef.vieworg[2] < (currententity->origin[2] + height))
		return;

	if (have_stencil) {

		qglDepthMask(0);

		qglEnable(GL_STENCIL_TEST);

		qglStencilFunc(GL_EQUAL,1,2);

		qglStencilOp(GL_KEEP,GL_KEEP,GL_INCR);

	}

	va=0;
	VArray = &VArrayVerts[0];
	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for (i=0; i<currentmodel->num_triangles; i++)
    {
        for (j=0; j<3; j++)
        {
            index_xyz = index_st = currentmodel->tris[i].vertex[j];
	
			memcpy( point, currentmodel->animatevertexes[index_xyz].position, sizeof( point )  );

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
				
			VArray[0] = point[0];
			VArray[1] = point[1];
			VArray[2] = point[2];

			VArray[3] = currentmodel->st[index_st].s;
            VArray[4] = currentmodel->st[index_st].t;

			// increment pointer and counter
			VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			va++;
		}
	}

	qglDrawArrays(GL_TRIANGLES,0,va);
	
	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_KillVArrays ();
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (have_stencil) 
		qglDisable(GL_STENCIL_TEST);
}

static qboolean R_CullIQMModel( void )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	trace_t r_trace;
	vec3_t	dist;
	vec3_t bbox[8];

	if (r_worldmodel ) {
		//occulusion culling - why draw entities we cannot see?
	
		r_trace = CM_BoxTrace(r_origin, currententity->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return true;
	}

	VectorSubtract(r_origin, currententity->origin, dist);

	/*
	** rotate the bounding box
	*/
	VectorCopy( currententity->angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( currentmodel->bbox[i], tmp );

		bbox[i][0] = DotProduct( vectors[0], tmp );
		bbox[i][1] = -DotProduct( vectors[1], tmp );
		bbox[i][2] = DotProduct( vectors[2], tmp );

		VectorAdd( currententity->origin, bbox[i], bbox[i] );
	}

	{
		int p, f, aggregatemask = ~0;

		for ( p = 0; p < 8; p++ )
		{
			int mask = 0;

			for ( f = 0; f < 4; f++ )
			{
				float dp = DotProduct( frustum[f].normal, bbox[p] );

				if ( ( dp - frustum[f].dist ) < 0 )
				{
					mask |= ( 1 << f );
				}
			}
			aggregatemask &= mask;
		}

		if ( aggregatemask && (VectorLength(dist) > 150)) //so shadows don't blatantly disappear when out of frustom
		{
			return true;
		}

		return false;
	}
}

qboolean inAnimGroup(int frame, int oldframe)
{
	//check if we are in a player anim group that is commonly looping
	if(frame >= 0 && frame <= 39 && oldframe >=0 && oldframe <= 39)
		return true; //standing, or 40 frame static mesh
	else if(frame >= 40 && frame <=45 && oldframe >= 40 && oldframe <=45)
		return true; //running
	else if(frame >= 66 && frame <= 71 && oldframe >= 66 && oldframe <= 71)
		return true; //jumping
	else if(frame >= 0 && frame <= 23 && oldframe >= 0 && oldframe <= 23)
		return true; //static meshes are 24 frames
	else 
		return false;
}

int NextFrame(int frame)
{
	int outframe;

	switch(frame)
	{
		case 23:
		case 39:
			outframe = 0;
			break;
		case 45:
			outframe = 40;
			break;
		case 71:
			outframe = 66;
			break;
		case 154:
			outframe = 136;
			break;
		case 160:
			outframe = 155;
			break;
		//deaths
		case 220:
			outframe = 220;
			break;
		case 238:
			outframe = 238;
			break;
		case 258:
			outframe = 258;
			break;
		default:
			outframe = frame + 1;
			break;
	}
	return outframe;
}
/*
=================
R_DrawINTERQUAKEMODEL
=================
*/

void R_DrawINTERQUAKEMODEL (entity_t *e)
{
	int			i;
	image_t		*skin;
	float		frame, time;
	int			nextframe;

	if(currententity->flags & RF_VIEWERMODEL) 
		return;

	if((r_newrefdef.rdflags & RDF_NOWORLDMODEL ) && !(e->flags & RF_MENUMODEL))
		return;
	
	//do culling
	if ( R_CullIQMModel() )
		return;
	
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
	else
	{
		skin = currentmodel->skins[0];
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
	
	//frame interpolation
	time = (Sys_Milliseconds() - currententity->frametime) / 100;
	if(time > 1.0)
		time = 1.0;

	
	if((currententity->frame == currententity->oldframe ) && !inAnimGroup(currententity->frame, currententity->oldframe)) 
		time = 0;

	//Check for stopped death anims
	if(currententity->frame == 257 || currententity->frame == 237 || currententity->frame == 219)
		time = 0;
	
	frame = currententity->frame + time;

	GL_AnimateIQMFrame(frame, NextFrame(currententity->frame));

	GL_DrawIQMFrame(skin->texnum);

	GL_TexEnv( GL_REPLACE );
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglDisable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//Basic stencil shadows
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL) && gl_shadows->value && !gl_shadowmaps->value && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
	{
		float casted;
		float an = currententity->angles[1]/180*M_PI;
		shadevector[0] = cos(-an);
		shadevector[1] = sin(-an);
		shadevector[2] = 1;
		VectorNormalize (shadevector);

		switch ((int)(gl_shadows->value))
		{
		case 0:
			break;
		case 1: //dynamic only - always cast something
			casted = R_ShadowLight (currententity->origin, shadevector, 0);
			qglPushMatrix ();
			qglTranslatef	(e->origin[0], e->origin[1], e->origin[2]);
			qglRotatef (e->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,0.3 * currententity->alpha); //Knightmare- variable alpha
			else
				qglColor4f (0,0,0,0.3);
			
			R_DrawIQMShadow ();
			
			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();

			break;
		case 2: //dynamic and world
			//world
			casted = R_ShadowLight (currententity->origin, shadevector, 1);
			qglPushMatrix ();
			qglTranslatef	(e->origin[0], e->origin[1], e->origin[2]);
			qglRotatef (e->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,casted * currententity->alpha);
			else
				qglColor4f (0,0,0,casted);

			R_DrawIQMShadow ();

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();
			//dynamic
			casted = 0;
		 	casted = R_ShadowLight (currententity->origin, shadevector, 0);
			if (casted > 0) 
			{ //only draw if there's a dynamic light there
				qglPushMatrix ();
				qglTranslatef	(e->origin[0], e->origin[1], e->origin[2]);
				qglRotatef (e->angles[1], 0, 0, 1);
				qglDisable (GL_TEXTURE_2D);
				qglEnable (GL_BLEND);

				if (currententity->flags & RF_TRANSLUCENT)
					qglColor4f (0,0,0,casted * currententity->alpha);
				else
					qglColor4f (0,0,0,casted);

				R_DrawIQMShadow ();

				qglEnable (GL_TEXTURE_2D);
				qglDisable (GL_BLEND);
				qglPopMatrix ();
			}

			break;
		}
	}
	qglColor4f (1,1,1,1);

	if(r_minimap->value)
    {
	   if ( currententity->flags & RF_MONSTER)
	   {
			RadarEnts[numRadarEnts].color[0]= 1.0;
			RadarEnts[numRadarEnts].color[1]= 0.0;
			RadarEnts[numRadarEnts].color[2]= 2.0;
			RadarEnts[numRadarEnts].color[3]= 1.0;
		}
	    else
			return;

		VectorCopy(currententity->origin,RadarEnts[numRadarEnts].org);
		numRadarEnts++;
	}
}	

void GL_DrawIQMCasterFrame ()
{
	int     i, j;
    int     index_xyz, index_st;
    int     va;

    va=0;

    R_InitVArrays (VERT_NO_TEXTURE);

    for (i=0; i<currentmodel->num_triangles; i++)
    {
        for (j=0; j<3; j++)
        {
            index_xyz = index_st = currentmodel->tris[i].vertex[j];

            VArray[0] = currentmodel->animatevertexes[index_xyz].position[0];
            VArray[1] = currentmodel->animatevertexes[index_xyz].position[1];
            VArray[2] = currentmodel->animatevertexes[index_xyz].position[2];

            VArray[3] = currentmodel->st[index_st].s;
            VArray[4] = currentmodel->st[index_st].t;

            // increment pointer and counter
            VArray += VertexSizes[VERT_NO_TEXTURE];
            va++;
        }
    }

    if(qglLockArraysEXT)
        qglLockArraysEXT(0, va);

    qglDrawArrays(GL_TRIANGLES,0,va);

    if(qglUnlockArraysEXT)
        qglUnlockArraysEXT();

    R_KillVArrays ();
}

void R_DrawIQMCaster (entity_t *e)
{
	dmdl_t		*paliashdr;

	if(e->team) //don't draw flag models, handled by sprites
		return;
	
	if ( e->flags & RF_WEAPONMODEL ) //don't draw weapon model shadow casters
		return;

	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE) ) //no shells
		return;

	if ( R_CullIQMModel() )
		return;

    qglPushMatrix ();
	e->angles[PITCH] = -e->angles[PITCH];
	R_RotateForEntity (e);
	e->angles[PITCH] = -e->angles[PITCH];

	GL_AnimateIQMFrame(currententity->frame, currententity->frame + 1);
	GL_DrawIQMCasterFrame();

	qglPopMatrix();
}