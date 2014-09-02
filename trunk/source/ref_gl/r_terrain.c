#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

void Terrain_LoadVBO (model_t *mod, float *vposition, float *vnormal, float *vtangent, float *vtexcoord, unsigned int *vtriangles)
{
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec2_t), vtexcoord, VBO_STORE_ST, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vposition, VBO_STORE_XYZ, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vnormal, VBO_STORE_NORMAL, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec4_t), vtangent, VBO_STORE_TANGENT, mod);
	R_VCLoadData(VBO_STATIC, mod->num_triangles*3*sizeof(unsigned int), vtriangles, VBO_STORE_INDICES, mod);
}

extern void MD2_VecsForTris(
		const float *v0,
		const float *v1,
		const float *v2,
		const float *st0,
		const float *st1,
		const float *st2,
		vec3_t Tangent
		);

void Mod_LoadTerrainModel (model_t *mod, void *_buf)
{
	int i;
	float *vnormal, *vtangent;
	image_t	*tex = NULL;
	terraindata_t data;
	int ndownward;
	
	LoadTerrainFile (&data, mod->name, false, 2.0, 32, (char *)_buf);
	
	mod->numvertexes = data.num_vertices;
	mod->num_triangles = data.num_triangles;
	
	tex = GL_FindImage (data.texture_path, it_wall);
	
	if (!tex)
		Com_Error (ERR_DROP, "Mod_LoadTerrainModel: Missing surface texture in %s!", mod->name);
	
	if (data.lightmap_path != NULL)
		mod->lightmap = GL_FindImage (data.lightmap_path, it_lightmap);
	
	VectorCopy (data.mins, mod->mins);
	VectorCopy (data.maxs, mod->maxs);
	
	mod->skins[0] = tex;
	mod->type = mod_terrain;
	
	if (mod->skins[0] != NULL)
		mod->script = mod->skins[0]->script;
	if (mod->script)
		RS_ReadyScript( mod->script );
	
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
	
	vnormal = Z_Malloc (mod->numvertexes*sizeof(vec3_t));
	vtangent = Z_Malloc (mod->numvertexes*sizeof(vec4_t));
	
	ndownward = 0;
	
	// Calculate normals and tangents
	for (i = 0; i < mod->num_triangles; i++)
	{
		int j;
		vec3_t v1, v2, normal, tangent;
		unsigned int *triangle = &data.tri_indices[3*i];
		
		// leave normal scaled proportional to triangle area
		VectorSubtract (&data.vert_positions[3*triangle[0]], &data.vert_positions[3*triangle[1]], v1);
		VectorSubtract (&data.vert_positions[3*triangle[2]], &data.vert_positions[3*triangle[1]], v2);
		CrossProduct (v2, v1, normal);
		VectorScale (normal, -1.0f, normal);
		
		if (normal[2] < 0.0f)
			ndownward++;
		
		MD2_VecsForTris (	&data.vert_positions[3*triangle[0]], 
							&data.vert_positions[3*triangle[1]], 
							&data.vert_positions[3*triangle[2]],
							&data.vert_texcoords[2*triangle[0]], 
							&data.vert_texcoords[2*triangle[1]], 
							&data.vert_texcoords[2*triangle[2]],
							tangent );
		
		for (j = 0; j < 3; j++)
		{
			VectorAdd (&vnormal[3*triangle[j]], normal, &vnormal[3*triangle[j]]);
			VectorAdd (&vtangent[4*triangle[j]], tangent, &vtangent[4*triangle[j]]);
		}
	}
	
	if (ndownward > 0)
		Com_Printf ("WARN: %d downward facing polygons in %s!\n", ndownward, mod->name);
	
	// Normalize the average normals and tangents
	for (i = 0; i < mod->numvertexes; i++)
	{
		VectorNormalize (&vnormal[3*i]);
		VectorNormalize (&vtangent[4*i]);
		vtangent[4*i+3] = 1.0;
	}
	
	Terrain_LoadVBO (mod, data.vert_positions, vnormal, vtangent, data.vert_texcoords, data.tri_indices);
	
	CleanupTerrainData (&data);
}

void Mod_LoadTerrainDecorations (char *path, vec3_t angles, vec3_t origin)
{
	char *buf;
	int len;
	int i;
	terraindata_t data;
	trace_t plant;
	vec3_t start, end;
	vec3_t color = {1, 1, 1};
	
	if (Q_strcasecmp (COM_FileExtension (path), "terrain"))
		return; // not a .terrain mesh
	
	len = FS_LoadFile (path, (void**)&buf);
	
	if (!buf)
	{
		Com_Printf ("WARN: Could not find %s\n", path);
		return;
	}
	
	LoadTerrainFile (&data, path, true, 0, 0, buf);
	
	FS_FreeFile ((void *)buf);
	
	// TODO: transformed terrain model support!
	for (i = 0; i < data.num_vegetation; i++)
	{
		vec3_t org;
		char *texture;
		static char *weedtextures[] = 
		{
			"gfx/weed1.tga",
			"gfx/weed2.tga",
			"gfx/weed3.tga",
			"gfx/weed4.tga"
		};
		static char *bushtextures[] = 
		{
			"gfx/bush1.tga",
			"gfx/bush2.tga"
		};
		int type;
		float size;
		
		VectorAdd (origin, data.vegetation[i].origin, org);
		size = data.vegetation[i].size;
		
		if (size < 3.0)
			continue;
		
		switch (data.vegetation[i].type)
		{
		default:
		case 0:
			texture = "gfx/grass.tga";
			type = 0;
			break;
		case 1:
			texture = weedtextures[rand()%4];
			size /= 2.0;
			type = 0;
			break;
		case 2:
			texture = bushtextures[rand()%2];
			size *= 1.5;
			type = 2;
			break;
		}
		
		// Figure out where to "plant" the plant.
		VectorCopy (org, start);
		start[2] += 256;
		VectorCopy (org, end);
		end[2] -= 256;
		plant = CM_BoxTrace (start, end, vec3_origin, vec3_origin, 0, MASK_ALL);
		
		Mod_AddVegetation (	org, plant.plane.normal,
							GL_FindImage (texture, it_wall), color,
							data.vegetation[i].size, texture, type );
	}
	
	for (i = 0; i < data.num_rocks; i++)
	{
		entity_t *ent;
		static char *rockmeshes[] =
		{
			"maps/meshes/rocks/rock1.md2",
			"maps/meshes/rocks/rock2.md2",
			"maps/meshes/rocks/rock3.md2",
			"maps/meshes/rocks/rock4.md2",
			"maps/meshes/rocks/rock5.md2"
		};
		
		if (num_rock_entities == MAX_ROCKS)
			break;
	
		ent = &rock_entities[num_rock_entities];
		memset (ent, 0, sizeof(*ent));
		ent->number = MAX_EDICTS+MAX_MAP_MODELS+num_rock_entities++;
		ent->model = R_RegisterModel (rockmeshes[rand()%5]);
		
		VectorAdd (origin, data.rocks[i].origin, ent->origin);
		
		// Figure out where to "plant" the rock.
		VectorCopy (ent->origin, start);
		start[2] += 256;
		VectorCopy (ent->origin, end);
		end[2] -= 256;
		plant = CM_BoxTrace (start, end, ent->model->mins, ent->model->maxs, 0, MASK_ALL);
		
		vectoangles (plant.plane.normal, ent->angles);
		// Entities are oriented such that their angles point forward, not up
		ent->angles[PITCH] += 90.0; 
		
/*		ent->angles[YAW] = 360.0*frand();*/
	}
	
	CleanupTerrainData (&data);
}
