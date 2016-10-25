#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

#include "binheap.h"
#include "libgarland.h"

static float grayscale_sample (const byte *texture, int tex_w, int tex_h, float u, float v, float *out_alpha)
{
    vec4_t res;
    
    bilinear_sample (texture, tex_w, tex_h, u, v, res);
    
    if (out_alpha != NULL)
    	*out_alpha = res[3];
    
    return (res[0] + res[1] + res[2]) / 3.0;
}

static terraindec_t *LoadTerrainDecorationType 
	(	char *texpath, const vec3_t mins, const vec3_t scale, float gridsz,
		const byte *hmtexdata, float hm_w, float hm_h,
		const int channeltypes[3], int *out_counter
	)
{
	byte *texdata_orig = NULL;
	byte *texdata_resampled = NULL;
	terraindec_t *ret = NULL;
	int i, j, k, w, h, w_orig, h_orig;
	int counter = 0;
	
	LoadTGA (texpath, &texdata_orig, &w_orig, &h_orig);
	
	if (texdata_orig == NULL)
		Com_Error (ERR_DROP, "LoadTerrainFile: Can't find file %s\n", texpath);
	
	Z_Free (texpath);
	
	// resample decorations texture to new resolution
	// this is to make decoration density independent of decoration texture
	w = ceilf (scale[0] / gridsz);
	h = ceilf (scale[1] / gridsz);
	texdata_resampled = Z_Malloc (w*h*3);
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			float s, t;
			vec4_t res;
			
			s = ((float)j)/(float)w;
			t = ((float)i)/(float)h;
			bilinear_sample (texdata_orig, w_orig, h_orig, s, t, res);
			
			for (k = 0; k < 3; k++)
				texdata_resampled[3*(i*w+j)+k] = floorf (res[k] * 255.0f);
		}
	}
	
	free (texdata_orig);
	
	// Count how many decorations we should allocate.
	for (j = 0; j < 3; j++)
	{
		if (channeltypes[j] == -1)
			continue;
		for (i = 0; i < w*h; i++)
		{
			if (texdata_resampled[i*3+j] != 0)
				counter++;
		}
	}
	
	*out_counter = counter;
	
	if (counter == 0)
	{
		Z_Free (texdata_resampled);
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
			
				size = texdata_resampled[((h-i-1)*w+j)*3+k];
				if (size == 0)
					continue;
		
				xrand = 2.0*(frand()-0.5);
				yrand = 2.0*(frand()-0.5);
		
				s = ((float)j+xrand)/(float)w;
				t = 1.0 - ((float)i+yrand)/(float)h;
		
				x = scale[0]*((float)j/(float)w) + mins[0] + scale[0]*xrand/(float)w;
				y = scale[1]*((float)i/(float)h) + mins[1] + scale[1]*yrand/(float)h;
				z = scale[2] * grayscale_sample (hmtexdata, hm_h, hm_w, s, t, NULL) + mins[2];
		
				VectorSet (ret[counter].origin, x, y, z);
				ret[counter].size = (float)size/16.0f;
				ret[counter].type = channeltypes[k];
		
				counter++;
			}
		}
	}
	
	assert (counter == *out_counter);
	
	Z_Free (texdata_resampled);
	
	return ret;
}

// TODO: separate function for decorations, currently this is kind of hacky.
#ifdef MESH_SIMPLIFICATION_VISUALIZER
static mesh_t mesh;
#endif
void LoadTerrainFile (terraindata_t *out, const char *name, qboolean decorations_only, float oversampling_factor, int reduction_amt, char *buf)
{
	int i, j, va, w, h, vtx_w, vtx_h;
	char	*vegtex_path = NULL, *rocktex_path = NULL;
	byte	*alphamask; // Bitmask of "holes" in terrain from alpha channel
#ifndef MESH_SIMPLIFICATION_VISUALIZER
	mesh_t	mesh;
#endif
	vec3_t	scale;
	const char *line;
	char	*token;
	byte	*texdata;
	int		start_time;
	
	memset (out, 0, sizeof(*out));
	
	line = strtok (buf, ";");
	while (line)
	{
		token = COM_Parse (&line);
		if (!line && !(line = strtok (NULL, ";")))
			break;

#define FILENAME_ATTR(cmd_name,out) \
		if (!Q_strcasecmp (token, cmd_name)) \
		{ \
			out = CopyString (COM_Parse (&line)); \
			if (!line) \
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting " cmd_name " filename! (File %s is invalid)", name); \
		} 
		
		FILENAME_ATTR ("heightmap", out->hmtex_path)
		FILENAME_ATTR ("texture", out->texture_path)
		FILENAME_ATTR ("lightmap", out->lightmap_path)
		FILENAME_ATTR ("vegetation", vegtex_path)
		FILENAME_ATTR ("rocks", rocktex_path);
		
#undef FILENAME_ATTR
		
		if (!Q_strcasecmp (token, "mins"))
		{
			for (i = 0; i < 3; i++)
			{
				out->mins[i] = atof (COM_Parse (&line));
				if (!line)
					Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting mins %c axis! (File %s is invalid)", "xyz"[i], name);
			}
		}
		if (!Q_strcasecmp (token, "maxs"))
		{
			for (i = 0; i < 3; i++)
			{
				out->maxs[i] = atof (COM_Parse (&line));
				if (!line)
					Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting maxs %c axis! (File %s is invalid)", "xyz"[i], name);
			}
		}
		
		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		line = strtok (NULL, ";");
	}
	
	if (!out->hmtex_path)
		Com_Error (ERR_DROP, "LoadTerrainFile: Missing heightmap texture in %s!", name);
	
	if (!out->texture_path)
		Com_Error (ERR_DROP, "LoadTerrainFile: Missing heightmap texture in %s!", name);
	
	VectorSubtract (out->maxs, out->mins, scale);
	
	LoadTGA (out->hmtex_path, &texdata, &w, &h);
	
	if (texdata == NULL)
		Com_Error (ERR_DROP, "LoadTerrainFile: Can't find file %s\n", out->hmtex_path);
	
	// Compile a list of all vegetation sprites that should be added to the
	// map.
	if (vegtex_path != NULL)
	{
		// Green pixels in the vegetation map indicate grass.
		// Red pixels indicate shrubbery.
		const int channeltypes[3] = {2, 0, 1};
		out->vegetation = LoadTerrainDecorationType (vegtex_path, out->mins, scale, 64.0f, texdata, h, w, channeltypes, &out->num_vegetation);
	}
	
	// Compile a list of all rock entities that should be added to the map
	if (rocktex_path != NULL)
	{
		// Red pixels in the rock map indicate smallish rocks.
		const int channeltypes[3] = {0, -1, -1};
		out->rocks = LoadTerrainDecorationType (rocktex_path, out->mins, scale, 64.0f, texdata, h, w, channeltypes, &out->num_rocks);
	}
	
	if (decorations_only)
	{
		free (texdata);
		return;
	}
	
	vtx_w = oversampling_factor*w+1;
	vtx_h = oversampling_factor*h+1;
	
	alphamask = Z_Malloc (vtx_w * vtx_h / 8 + 1);
	
	out->num_vertices = vtx_w*vtx_h;
	out->num_triangles = 2*(vtx_w-1)*(vtx_h-1);
	
	out->vert_texcoords = Z_Malloc (out->num_vertices*sizeof(vec2_t));
	out->vert_positions = Z_Malloc (out->num_vertices*sizeof(vec3_t));
	out->tri_indices = Z_Malloc (out->num_triangles*3*sizeof(unsigned int));
	
	for (i = 0; i < vtx_h; i++)
	{
		for (j = 0; j < vtx_w; j++)
		{
			float x, y, z, s, t, alpha;
			
			s = (float)j/(float)vtx_w;
			t = 1.0 - (float)i/(float)vtx_h;
			
			x = scale[0]*((float)j/(float)vtx_w) + out->mins[0];
			y = scale[1]*((float)i/(float)vtx_h) + out->mins[1];
			z = scale[2] * grayscale_sample (texdata, h, w, s, t, &alpha) + out->mins[2];
			
			// If the alpha of the heightmap texture is less than 1.0, the 
			// terrain has a "hole" in it and any tris that would have
			// included this vertex should be skipped.
			alphamask[(i*vtx_w+j)/8] |= (alpha == 1.0) << ((i*vtx_w+j)%8);
			
			VectorSet (&out->vert_positions[(i*vtx_w+j)*3], x, y, z);
			out->vert_texcoords[(i*vtx_w+j)*2] = s;
			out->vert_texcoords[(i*vtx_w+j)*2+1] = t;
		}
	}
	
	va = 0;
	for (i = 0; i < vtx_h-1; i++)
	{
		for (j = 0; j < vtx_w-1; j++)
		{
// Yeah, this was actually the least embarrassing way to do it
#define IDX_NOT_HOLE(idx) ((alphamask[(idx) / 8] & (1 << ((idx) % 8))) != 0)
#define ADDTRIANGLE(idx1,idx2,idx3) \
			if (IDX_NOT_HOLE (idx1) && IDX_NOT_HOLE (idx2) && IDX_NOT_HOLE (idx3)) \
			{ \
				out->tri_indices[va++] = idx1; \
				out->tri_indices[va++] = idx2; \
				out->tri_indices[va++] = idx3; \
			}
			
			if ((i+j)%2 == 1)
			{
				ADDTRIANGLE ((i+1)*vtx_w+j, i*vtx_w+j+1, i*vtx_w+j);
				ADDTRIANGLE (i*vtx_w+j+1, (i+1)*vtx_w+j, (i+1)*vtx_w+j+1);
			}
			else
			{
				ADDTRIANGLE ((i+1)*vtx_w+j+1, i*vtx_w+j+1, i*vtx_w+j);
				ADDTRIANGLE (i*vtx_w+j, (i+1)*vtx_w+j, (i+1)*vtx_w+j+1);
			}
		}
	}
	
	assert (va % 3 == 0 && va / 3 <= out->num_triangles);
	
	free (texdata);
	
	Z_Free (alphamask);
	
	mesh.num_verts = out->num_vertices;
	mesh.num_tris = va / 3;
	mesh.vcoords = out->vert_positions;
	mesh.vtexcoords = out->vert_texcoords;
	mesh.tris = out->tri_indices;
	
#ifndef MESH_SIMPLIFICATION_VISUALIZER
	start_time = Sys_Milliseconds ();
	simplify_mesh (&mesh, out->num_triangles/reduction_amt);
	Com_Printf ("Simplified mesh in %f seconds.\n", (float)(Sys_Milliseconds () - start_time)/1000.0f);
	Com_Printf ("%d to %d\n", out->num_triangles, mesh.num_tris);
	
	out->num_vertices = mesh.num_verts;
	out->num_triangles = mesh.num_tris;
#else
    simplify_init (&mesh);
#endif
}

#ifdef MESH_SIMPLIFICATION_VISUALIZER
void visualizer_step (terraindata_t *out, qboolean export)
{
    simplify_step (&mesh, out->num_triangles - 1);
    if (export)
        export_mesh (&mesh);
    out->num_vertices = mesh.num_verts;
	out->num_triangles = mesh.simplified_num_tris;
}
#endif

void CleanupTerrainData (terraindata_t *dat)
{
#define CLEANUPFIELD(field) \
	if (dat->field != NULL) \
	{ \
		Z_Free (dat->field); \
		dat->field = NULL; \
	}
	
	CLEANUPFIELD (hmtex_path)
	CLEANUPFIELD (texture_path)
	CLEANUPFIELD (lightmap_path)
	CLEANUPFIELD (vert_positions)
	CLEANUPFIELD (vert_texcoords)
	CLEANUPFIELD (tri_indices)
	CLEANUPFIELD (vegetation)
	CLEANUPFIELD (rocks)
	
#undef CLEANUPFIELD
}
