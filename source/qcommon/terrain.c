#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

#include "binheap.h"
#include "libgarland.h"

static float grayscale_sample (const byte *texture, int tex_w, int tex_h, float u, float v)
{
    vec3_t res;
    
    bilinear_sample (texture, tex_w, tex_h, u, v, res);
    
    return (res[0] + res[1] + res[2]) / 3.0;
}

terraindec_t *LoadTerrainDecorationType 
	(	char *texpath, const vec3_t mins, const vec3_t scale, 
		const byte *hmtexdata, float hm_w, float hm_h,
		const int channeltypes[3], int *out_counter
	)
{
	byte *texdata = NULL;
	terraindec_t *ret = NULL;
	int i, j, k, w, h;
	int counter = 0;
	
	LoadTGA (texpath, &texdata, &w, &h);
	
	if (texdata == NULL)
		Com_Error (ERR_DROP, "LoadTerrainFile: Can't find file %s\n", texpath);
	
	Z_Free (texpath);
	
	// Count how many decorations we should allocate.
	for (j = 0; j < 3; j++)
	{
		if (channeltypes[j] == -1)
			continue;
		for (i = 0; i < w*h; i++)
		{
			if (texdata[i*4+j] != 0)
				counter++;
		}
	}
	
	*out_counter = counter;
	
	if (counter == 0)
	{
		free (texdata);
		return NULL;
	}
	
	ret = Z_Malloc (counter * sizeof(terraindec_t));
	
	counter = 0;
	
	// Fill in the decorations
	for (k = 0; k < 3; k++)
	{
		if (channeltypes[k] == -1)
			continue;
		
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < w; j++)
			{
				float x, y, z, s, t, xrand, yrand;
				byte size;
			
				size = texdata[((h-i-1)*w+j)*4+k];
				if (size == 0)
					continue;
		
				xrand = 2.0*(frand()-0.5);
				yrand = 2.0*(frand()-0.5);
		
				s = ((float)j+xrand)/(float)w;
				t = 1.0 - ((float)i+yrand)/(float)h;
		
				x = scale[0]*((float)j/(float)w) + mins[0] + scale[0]*xrand/(float)w;
				y = scale[1]*((float)i/(float)h) + mins[1] + scale[1]*yrand/(float)h;
				z = scale[2] * grayscale_sample (hmtexdata, hm_h, hm_w, s, t) + mins[2];
		
				VectorSet (ret[counter].origin, x, y, z);
				ret[counter].size = (float)size/16.0f;
				ret[counter].type = channeltypes[k];
		
				counter++;
			}
		}
	}
	
	assert (counter == *out_counter);
	
	free (texdata);
	
	return ret;
}

// TODO: separate function for decorations, currently this is kind of hacky.
void LoadTerrainFile (terraindata_t *out, const char *name, qboolean decorations_only, float oversampling_factor, int reduction_amt, char *buf)
{
	int i, j, va, w, h, vtx_w, vtx_h;
	char	*hmtex_path = NULL, *vegtex_path = NULL, *rocktex_path = NULL;
	mesh_t	mesh;
	vec3_t	scale;
	char	*token;
	byte	*texdata;
	int		start_time;
	
	memset (out, 0, sizeof(*out));
	
	buf = strtok (buf, ";");
	while (buf)
	{
		token = COM_Parse (&buf);
		if (!buf && !(buf = strtok (NULL, ";")))
			break;

#define FILENAME_ATTR(cmd_name,out) \
		if (!Q_strcasecmp (token, cmd_name)) \
		{ \
			out = CopyString (COM_Parse (&buf)); \
			if (!buf) \
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting " cmd_name " filename! (File %s is invalid)", name); \
		} 
		
		FILENAME_ATTR ("heightmap", hmtex_path)
		FILENAME_ATTR ("texture", out->texture_path)
		FILENAME_ATTR ("lightmap", out->lightmap_path)
		FILENAME_ATTR ("vegetation", vegtex_path)
		FILENAME_ATTR ("rocks", rocktex_path);
		
#undef FILENAME_ATTR
		
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
	
	// Compile a list of all vegetation sprites that should be added to the
	// map.
	if (vegtex_path != NULL)
	{
		// Green pixels in the vegetation map indicate grass.
		// Red pixels indicate shrubbery.
		const int channeltypes[3] = {2, 0, -1};
		out->vegetation = LoadTerrainDecorationType (vegtex_path, out->mins, scale, texdata, h, w, channeltypes, &out->num_vegetation);
	}
	
	// Compile a list of all rock entities that should be added to the map
	if (rocktex_path != NULL)
	{
		// Red pixels in the rock map indicate smallish rocks.
		const int channeltypes[3] = {0, -1, -1};
		out->rocks = LoadTerrainDecorationType (rocktex_path, out->mins, scale, texdata, h, w, channeltypes, &out->num_rocks);
	}
	
	if (decorations_only)
	{
		free (texdata);
		return;
	}
	
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
			float x, y, z, s, t;
			
			s = (float)j/(float)vtx_w;
			t = 1.0 - (float)i/(float)vtx_h;
			
			x = scale[0]*((float)j/(float)vtx_w) + out->mins[0];
			y = scale[1]*((float)i/(float)vtx_h) + out->mins[1];
			z = scale[2] * grayscale_sample (texdata, h, w, s, t) + out->mins[2];
			
			VectorSet (&out->vert_positions[(i*vtx_w+j)*3], x, y, z);
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

void CleanupTerrainData (terraindata_t *dat)
{
#define CLEANUPFIELD(field) \
	if (dat->field != NULL) \
	{ \
		Z_Free (dat->field); \
		dat->field = NULL; \
	}
	
	CLEANUPFIELD (texture_path)
	CLEANUPFIELD (lightmap_path)
	CLEANUPFIELD (vert_positions)
	CLEANUPFIELD (vert_texcoords)
	CLEANUPFIELD (tri_indices)
	CLEANUPFIELD (vegetation)
	CLEANUPFIELD (rocks)
	
#undef CLEANUPFIELD
}
