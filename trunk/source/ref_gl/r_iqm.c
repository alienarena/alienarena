/*
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_iqm.h"
#include "r_ragdoll.h"
#include "r_lodcalc.h"

#define RAGDOLLVBO 1

#if !defined max
#define max(a,b)  (((a)<(b)) ? (b) : (a))
#endif

static vec3_t NormalsArray[MAX_VERTICES];
static vec4_t TangentsArray[MAX_VERTICES];

static vertCache_t	*vbo_st;
static vertCache_t	*vbo_xyz;
static vertCache_t	*vbo_normals;
static vertCache_t *vbo_tangents;
static vertCache_t *vbo_indices;

extern  void Q_strncpyz( char *dest, const char *src, size_t size );
extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// This function will compute the quaternion's W based on its X, Y, and Z.)
void Vec4_CompleteQuatW (vec4_t q)
{
	vec3_t q_xyz;
	
	VectorCopy (q, q_xyz);
	q[3] = -sqrt (max (1.0 - pow (VectorLength (q_xyz), 2), 0.0));
}

//these matrix functions should be moved to matrixlib.c or similar

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

void Matrix3x4_Subtract(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2)
{
	Vector4Sub(mat1.a, mat2.a, out->a);
	Vector4Sub(mat1.b, mat2.b, out->b);
	Vector4Sub(mat1.c, mat2.c, out->c);
}

void Matrix3x4_Copy(matrix3x4_t *out, matrix3x4_t in)
{
	Vector4Copy(in.a, out->a);
	Vector4Copy(in.b, out->b);
	Vector4Copy(in.c, out->c);
}

void Matrix3x4GenRotate(matrix3x4_t *out, float angle, const vec3_t axis)
{
	float ck = cos(angle), sk = sin(angle);

	Vector4Set(out->a, axis[0]*axis[0]*(1-ck)+ck, axis[0]*axis[1]*(1-ck)-axis[2]*sk, axis[0]*axis[2]*(1-ck)+axis[1]*sk, 0);
	Vector4Set(out->b, axis[1]*axis[0]*(1-ck)+axis[2]*sk, axis[1]*axis[1]*(1-ck)+ck, axis[1]*axis[2]*(1-ck)-axis[0]*sk, 0);
	Vector4Set(out->c, axis[0]*axis[2]*(1-ck)-axis[1]*sk, axis[1]*axis[2]*(1-ck)+axis[0]*sk, axis[2]*axis[2]*(1-ck)+ck, 0);
}

void Matrix3x4ForEntity(matrix3x4_t *out, entity_t *ent, float z)
{
    matrix3x4_t rotmat;
    vec3_t rotaxis;
    Vector4Set(out->a, 1, 0, 0, 0);
    Vector4Set(out->b, 0, 1, 0, 0);
    Vector4Set(out->c, 0, 0, 1, 0);
    if(ent->angles[2])
    {
        VectorSet(rotaxis, -1, 0, 0);
        Matrix3x4GenRotate(&rotmat, ent->angles[2]*pi/180, rotaxis);
        Matrix3x4_Multiply(out, rotmat, *out);
    }
    if(ent->angles[0])
    {
        VectorSet(rotaxis, 0, -1, 0);
        Matrix3x4GenRotate(&rotmat, ent->angles[0]*pi/180, rotaxis);
        Matrix3x4_Multiply(out, rotmat, *out);
    }
    if(ent->angles[1])
    {
        VectorSet(rotaxis, 0, 0, 1);
        Matrix3x4GenRotate(&rotmat, ent->angles[1]*pi/180, rotaxis);
        Matrix3x4_Multiply(out, rotmat, *out);
    }
    out->a[3] += ent->origin[0];
    out->b[3] += ent->origin[1];
    out->c[3] += ent->origin[2];
}

void Matrix3x4GenFromODE(matrix3x4_t *out, const dReal *rot, const dReal *trans)
{
	Vector4Set(out->a, rot[0], rot[1], rot[2], trans[0]);
	Vector4Set(out->b, rot[4], rot[5], rot[6], trans[1]);
	Vector4Set(out->c, rot[8], rot[9], rot[10], trans[2]);
}

double degreeToRadian(double degree)
{
	double radian = 0;
	radian = degree * (pi/180);
	return radian;
}

void IQM_ByteswapVertexArrays(model_t *iqmmodel, float *vposition, float *vnormal, float *vtangent, float *vtexcoord, int *vtriangles)
{
	int i;

	if(iqmmodel->numvertexes > 16384)
		return;
	
	for(i = 0; i < iqmmodel->numvertexes*2; i++)
	{
		*vtexcoord = LittleFloat (*vtexcoord);
		vtexcoord++;
	}

	for(i = 0; i < iqmmodel->numvertexes*3; i++)
	{
		*vposition = LittleFloat (*vposition);
		*vnormal = LittleFloat (*vnormal);
		*vtriangles = LittleLong (*vtriangles);
		vposition++;
		vnormal++;
		vtriangles++;
	}
	
	for (i = 0; i < iqmmodel->numvertexes*4; i++)
	{
		*vtangent = LittleFloat (*vtangent);
		vtangent++;
	}
}

qboolean IQM_ReadSkinFile(char skin_file[MAX_OSPATH], char *skinpath)
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
		size_t sz;
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		buffer = malloc( length + 1 );
		sz = fread( buffer, length, 1, fp );
		buffer[length] = 0;
	}
	s = buffer;

	strcpy( skinpath, COM_Parse( &s ) );
	skinpath[length] = 0; //clear any possible garbage

	if ( fp != 0 )
	{
		fclose(fp);
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}
	return true;
}

qboolean IQM_ReadRagDollFile(char ragdoll_file[MAX_OSPATH], model_t *mod)
{
	FILE *fp;
	int length;
	char *buffer;
	char *s;
	int result;
	char a_string[128];
	int i;

	if((fp = fopen(ragdoll_file, "rb" )) == NULL)
	{
		return false;
	}
	else
	{
		length = FS_filelength( fp );

		buffer = malloc( length + 1 );
		if ( buffer != NULL )
		{
			buffer[length] = 0;
			result = fread( buffer, length, 1, fp );
			if ( result == 1 )
			{
				s = buffer;

				for(i = 0; i < RAGDOLL_DIMS; i++)
				{
					strcpy( a_string, COM_Parse( &s ) );
					mod->ragdoll.RagDollDims[i] = atof(a_string);
				}
				strcpy( a_string, COM_Parse( &s ) );
				mod->ragdoll.hasHelmet = atoi(a_string);
			}
			else
			{
				Com_Printf("IQM_ReadRagDollFile: read fail\n");
			}
			free( buffer );
		}
		fclose( fp );
	}

	return true;
}

void IQM_LoadVBO (model_t *mod, float *vposition, float *vnormal, float *vtangent, float *vtexcoord, int *vtriangles)
{
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vposition, VBO_STORE_XYZ, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec2_t), vtexcoord, VBO_STORE_ST, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vnormal, VBO_STORE_NORMAL, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec4_t), vtangent, VBO_STORE_TANGENT, mod);
	R_VCLoadData(VBO_STATIC, mod->num_triangles*3*sizeof(unsigned int), vtriangles, VBO_STORE_INDICES, mod);
}

void IQM_FindVBO (model_t *mod)
{
	vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, mod);
	vbo_st = R_VCFindCache(VBO_STORE_ST, mod);
	vbo_normals = R_VCFindCache(VBO_STORE_NORMAL, mod);
	vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT, mod);
	vbo_indices = R_VCFindCache(VBO_STORE_INDICES, mod);
	if (vbo_xyz && vbo_st && vbo_normals && vbo_tangents && vbo_indices)
		return;
	Com_Error (ERR_DROP, "Cannot find VBO for %s\n", mod->name);
}

void IQM_FreeVBO (model_t *mod)
{
	IQM_FindVBO (mod);
	R_VCFree (vbo_xyz);
	R_VCFree (vbo_st);
	R_VCFree (vbo_normals);
	R_VCFree (vbo_tangents);
	R_VCFree (vbo_indices);
}

// NOTE: this should be the only function that needs to care what version the
// IQM file is.
qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer)
{
	iqmheader_t *header;
	int i, j, k;
	const int *inelements;
	int *vtriangles = NULL;
	float *vposition = NULL, *vtexcoord = NULL, *vnormal = NULL, *vtangent = NULL;
	//unsigned char *vblendweights = NULL;
	unsigned char *pbase;
	iqmjoint_t *joint = NULL;
	iqmjoint2_t *joint2 = NULL;
	matrix3x4_t	*inversebaseframe;
	iqmpose_t *poses;
	iqmpose2_t *poses2;
	iqmbounds_t *bounds;
	iqmvertexarray_t *va;
	unsigned short *framedata;
	char *text;
	char skinname[MAX_QPATH], shortname[MAX_QPATH], fullname[MAX_OSPATH];
	char *pskinpath_buffer;
	int skinpath_buffer_length;
	char *parse_string;

	pbase = (unsigned char *)buffer;
	header = (iqmheader_t *)buffer;
	if (memcmp(header->id, "INTERQUAKEMODEL", 16))
	{
		Com_Printf ("Mod_INTERQUAKEMODEL_Load: %s is not an Inter-Quake Model", mod->name);
		return false;
	}

	if (LittleLong(header->version) != 1 && LittleLong(header->version) != 2)
	{
		Com_Printf ("Mod_INTERQUAKEMODEL_Load: only version 1 or 2 models are currently supported (name = %s)", mod->name);
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
	header->ofs_bounds = LittleLong(header->ofs_bounds);
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

	mod->extradata = Hunk_Begin (0x150000);

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
			{
				mod->blendindexes = (unsigned char *)Hunk_Alloc(header->num_vertexes * 4 * sizeof(unsigned char));
				memcpy(mod->blendindexes, (unsigned char *)(pbase + va[i].offset), header->num_vertexes * 4 * sizeof(unsigned char));
			}
			break;
		case IQM_BLENDWEIGHTS:
			if (va[i].format == IQM_UBYTE && va[i].size == 4)
			{
				/*vblendweights = (unsigned char *)Hunk_Alloc(header->num_vertexes * 4 * sizeof(unsigned char));
				memcpy(vblendweights, (unsigned char *)(pbase + va[i].offset), header->num_vertexes * 4 * sizeof(unsigned char));*/
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

	text = header->num_text && header->ofs_text ? (char *)(pbase + header->ofs_text) : "";

	mod->jointname = (char *)Hunk_Alloc(header->num_text * sizeof(char *));
	memcpy(mod->jointname, text, header->num_text * sizeof(char *));

	mod->version = header->version;
	mod->num_frames = header->num_anims;
	mod->num_joints = header->num_joints;
	mod->num_poses = header->num_frames;
	mod->numvertexes = header->num_vertexes;
	mod->num_triangles = header->num_triangles;

	// load the joints
	mod->joints = (iqmjoint2_t*)Hunk_Alloc (header->num_joints * sizeof(iqmjoint2_t));
	if( header->version == 1 )
	{
		joint = (iqmjoint_t *) (pbase + header->ofs_joints);
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
			// If the quaternion w is not already included, calculate it.
			Vec4_CompleteQuatW (mod->joints[i].rotation);
		}
	}
	else
	{
		joint2 = (iqmjoint2_t *) (pbase + header->ofs_joints);
		for (i = 0;i < mod->num_joints;i++)
		{
			mod->joints[i].name = LittleLong(joint2[i].name);
			mod->joints[i].parent = LittleLong(joint2[i].parent);
			for (j = 0;j < 3;j++)
			{
				mod->joints[i].origin[j] = LittleFloat(joint2[i].origin[j]);
				mod->joints[i].rotation[j] = LittleFloat(joint2[i].rotation[j]);
				mod->joints[i].scale[j] = LittleFloat(joint2[i].scale[j]);
			}
			mod->joints[i].rotation[3] = LittleFloat(joint2[i].rotation[3]);
		}
	}
	
	// needed for bending the model in other ways besides the built-in
	// animation.
	mod->baseframe = (matrix3x4_t*)Hunk_Alloc (header->num_joints * sizeof(matrix3x4_t));
	
	// this doesn't need to be a part of mod - remember to free it
	inversebaseframe = (matrix3x4_t*)malloc (header->num_joints * sizeof(matrix3x4_t));

	for(i = 0; i < (int)header->num_joints; i++)
	{
		iqmjoint2_t j = mod->joints[i];

		Matrix3x4_FromQuatAndVectors(&mod->baseframe[i], j.rotation, j.origin, j.scale);
		Matrix3x4_Invert(&inversebaseframe[i], mod->baseframe[i]);

		assert(j.parent < (int)header->num_joints);

		if(j.parent >= 0)
		{
			matrix3x4_t temp;
			Matrix3x4_Multiply(&temp, mod->baseframe[j.parent], mod->baseframe[i]);
			mod->baseframe[i] = temp;
			Matrix3x4_Multiply(&temp, inversebaseframe[i], inversebaseframe[j.parent]);
			inversebaseframe[i] = temp;
		}
	}

	{
		int translate_size, rotate_size, scale_size;
		
		if (header->version == 1)
		{
			translate_size = 3;
			rotate_size = 3;
			scale_size = 3;
		}
		else
		{
			translate_size = 3;
			rotate_size = 4; // quaternion w is included in the v2 file format
			scale_size = 3;
		}
		
		poses = (iqmpose_t *) (pbase + header->ofs_poses);
		poses2 = (iqmpose2_t *) (pbase + header->ofs_poses);
		mod->frames = (matrix3x4_t*)Hunk_Alloc (header->num_frames * header->num_poses * sizeof(matrix3x4_t));
		framedata = (unsigned short *) (pbase + header->ofs_frames);

		for(i = 0; i < header->num_frames; i++)
		{
			for(j = 0; j < header->num_poses; j++)
			{
				int datanum;
				
				signed int parent;
				unsigned int channelmask;
				float *channeloffset, *channelscale;
				vec3_t translate, scale;
				vec4_t rotate;
				matrix3x4_t m, temp;
				
				if (header->version == 1)
				{
					parent = LittleLong(poses[j].parent);
					channelmask = LittleLong(poses[j].channelmask);
					channeloffset = poses[j].channeloffset;
					channelscale = poses[j].channelscale;
				}
				else
				{
					parent = LittleLong(poses2[j].parent);
					channelmask = LittleLong(poses2[j].channelmask);
					channeloffset = poses2[j].channeloffset;
					channelscale = poses2[j].channelscale;
				}

				datanum = 0;
				for (k = 0; k < translate_size; k++, datanum++)
				{
					translate[k] = LittleFloat (channeloffset[datanum]);
					if ((channelmask & (1<<datanum)))
						translate[k] += (unsigned short)LittleShort(*framedata++) * LittleFloat (channelscale[datanum]);
				}
				for (k = 0; k < rotate_size; k++, datanum++)
				{
					rotate[k] = LittleFloat (channeloffset[datanum]);
					if ((channelmask & (1<<datanum)))
						rotate[k] += (unsigned short)LittleShort(*framedata++) * LittleFloat (channelscale[datanum]);
				}
				for (k = 0; k < scale_size; k++, datanum++)
				{
					scale[k] = LittleFloat (channeloffset[datanum]);
					if ((channelmask & (1<<datanum)))
						scale[k] += (unsigned short)LittleShort(*framedata++) * LittleFloat (channelscale[datanum]);
				}
				// Concatenate each pose with the inverse base pose to avoid doing this at animation time.
				// If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
				// Thus it all negates at animation time like so:
				//   (parentPose * parentInverseBasePose) * (parentBasePose * childPose * childInverseBasePose) =>
				//   parentPose * (parentInverseBasePose * parentBasePose) * childPose * childInverseBasePose =>
				//   parentPose * childPose * childInverseBasePose

				// If the quaternion w is not already included, calculate it.
				if (header->version == 1)
					Vec4_CompleteQuatW (rotate);

				Matrix3x4_FromQuatAndVectors(&m, rotate, translate, scale);

				if(parent >= 0)
				{
					Matrix3x4_Multiply(&temp, mod->baseframe[parent], m);
					Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], temp, inversebaseframe[j]);
				}
				else
					Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], m, inversebaseframe[j]);
			}
		}
	}

	mod->outframe = (matrix3x4_t *)Hunk_Alloc(mod->num_joints * sizeof(matrix3x4_t));

	// load bounding box data
	if (header->ofs_bounds)
	{
		float radius = 0;
		bounds = (iqmbounds_t *) (pbase + header->ofs_bounds);
		VectorClear(mod->mins);
		VectorClear(mod->maxs);
		for (i = 0; i < (int)header->num_frames;i++)
		{
			for (j = 0; j < 3; j++)
			{
				bounds[i].mins[j] = LittleFloat(bounds[i].mins[j]);
				if (mod->mins[j] > bounds[i].mins[j])
					mod->mins[j] = bounds[i].mins[j];
				
				bounds[i].maxs[j] = LittleFloat(bounds[i].maxs[j]);
				if (mod->maxs[j] < bounds[i].maxs[j])
					mod->maxs[j] = bounds[i].maxs[j];
			}
			
			bounds[i].radius = LittleFloat(bounds[i].radius);
			if (bounds[i].radius > radius)
				radius = bounds[i].radius;
		}

		mod->radius = radius;
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

	vtriangles = (const int *) (pbase + header->ofs_triangles);

	// load triangle neighbors
	// TODO: we can remove this when shadow volumes are gone, and simply not
	// load this data from the file.
	if (header->ofs_neighbors)
	{
		inelements = (const int *) (pbase + header->ofs_neighbors);

		mod->neighbors = (neighbors_t*)Hunk_Alloc(header->num_triangles * sizeof(neighbors_t));

		for (i = 0;i < (int)header->num_triangles;i++)
		{
			mod->neighbors[i].n[0] = LittleLong(inelements[0]);
			mod->neighbors[i].n[1] = LittleLong(inelements[1]);
			mod->neighbors[i].n[2] = LittleLong(inelements[2]);
			inelements += 3;
		}
	}

	IQM_ByteswapVertexArrays(mod, vposition, vnormal, vtangent, vtexcoord, vtriangles);

	/*
	 * get skin pathname from <model>.skin file and initialize skin
	 */
	COM_StripExtension( mod->name, shortname );
	strcat( shortname, ".skin" );
	skinpath_buffer_length = FS_LoadFile( shortname, (void**)&pskinpath_buffer );
		// note: FS_LoadFile handles upper/lowercase, nul-termination,
		//  and path search sequence

	if ( skinpath_buffer_length > 0 )
	{ // <model>.skin file found and read,
		// data is in Z_Malloc'd buffer pointed to by pskin_buffer

		// get relative image pathname for model's skin from .skin file data
		parse_string = pskinpath_buffer;
		strcpy( skinname, COM_Parse( &parse_string ) );
		Z_Free( pskinpath_buffer ); // free Z_Malloc'd read buffer

		// get image file for skin
		mod->skins[0] = GL_FindImage( skinname, it_skin );
		if ( mod->skins[0] != NULL )
		{ // image file for skin exists
			strcpy( mod->skinname, skinname );

			//load shader for skin
			mod->script = mod->skins[0]->script;
			if ( mod->script )
				RS_ReadyScript( (rscript_t *)mod->script );
		}
	}
	
	/*
	 * get ragdoll info from <model>.rgd file
	 */
	mod->hasRagDoll = false;
	COM_StripExtension( mod->name, shortname );
	strcat( shortname, ".rgd" );
	if ( FS_FullPath( fullname, sizeof(fullname), shortname ) )
	{
		mod->hasRagDoll = IQM_ReadRagDollFile( fullname, mod );
	}
	
	//free temp non hunk mem
	if(inversebaseframe)
		free(inversebaseframe);
	
	// load the VBO data

	IQM_LoadVBO (mod, vposition, vnormal, vtangent, vtexcoord, vtriangles);
	
	return true;
}

float IQM_SelectFrame (void)
{
	float		time;
	
	//frame interpolation
	time = (Sys_Milliseconds() - currententity->frametime) / 100;
	if(time > 1.0)
		time = 1.0;

	if((currententity->frame == currententity->oldframe ) && !IQM_InAnimGroup(currententity->frame, currententity->oldframe))
		time = 0;

	//Check for stopped death anims
	if(currententity->frame == 257 || currententity->frame == 237 || currententity->frame == 219)
		time = 0;

	return currententity->frame + time;
}

int IQM_NextFrame(int frame)
{
	int outframe;

	//just for now
	if(currententity->flags & RF_WEAPONMODEL)
	{
		outframe = frame + 1;
		return outframe;
	}

	switch(frame)
	{
		//map models can be 24 or 40 frames
		case 23:
			if(currentmodel->num_poses > 24)
				outframe = frame + 1;
			else
				outframe = 0;
			break;
		//player standing
		case 39:
			outframe = 0;
			break;
		//player running
		case 45:
			outframe = 40;
			break;
		//player shooting
		case 53:
			outframe = 46;
			break;
		//player jumping
		case 71:
			outframe = 0;
			break;
		//player crouched
		case 153:
			outframe = 135;
			break;
		//player crouched walking
		case 159:
			outframe = 154;
			break;
		case 168:
			outframe = 160;
			break;
		//deaths
		case 219:
			outframe = 219;
			break;
		case 237:
			outframe = 237;
			break;
		case 257:
			outframe = 257;
			break;
		default:
			outframe = frame + 1;
			break;
	}
	return outframe;
}

static inline void IQM_Bend (matrix3x4_t *temp, matrix3x4_t rmat, int jointnum)
{
	vec3_t		basePosition, oldPosition, newPosition;
	matrix3x4_t	*basejoint, *outjoint;
	
	basejoint = &currentmodel->baseframe[jointnum];
	outjoint = &currentmodel->outframe[jointnum];
	
	// concatenate the rotation with the bone
	Matrix3x4_Multiply(temp, rmat, *outjoint);

	// get the position of the bone in the base frame
	VectorSet(basePosition, basejoint->a[3], basejoint->b[3], basejoint->c[3]);

	// add in the correct old bone position and subtract off the wrong new bone position to get the correct rotation pivot
	VectorSet(oldPosition,  DotProduct(basePosition, outjoint->a) + outjoint->a[3],
		 DotProduct(basePosition, outjoint->b) + outjoint->b[3],
		 DotProduct(basePosition, outjoint->c) + outjoint->c[3]);

	VectorSet(newPosition, DotProduct(basePosition, temp->a) + temp->a[3],
		 DotProduct(basePosition, temp->b) + temp->b[3],
		 DotProduct(basePosition, temp->c) + temp->c[3]);

	temp->a[3] += oldPosition[0] - newPosition[0];
	temp->b[3] += oldPosition[1] - newPosition[1];
	temp->c[3] += oldPosition[2] - newPosition[2];

	// replace the old matrix with the rotated one
	Matrix3x4_Copy(outjoint, *temp);
}

void IQM_AnimateFrame (void)
{
	int i;
	int frame1, frame2;
	float frameoffset;
	float curframe;
	int nextframe;
	float modelpitch;
	float modelroll;
	
	modelpitch = degreeToRadian(currententity->angles[PITCH]); 
	modelroll = degreeToRadian(currententity->angles[ROLL]);
	
	curframe = IQM_SelectFrame ();
	nextframe = IQM_NextFrame (currententity->frame);

	frame1 = (int)floor(curframe);
	frame2 = nextframe;
	frameoffset = curframe - frame1;
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
			const char *currentjoint_name;
			signed int currentjoint_parent;
			matrix3x4_t mat, rmat, temp;
			vec3_t rot;
			Matrix3x4_Scale(&mat, mat1[i], 1-frameoffset);
			Matrix3x4_Scale(&temp, mat2[i], frameoffset);

			Matrix3x4_Add(&mat, mat, temp);
			
			currentjoint_name = &currentmodel->jointname[currentmodel->joints[i].name];
			currentjoint_parent = currentmodel->joints[i].parent;

			if(currentjoint_parent >= 0)
				Matrix3x4_Multiply(&currentmodel->outframe[i], currentmodel->outframe[currentjoint_parent], mat);
			else
				Matrix3x4_Copy(&currentmodel->outframe[i], mat);

			//bend the model at the waist for player pitch
			if(!strcmp(currentjoint_name, "Spine") || !strcmp(currentjoint_name, "Spine.001"))
			{
				VectorSet(rot, 0, 1, 0); //remember .iqm's are 90 degrees rotated from reality, so this is the pitch axis
				Matrix3x4GenRotate(&rmat, modelpitch, rot);
				
				IQM_Bend (&temp, rmat, i);
			}
			//now rotate the legs back
			if(!strcmp(currentjoint_name, "hip.l") || !strcmp(currentjoint_name, "hip.r"))
			{
				VectorSet(rot, 0, 1, 0);
				Matrix3x4GenRotate(&rmat, -modelpitch, rot);
				
				IQM_Bend (&temp, rmat, i);
			}

			//bend the model at the waist for player roll
			if(!strcmp(currentjoint_name, "Spine") || !strcmp(currentjoint_name, "Spine.001"))
			{
				VectorSet(rot, 1, 0, 0); //remember .iqm's are 90 degrees rotated from reality, so this is the pitch axis
				Matrix3x4GenRotate(&rmat, modelroll, rot);
				
				IQM_Bend (&temp, rmat, i);
			}
			//now rotate the legs back
			if(!strcmp(currentjoint_name, "hip.l") || !strcmp(currentjoint_name, "hip.r"))
			{
				VectorSet(rot, 1, 0, 0);
				Matrix3x4GenRotate(&rmat, -modelroll, rot);
				
				IQM_Bend (&temp, rmat, i);
			}
		}
	}
}

void IQM_AnimateRagdoll(int RagDollID, int shellEffect)
{
	//we only deal with one frame

	//animate using the rotations from our corresponding ODE objects.
	int i, j;
	{
		for(i = 0; i < RagDoll[RagDollID].ragDollMesh->num_joints; i++)
		{
			matrix3x4_t mat, rmat;
            int parent;

            for(j = 0; j < RagDollBindsCount; j++)
            {
				if(!strcmp(&RagDoll[RagDollID].ragDollMesh->jointname[RagDoll[RagDollID].ragDollMesh->joints[i].name], RagDollBinds[j].name))
				{
					int object = RagDollBinds[j].object;
					const dReal *odeRot = dBodyGetRotation(RagDoll[RagDollID].RagDollObject[object].body);
					const dReal *odePos = dBodyGetPosition(RagDoll[RagDollID].RagDollObject[object].body);
					Matrix3x4GenFromODE(&rmat, odeRot, odePos);
					Matrix3x4_Multiply(&RagDoll[RagDollID].ragDollMesh->outframe[i], rmat, RagDoll[RagDollID].RagDollObject[object].initmat);
					goto nextjoint;
				}
			}

			parent = RagDoll[RagDollID].ragDollMesh->joints[i].parent;
            if(parent >= 0)
            {
                Matrix3x4_Invert(&mat, RagDoll[RagDollID].initframe[parent]);
                Matrix3x4_Multiply(&mat, mat, RagDoll[RagDollID].initframe[i]);
                Matrix3x4_Multiply(&RagDoll[RagDollID].ragDollMesh->outframe[i], RagDoll[RagDollID].ragDollMesh->outframe[parent], mat);
            }
            else
                memset(&RagDoll[RagDollID].ragDollMesh->outframe[i], 0, sizeof(matrix3x4_t));

        nextjoint:;
		}
	}
}

void IQM_DrawVBO (void)
{
	IQM_FindVBO (currentmodel);
	
	qglEnableClientState( GL_VERTEX_ARRAY );
	GL_BindVBO(vbo_xyz);
	qglVertexPointer(3, GL_FLOAT, 0, 0);

	KillFlags |= KILL_TMU0_POINTER;
	qglClientActiveTextureARB (GL_TEXTURE0);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_BindVBO(vbo_st);
	qglTexCoordPointer(2, GL_FLOAT, 0, 0);

	KillFlags |= KILL_NORMAL_POINTER;
	qglEnableClientState( GL_NORMAL_ARRAY );
	GL_BindVBO(vbo_normals);
	qglNormalPointer(GL_FLOAT, 0, 0);

	glEnableVertexAttribArrayARB (ATTR_TANGENT_IDX);
	GL_BindVBO(vbo_tangents);
	glVertexAttribPointerARB (ATTR_TANGENT_IDX, 4, GL_FLOAT, GL_FALSE, 0, 0);
		
	GL_BindVBO(NULL);

	GL_BindIBO(vbo_indices);								

	glEnableVertexAttribArrayARB(ATTR_WEIGHTS_IDX);
	glEnableVertexAttribArrayARB(ATTR_BONES_IDX);
	glVertexAttribPointerARB(ATTR_WEIGHTS_IDX, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char)*4, currentmodel->blendweights);
	glVertexAttribPointerARB(ATTR_BONES_IDX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char)*4, currentmodel->blendindexes); 				
	
	qglDrawElements(GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);
	
	GL_BindIBO(NULL);
	
	glDisableVertexAttribArrayARB(ATTR_TANGENT_IDX);
	glDisableVertexAttribArrayARB(ATTR_WEIGHTS_IDX);
	glDisableVertexAttribArrayARB(ATTR_BONES_IDX);

	R_KillVArrays ();

}

qboolean R_Mesh_CullBBox (vec3_t bbox[8]);

qboolean IQM_CullModel( void )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	vec3_t	dist;
	vec3_t bbox[8];

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

	// Keep nearby meshes so shadows don't blatantly disappear when out of frustom
	if (VectorLength(dist) > 150 && R_Mesh_CullBBox (bbox))
		return true;
	
	// TODO: could probably find a better place for this.
	if(r_ragdolls->integer)
	{
		//Ragdolls take over at beginning of each death sequence
		if	(	!(currententity->flags & RF_TRANSLUCENT) &&
				currentmodel->hasRagDoll && 
				(currententity->frame == 199 || 
				currententity->frame == 220 ||
				currententity->frame == 238)
			)
		{
			// Make sure the positions of all the arms and legs and stuff have
			// been initialized-- necessary for the current player's avatar,
			// which has not yet been rendered.
			IQM_AnimateFrame (); 
			RGD_AddNewRagdoll(currententity->origin, currententity->name);
		}
		//Do not render deathframes if using ragdolls - do not render translucent helmets
		if((currentmodel->hasRagDoll || (currententity->flags & RF_TRANSLUCENT)) && currententity->frame > 198)
			return true;
	}
	
	return false;
}

//Can these next two be replaced with some type of animation grouping from the model?
qboolean IQM_InAnimGroup(int frame, int oldframe)
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

void IQM_DrawCasterFrame ()
{
	//just use a very basic shader for this instead of the normal mesh shader
	glUseProgramObjectARB( g_blankmeshprogramObj );

	//send outframe, blendweights, and blendindexes to shader
	glUniformMatrix3x4fvARB( g_location_bm_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );
	
	//tell it to use IQM skeletal animation
	glUniform1iARB(g_location_bm_useGPUanim, 1);

	IQM_DrawVBO ();

	glUseProgramObjectARB( 0 );
}

void IQM_DrawCaster ( void )
{
	vec3_t		sv_angles;

	if(currententity->team) //don't draw flag models, handled by sprites
		return;

	if ( currententity->flags & RF_WEAPONMODEL ) //don't draw weapon model shadow casters
		return;

	if ( currententity->flags & RF_SHELL_ANY ) //no shells
		return;
	
	qglPushMatrix ();
	
	IQM_AnimateFrame();

	VectorCopy(currententity->angles, sv_angles);
	currententity->angles[PITCH] = currententity->angles[ROLL] = 0;
	R_RotateForEntity (currententity);
	VectorCopy(sv_angles, currententity->angles);

	IQM_DrawCasterFrame();

	qglPopMatrix();
}

void IQM_DrawRagDollCaster ( int RagDollID )
{
	if ( RGD_CullRagDolls( RagDollID ) )
		return;

    qglPushMatrix ();

	IQM_AnimateRagdoll(RagDollID, false);

	currentmodel = RagDoll[RagDollID].ragDollMesh;

	IQM_DrawCasterFrame();

	qglPopMatrix();
}
