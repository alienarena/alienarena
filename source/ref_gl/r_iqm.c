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
qboolean has_vbo;

float modelpitch;

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

void Matrix3x4_ScaleAdd (matrix3x4_t *out, matrix3x4_t *base, float scale, matrix3x4_t *add)
{
   out->a[0] = base->a[0] * scale + add->a[0];
   out->a[1] = base->a[1] * scale + add->a[1];
   out->a[2] = base->a[2] * scale + add->a[2];
   out->a[3] = base->a[3] * scale + add->a[3];

   out->b[0] = base->b[0] * scale + add->b[0];
   out->b[1] = base->b[1] * scale + add->b[1];
   out->b[2] = base->b[2] * scale + add->b[2];
   out->b[3] = base->b[3] * scale + add->b[3];

   out->c[0] = base->c[0] * scale + add->c[0];
   out->c[1] = base->c[1] * scale + add->c[1];
   out->c[2] = base->c[2] * scale + add->c[2];
   out->c[3] = base->c[3] * scale + add->c[3];
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

void Matrix3x4_Transform(mvertex_t *out, matrix3x4_t mat, const mvertex_t in)
{
	out->position[0] = DotProduct(mat.a, in.position) + mat.a[3];
    out->position[1] = DotProduct(mat.b, in.position) + mat.b[3];
    out->position[2] = DotProduct(mat.c, in.position) + mat.c[3];
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

void IQM_LoadVertexArrays(model_t *iqmmodel, float *vposition, float *vnormal, float *vtangent)
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

qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer)
{
	iqmheader_t *header;
	int i, j, k;
	const int *inelements;
	float *vposition = NULL, *vtexcoord = NULL, *vnormal = NULL, *vtangent = NULL;
	unsigned char *vblendweights = NULL;
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
	if( header->version == 1 )
	{
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
	}
	else
	{
		joint2 = (iqmjoint2_t *) (pbase + header->ofs_joints);
		mod->joints2 = (iqmjoint2_t*)Hunk_Alloc (header->num_joints * sizeof(iqmjoint2_t));
		for (i = 0;i < mod->num_joints;i++)
		{
			mod->joints2[i].name = LittleLong(joint2[i].name);
			mod->joints2[i].parent = LittleLong(joint2[i].parent);
			for (j = 0;j < 3;j++)
			{
				mod->joints2[i].origin[j] = LittleFloat(joint2[i].origin[j]);
				mod->joints2[i].rotation[j] = LittleFloat(joint2[i].rotation[j]);
				mod->joints2[i].scale[j] = LittleFloat(joint2[i].scale[j]);
			}
			mod->joints2[i].rotation[3] = LittleFloat(joint2[i].rotation[3]);
		}
	}
	//these don't need to be a part of mod - remember to free them
	mod->baseframe = (matrix3x4_t*)Hunk_Alloc (header->num_joints * sizeof(matrix3x4_t));
	inversebaseframe = (matrix3x4_t*)malloc (header->num_joints * sizeof(matrix3x4_t));

	if( header->version == 1 )
	{
		for(i = 0; i < (int)header->num_joints; i++)
		{
			vec3_t rot;
			vec4_t q_rot;
			iqmjoint_t j = mod->joints[i];

			//first need to make a vec4 quat from our rotation vec
			VectorSet(rot, j.rotation[0], j.rotation[1], j.rotation[2]);
			Vector4Set(q_rot, j.rotation[0], j.rotation[1], j.rotation[2], -sqrt(max(1.0 - pow(VectorLength(rot),2), 0.0)));

			Matrix3x4_FromQuatAndVectors(&mod->baseframe[i], q_rot, j.origin, j.scale);
			Matrix3x4_Invert(&inversebaseframe[i], mod->baseframe[i]);

			if(j.parent >= 0)
			{
				matrix3x4_t temp;
				Matrix3x4_Multiply(&temp, mod->baseframe[j.parent], mod->baseframe[i]);
				mod->baseframe[i] = temp;
				Matrix3x4_Multiply(&temp, inversebaseframe[i], inversebaseframe[j.parent]);
				inversebaseframe[i] = temp;
			}
		}
	}
	else
	{
		for(i = 0; i < (int)header->num_joints; i++)
		{
			iqmjoint2_t j = mod->joints2[i];

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
	}

	if( header->version == 1 )
	{
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

				p.parent = LittleLong(p.parent);
                                p.channelmask = LittleLong(p.channelmask);
                                for(k = 0; k < 9; k++)
                                {
                                        p.channeloffset[k] = LittleFloat(p.channeloffset[k]);
                                        p.channelscale[k] = LittleFloat(p.channelscale[k]);
                                }

				translate[0] = p.channeloffset[0]; if(p.channelmask&0x01) translate[0] += (unsigned short)LittleShort(*framedata++) * p.channelscale[0];
				translate[1] = p.channeloffset[1]; if(p.channelmask&0x02) translate[1] += (unsigned short)LittleShort(*framedata++) * p.channelscale[1];
				translate[2] = p.channeloffset[2]; if(p.channelmask&0x04) translate[2] += (unsigned short)LittleShort(*framedata++) * p.channelscale[2];
				rotate[0] = p.channeloffset[3]; if(p.channelmask&0x08) rotate[0] += (unsigned short)LittleShort(*framedata++) * p.channelscale[3];
				rotate[1] = p.channeloffset[4]; if(p.channelmask&0x10) rotate[1] += (unsigned short)LittleShort(*framedata++) * p.channelscale[4];
				rotate[2] = p.channeloffset[5]; if(p.channelmask&0x20) rotate[2] += (unsigned short)LittleShort(*framedata++) * p.channelscale[5];
				scale[0] = p.channeloffset[6]; if(p.channelmask&0x40) scale[0] += (unsigned short)LittleShort(*framedata++) * p.channelscale[6];
				scale[1] = p.channeloffset[7]; if(p.channelmask&0x80) scale[1] += (unsigned short)LittleShort(*framedata++) * p.channelscale[7];
				scale[2] = p.channeloffset[8]; if(p.channelmask&0x100) scale[2] += (unsigned short)LittleShort(*framedata++) * p.channelscale[8];
				// Concatenate each pose with the inverse base pose to avoid doing this at animation time.
				// If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
				// Thus it all negates at animation time like so:
				//   (parentPose * parentInverseBasePose) * (parentBasePose * childPose * childInverseBasePose) =>
				//   parentPose * (parentInverseBasePose * parentBasePose) * childPose * childInverseBasePose =>
				//   parentPose * childPose * childInverseBasePose

				Vector4Set(q_rot, rotate[0], rotate[1], rotate[2], -sqrt(max(1.0 - pow(VectorLength(rotate),2), 0.0)));

				Matrix3x4_FromQuatAndVectors(&m, q_rot, translate, scale);

				if(p.parent >= 0)
				{
					Matrix3x4_Multiply(&temp, mod->baseframe[p.parent], m);
					Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], temp, inversebaseframe[j]);
				}
				else
					Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], m, inversebaseframe[j]);
			}
		}
	}
	else
	{
		poses2 = (iqmpose2_t *) (pbase + header->ofs_poses);
		mod->frames = (matrix3x4_t*)Hunk_Alloc (header->num_frames * header->num_poses * sizeof(matrix3x4_t));
		framedata = (unsigned short *) (pbase + header->ofs_frames);

		for(i = 0; i < header->num_frames; i++)
		{
			for(j = 0; j < header->num_poses; j++)
			{
				iqmpose2_t p = poses2[j];
				vec3_t translate, scale;
				vec4_t rotate;
				matrix3x4_t m, temp;

				p.parent = LittleLong(p.parent);
				p.channelmask = LittleLong(p.channelmask);
				for(k = 0; k < 10; k++)
				{
					p.channeloffset[k] = LittleFloat(p.channeloffset[k]);
					p.channelscale[k] = LittleFloat(p.channelscale[k]);
				}

				translate[0] = p.channeloffset[0]; if(p.channelmask&0x01) translate[0] += (unsigned short)LittleShort(*framedata++) * p.channelscale[0];
				translate[1] = p.channeloffset[1]; if(p.channelmask&0x02) translate[1] += (unsigned short)LittleShort(*framedata++) * p.channelscale[1];
				translate[2] = p.channeloffset[2]; if(p.channelmask&0x04) translate[2] += (unsigned short)LittleShort(*framedata++) * p.channelscale[2];
				rotate[0] = p.channeloffset[3]; if(p.channelmask&0x08) rotate[0] += (unsigned short)LittleShort(*framedata++) * p.channelscale[3];
				rotate[1] = p.channeloffset[4]; if(p.channelmask&0x10) rotate[1] += (unsigned short)LittleShort(*framedata++) * p.channelscale[4];
				rotate[2] = p.channeloffset[5]; if(p.channelmask&0x20) rotate[2] += (unsigned short)LittleShort(*framedata++) * p.channelscale[5];
				rotate[3] = p.channeloffset[6]; if(p.channelmask&0x40) rotate[3] += (unsigned short)LittleShort(*framedata++) * p.channelscale[6];
				scale[0] = p.channeloffset[7]; if(p.channelmask&0x80) scale[0] += (unsigned short)LittleShort(*framedata++) * p.channelscale[7];
				scale[1] = p.channeloffset[8]; if(p.channelmask&0x100) scale[1] += (unsigned short)LittleShort(*framedata++) * p.channelscale[8];
				scale[2] = p.channeloffset[9]; if(p.channelmask&0x200) scale[2] += (unsigned short)LittleShort(*framedata++) * p.channelscale[9];
				// Concatenate each pose with the inverse base pose to avoid doing this at animation time.
				// If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
				// Thus it all negates at animation time like so:
				//   (parentPose * parentInverseBasePose) * (parentBasePose * childPose * childInverseBasePose) =>
				//   parentPose * (parentInverseBasePose * parentBasePose) * childPose * childInverseBasePose =>
				//   parentPose * childPose * childInverseBasePose

				Matrix3x4_FromQuatAndVectors(&m, rotate, translate, scale);

				if(p.parent >= 0)
				{
					Matrix3x4_Multiply(&temp, mod->baseframe[p.parent], m);
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

		mod->neighbors = (neighbors_t*)Hunk_Alloc(header->num_triangles * sizeof(neighbors_t));

		for (i = 0;i < (int)header->num_triangles;i++)
		{
			mod->neighbors[i].n[0] = LittleLong(inelements[0]);
			mod->neighbors[i].n[1] = LittleLong(inelements[1]);
			mod->neighbors[i].n[2] = LittleLong(inelements[2]);
			inelements += 3;
		}
	}

	// load vertex data
	IQM_LoadVertexArrays(mod, vposition, vnormal, vtangent);

	// load texture coodinates
    mod->st = (fstvert_t*)Hunk_Alloc (header->num_vertexes * sizeof(fstvert_t));
	//mod->blendweights = (float *)Hunk_Alloc(header->num_vertexes * 4 * sizeof(float));

	for (i = 0;i < (int)header->num_vertexes;i++)
	{
		mod->st[i].s = LittleFloat(vtexcoord[0]);
		mod->st[i].t = LittleFloat(vtexcoord[1]);

		vtexcoord+=2;

		/*for(j = 0; j < 4; j++)
			mod->blendweights[i*4+j] = vblendweights[j]blendweights;

		vblendweights+=4;*/
	}

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
			COM_StripExtension( skinname, shortname );
			mod->script = RS_FindScript( shortname );
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

	//load VBO
	if(gl_state.vbo)
	{
		//shouldn't really need this, but make sure a cache doesn't already exist for this model 
		//in the event this routine is called for any reason(model being flushed, etc).
		vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, mod);
		vbo_st = R_VCFindCache(VBO_STORE_ST, mod);
		vbo_normals = R_VCFindCache(VBO_STORE_NORMAL, mod);
		vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT, mod);
		vbo_indices = R_VCFindCache(VBO_STORE_INDICES, mod);

		//if any are missing, go ahead and reload the VBO
		if(!(vbo_xyz && vbo_st && vbo_normals && vbo_tangents && vbo_indices))
		{
			R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), mod->vertexes, VBO_STORE_XYZ, mod);
			R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec2_t), mod->st, VBO_STORE_ST, mod);
			R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), mod->normal, VBO_STORE_NORMAL, mod);
			R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec4_t), mod->tangent, VBO_STORE_TANGENT, mod);
			R_VCLoadData(VBO_STATIC, mod->num_triangles*3*sizeof(unsigned int), mod->tris, VBO_STORE_INDICES, mod);
		}
	}

	//free temp non hunk mem
	if(inversebaseframe)
		free(inversebaseframe);

	return true;
}

void IQM_AnimateFrame(float curframe, int nextframe)
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
			matrix3x4_t mat, rmat, temp;
			vec3_t rot;
			Matrix3x4_Scale(&mat, mat1[i], 1-frameoffset);
			Matrix3x4_Scale(&temp, mat2[i], frameoffset);

			Matrix3x4_Add(&mat, mat, temp);

			if(currentmodel->version == 1)
			{
				if(currentmodel->joints[i].parent >= 0)
					Matrix3x4_Multiply(&currentmodel->outframe[i], currentmodel->outframe[currentmodel->joints[i].parent], mat);
				else
					Matrix3x4_Copy(&currentmodel->outframe[i], mat);
			}
			else
			{
				if(currentmodel->joints2[i].parent >= 0)
					Matrix3x4_Multiply(&currentmodel->outframe[i], currentmodel->outframe[currentmodel->joints2[i].parent], mat);
				else
					Matrix3x4_Copy(&currentmodel->outframe[i], mat);
			}

			//bend the model at the waist for player pitch
			if(currentmodel->version == 1)
			{
				if(!strcmp(&currentmodel->jointname[currentmodel->joints[i].name], "Spine")||
					!strcmp(&currentmodel->jointname[currentmodel->joints[i].name], "Spine.001"))
				{
					vec3_t basePosition, oldPosition, newPosition;
					VectorSet(rot, 0, 1, 0); //remember .iqm's are 90 degrees rotated from reality, so this is the pitch axis
					Matrix3x4GenRotate(&rmat, modelpitch, rot);

					// concatenate the rotation with the bone
					Matrix3x4_Multiply(&temp, rmat, currentmodel->outframe[i]);

					// get the position of the bone in the base frame
					VectorSet(basePosition, currentmodel->baseframe[i].a[3], currentmodel->baseframe[i].b[3], currentmodel->baseframe[i].c[3]);

					// add in the correct old bone position and subtract off the wrong new bone position to get the correct rotation pivot
					VectorSet(oldPosition,  DotProduct(basePosition, currentmodel->outframe[i].a) + currentmodel->outframe[i].a[3],
						 DotProduct(basePosition, currentmodel->outframe[i].b) + currentmodel->outframe[i].b[3],
						 DotProduct(basePosition, currentmodel->outframe[i].c) + currentmodel->outframe[i].c[3]);

					VectorSet(newPosition, DotProduct(basePosition, temp.a) + temp.a[3],
	   					 DotProduct(basePosition, temp.b) + temp.b[3],
						 DotProduct(basePosition, temp.c) + temp.c[3]);

					temp.a[3] += oldPosition[0] - newPosition[0];
					temp.b[3] += oldPosition[1] - newPosition[1];
					temp.c[3] += oldPosition[2] - newPosition[2];

					// replace the old matrix with the rotated one
					Matrix3x4_Copy(&currentmodel->outframe[i], temp);
				}
				//now rotate the legs back
				if(!strcmp(&currentmodel->jointname[currentmodel->joints[i].name], "hip.l")||
					!strcmp(&currentmodel->jointname[currentmodel->joints[i].name], "hip.r"))
				{
					vec3_t basePosition, oldPosition, newPosition;
					VectorSet(rot, 0, 1, 0);
					Matrix3x4GenRotate(&rmat, -modelpitch, rot);

					// concatenate the rotation with the bone
					Matrix3x4_Multiply(&temp, rmat, currentmodel->outframe[i]);

					// get the position of the bone in the base frame
					VectorSet(basePosition, currentmodel->baseframe[i].a[3], currentmodel->baseframe[i].b[3], currentmodel->baseframe[i].c[3]);

					// add in the correct old bone position and subtract off the wrong new bone position to get the correct rotation pivot
					VectorSet(oldPosition,  DotProduct(basePosition, currentmodel->outframe[i].a) + currentmodel->outframe[i].a[3],
						 DotProduct(basePosition, currentmodel->outframe[i].b) + currentmodel->outframe[i].b[3],
						 DotProduct(basePosition, currentmodel->outframe[i].c) + currentmodel->outframe[i].c[3]);

					VectorSet(newPosition, DotProduct(basePosition, temp.a) + temp.a[3],
	   					 DotProduct(basePosition, temp.b) + temp.b[3],
						 DotProduct(basePosition, temp.c) + temp.c[3]);

					temp.a[3] += oldPosition[0] - newPosition[0];
					temp.b[3] += oldPosition[1] - newPosition[1];
					temp.c[3] += oldPosition[2] - newPosition[2];

					// replace the old matrix with the rotated one
					Matrix3x4_Copy(&currentmodel->outframe[i], temp);
				}
			}
			else
			{
				if(!strcmp(&currentmodel->jointname[currentmodel->joints2[i].name], "Spine")||
				!strcmp(&currentmodel->jointname[currentmodel->joints2[i].name], "Spine.001"))
				{
					vec3_t basePosition, oldPosition, newPosition;
					VectorSet(rot, 0, 1, 0); //remember .iqm's are 90 degrees rotated from reality, so this is the pitch axis
					Matrix3x4GenRotate(&rmat, modelpitch, rot);

					// concatenate the rotation with the bone
					Matrix3x4_Multiply(&temp, rmat, currentmodel->outframe[i]);

					// get the position of the bone in the base frame
					VectorSet(basePosition, currentmodel->baseframe[i].a[3], currentmodel->baseframe[i].b[3], currentmodel->baseframe[i].c[3]);

					// add in the correct old bone position and subtract off the wrong new bone position to get the correct rotation pivot
					VectorSet(oldPosition,  DotProduct(basePosition, currentmodel->outframe[i].a) + currentmodel->outframe[i].a[3],
						 DotProduct(basePosition, currentmodel->outframe[i].b) + currentmodel->outframe[i].b[3],
						 DotProduct(basePosition, currentmodel->outframe[i].c) + currentmodel->outframe[i].c[3]);

					VectorSet(newPosition, DotProduct(basePosition, temp.a) + temp.a[3],
	   					 DotProduct(basePosition, temp.b) + temp.b[3],
						 DotProduct(basePosition, temp.c) + temp.c[3]);

					temp.a[3] += oldPosition[0] - newPosition[0];
					temp.b[3] += oldPosition[1] - newPosition[1];
					temp.c[3] += oldPosition[2] - newPosition[2];

					// replace the old matrix with the rotated one
					Matrix3x4_Copy(&currentmodel->outframe[i], temp);
				}
				//now rotate the legs back
				if(!strcmp(&currentmodel->jointname[currentmodel->joints2[i].name], "hip.l")||
					!strcmp(&currentmodel->jointname[currentmodel->joints2[i].name], "hip.r"))
				{
					vec3_t basePosition, oldPosition, newPosition;
					VectorSet(rot, 0, 1, 0);
					Matrix3x4GenRotate(&rmat, -modelpitch, rot);

					// concatenate the rotation with the bone
					Matrix3x4_Multiply(&temp, rmat, currentmodel->outframe[i]);

					// get the position of the bone in the base frame
					VectorSet(basePosition, currentmodel->baseframe[i].a[3], currentmodel->baseframe[i].b[3], currentmodel->baseframe[i].c[3]);

					// add in the correct old bone position and subtract off the wrong new bone position to get the correct rotation pivot
					VectorSet(oldPosition,  DotProduct(basePosition, currentmodel->outframe[i].a) + currentmodel->outframe[i].a[3],
						 DotProduct(basePosition, currentmodel->outframe[i].b) + currentmodel->outframe[i].b[3],
						 DotProduct(basePosition, currentmodel->outframe[i].c) + currentmodel->outframe[i].c[3]);

					VectorSet(newPosition, DotProduct(basePosition, temp.a) + temp.a[3],
	   					 DotProduct(basePosition, temp.b) + temp.b[3],
						 DotProduct(basePosition, temp.c) + temp.c[3]);

					temp.a[3] += oldPosition[0] - newPosition[0];
					temp.b[3] += oldPosition[1] - newPosition[1];
					temp.c[3] += oldPosition[2] - newPosition[2];

					// replace the old matrix with the rotated one
					Matrix3x4_Copy(&currentmodel->outframe[i], temp);
				}
			}
		}
	}
	// to do - this entire section could be handled in a glsl shader, saving huge amounts of cpu overhead
	// The actual vertex generation based on the matrixes follows...
	{
		const mvertex_t *srcpos = (const mvertex_t *)currentmodel->vertexes;
		const mnormal_t *srcnorm = (const mnormal_t *)currentmodel->normal;
		const mtangent_t *srctan = (const mtangent_t *)currentmodel->tangent;
				
		mvertex_t *dstpos = (mvertex_t *)currentmodel->animatevertexes;
		mnormal_t *dstnorm = (mnormal_t *)currentmodel->animatenormal;
		mtangent_t *dsttan = (mtangent_t *)currentmodel->animatetangent;

		const unsigned char *index = currentmodel->blendindexes;

		const unsigned char *weight = currentmodel->blendweights;

		//we need to skip this vbo check if not using a shader - since the animation is done in the shader (might want to check for normalmap stage as well)
		has_vbo = false;
		//a lot of conditions need to be enabled in order to use GPU animation
		if ((gl_state.vbo && gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer && r_gpuanim->integer) &&
			(r_shaders->integer || ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))))
		{
			vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, currentmodel);
			vbo_st = R_VCFindCache(VBO_STORE_ST, currentmodel);
			vbo_normals = R_VCFindCache(VBO_STORE_NORMAL, currentmodel);
			vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT, currentmodel);
			vbo_indices = R_VCFindCache(VBO_STORE_INDICES, currentmodel);
			if(vbo_xyz && vbo_st && vbo_normals && vbo_tangents && vbo_indices)
			{
				has_vbo = true;
				goto skipLoad;
			}
		}					

		for(i = 0; i < currentmodel->numvertexes; i++)
		{
			matrix3x4_t mat;

			// Blend matrixes for this vertex according to its blend weights.
			// the first index/weight is always present, and the weights are
			// guaranteed to add up to 255. So if only the first weight is
			// presented, you could optimize this case by skipping any weight
			// multiplies and intermediate storage of a blended matrix.
			// There are only at most 4 weights per vertex, and they are in
			// sorted order from highest weight to lowest weight. Weights with
			// 0 values, which are always at the end, are unused.

			Matrix3x4_Scale(&mat, currentmodel->outframe[index[0]], weight[0]/255.0f);

			for(j = 1; j < 4 && weight[j]; j++)
			{
				Matrix3x4_ScaleAdd (&mat, &currentmodel->outframe[index[j]], weight[j]/255.0f, &mat);
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
skipLoad: ;
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
				if(RagDoll[RagDollID].ragDollMesh->version == 1)
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
				else
				{
					if(!strcmp(&RagDoll[RagDollID].ragDollMesh->jointname[RagDoll[RagDollID].ragDollMesh->joints2[i].name], RagDollBinds[j].name))
					{
						int object = RagDollBinds[j].object;
						const dReal *odeRot = dBodyGetRotation(RagDoll[RagDollID].RagDollObject[object].body);
						const dReal *odePos = dBodyGetPosition(RagDoll[RagDollID].RagDollObject[object].body);
						Matrix3x4GenFromODE(&rmat, odeRot, odePos);
						Matrix3x4_Multiply(&RagDoll[RagDollID].ragDollMesh->outframe[i], rmat, RagDoll[RagDollID].RagDollObject[object].initmat);
						goto nextjoint;
					}
				}
			}

			if(RagDoll[RagDollID].ragDollMesh->version == 1)
				parent = RagDoll[RagDollID].ragDollMesh->joints[i].parent;
			else
				parent = RagDoll[RagDollID].ragDollMesh->joints2[i].parent;
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

	{
		const mvertex_t *srcpos = (const mvertex_t *)RagDoll[RagDollID].ragDollMesh->vertexes;
		const mnormal_t *srcnorm = (const mnormal_t *)RagDoll[RagDollID].ragDollMesh->normal;
		const mtangent_t *srctan = (const mtangent_t *)RagDoll[RagDollID].ragDollMesh->tangent;

		mvertex_t *dstpos = (mvertex_t *)RagDoll[RagDollID].ragDollMesh->animatevertexes;
		mnormal_t *dstnorm = (mnormal_t *)RagDoll[RagDollID].ragDollMesh->animatenormal;
		mtangent_t *dsttan = (mtangent_t *)RagDoll[RagDollID].ragDollMesh->animatetangent;

		const unsigned char *index = RagDoll[RagDollID].ragDollMesh->blendindexes;
		const unsigned char *weight = RagDoll[RagDollID].ragDollMesh->blendweights;

		//we need to skip this vbo check if not using a shader - since the animation is done in the shader (might want to check for normalmap stage)
		has_vbo = false;
		//a lot of conditions need to be enabled in order to use GPU animation

		if ((gl_state.vbo && RagDoll[RagDollID].script && gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer && r_gpuanim->integer) &&
			(r_shaders->integer || shellEffect))
		{
			vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, RagDoll[RagDollID].ragDollMesh);
			vbo_st = R_VCFindCache(VBO_STORE_ST, RagDoll[RagDollID].ragDollMesh);
			vbo_normals = R_VCFindCache(VBO_STORE_NORMAL, RagDoll[RagDollID].ragDollMesh);
			vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT, RagDoll[RagDollID].ragDollMesh);
			vbo_indices = R_VCFindCache(VBO_STORE_INDICES, RagDoll[RagDollID].ragDollMesh);
			if(vbo_xyz && vbo_st && vbo_normals && vbo_tangents && vbo_indices)
			{
				has_vbo = true;
				goto skipLoad;
			}
		}		

		for(i = 0; i < RagDoll[RagDollID].ragDollMesh->numvertexes; i++)
		{
			matrix3x4_t mat;

			Matrix3x4_Scale(&mat, RagDoll[RagDollID].ragDollMesh->outframe[index[0]], weight[0]/255.0f);

			for(j = 1; j < 4 && weight[j]; j++)
			{
				Matrix3x4_ScaleAdd (&mat, &RagDoll[RagDollID].ragDollMesh->outframe[index[j]], weight[j]/255.0f, &mat);
			}

			Matrix3x4_Transform(dstpos, mat, *srcpos);

			Matrix3x4_TransformNormal(dstnorm, mat, *srcnorm);

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
skipLoad: ;
	}
}

void IQM_Vlight (vec3_t baselight, mnormal_t *normal, vec3_t angles, vec3_t lightOut)
{
	float l;
	float lscale;

	VectorScale(baselight, gl_modulate->value, lightOut);

	//probably will remove this - maybe we can get a faster algorithm and redo one day
	if(!gl_vlights->integer)
		return;

	lscale = 3.0;

    l = lscale * VLight_GetLightValue (normal->dir, lightPosition, angles[PITCH], angles[YAW]);

    VectorScale(baselight, l, lightOut);
}

void IQM_DrawFrame(int skinnum)
{
	int		i, j;
	vec3_t	vectors[3];
	rscript_t *rs = NULL;
	float	shellscale;
	float	alpha, basealpha;
	vec3_t	lightcolor;
	int		index_xyz, index_st;
	int		va = 0;
	qboolean mirror = false;
	qboolean glass = false;
	qboolean depthmaskrscipt = false;

	if (r_shaders->integer)
		rs = currententity->script;

	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	if (currententity->flags & RF_TRANSLUCENT)
	{
		alpha = currententity->alpha;
		if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		{
			if(gl_mirror->integer)
				mirror = true;
			else
				glass = true;
		}
		else
			glass = true;
	}
	else
		alpha = basealpha = 1.0;

	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	//render the model

	if(( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) ) )
	{
		//shell render

		if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer) 
		{
            vec3_t lightVec, lightVal;

			if(!has_vbo)
			{
				R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
				qglNormalPointer(GL_FLOAT, 0, NormalsArray);
				glEnableVertexAttribArrayARB (1);
				glVertexAttribPointerARB(1, 4, GL_FLOAT,GL_FALSE, 0, TangentsArray);
			}

            R_GetLightVals(currententity->origin, false, false);

            //send light level and color to shader, ramp up a bit
            VectorCopy(lightcolor, lightVal);
           
            //simple directional(relative light position)
            VectorSubtract(lightPosition, currententity->origin, lightVec);
            VectorMA(lightPosition, 1.0, lightVec, lightPosition);
            R_ModelViewTransform(lightPosition, lightVec);

            GL_EnableMultitexture( true );

            glUseProgramObjectARB( g_meshprogramObj );

            glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);

            GL_SelectTexture( GL_TEXTURE1);
            qglBindTexture (GL_TEXTURE_2D, r_shelltexture2->texnum);
            glUniform1iARB( g_location_baseTex, 1);

            GL_SelectTexture( GL_TEXTURE0);
            qglBindTexture (GL_TEXTURE_2D, r_shellnormal->texnum);
            glUniform1iARB( g_location_normTex, 0);

            GL_SelectTexture( GL_TEXTURE0);

            glUniform1iARB( g_location_useFX, 0);

            glUniform1iARB( g_location_useGlow, 0);

			glUniform1iARB( g_location_useScatter, 0);

            glUniform3fARB( g_location_color, 1.0, 1.0, 1.0);

            glUniform1fARB( g_location_meshTime, rs_realtime);

            glUniform1iARB( g_location_meshFog, map_fog);
        }
		else
		{
			GL_Bind(r_shelltexture2->texnum);
			R_InitVArrays (VERT_COLOURED_TEXTURED);
		}

		if((currententity->flags & (RF_WEAPONMODEL | RF_SHELL_GREEN)) || (gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer))
			shellscale = 0.4;
		else
			shellscale = 1.6;

		if(!has_vbo)
		{
			if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
			{
				glUniform1iARB(g_location_useGPUanim, 0);
				glUniform1iARB( g_location_useShell, 0);
			}

			for (i=0; i<currentmodel->num_triangles; i++)
			{
				for (j=0; j<3; j++)
				{
					index_xyz = index_st = currentmodel->tris[i].vertex[j];

					VArray[0] = currentmodel->animatevertexes[index_xyz].position[0] + currentmodel->animatenormal[index_xyz].dir[0]*shellscale;
					VArray[1] = currentmodel->animatevertexes[index_xyz].position[1] + currentmodel->animatenormal[index_xyz].dir[1]*shellscale;
					VArray[2] = currentmodel->animatevertexes[index_xyz].position[2] + currentmodel->animatenormal[index_xyz].dir[2]*shellscale;

					VArray[3] = (currentmodel->animatevertexes[index_xyz].position[1] + currentmodel->animatevertexes[index_xyz].position[0]) * (1.0f/40.f);
					VArray[4] = currentmodel->animatevertexes[index_xyz].position[2] * (1.0f/40.f) - r_newrefdef.time * 0.25f;

					VArray[5] = shadelight[0];
					VArray[6] = shadelight[1];
					VArray[7] = shadelight[2];
					VArray[8] = 0.33;

					if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer) {
						VectorCopy(currentmodel->animatenormal[index_xyz].dir, NormalsArray[va]); //shader needs normal array
						Vector4Copy(currentmodel->animatetangent[index_xyz].dir, TangentsArray[va]);
						VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED]; // increment pointer and counter
					} else
						VArray += VertexSizes[VERT_COLOURED_TEXTURED]; // increment pointer and counter
					va++;
				}
			}

			if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL ) ) )
			{
				R_DrawVarrays(GL_TRIANGLES, 0, va, false);
			}
		}		
		else
		{			
			KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_NORMAL_POINTER);
										
			glUniform1iARB(g_location_useGPUanim, 1);
			glUniform1iARB( g_location_useShell, 1);

			//send outframe, blendweights, and blendindexes to shader
			glUniformMatrix3x4fvARB( g_location_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );

			qglEnableClientState( GL_VERTEX_ARRAY );
			GL_BindVBO(vbo_xyz);
			qglVertexPointer(3, GL_FLOAT, 0, 0);

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GL_BindVBO(vbo_st);
			qglTexCoordPointer(2, GL_FLOAT, 0, 0);

			qglEnableClientState( GL_NORMAL_ARRAY );
			GL_BindVBO(vbo_normals);
			qglNormalPointer(GL_FLOAT, 0, 0);

			glEnableVertexAttribArrayARB (1);
			GL_BindVBO(vbo_tangents);
			glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
				
			GL_BindVBO(NULL);

			GL_BindIBO(vbo_indices);								

			glEnableVertexAttribArrayARB(6);
			glEnableVertexAttribArrayARB(7);
			glVertexAttribPointerARB(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char)*4, currentmodel->blendweights);
			glVertexAttribPointerARB(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char)*4, currentmodel->blendindexes); 				

			if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL )))
			{
				qglDrawElements(GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);	
			}

			GL_BindIBO(NULL);
	    }

		if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
		{
            glUseProgramObjectARB( 0 );
            GL_EnableMultitexture( false );
        }
	}	
	else if((mirror || glass ) && has_vbo)
	{
		//render glass with glsl shader
		vec3_t lightVec;
		
		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_TMU2_POINTER | KILL_NORMAL_POINTER);

		//get lighting
		R_GetLightVals(currententity->origin, false, true);

		R_ModelViewTransform(lightPosition, lightVec);		
				
		glUseProgramObjectARB( g_glassprogramObj );

		glUniform3fARB( g_location_gLightPos, lightVec[0], lightVec[1], lightVec[2]);
	
		GL_SelectTexture( GL_TEXTURE1);
		if(mirror)
		{			
			qglBindTexture (GL_TEXTURE_2D, r_mirrortexture->texnum);
		}
		else
		{
			qglBindTexture (GL_TEXTURE_2D, r_mirrorspec->texnum);			
		}
		glUniform1iARB( g_location_gmirTexture, 1);

		GL_SelectTexture( GL_TEXTURE0);
		qglBindTexture (GL_TEXTURE_2D, r_mirrorspec->texnum);
		glUniform1iARB( g_location_grefTexture, 0);		

		glUniform1iARB( g_location_gFog, map_fog);
										
		glUniform1iARB(g_location_useGPUanim, 1);

		glUniformMatrix3x4fvARB( g_location_gOutframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );

		qglEnableClientState( GL_VERTEX_ARRAY );
		GL_BindVBO(vbo_xyz);
		qglVertexPointer(3, GL_FLOAT, 0, 0);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		GL_BindVBO(vbo_st);
		qglTexCoordPointer(2, GL_FLOAT, 0, 0);

		qglEnableClientState( GL_NORMAL_ARRAY );
		GL_BindVBO(vbo_normals);
		qglNormalPointer(GL_FLOAT, 0, 0);
				
		GL_BindVBO(NULL);

		GL_BindIBO(vbo_indices);								

		glEnableVertexAttribArrayARB(6);
		glEnableVertexAttribArrayARB(7);
		glVertexAttribPointerARB(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char)*4, currentmodel->blendweights); //to do - these should also be vbo
		glVertexAttribPointerARB(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char)*4, currentmodel->blendindexes); //to do - these should also be vbo				

		if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL )))
		{
			qglDrawElements(GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);	
		}

		GL_BindIBO(NULL);

		glUseProgramObjectARB( 0 );
	}
	else if(mirror || glass)
	{
		//glass surfaces
		//base render, no vbo, no shaders

		if(mirror && !(currententity->flags & RF_WEAPONMODEL))
			R_InitVArrays(VERT_COLOURED_MULTI_TEXTURED);
		else
			R_InitVArrays (VERT_COLOURED_TEXTURED);

		qglDepthMask(false);

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
			qglBindTexture (GL_TEXTURE_2D, r_reflecttexture->texnum);
		}
		
		for (i=0; i<currentmodel->num_triangles; i++)
		{
			for (j=0; j<3; j++)
			{
				index_xyz = index_st = currentmodel->tris[i].vertex[j];

				VArray[0] = currentmodel->animatevertexes[index_xyz].position[0];
				VArray[1] = currentmodel->animatevertexes[index_xyz].position[1];
				VArray[2] = currentmodel->animatevertexes[index_xyz].position[2];

				if(mirror)
				{
					VArray[5] = VArray[3] = -(currentmodel->st[index_st].s - DotProduct (currentmodel->animatenormal[index_xyz].dir, vectors[1]));
					VArray[6] = VArray[4] = currentmodel->st[index_st].t + DotProduct (currentmodel->animatenormal[index_xyz].dir, vectors[2]);
				}
				else
				{
					VArray[3] = -(currentmodel->st[index_st].s - DotProduct (currentmodel->animatenormal[index_xyz].dir, vectors[1]));
					VArray[4] = currentmodel->st[index_st].t + DotProduct (currentmodel->animatenormal[index_xyz].dir, vectors[2]);
				}
				
				IQM_Vlight (shadelight, &currentmodel->animatenormal[index_xyz], currententity->angles, lightcolor);

				if(mirror && !(currententity->flags & RF_WEAPONMODEL) )
				{
					VArray[7] = lightcolor[0];
					VArray[8] = lightcolor[1];
					VArray[9] = lightcolor[2];
					VArray[10] = alpha;
					VArray += VertexSizes[VERT_COLOURED_MULTI_TEXTURED]; // increment pointer and counter
				}
				else
				{
					VArray[5] = lightcolor[0];
					VArray[6] = lightcolor[1];
					VArray[7] = lightcolor[2];
					VArray[8] = alpha;
					VArray += VertexSizes[VERT_COLOURED_TEXTURED]; // increment pointer and counter
				}

				va++;
			}
		}

		if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL ) ) )
		{
			R_DrawVarrays(GL_TRIANGLES, 0, va, false);
		}

		if(mirror && !(currententity->flags & RF_WEAPONMODEL))
			GL_EnableMultitexture( false );

		qglDepthMask(true);
	}
	else if(rs && rs->stage->normalmap && gl_normalmaps->integer && gl_glsl_shaders->integer && gl_state.glsl_shaders)
	{	
		//render with shaders - assume correct single pass glsl shader struct(let's put some checks in for this)
		vec3_t lightVec, lightVal;

		GLSTATE_ENABLE_ALPHATEST		

		R_GetLightVals(currententity->origin, false, true);

		//send light level and color to shader, ramp up a bit
		VectorCopy(lightcolor, lightVal);
		for(i = 0; i < 3; i++)
		{
			if(lightVal[i] < shadelight[i]/2)
				lightVal[i] = shadelight[i]/2; //never go completely black
			lightVal[i] *= 5;
			lightVal[i] += dynFactor;
			if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
			{
				if(lightVal[i] > 1.5)
					lightVal[i] = 1.5;
			}
			else
			{
				if(lightVal[i] > 1.0+dynFactor)
					lightVal[i] = 1.0+dynFactor;
			}
		}
		
		if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) //menu model
		{
			//fixed light source pointing down, slightly forward and to the left
			lightPosition[0] = -25.0;
			lightPosition[1] = 300.0;
			lightPosition[2] = 400.0;
			VectorMA(lightPosition, 5.0, lightVec, lightPosition);
			R_ModelViewTransform(lightPosition, lightVec);

			for (i = 0; i < 3; i++ )
			{
				lightVal[i] = 1.1;
			}
		}
		else
		{
			//simple directional(relative light position)
			VectorSubtract(lightPosition, currententity->origin, lightVec);
			VectorMA(lightPosition, 5.0, lightVec, lightPosition);
			R_ModelViewTransform(lightPosition, lightVec);

			//brighten things slightly
			for (i = 0; i < 3; i++ )
			{
				lightVal[i] *= 1.05;
			}
		}

		GL_EnableMultitexture( true );

		glUseProgramObjectARB( g_meshprogramObj );

		glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);
		glUniform3fARB( g_location_color, lightVal[0], lightVal[1], lightVal[2]);
		//if using shadowmaps, offset self shadowed areas a bit so not to get too dark
		if(gl_shadowmaps->integer && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
			glUniform1fARB( g_location_minLight, 0.20);
		else
			glUniform1fARB( g_location_minLight, 0.15);

		GL_SelectTexture( GL_TEXTURE1);
		qglBindTexture (GL_TEXTURE_2D, skinnum);
		glUniform1iARB( g_location_baseTex, 1);

		GL_SelectTexture( GL_TEXTURE0);
		qglBindTexture (GL_TEXTURE_2D, rs->stage->texture->texnum);
		glUniform1iARB( g_location_normTex, 0);

		GL_SelectTexture( GL_TEXTURE2);
		qglBindTexture (GL_TEXTURE_2D, rs->stage->texture2->texnum);
		glUniform1iARB( g_location_fxTex, 2);

		GL_SelectTexture( GL_TEXTURE0);

		if(rs->stage->fx)
			glUniform1iARB( g_location_useFX, 1);
		else
			glUniform1iARB( g_location_useFX, 0);

		if(rs->stage->glow)
			glUniform1iARB( g_location_useGlow, 1);
		else
			glUniform1iARB( g_location_useGlow, 0);

		glUniform1iARB( g_location_useScatter, 1);

		glUniform1iARB( g_location_useShell, 0);				

		glUniform1fARB( g_location_meshTime, rs_realtime);

		glUniform1iARB( g_location_meshFog, map_fog);
				
		if(!has_vbo)
		{
			glUniform1iARB(g_location_useGPUanim, 0);

			R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
			qglNormalPointer(GL_FLOAT, 0, NormalsArray);
			glEnableVertexAttribArrayARB (1);
			glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, 0, TangentsArray);

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

                    VectorCopy(currentmodel->animatenormal[index_xyz].dir, NormalsArray[va]);
                    Vector4Copy(currentmodel->animatetangent[index_xyz].dir, TangentsArray[va]);

                    VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED]; // increment pointer and counter                   
                    va++;
                }
            }
			
			if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL ) ) )
			{
				R_DrawVarrays(GL_TRIANGLES, 0, va, false);
			}
		}					
		else
		{			
			KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_TMU2_POINTER | KILL_NORMAL_POINTER);
										
			glUniform1iARB(g_location_useGPUanim, 1);

			glUniformMatrix3x4fvARB( g_location_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );

			qglEnableClientState( GL_VERTEX_ARRAY );
			GL_BindVBO(vbo_xyz);
			qglVertexPointer(3, GL_FLOAT, 0, 0);

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GL_BindVBO(vbo_st);
			qglTexCoordPointer(2, GL_FLOAT, 0, 0);

			qglEnableClientState( GL_NORMAL_ARRAY );
			GL_BindVBO(vbo_normals);
			qglNormalPointer(GL_FLOAT, 0, 0);

			glEnableVertexAttribArrayARB (1);
			GL_BindVBO(vbo_tangents);
			glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
				
			GL_BindVBO(NULL);

			GL_BindIBO(vbo_indices);								

			glEnableVertexAttribArrayARB(6);
			glEnableVertexAttribArrayARB(7);
			glVertexAttribPointerARB(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char)*4, currentmodel->blendweights); //to do - these should also be vbo
			glVertexAttribPointerARB(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char)*4, currentmodel->blendindexes); //to do - these should also be vbo				

			if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL )))
			{
				qglDrawElements(GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);	
			}

			GL_BindIBO(NULL);
	    }

		qglColor4f(1,1,1,1);

		glUseProgramObjectARB( 0 );
		GL_EnableMultitexture( false );
	}
	else
	{	
		//base render no shaders

		R_InitVArrays (VERT_COLOURED_TEXTURED);

		GL_SelectTexture( GL_TEXTURE0);
		qglBindTexture (GL_TEXTURE_2D, skinnum);

 		R_GetLightVals(currententity->origin, false, false);
		
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
				
				IQM_Vlight (shadelight, &currentmodel->animatenormal[index_xyz], currententity->angles, lightcolor);

				VArray[5] = lightcolor[0];
				VArray[6] = lightcolor[1];
				VArray[7] = lightcolor[2];
				VArray[8] = alpha;
				VArray += VertexSizes[VERT_COLOURED_TEXTURED]; // increment pointer and counter				
				va++;
			}
		}

		if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL ) ) )
		{
			R_DrawVarrays(GL_TRIANGLES, 0, va, false);
		}		
	}

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN
	
	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(6);
	glDisableVertexAttribArrayARB(7);

	R_KillVArrays ();

	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
		qglEnable( GL_TEXTURE_2D );
}

//Similar to above, but geared more specifically toward ragdoll player models
void IQM_DrawRagDollFrame(int RagDollID, int skinnum, float shellAlpha, int shellEffect)
{
	int		i, j;
	vec3_t	vectors[3];
	rscript_t *rs = NULL;
	vec3_t	lightcolor;
	int		index_xyz, index_st;
	int		va = 0;

	if (r_shaders->integer && RagDoll[RagDollID].script)
		rs = RagDoll[RagDollID].script;

	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	AngleVectors (RagDoll[RagDollID].angles, vectors[0], vectors[1], vectors[2]);

	if(shellEffect)
	{
		//shell render
		float shellscale = 1.6;

		qglEnable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer) 
		{
            vec3_t lightVec, lightVal;

            R_GetLightVals(RagDoll[RagDollID].curPos, true, true);

            //send light level and color to shader, ramp up a bit
            VectorCopy(lightcolor, lightVal);
            for(i = 0; i < 3; i++) 
			{
                if(lightVal[i] < shadelight[i]/2)
                    lightVal[i] = shadelight[i]/2; //never go completely black
                lightVal[i] *= 5;
                lightVal[i] += dynFactor;
                if(lightVal[i] > 1.0+dynFactor)
                    lightVal[i] = 1.0+dynFactor;
            }

            //simple directional(relative light position)
            VectorSubtract(lightPosition, RagDoll[RagDollID].curPos, lightVec);
            VectorMA(lightPosition, 1.0, lightVec, lightPosition);
            R_ModelViewTransform(lightPosition, lightVec);

            //brighten things slightly
            for (i = 0; i < 3; i++ )
                lightVal[i] *= 1.25;

            GL_EnableMultitexture( true );

            glUseProgramObjectARB( g_meshprogramObj );

            glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);

            GL_SelectTexture( GL_TEXTURE1);
            qglBindTexture (GL_TEXTURE_2D, r_shelltexture2->texnum);
            glUniform1iARB( g_location_baseTex, 1);

            GL_SelectTexture( GL_TEXTURE0);
            qglBindTexture (GL_TEXTURE_2D, r_shellnormal->texnum);
            glUniform1iARB( g_location_normTex, 0);

            GL_SelectTexture( GL_TEXTURE0);

            glUniform1iARB( g_location_useFX, 0);

            glUniform1iARB( g_location_useGlow, 0);

			glUniform1iARB( g_location_useScatter, 0);

            glUniform3fARB( g_location_color, lightVal[0], lightVal[1], lightVal[2]);

            glUniform1fARB( g_location_meshTime, rs_realtime);

            glUniform1iARB( g_location_meshFog, map_fog);
        }
		else
		{
			GL_Bind(r_shelltexture2->texnum);
			R_InitVArrays (VERT_COLOURED_TEXTURED);
		}

		if(!has_vbo)
		{
			if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
			{
				R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
				qglNormalPointer(GL_FLOAT, 0, NormalsArray);
				glEnableVertexAttribArrayARB (1);
				glVertexAttribPointerARB(1, 4, GL_FLOAT,GL_FALSE, 0, TangentsArray);

				glUniform1iARB(g_location_useGPUanim, 0);
				glUniform1iARB( g_location_useShell, 1);
			}

			for (i=0; i<RagDoll[RagDollID].ragDollMesh->num_triangles; i++)
			{
				for (j=0; j<3; j++)
				{
					index_xyz = index_st = RagDoll[RagDollID].ragDollMesh->tris[i].vertex[j];

					VArray[0] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[0] +
						RagDoll[RagDollID].ragDollMesh->animatenormal[index_xyz].dir[0]*shellscale;
					VArray[1] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[1] +
						RagDoll[RagDollID].ragDollMesh->animatenormal[index_xyz].dir[1]*shellscale;
					VArray[2] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[2] +
						RagDoll[RagDollID].ragDollMesh->animatenormal[index_xyz].dir[2]*shellscale;

					VArray[3] = (RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[1] +
						RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[0]) * (1.0f/40.f);
					VArray[4] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[2] * (1.0f/40.f) - r_newrefdef.time * 0.25f;

					VArray[5] = shadelight[0];
					VArray[6] = shadelight[1];
					VArray[7] = shadelight[2];
					VArray[8] = shellAlpha; //decrease over time

					if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
					{
						VectorCopy(RagDoll[RagDollID].ragDollMesh->animatenormal[index_xyz].dir, NormalsArray[va]); //shader needs normal array
						Vector4Copy(RagDoll[RagDollID].ragDollMesh->animatetangent[index_xyz].dir, TangentsArray[va]);
						VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED]; // increment pointer and counter
					}
					else
						VArray += VertexSizes[VERT_COLOURED_TEXTURED]; // increment pointer and counter
					va++;
				}
			}

			R_DrawVarrays(GL_TRIANGLES, 0, va, false);
		}
		else
		{			
			KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_NORMAL_POINTER);
										
			glUniform1iARB(g_location_useGPUanim, 1);
			glUniform1iARB( g_location_useShell, 1);

			//send outframe, blendweights, and blendindexes to shader
			glUniformMatrix3x4fvARB( g_location_outframe, RagDoll[RagDollID].ragDollMesh->num_joints, GL_FALSE, (const GLfloat *) RagDoll[RagDollID].ragDollMesh->outframe );

			qglEnableClientState( GL_VERTEX_ARRAY );
			GL_BindVBO(vbo_xyz);
			qglVertexPointer(3, GL_FLOAT, 0, 0);

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GL_BindVBO(vbo_st);
			qglTexCoordPointer(2, GL_FLOAT, 0, 0);

			qglEnableClientState( GL_NORMAL_ARRAY );
			GL_BindVBO(vbo_normals);
			qglNormalPointer(GL_FLOAT, 0, 0);

			glEnableVertexAttribArrayARB (1);
			GL_BindVBO(vbo_tangents);
			glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
				
			GL_BindVBO(NULL);

			GL_BindIBO(vbo_indices);								

			glEnableVertexAttribArrayARB(6);
			glEnableVertexAttribArrayARB(7);
			glVertexAttribPointerARB(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char)*4, RagDoll[RagDollID].ragDollMesh->blendweights);
			glVertexAttribPointerARB(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char)*4, RagDoll[RagDollID].ragDollMesh->blendindexes); 				

			qglDrawElements(GL_TRIANGLES, RagDoll[RagDollID].ragDollMesh->num_triangles*3, GL_UNSIGNED_INT, 0);	
			
			GL_BindIBO(NULL);
	    }

		if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
		{
            glUseProgramObjectARB( 0 );
            GL_EnableMultitexture( false );
        }

	}	
	else if(rs && rs->stage->normalmap && gl_normalmaps->integer && gl_glsl_shaders->integer && gl_state.glsl_shaders)
	{	
		//render with shaders
		vec3_t lightVec, lightVal;

		R_GetLightVals(RagDoll[RagDollID].curPos, true, true);

		//send light level and color to shader, ramp up a bit
		VectorCopy(lightcolor, lightVal);
		for(i = 0; i < 3; i++)
		{
			if(lightVal[i] < shadelight[i]/2)
				lightVal[i] = shadelight[i]/2; //never go completely black
			lightVal[i] *= 5;
			lightVal[i] += dynFactor;
			if(lightVal[i] > 1.0+dynFactor)
				lightVal[i] = 1.0+dynFactor;
		}

		//simple directional(relative light position)
		VectorSubtract(lightPosition, RagDoll[RagDollID].curPos, lightVec);
		VectorMA(lightPosition, 5.0, lightVec, lightPosition);
		R_ModelViewTransform(lightPosition, lightVec);

		//brighten things slightly
		for (i = 0; i < 3; i++ )
		{
			lightVal[i] *= 1.05;
		}

		GL_EnableMultitexture( true );

		glUseProgramObjectARB( g_meshprogramObj );

		glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);

		GL_SelectTexture( GL_TEXTURE1);
		qglBindTexture (GL_TEXTURE_2D, skinnum);
		glUniform1iARB( g_location_baseTex, 1);

		GL_SelectTexture( GL_TEXTURE0);
		qglBindTexture (GL_TEXTURE_2D, rs->stage->texture->texnum);
		glUniform1iARB( g_location_normTex, 0);

		GL_SelectTexture( GL_TEXTURE2);
		qglBindTexture (GL_TEXTURE_2D, rs->stage->texture2->texnum);
		glUniform1iARB( g_location_fxTex, 2);

		GL_SelectTexture( GL_TEXTURE0);

		if(rs->stage->fx)
			glUniform1iARB( g_location_useFX, 1);
		else
			glUniform1iARB( g_location_useFX, 0);

		if(rs->stage->glow)
			glUniform1iARB( g_location_useGlow, 1);
		else
			glUniform1iARB( g_location_useGlow, 0);

		glUniform1iARB( g_location_useScatter, 1);

		glUniform1iARB( g_location_useShell, 0);

		glUniform3fARB( g_location_color, lightVal[0], lightVal[1], lightVal[2]);

		//if using shadowmaps, offset self shadowed areas a bit so not to get too dark
		if(gl_shadowmaps->integer)
			glUniform1fARB( g_location_minLight, 0.20);
		else
			glUniform1fARB( g_location_minLight, 0.15);

		glUniform1fARB( g_location_meshTime, rs_realtime);

		glUniform1iARB( g_location_meshFog, map_fog);
		
		if(!has_vbo)
		{				
			R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
			qglNormalPointer(GL_FLOAT, 0, NormalsArray);
			glEnableVertexAttribArrayARB (1);
			glVertexAttribPointerARB(1, 4, GL_FLOAT,GL_FALSE, 0, TangentsArray);

			glUniform1iARB(g_location_useGPUanim, 0);
				
			for (i=0; i<RagDoll[RagDollID].ragDollMesh->num_triangles; i++)
			{
				for (j=0; j<3; j++)
				{
					index_xyz = index_st = RagDoll[RagDollID].ragDollMesh->tris[i].vertex[j];

					VArray[0] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[0];
					VArray[1] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[1];
					VArray[2] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[2];

					VArray[3] = RagDoll[RagDollID].ragDollMesh->st[index_st].s;
					VArray[4] = RagDoll[RagDollID].ragDollMesh->st[index_st].t;

					//send normals and tangents to shader
					VectorCopy(RagDoll[RagDollID].ragDollMesh->animatenormal[index_xyz].dir, NormalsArray[va]);
					Vector4Copy(RagDoll[RagDollID].ragDollMesh->animatetangent[index_xyz].dir, TangentsArray[va]);

					VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED]; // increment pointer and counter					
					va++;
				}
			}
			R_DrawVarrays(GL_TRIANGLES, 0, va, false);
		}			
		else
		{				
			KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_TMU2_POINTER | KILL_NORMAL_POINTER);
										
			glUniform1iARB(g_location_useGPUanim, 1);

			//send outframe, blendweights, and blendindexes to shader
			glUniformMatrix3x4fvARB( g_location_outframe, RagDoll[RagDollID].ragDollMesh->num_joints, GL_FALSE, (const GLfloat *) RagDoll[RagDollID].ragDollMesh->outframe );
		
			qglEnableClientState( GL_VERTEX_ARRAY );
			GL_BindVBO(vbo_xyz);
			qglVertexPointer(3, GL_FLOAT, 0, 0);

			qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GL_BindVBO(vbo_st);
			qglTexCoordPointer(2, GL_FLOAT, 0, 0);

			qglEnableClientState( GL_NORMAL_ARRAY );
			GL_BindVBO(vbo_normals);
			qglNormalPointer(GL_FLOAT, 0, 0);
				
			glEnableVertexAttribArrayARB (1);
			GL_BindVBO(vbo_tangents);
			glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
				
			GL_BindVBO(NULL);

			GL_BindIBO(vbo_indices);								

			glEnableVertexAttribArrayARB(6);
			glEnableVertexAttribArrayARB(7);
			glVertexAttribPointerARB(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(unsigned char)*4, RagDoll[RagDollID].ragDollMesh->blendweights);
			glVertexAttribPointerARB(7, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(unsigned char)*4, RagDoll[RagDollID].ragDollMesh->blendindexes); 				

			qglDrawElements(GL_TRIANGLES, RagDoll[RagDollID].ragDollMesh->num_triangles*3, GL_UNSIGNED_INT, 0);	
				
			GL_BindIBO(NULL);
		}			

		glUseProgramObjectARB( 0 );
		GL_EnableMultitexture( false );
	}
	else
	{	
		//base render no shaders

		R_InitVArrays (VERT_COLOURED_TEXTURED);

		GL_SelectTexture( GL_TEXTURE0);
		qglBindTexture (GL_TEXTURE_2D, skinnum);

		R_GetLightVals(RagDoll[RagDollID].curPos, true, false);
		
		for (i=0; i<RagDoll[RagDollID].ragDollMesh->num_triangles; i++)
		{
			for (j=0; j<3; j++)
			{
				index_xyz = index_st = RagDoll[RagDollID].ragDollMesh->tris[i].vertex[j];

				VArray[0] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[0];
				VArray[1] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[1];
				VArray[2] = RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position[2];

				VArray[3] = RagDoll[RagDollID].ragDollMesh->st[index_st].s;
				VArray[4] = RagDoll[RagDollID].ragDollMesh->st[index_st].t;
				
				IQM_Vlight (shadelight, &RagDoll[RagDollID].ragDollMesh->animatenormal[index_xyz], RagDoll[RagDollID].angles, lightcolor);

				VArray[5] = lightcolor[0];
				VArray[6] = lightcolor[1];
				VArray[7] = lightcolor[2];
				VArray[8] = 1.0;

				// increment pointer and counter
				VArray += VertexSizes[VERT_COLOURED_TEXTURED];
				va++;
			}
		}

		R_DrawVarrays(GL_TRIANGLES, 0, va, false);
	}

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN

	glDisableVertexAttribArrayARB(1);
	glDisableVertexAttribArrayARB(6);
	glDisableVertexAttribArrayARB(7);

	R_KillVArrays ();

	if ( shellEffect )
		qglEnable( GL_TEXTURE_2D );
}

void IQM_DrawShadow(vec3_t origin)
{
	vec_t	*position;
	float	height, lheight;
	int		i;
	int		index_vert;
	int 	vertsize;

	lheight = origin[2] - lightspot[2];

	height = -lheight + 0.1f;

	// if above entity's origin, skip
	if ((origin[2]+height) > origin[2])
		return;

	if (r_newrefdef.vieworg[2] < (origin[2] + height))
		return;

	if (have_stencil)
	{
		qglDepthMask(0);
		qglEnable(GL_STENCIL_TEST);
		qglStencilFunc(GL_EQUAL,1,2);
		qglStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
	}
	
	R_InitVArrays (VERT_SINGLE_TEXTURED);
	vertsize = VertexSizes[VERT_SINGLE_TEXTURED];

	#define SHADOW_VARRAY_VERTEX(trinum,vnum) \
		index_vert = currentmodel->tris[trinum].vertex[vnum];\
        position = currentmodel->animatevertexes[index_vert].position;\
        VArray[0] = position[0]-shadevector[0]*(position[2]+lheight);\
        VArray[1] = position[1]-shadevector[1]*(position[2]+lheight);\
        VArray[2] = height;\
		VArray[3] = currentmodel->st[index_vert].s;\
        VArray[4] = currentmodel->st[index_vert].t;\
		VArray += vertsize;
	for (i=0; i<currentmodel->num_triangles; i++)
    {
        SHADOW_VARRAY_VERTEX(i,0);
        SHADOW_VARRAY_VERTEX(i,1);
        SHADOW_VARRAY_VERTEX(i,2);
	}

	R_DrawVarrays(GL_TRIANGLES, 0, currentmodel->num_triangles*3, false);
		
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_KillVArrays ();
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (have_stencil)
		qglDisable(GL_STENCIL_TEST);
}

static qboolean IQM_CullModel( void )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	trace_t r_trace;
	vec3_t	dist;
	vec3_t bbox[8];

	if (r_worldmodel )
	{
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
/*
=================
R_DrawINTERQUAKEMODEL
=================
*/

void R_DrawINTERQUAKEMODEL ( void )
{
	int			i;
	image_t		*skin;
	float		frame, time;

	if((r_newrefdef.rdflags & RDF_NOWORLDMODEL ) && !(currententity->flags & RF_MENUMODEL))
		return;

	//do culling
	if ( IQM_CullModel() )
		return;

	if(r_ragdolls->integer)
	{
		//Ragdolls take over at beginning of each death sequence
		if(!(currententity->flags & RF_TRANSLUCENT))
		{
			if(currententity->frame == 199 || currententity->frame == 220 || currententity->frame == 238)
				if(currentmodel->hasRagDoll)
					RGD_AddNewRagdoll(currententity->origin, currententity->name);
		}
		//Do not render deathframes if using ragdolls - do not render translucent helmets
		if((currentmodel->hasRagDoll || (currententity->flags & RF_TRANSLUCENT)) && currententity->frame > 198)
			return;
	}

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

		if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
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
		if(gl_glsl_shaders->integer && gl_state.glsl_shaders && gl_normalmaps->integer)
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

	//modelpitch = 0.52 * sinf(rs_realtime); //use this for testing only
	modelpitch = degreeToRadian(currententity->angles[PITCH]);

    qglPushMatrix ();
	currententity->angles[PITCH] = currententity->angles[ROLL] = 0;
	R_RotateForEntity (currententity);

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

	if((currententity->frame == currententity->oldframe ) && !IQM_InAnimGroup(currententity->frame, currententity->oldframe))
		time = 0;

	//Check for stopped death anims
	if(currententity->frame == 257 || currententity->frame == 237 || currententity->frame == 219)
		time = 0;

	frame = currententity->frame + time;

	IQM_AnimateFrame(frame, IQM_NextFrame(currententity->frame));

	if(!(currententity->flags & RF_VIEWERMODEL))
		IQM_DrawFrame(skin->texnum);

	GL_TexEnv( GL_REPLACE );
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglDisable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//Basic stencil shadows
	if	(	!(r_newrefdef.rdflags & RDF_NOWORLDMODEL) && fadeShadow >= 0.01 && 
		gl_shadows->integer && !gl_shadowmaps->integer && !r_gpuanim->integer &&
			!(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS))
		)
	{
		float casted;
		float an = currententity->angles[1]/180*M_PI;
		shadevector[0] = cos(-an);
		shadevector[1] = sin(-an);
		shadevector[2] = 1;
		VectorNormalize (shadevector);

		switch (gl_shadows->integer)
		{
		case 0:
			break;
		case 1: //dynamic only - always cast something
			casted = SHD_ShadowLight (currententity->origin, currententity->angles, shadevector, 0);
			qglPushMatrix ();
			qglTranslatef	(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
			qglRotatef (currententity->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,0.3 * fadeShadow * currententity->alpha); //Knightmare- variable alpha
			else
				qglColor4f (0,0,0,0.3 * fadeShadow);

			IQM_DrawShadow (currententity->origin);

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();

			break;
		case 2: //dynamic and world
			//world
			casted = SHD_ShadowLight (currententity->origin, currententity->angles, shadevector, 1);
			qglPushMatrix ();
			qglTranslatef	(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
			qglRotatef (currententity->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,casted * fadeShadow * currententity->alpha);
			else
				qglColor4f (0,0,0,casted * fadeShadow);

			IQM_DrawShadow (currententity->origin);

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();
			//dynamic
			casted = 0;
		 	casted = SHD_ShadowLight (currententity->origin, currententity->angles, shadevector, 0);
			if (casted > 0)
			{ //only draw if there's a dynamic light there
				qglPushMatrix ();
				qglTranslatef	(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
				qglRotatef (currententity->angles[1], 0, 0, 1);
				qglDisable (GL_TEXTURE_2D);
				qglEnable (GL_BLEND);

				if (currententity->flags & RF_TRANSLUCENT)
					qglColor4f (0,0,0,casted * fadeShadow * currententity->alpha);
				else
					qglColor4f (0,0,0,casted * fadeShadow);

				IQM_DrawShadow (currententity->origin);

				qglEnable (GL_TEXTURE_2D);
				qglDisable (GL_BLEND);
				qglPopMatrix ();
			}

			break;
		}
	}
	qglColor4f (1,1,1,1);

	if(r_minimap->integer)
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

void IQM_DrawCasterFrame ()
{
	int     i, j;
    int     index_xyz, index_st;
    int     va = 0;
	
	if(!has_vbo)
	{
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

		R_DrawVarrays(GL_TRIANGLES, 0, va, false);
	}
	else 
	{			
		//no need to load up a new vbo - this has already been done by the full mesh render!

		//to do - just use a very basic shader for this instead of the normal mesh shader
		glUseProgramObjectARB( g_meshprogramObj );
		    
        glUniform1iARB( g_location_useFX, 0);

        glUniform1iARB( g_location_useGlow, 0);

		glUniform1iARB( g_location_useScatter, 0);
		        
        glUniform1iARB( g_location_meshFog, 0);	

		glUniform1iARB(g_location_useGPUanim, 1);

		//send outframe, blendweights, and blendindexes to shader
		glUniformMatrix3x4fvARB( g_location_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );

		qglEnableClientState( GL_VERTEX_ARRAY );
		GL_BindVBO(vbo_xyz);
		qglVertexPointer(3, GL_FLOAT, 0, 0);

		GL_BindVBO(NULL);

		GL_BindIBO(vbo_indices);								

		qglDrawElements(GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);	
	
		GL_BindIBO(NULL);

		glUseProgramObjectARB( 0 );
	}

	R_KillVArrays ();
}

void IQM_DrawCaster ( void )
{
	float		frame, time;

	if(currententity->team) //don't draw flag models, handled by sprites
		return;

	if ( currententity->flags & RF_WEAPONMODEL ) //don't draw weapon model shadow casters
		return;

	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE) ) //no shells
		return;

	if ( IQM_CullModel() )
		return;

	//modelpitch = 0.52 * sinf(rs_realtime); //use this for testing only
	modelpitch = degreeToRadian(currententity->angles[PITCH]);

    qglPushMatrix ();
	currententity->angles[PITCH] = currententity->angles[ROLL] = 0;
	R_RotateForEntity (currententity);

	//frame interpolation
	time = (Sys_Milliseconds() - currententity->frametime) / 100;
	if(time > 1.0)
		time = 1.0;

	if((currententity->frame == currententity->oldframe ) && !IQM_InAnimGroup(currententity->frame, currententity->oldframe))
		time = 0;

	//Check for stopped death anims
	if(currententity->frame == 257 || currententity->frame == 237 || currententity->frame == 219)
		time = 0;

	frame = currententity->frame + time;

	IQM_AnimateFrame(frame, IQM_NextFrame(currententity->frame));

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
