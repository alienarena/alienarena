#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

#include "binheap.h"
#include "libgarland.h"

float raw_sample (byte *texture, int tex_w, int tex_h, int s, int t)
{
	if (s >= tex_w)
		s = tex_w-1;
	if (s < 0)
		s = 0;
	if (t >= tex_h)
		t = tex_h-1;
	if (t < 0)
		t = 0;
	
	return 1.0/3.0 *	(	texture[(t*tex_w+s)*4] +
							texture[(t*tex_w+s)*4+1] +
							texture[(t*tex_w+s)*4+2]
						);
}

float bilinear_sample(byte *texture, int tex_w, int tex_h, float u, float v)
{
 	int x, y;
 	float u_ratio, v_ratio, u_opposite, v_opposite;
	u = u * tex_w - 0.5;
	v = v * tex_h - 0.5;
	x = floor(u);
	y = floor(v);
	u_ratio = u - x;
	v_ratio = v - y;
	u_opposite = 1 - u_ratio;
	v_opposite = 1 - v_ratio;
#define SAMPLE(x,y) raw_sample (texture, tex_w, tex_h, x, y)
	return	(SAMPLE (x, y) * u_opposite + SAMPLE (x+1, y) * u_ratio) * v_opposite +
			(SAMPLE (x, y+1) * u_opposite + SAMPLE (x+1, y+1) * u_ratio) * v_ratio;	
#undef SAMPLE
}

void LoadTerrainFile (terraindata_t *out, const char *name, float oversampling_factor, int reduction_amt, char *buf)
{
	int i, j, va, w, h, vtx_w, vtx_h;
	char	*hmtex_path = NULL;
	mesh_t	mesh;
	vec3_t	scale;
	char	*token;
	byte	*texdata;
	int		start_time;
	
	out->texture_path = NULL;
	
	buf = strtok (buf, ";");
	while (buf)
	{
		token = COM_Parse (&buf);
		if (!buf && !(buf = strtok (NULL, ";")))
			break;

		if (!Q_strcasecmp (token, "heightmap"))
		{
			hmtex_path = CopyString (COM_Parse (&buf));
			if (!buf)
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting heightmap filename! (File %s is invalid)", name);
		}
		if (!Q_strcasecmp (token, "texture"))
		{
			out->texture_path = CopyString (COM_Parse (&buf));
			if (!buf)
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting texture filename! (File %s is invalid)", name);
		}
		if (!Q_strcasecmp (token, "mins"))
		{
			for (i = 0; i < 3; i++)
			{
				out->mins[i] = atof (COM_Parse (&buf));
				if (!buf)
					Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting mins %c axis! (File %s is invalid)", "xyz"[i], name);
			}
		}
		if (!Q_strcasecmp (token, "maxs"))
		{
			for (i = 0; i < 3; i++)
			{
				out->maxs[i] = atof (COM_Parse (&buf));
				if (!buf)
					Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting maxs %c axis! (File %s is invalid)", "xyz"[i], name);
			}
		}
		
		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		buf = strtok (NULL, ";");
	}
	
	if (!hmtex_path)
		Com_Error (ERR_DROP, "LoadTerrainFile: Missing heightmap texture in %s!", name);
	
	if (!out->texture_path)
		Com_Error (ERR_DROP, "LoadTerrainFile: Missing heightmap texture in %s!", name);
	
	VectorSubtract (out->maxs, out->mins, scale);
	
	LoadTGA (hmtex_path, &texdata, &w, &h);
	
	if (texdata == NULL)
		Com_Error (ERR_DROP, "LoadTerrainFile: Can't find file %s\n", hmtex_path);
	
	Z_Free (hmtex_path);
	
	vtx_w = oversampling_factor*w+1;
	vtx_h = oversampling_factor*h+1;
	
	out->num_vertices = vtx_w*vtx_h;
	out->num_triangles = 2*(vtx_w-1)*(vtx_h-1);
	
	out->vert_texcoords = Z_Malloc (out->num_vertices*sizeof(vec2_t));
	out->vert_positions = Z_Malloc (out->num_vertices*sizeof(vec3_t));
	out->tri_indices = Z_Malloc (out->num_triangles*3*sizeof(unsigned int));
	
	va = 0;
	for (i = 0; i < vtx_h-1; i++)
	{
		for (j = 0; j < vtx_w-1; j++)
		{
			if ((i+j)%2 == 1)
			{
				out->tri_indices[va++] = (i+1)*vtx_w+j;
				out->tri_indices[va++] = i*vtx_w+j+1;
				out->tri_indices[va++] = i*vtx_w+j;
				out->tri_indices[va++] = i*vtx_w+j+1;
				out->tri_indices[va++] = (i+1)*vtx_w+j;
				out->tri_indices[va++] = (i+1)*vtx_w+j+1;
			}
			else
			{
				out->tri_indices[va++] = (i+1)*vtx_w+j+1;
				out->tri_indices[va++] = i*vtx_w+j+1;
				out->tri_indices[va++] = i*vtx_w+j;
				out->tri_indices[va++] = i*vtx_w+j;
				out->tri_indices[va++] = (i+1)*vtx_w+j;
				out->tri_indices[va++] = (i+1)*vtx_w+j+1;
			}
		}
	}
	
	assert (va == out->num_triangles*3);
	
	for (i = 0; i < vtx_h; i++)
	{
		for (j = 0; j < vtx_w; j++)
		{
			float x, y, s, t;
			
			x = scale[0]*((float)j/(float)vtx_w) + out->mins[0];
			y = scale[1]*((float)i/(float)vtx_h) + out->mins[1];
			s = (float)j/(float)vtx_w;
			t = 1.0 - (float)i/(float)vtx_h;
			
			VectorSet (&out->vert_positions[(i*vtx_w+j)*3], x, y, scale[2] * bilinear_sample (texdata, h, w, s, t) / 255.0 + out->mins[2]);
			out->vert_texcoords[(i*vtx_w+j)*2] = s;
			out->vert_texcoords[(i*vtx_w+j)*2+1] = t;
		}
	}
	
	free (texdata);
	
	mesh.num_verts = out->num_vertices;
	mesh.num_tris = out->num_triangles;
	mesh.vcoords = out->vert_positions;
	mesh.vtexcoords = out->vert_texcoords;
	mesh.tris = out->tri_indices;
	
	start_time = Sys_Milliseconds ();
	simplify_mesh (&mesh, out->num_triangles/reduction_amt);
	Com_Printf ("Simplified mesh in %f seconds.\n", (float)(Sys_Milliseconds () - start_time)/1000.0f);
	
	out->num_triangles = mesh.num_verts;
	out->num_triangles = mesh.num_tris;
}
