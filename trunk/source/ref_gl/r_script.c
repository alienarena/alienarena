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

// gl_script.c - scripted texture rendering

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

#if defined WIN32_VARIANT
#include <io.h>
#endif

extern float	r_turbsin[];

#define		TOK_DELIMINATORS "\r\n\t "

float		rs_realtime = 0;
rscript_t	*rs_rootscript = NULL;

int RS_Animate (rs_stage_t *stage)
{
	anim_stage_t	*anim = stage->last_anim;

/* unused
	float			time = rs_realtime * 1000 -
		(stage->last_anim_time + stage->anim_delay);
*/

	while (stage->last_anim_time < rs_realtime)
	{
		anim = anim->next;
		if (!anim)
			anim = stage->anim_stage;
		stage->last_anim_time += stage->anim_delay;
	}

	stage->last_anim = anim;

	return anim->texture->texnum;
}
void *RS_AnimateSkin (rs_stage_t *stage)
{
	anim_stage_t	*anim = stage->last_anim;

/* unused
	float			time = rs_realtime * 1000 -
		(stage->last_anim_time + stage->anim_delay);
*/

	while (stage->last_anim_time < rs_realtime)
	{
		anim = anim->next;
		if (!anim)
			anim = stage->anim_stage;
		stage->last_anim_time += stage->anim_delay;
	}

	stage->last_anim = anim;

	return anim->texture;
}

float cutDot (vec3_t vec1, vec3_t vec2)
{
	float dot = DotProduct(vec1, vec2);
	if (dot>1) return 1;
	if (dot<-1) return -1;
	return dot;
}

void RS_ResetScript (rscript_t *rs)
{
	rs_stage_t		*stage = rs->stage, *tmp_stage;
	anim_stage_t	*anim, *tmp_anim;
	random_stage_t	*randStage, *tmp_rand;

	rs->name[0] = 0;

	while (stage != NULL)
	{
		if (stage->anim_count)
		{
			anim = stage->anim_stage;
			while (anim != NULL)
			{
				tmp_anim = anim;
				
				anim = anim->next;
				free (tmp_anim);
			}
		}
		if (stage->rand_count)
		{
			randStage = stage->rand_stage;
			while (randStage != NULL)
			{
				tmp_rand = randStage;

				randStage = randStage->next;
				free (tmp_rand);
			}
		}

		tmp_stage = stage;
		stage = stage->next;

		free (tmp_stage);
	}
	
	rs->stage = NULL;
	rs->dontflush = false;	
	rs->ready = false;
}

void rs_free_if_subexpr (rs_cond_val_t *expr)
{
	if (expr->lval.string)
		Z_Free (expr->lval.string);
	if (expr->subexpr1)
		rs_free_if_subexpr (expr->subexpr1);
	if (expr->subexpr2)
		rs_free_if_subexpr (expr->subexpr2);
	Z_Free (expr);
}

void RS_ClearStage (rs_stage_t *stage)
{
	anim_stage_t	*anim = stage->anim_stage, *tmp_anim;
	random_stage_t	*randStage = stage->rand_stage, *tmp_rand;


	if (stage->condv != NULL)
	{
		rs_free_if_subexpr (stage->condv);
		stage->condv = NULL;
	}
	while (anim != NULL)
	{
		tmp_anim = anim;
		anim = anim->next;
		free (tmp_anim);
	}
	while (randStage != NULL)
	{
		tmp_rand = randStage;
		randStage = randStage->next;
		free (tmp_rand);
	}

	stage->last_anim = 0;
	stage->last_anim_time = 0;
	stage->anim_count = 0;

	stage->anim_delay = 0;
	stage->anim_stage = NULL;

	stage->rand_count = 0;
	stage->rand_stage = NULL;

	stage->has_alpha = false;
	stage->alphamask = false;

	stage->alphafunc = 0;

	stage->alphashift.max = 0;
	stage->alphashift.min = 0;
	stage->alphashift.speed = 0;

	stage->blendfunc.blend = false;
	stage->blendfunc.dest = stage->blendfunc.source = 0;

	stage->colormap.enabled = false;
	stage->colormap.red = 0;
	stage->colormap.green = 0;
	stage->colormap.blue = 0;

	stage->scale.scaleX = 0;
	stage->scale.scaleY = 0;
	stage->scale.typeX = 0;
	stage->scale.typeY = 0;

	stage->scroll.speedX = 0;
	stage->scroll.speedY = 0;
	stage->scroll.typeX = 0;
	stage->scroll.typeY = 0;

	stage->frames.enabled = false;
	stage->frames.start = 0;
	stage->frames.end = 0;

	stage->rot_speed = 0;

	VectorClear(stage->origin);
	VectorClear(stage->angle);

	stage->texture = NULL;
	stage->texture2 = NULL;
	stage->texture3 = NULL;

	stage->depthhack = false;
	stage->envmap = false;
	stage->dynamic = false;
	stage->lensflare = false;
	stage->flaretype = 0;
	stage->normalmap = false;
	stage->grass = false;
	stage->grasstype = 0;
	stage->beam = false;
	stage->beamtype = 0;
	stage->xang = 0;
	stage->yang = 0;
	stage->rotating = false;
	stage->fx = false;
	stage->glow = false;
	stage->cube = false;

	stage->lightmap = true;

	stage->next = NULL;
}

rscript_t *RS_NewScript (char *name)
{
	rscript_t	*rs;
	unsigned int	i;

	if (!rs_rootscript)
	{
		rs_rootscript = (rscript_t *)malloc(sizeof(rscript_t));
		rs = rs_rootscript;
	}
	else
	{
		rs = rs_rootscript;

		while (rs->next != NULL)
			rs = rs->next;

		rs->next = (rscript_t *)malloc(sizeof(rscript_t));
		rs = rs->next;
	}

	COMPUTE_HASH_KEY(rs->hash_key, name, i);
	strncpy (rs->name, name, sizeof(rs->name));

	rs->stage = NULL;
	rs->next = NULL;
	rs->dontflush = false;	
	rs->ready = false;	

	return rs;
}

rs_stage_t *RS_NewStage (rscript_t *rs)
{
	rs_stage_t	*stage;

	if (rs->stage == NULL)
	{
		rs->stage = (rs_stage_t *)malloc(sizeof(rs_stage_t));
		stage = rs->stage;
	}
	else
	{
		stage = rs->stage;
		while (stage->next != NULL)
			stage = stage->next;

		stage->next = (rs_stage_t *)malloc(sizeof(rs_stage_t));
		stage = stage->next;
	}

	strncpy (stage->name, "***r_notexture***", sizeof(stage->name));
	strncpy (stage->name2, "***r_notexture***", sizeof(stage->name2));
	strncpy (stage->name3, "***r_notexture***", sizeof(stage->name3));

	stage->rand_stage = NULL;
	stage->anim_stage = NULL;
	stage->next = NULL;
	stage->last_anim = NULL;
	stage->condv = NULL;

	RS_ClearStage (stage);

	return stage;
}

void RS_FreeAllScripts (void)
{
	rscript_t	*rs = rs_rootscript, *tmp_rs;

	while (rs != NULL)
	{
		tmp_rs = rs->next;
		RS_ResetScript(rs);
		free (rs);
		rs = tmp_rs;
	}
}

void RS_ReloadImageScriptLinks (void)
{
	image_t		*image;
	int	i;

	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
		image->script = RS_FindScript(image->bare_name);

}

void RS_FreeScript(rscript_t *rs)
{
	rscript_t	*tmp_rs;

	if (!rs)
		return;

	if (rs_rootscript == rs)
	{
		rs_rootscript = rs_rootscript->next;
		RS_ResetScript(rs);
		free (rs);
		return;
	}

	tmp_rs = rs_rootscript;
	while (tmp_rs->next != rs)
		tmp_rs = tmp_rs->next;
	tmp_rs->next = rs->next;

	RS_ResetScript (rs);

	free (rs);
}

void RS_FreeUnmarked (void)
{
	rscript_t	*rs = rs_rootscript, *tmp_rs;

	while (rs != NULL)
	{
		tmp_rs = rs->next;

		if (!rs->dontflush)
			RS_FreeScript(rs);

		rs = tmp_rs;
	}
}

rscript_t *RS_FindScript(char *name)
{
	rscript_t	*rs = rs_rootscript;
	unsigned int	i, hash_key;

	COMPUTE_HASH_KEY(hash_key, name, i);

	while (rs != NULL)
	{
		if (rs->hash_key == hash_key && !Q_strcasecmp(rs->name, name))
		{
			if (! rs->stage)
				rs = NULL;
			break;
		}

		rs = rs->next;
	}

	return rs;
}

void RS_ReadyScript (rscript_t *rs)
{
	rs_stage_t		*stage;
	anim_stage_t	*anim;
	random_stage_t	*randStage;
	char			mode;

	if (rs->ready)
		return;

	mode = (rs->dontflush) ? it_pic : it_wall;
	stage = rs->stage;

	while (stage != NULL)
	{
		//if normalmap, set mode
		if(stage->normalmap)
			mode = it_bump;

		//set anim
		anim = stage->anim_stage;
		while (anim != NULL)
		{
			anim->texture = GL_FindImage (anim->name, mode);
			if (!anim->texture)
				anim->texture = r_notexture;

			anim = anim->next;
		}

		//set tiling
		randStage = stage->rand_stage;
		while (randStage != NULL)
		{
			randStage->texture = GL_FindImage (randStage->name, mode);
			if (!randStage->texture)
				randStage->texture = r_notexture;

			randStage = randStage->next;
		}

		//set name
		if (stage->name[0])
			stage->texture = GL_FindImage (stage->name, mode);
		if (!stage->texture)
			stage->texture = r_notexture;
		if (stage->name2[0])
			stage->texture2 = GL_FindImage (stage->name2, mode);
		if (!stage->texture2)
			stage->texture2 = r_notexture;
		if (stage->name3[0])
			stage->texture3 = GL_FindImage (stage->name3, mode);
		if (!stage->texture3)
			stage->texture3 = r_notexture;

		//check alpha
		if (stage->blendfunc.blend)
			stage->has_alpha = true;
		else
			stage->has_alpha = false;

		stage = stage->next;
	}

	rs->ready = true;
}

int RS_BlendID (char *blend)
{
	if (!blend[0])
		return 0;
	if (!Q_strcasecmp (blend, "GL_ZERO"))
		return GL_ZERO;
	if (!Q_strcasecmp (blend, "GL_ONE"))
		return GL_ONE;
	if (!Q_strcasecmp (blend, "GL_DST_COLOR"))
		return GL_DST_COLOR;
	if (!Q_strcasecmp (blend, "GL_ONE_MINUS_DST_COLOR"))
		return GL_ONE_MINUS_DST_COLOR;
	if (!Q_strcasecmp (blend, "GL_SRC_ALPHA"))
		return GL_SRC_ALPHA;
	if (!Q_strcasecmp (blend, "GL_ONE_MINUS_SRC_ALPHA"))
		return GL_ONE_MINUS_SRC_ALPHA;
	if (!Q_strcasecmp (blend, "GL_DST_ALPHA"))
		return GL_DST_ALPHA;
	if (!Q_strcasecmp (blend, "GL_ONE_MINUS_DST_ALPHA"))
		return GL_ONE_MINUS_DST_ALPHA;
	if (!Q_strcasecmp (blend, "GL_SRC_ALPHA_SATURATE"))
		return GL_SRC_ALPHA_SATURATE;
	if (!Q_strcasecmp (blend, "GL_SRC_COLOR"))
		return GL_SRC_COLOR;
	if (!Q_strcasecmp (blend, "GL_ONE_MINUS_SRC_COLOR"))
		return GL_ONE_MINUS_SRC_COLOR;

	return 0;
}

int RS_FuncName (char *text)
{
	if (!Q_strcasecmp (text, "static"))			// static
		return 0;
	else if (!Q_strcasecmp (text, "sine"))		// sine wave
		return 1;
	else if (!Q_strcasecmp (text, "cosine"))	// cosine wave
		return 2;

	return 0;
}

/*
scriptname
{
	subdivide <size>
	vertexwarp <speed> <distance> <smoothness>
	safe
	{
		map <texturename>
		map2 <texturename> - specific to "fx"
		map3 <texturename> - specific to "cube"
		scroll <xtype> <xspeed> <ytype> <yspeed>
		blendfunc <source> <dest>
		alphashift <speed> <min> <max>
		anim <delay> <tex1> <tex2> <tex3> ... end
		envmap
		nolightmap
		alphamask
		lensflare
		flaretype
		normalmap
		grass
		grasstype
		beam
		beamtype
		xang
		yang
		rotating
		fx
		glow
		cube
	}
}
*/

void rs_stage_map (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);

	strncpy (stage->name, *token, sizeof(stage->name));
}

void rs_stage_map2 (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);

	strncpy (stage->name2, *token, sizeof(stage->name2));
}

void rs_stage_map3 (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);

	strncpy (stage->name3, *token, sizeof(stage->name3));
}

void rs_stage_model (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);

	strncpy (stage->model, *token, sizeof(stage->model));
}

void rs_stage_colormap (rs_stage_t *stage, char **token)
{
	stage->colormap.enabled = true;

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->colormap.red = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->colormap.green = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->colormap.blue = atof(*token);
}

void rs_stage_frames (rs_stage_t *stage, char **token)
{
	stage->frames.enabled = true;

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->frames.speed = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->frames.start = atoi(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->frames.end = atoi(*token);
}

void rs_stage_scroll (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.speedX = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.speedY = atof(*token);
}

void rs_stage_blendfunc (rs_stage_t *stage, char **token)
{
	stage->blendfunc.blend = true;

	*token = strtok (NULL, TOK_DELIMINATORS);

	if (!Q_strcasecmp (*token, "add"))
	{
		stage->blendfunc.source = GL_ONE;
		stage->blendfunc.dest = GL_ONE;
	}
	else if (!Q_strcasecmp (*token, "blend"))
	{
		stage->blendfunc.source = GL_SRC_ALPHA;
		stage->blendfunc.dest = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_strcasecmp (*token, "filter"))
	{
		stage->blendfunc.source = GL_ZERO;
		stage->blendfunc.dest = GL_SRC_COLOR;
	}
	else
	{	
		stage->blendfunc.source = RS_BlendID (*token);

		*token = strtok (NULL, TOK_DELIMINATORS);
		stage->blendfunc.dest = RS_BlendID (*token);
	}
}

void rs_stage_alphashift (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.speed = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.min = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.max = (float)atof(*token);
}

void rs_stage_random (rs_stage_t *stage, char **token)
{
	random_stage_t	*rand = (random_stage_t *)malloc(sizeof(random_stage_t));

	stage->rand_stage = rand;
	stage->rand_count = 0;

	*token = strtok(NULL, TOK_DELIMINATORS);

	while (Q_strcasecmp (*token, "end"))
	{
		stage->rand_count++;

		strncpy (stage->name, *token, sizeof(stage->name));

		stage->texture = NULL;

		*token = strtok(NULL, TOK_DELIMINATORS);

		if (!Q_strcasecmp (*token, "end"))
		{
			rand->next = NULL;
			break;
		}

		rand->next = (random_stage_t *)malloc(sizeof(random_stage_t));
		rand = rand->next;
	}
}

void rs_stage_anim (rs_stage_t *stage, char **token)
{
	anim_stage_t	*anim = (anim_stage_t *)malloc(sizeof(anim_stage_t));

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->anim_delay = (float)atof(*token);

	stage->anim_stage = anim;
	stage->last_anim = anim;

	*token = strtok(NULL, TOK_DELIMINATORS);

	while (Q_strcasecmp (*token, "end"))
	{
		stage->anim_count++;

		strncpy (anim->name, *token, sizeof(anim->name));

		anim->texture = NULL;

		*token = strtok(NULL, TOK_DELIMINATORS);

		if (!Q_strcasecmp (*token, "end"))
		{
			anim->next = NULL;
			break;
		}

		anim->next = (anim_stage_t *)malloc(sizeof(anim_stage_t));
		anim = anim->next;
	}
}

void rs_stage_depthhack (rs_stage_t *stage, char **token)
{
	stage->depthhack = true;
}

void rs_stage_envmap (rs_stage_t *stage, char **token)
{
	stage->envmap = true;
}

void rs_stage_dynamic (rs_stage_t *stage, char **token)
{
	stage->dynamic = true;
}

void rs_stage_nolightmap (rs_stage_t *stage, char **token)
{
	stage->lightmap = false;
}

void rs_stage_alphamask (rs_stage_t *stage, char **token)
{
	stage->alphamask = true;
}

void rs_stage_rotate (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->rot_speed = (float)atof(*token);
}

void rs_stage_origin (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->origin[0] = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->origin[1] = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->origin[2] = (float)atof(*token);
}

void rs_stage_angle (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->angle[0] = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->angle[1] = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->angle[2] = (float)atof(*token);
}

void rs_stage_scale (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.scaleX = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.scaleY = atof(*token);
}

void rs_stage_alphafunc (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);

	if (!Q_strcasecmp (*token, "normal"))
		stage->alphafunc = ALPHAFUNC_NORMAL;
	else if (!Q_strcasecmp (*token, "-normal"))
		stage->alphafunc = -ALPHAFUNC_NORMAL;
	else if (!Q_strcasecmp (*token, "gloss"))
		stage->alphafunc = ALPHAFUNC_GLOSS;
	else if (!Q_strcasecmp (*token, "-gloss"))
		stage->alphafunc = -ALPHAFUNC_GLOSS;
	else if (!Q_strcasecmp (*token, "basic"))
		stage->alphafunc = ALPHAFUNC_BASIC;
	else if (!Q_strcasecmp (*token, "-basic"))
		stage->alphafunc = -ALPHAFUNC_BASIC;
}
void rs_stage_lensflare (rs_stage_t *stage, char **token)
{
	stage->lensflare = true;
}
void rs_stage_flaretype (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->flaretype = atoi(*token);
}
void rs_stage_normalmap (rs_stage_t *stage, char **token)
{
	stage->normalmap = true;
}
void rs_stage_grass (rs_stage_t *stage, char **token)
{
	stage->grass = true;
}
void rs_stage_grasstype (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->grasstype = atoi(*token);
}
void rs_stage_beam (rs_stage_t *stage, char **token)
{
	stage->beam = true;
}
void rs_stage_beamtype (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->beamtype = atoi(*token);
}
void rs_stage_xang (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->xang = atof(*token);
}
void rs_stage_yang (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->yang = atof(*token);
}
void rs_stage_rotating (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->rotating = atoi(*token);
}
void rs_stage_fx (rs_stage_t *stage, char **token)
{
	stage->fx = true;
}
void rs_stage_glow (rs_stage_t *stage, char **token)
{
	stage->glow = true;
}
void rs_stage_cube (rs_stage_t *stage, char **token)
{
	stage->cube = true;
}

typedef struct 
{
	char			*opname;
	rs_cond_op_t	op;
} rs_cond_op_key_t;
static rs_cond_op_key_t rs_cond_op_keys[] = 
{
	{	"==",	rs_cond_eq		},
	{	"!=",	rs_cond_neq		},
	{	">",	rs_cond_gt		},
	{	"<=",	rs_cond_ngt		},
	{	"<",	rs_cond_lt		},
	{	">=",	rs_cond_nlt		},
	{	"&&",	rs_cond_and		},
	{	"||",	rs_cond_or		},
	
	{	NULL,	0				}
};
rs_cond_val_t *rs_stage_if_subexpr (char **token)
{
	int i;
	rs_cond_val_t *res = Z_Malloc (sizeof(rs_cond_val_t));
	*token = strtok (NULL, TOK_DELIMINATORS);
	if (!(*token)) 
	{
		Z_Free (res);
		return NULL;
	}
	if (!Q_strcasecmp (*token, "(")) 
	{
		res->subexpr1 = rs_stage_if_subexpr (token);
		if (!res->subexpr1) //there was an error somewhere
		{
			Z_Free (res);
			return NULL;
		}
		*token = strtok (NULL, TOK_DELIMINATORS);
		if (!(*token)) {
			Com_Printf ("Ran out of tokens in stage conditional!\n");
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		if (!Q_strcasecmp (*token, ")"))
		{
			res->optype = rs_cond_is;
			return res;
		}
		for (i = 0; rs_cond_op_keys[i].opname; i++)
			if (!Q_strcasecmp (*token, rs_cond_op_keys[i].opname))
				break;
		if (!rs_cond_op_keys[i].opname) 
		{
			Com_Printf ("Invalid stage conditional operation %s\n", *token);
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		res->optype = rs_cond_op_keys[i].op;
		res->subexpr2 = rs_stage_if_subexpr (token);
		if (!res->subexpr2) //there was an error somewhere
		{
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		*token = strtok (NULL, TOK_DELIMINATORS);
		if (!(*token) || Q_strcasecmp (*token, ")")) 
		{
			Com_Printf ("Missing ) in stage conditional!\n");
			if (*token)
				Com_Printf ("Instead got %s\n", *token);
			rs_free_if_subexpr (res->subexpr1);
			rs_free_if_subexpr (res->subexpr2);
			Z_Free (res);
			return NULL;
		}
		return res;
	} 
	else if (!Q_strcasecmp (*token, "!"))
	{
		res->optype = rs_cond_lnot;
		res->subexpr1 = rs_stage_if_subexpr (token);
		if (!res->subexpr1) // there was an error somewhere
		{
			Z_Free (res);
			return NULL;
		}
		return res;
	}
	else if (!Q_strcasecmp (*token, "$"))
	{
		res->optype = rs_cond_none;
		*token = strtok (NULL, TOK_DELIMINATORS);
		if (!(*token)) {
			Com_Printf ("Missing cvar name in stage conditional!\n");
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		res->val = Cvar_Get (*token, "0", 0);
		return res;
	}
	else
	{
		res->optype = rs_cond_none;
		res->val = &(res->lval);
		Anon_Cvar_Set (res->val, *token);
		return res;
	}
}
void rs_stage_if (rs_stage_t *stage, char **token)
{
	stage->condv = rs_stage_if_subexpr (token);
	if (!stage->condv)
		Com_Printf ("ERROR in stage conditional!\n");
}

static rs_stagekey_t rs_stagekeys[] =
{
	{	"colormap",		&rs_stage_colormap		},
	{	"map",			&rs_stage_map			},
	{	"map2",			&rs_stage_map2			},
	{	"map3",			&rs_stage_map3			},
	{	"model",		&rs_stage_model			},
	{	"scroll",		&rs_stage_scroll		},
	{	"frames",		&rs_stage_frames		},
	{	"blendfunc",	&rs_stage_blendfunc		},
	{	"alphashift",	&rs_stage_alphashift	},
	{	"rand",			&rs_stage_random		},
	{	"anim",			&rs_stage_anim			},
	{	"envmap",		&rs_stage_envmap		},
	{	"depthhack",	&rs_stage_depthhack		},
	{	"nolightmap",	&rs_stage_nolightmap	},
	{	"alphamask",	&rs_stage_alphamask		},
	{	"rotate",		&rs_stage_rotate		},
	{	"origin",		&rs_stage_origin		},
	{	"angle",		&rs_stage_angle			},
	{	"scale",		&rs_stage_scale			},
	{	"dynamic",		&rs_stage_dynamic		},
	{	"alphafunc",	&rs_stage_alphafunc		},
	{	"lensflare",	&rs_stage_lensflare		},
	{	"flaretype",	&rs_stage_flaretype		},
	{   "normalmap",	&rs_stage_normalmap		},
	{	"grass",		&rs_stage_grass			},
	{	"grasstype",	&rs_stage_grasstype		},
	{	"beam",			&rs_stage_beam			},
	{	"beamtype",		&rs_stage_beamtype		},
	{	"xang",			&rs_stage_xang			},
	{	"yang",			&rs_stage_yang			},
	{	"rotating",		&rs_stage_rotating		},
	{	"fx",			&rs_stage_fx			},
	{	"glow",			&rs_stage_glow			},
	{	"cube",			&rs_stage_cube			},
	{	"if",			&rs_stage_if			},

	{	NULL,			NULL					}
};

static int num_stagekeys = sizeof (rs_stagekeys) / sizeof(rs_stagekeys[0]) - 1;

// =====================================================

void RS_LoadScript(char *script)
{
	qboolean		inscript = false, instage = false;
	char			ignored = 0;
	char			*token, *fbuf, *buf;
	rscript_t		*rs = NULL;
	rs_stage_t		*stage = NULL;
	// unsigned char	tcmod = 0; // unused
	unsigned int	len, i;

	len = FS_LoadFile (script, (void **)&fbuf);

	if (!fbuf || len < 16)
	{
	//	Con_Printf (PRINT_ALL, "Could not load script %s\n", script);
		return;
	}

	buf = (char *)malloc(len+1);
	memcpy (buf, fbuf, len);
	buf[len] = 0;

	FS_FreeFile (fbuf);

	token = strtok (buf, TOK_DELIMINATORS);

	while (token != NULL)
	{
		if (!Q_strcasecmp (token, "/*") || !Q_strcasecmp (token, "["))
			ignored++;
		else if (!Q_strcasecmp (token, "*/") || !Q_strcasecmp (token, "]"))
			ignored--;

		if (!Q_strcasecmp (token, "//"))
		{
			//IGNORE
		}
		else if (!inscript && !ignored)
		{
			if (!Q_strcasecmp (token, "{"))
			{
				inscript = true;
			}
			else
			{
				rs = RS_FindScript(token);

				if (rs)
					RS_FreeScript(rs);

				rs = RS_NewScript(token);
			}
		}
		else if (inscript && !ignored)
		{
			if (!Q_strcasecmp(token, "}"))
			{
				if (instage)
				{
					instage = false;
				}
				else
				{
					inscript = false;
				}
			}
			else if (!Q_strcasecmp(token, "{"))
			{
				if (!instage) {
					instage = true;
					stage = RS_NewStage(rs);
				}
			}
			else
			{
				if (instage && !ignored)
				{
					for (i = 0; i < num_stagekeys; i++) {
						if (!Q_strcasecmp (rs_stagekeys[i].stage, token))
						{
							rs_stagekeys[i].func (stage, &token);
							break;
						}
					}
				}
			}
		}

		token = strtok (NULL, TOK_DELIMINATORS);
	}

	free(buf);
}

void RS_ScanPathForScripts (void)
{
	char	script[MAX_OSPATH];
	char	**script_list, *c;
	int	script_count, i;


	script_list = FS_ListFilesInFS("scripts/*.rscript", &script_count, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

	if(script_list) {
		for (i = 0; i < script_count; i++)
		{
			c = COM_SkipPath(script_list[i]);
			Com_sprintf(script, MAX_OSPATH, "scripts/%s", c);
			RS_LoadScript(script);
		}

		FS_FreeFileList(script_list, script_count);
	}
	
	script_count = 0;
	//TODO: gl_interactivescripts cvar
	script_list = FS_ListFilesInFS("scripts/interactive/*.rscript", &script_count, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

	if(script_list) {
		for (i = 0; i < script_count; i++)
		{
			c = COM_SkipPath(script_list[i]);
			Com_sprintf(script, MAX_OSPATH, "scripts/interactive/%s", c);
			RS_LoadScript(script);
		}

		FS_FreeFileList(script_list, script_count);
	}

	script_count = 0;
	if(gl_normalmaps->value) { //search for normal map scripts ONLY if we are using normal mapping, do last to overide anything

		script_list = FS_ListFilesInFS("scripts/normals/*.rscript", &script_count, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

		if(script_list) {
			for (i = 0; i < script_count; i++)
			{
				c = COM_SkipPath(script_list[i]);
				Com_sprintf(script, MAX_OSPATH, "scripts/normals/%s", c);
				RS_LoadScript(script);
			}

			FS_FreeFileList(script_list, script_count);
		}
	}
}

void RS_SetEnvmap (vec3_t v, float *os, float *ot)
{
	vec3_t vert;

	vert[0] = v[0]*r_world_matrix[0]+v[1]*r_world_matrix[4]+v[2]*r_world_matrix[8] +r_world_matrix[12];
	vert[1] = v[0]*r_world_matrix[1]+v[1]*r_world_matrix[5]+v[2]*r_world_matrix[9] +r_world_matrix[13];
	vert[2] = v[0]*r_world_matrix[2]+v[1]*r_world_matrix[6]+v[2]*r_world_matrix[10]+r_world_matrix[14];

	VectorNormalize (vert);

	*os = vert[0];
	*ot = vert[1];
}

void RS_ScaleTexcoords (rs_stage_t *stage, float *os, float *ot)
{
	// float	txm = 0, tym = 0; // unused

	// scale
	if (stage->scale.scaleX)
	{
		switch (stage->scale.typeX)
		{
		case 0:	// static
			*os *= stage->scale.scaleX;
			break;
		case 1:	// sine
			*os *= stage->scale.scaleX*sin(rs_realtime*0.05);
			break;
		case 2:	// cosine
			*os *= stage->scale.scaleX*cos(rs_realtime*0.05);
			break;
		}
	}

	if (stage->scale.scaleY)
	{
		switch (stage->scale.typeY)
		{
		case 0:	// static
			*ot *= stage->scale.scaleY;
			break;
		case 1:	// sine
			*ot *= stage->scale.scaleY*sin(rs_realtime*0.05);
			break;
		case 2:	// cosine
			*ot *= stage->scale.scaleY*cos(rs_realtime*0.05);
			break;
		}
	}
}

inline void RS_RotateST (float *os, float *ot, float degrees, msurface_t *fa)
{
	float cost = cos(degrees), sint = sin(degrees);
	float is = *os, it = *ot, c_s, c_t;

	c_s = fa->c_s - (int)fa->c_s;
	c_t = fa->c_t - (int)fa->c_t;

	*os = cost * (is - c_s) + sint * (c_t - it) + c_s;
	*ot = cost * (it - c_t) + sint * (is - c_s) + c_t;

}

inline void RS_RotateST2 (float *os, float *ot, float degrees)
{
	float cost = cos(degrees), sint = sin(degrees);
	float is = *os, it = *ot;

	*os = cost * (is - 0.5) + sint * (0.5 - it) + 0.5;
	*ot = cost * (it - 0.5) + sint * (is - 0.5) + 0.5;
}

void RS_SetTexcoords (rs_stage_t *stage, float *os, float *ot, msurface_t *fa)
{
	RS_ScaleTexcoords(stage, os, ot);

	// rotate
	if (stage->rot_speed)
		RS_RotateST (os, ot, -stage->rot_speed * rs_realtime * 0.0087266388888888888888888888888889, fa);

}
void RS_SetTexcoords2D (rs_stage_t *stage, float *os, float *ot)
{
	float	txm = 0, tym = 0;

	// scale
	if (stage->scale.scaleX)
	{
		switch (stage->scale.typeX)
		{
		case 0:	// static
			*os *= stage->scale.scaleX;
			break;
		case 1:	// sine
			*os *= stage->scale.scaleX*sin(rs_realtime*0.05);
			break;
		case 2:	// cosine
			*os *= stage->scale.scaleX*cos(rs_realtime*0.05);
			break;
		}
	}

	if (stage->scale.scaleY)
	{
		switch (stage->scale.typeY)
		{
		case 0:	// static
			*ot *= stage->scale.scaleY;
			break;
		case 1:	// sine
			*ot *= stage->scale.scaleY*sin(rs_realtime*0.05);
			break;
		case 2:	// cosine
			*ot *= stage->scale.scaleY*cos(rs_realtime*0.05);
			break;
		}

	}

	// rotate
	if (stage->rot_speed)
		RS_RotateST2 (os, ot, -stage->rot_speed * rs_realtime * 0.0087266388888888888888888888888889);

	if (stage->scroll.speedX)
	{
		switch(stage->scroll.typeX)
		{
			case 0:	// static
				txm=rs_realtime*stage->scroll.speedX;
				break;
			case 1:	// sine
				txm=sin(rs_realtime*stage->scroll.speedX);
				break;
			case 2:	// cosine
				txm=cos(rs_realtime*stage->scroll.speedX);
				break;
		}
	}
	else
		txm=0;

	if (stage->scroll.speedY)
	{
		switch(stage->scroll.typeY)
		{
			case 0:	// static
				tym=rs_realtime*stage->scroll.speedY;
				break;
			case 1:	// sine
				tym=sin(rs_realtime*stage->scroll.speedY);
				break;
			case 2:	// cosine
				tym=cos(rs_realtime*stage->scroll.speedY);
				break;
		}
	}
	else
		tym=0;

	*os += txm;
	*ot += tym;
}

float RS_AlphaFunc (int alphafunc, float alpha, vec3_t normal, vec3_t org)
{
	vec3_t dir;

	if (!abs(alphafunc))
		goto endalpha;

	if (alphafunc == ALPHAFUNC_GLOSS)
	{
		//glossmap stuff here...
	}
	else if (alphafunc == ALPHAFUNC_NORMAL)
	{
		VectorSubtract(org, r_newrefdef.vieworg, dir);
		VectorNormalize(dir);
		alpha *= fabs(cutDot(dir, normal));
	}

endalpha:

	if (alpha<0) alpha = 0;
	if (alpha>1) alpha = 1;
	if (alphafunc<0) alpha = 1-alpha;

	return alpha;
}

image_t *BSP_TextureAnimation (mtexinfo_t *tex);
rscript_t	*surfaceScript(msurface_t *surf)
{
	image_t *image = BSP_TextureAnimation( surf->texinfo );

	if (image && image->script)
	{
		rscript_t *rs = image->script;

		RS_ReadyScript(rs);

		return rs;
	}
	return NULL;
}
void SetVertexOverbrights (qboolean toggle)
{
	if (!r_overbrightbits->value)
		return;

	if (toggle)//turn on
	{
		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);

		GL_TexEnv( GL_COMBINE_EXT );
	}
	else //turn off
	{
		GL_TexEnv( GL_MODULATE );
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	}
}

void SetLightingMode (void)
{
	GL_SelectTexture( GL_TEXTURE0);

	if ( !gl_config.mtexcombine ) 
	{
		GL_TexEnv( GL_REPLACE );
		GL_SelectTexture( GL_TEXTURE1);

		if ( gl_lightmap->value )
			GL_TexEnv( GL_REPLACE );
		else 
			GL_TexEnv( GL_MODULATE );
	}
	else 
	{
		GL_TexEnv ( GL_COMBINE_EXT );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );

		GL_SelectTexture( GL_TEXTURE1 );
		GL_TexEnv ( GL_COMBINE_EXT );
		if ( gl_lightmap->value ) 
		{
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
		} 
		else 
		{
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );

			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT );
		}

		if ( r_overbrightbits->value )
		{
			qglTexEnvi ( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, r_overbrightbits->value );
		}
	}
}

qboolean lightmaptoggle;
void ToggleLightmap (qboolean toggle)
{
	if (toggle==lightmaptoggle)
		return;

	lightmaptoggle = toggle;
	if (toggle)
	{
		SetVertexOverbrights(false);
		GL_EnableMultitexture( true );
		SetLightingMode ();
	}
	else
	{
		GL_EnableMultitexture( false );
		SetVertexOverbrights(true);
	}
}

static cvar_t *rs_eval_if_subexpr (rs_cond_val_t *expr)
{
	int resv;
	switch (expr->optype)
	{
		case rs_cond_none:
			return expr->val;
		case rs_cond_is:
			return rs_eval_if_subexpr (expr->subexpr1);
		case rs_cond_lnot:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value == 0);
			break;
		case rs_cond_eq:
			resv = Q_strcasecmp (
					rs_eval_if_subexpr (expr->subexpr1)->string,
					rs_eval_if_subexpr (expr->subexpr2)->string
				) == 0;
			break;
		case rs_cond_neq:
			resv = Q_strcasecmp (
					rs_eval_if_subexpr (expr->subexpr1)->string,
					rs_eval_if_subexpr (expr->subexpr2)->string
				) != 0;
			break;
		case rs_cond_gt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value >
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_ngt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value <=
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_lt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value <
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_nlt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value >=
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_and:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value &&
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_or:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value ||
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		default:
			Com_Printf ("Unknown optype %d! (Can't happen)\n", expr->optype);
			resv = 0;
			break;
	}
	Anon_Cvar_Set (&(expr->lval), va ("%d", resv));
	return &(expr->lval);
}

//This is the shader drawing routine for bsp surfaces - it will draw on top of the
//existing texture.
void RS_DrawSurfaceTexture (msurface_t *surf, rscript_t *rs)
{
	glpoly_t	*p = surf->polys;
	unsigned	lmtex = surf->lightmaptexturenum;
	float		*v;
	int			i, nv;
	vec3_t		vectors[3];
	rs_stage_t	*stage;
	float		os, ot, alpha;
	float		time, txm=0.0f, tym=0.0f;
	int			VertexCounter;

	if (!rs)
		return;

	nv = surf->polys->numverts;
	stage = rs->stage;
	time = rs_realtime;

	//for envmap by normals
	AngleVectors (r_newrefdef.viewangles, vectors[0], vectors[1], vectors[2]);

	lightmaptoggle = true;
	do
	{
		if (stage->lensflare || stage->grass || stage->beam || stage->cube)
			break; //handled elsewhere
		if (stage->condv && !(rs_eval_if_subexpr(stage->condv)->value))
			continue; //stage should not execute

		if(stage->lightmap)
		{
			ToggleLightmap(true);
			qglShadeModel (GL_FLAT);

			GL_MBind (GL_TEXTURE1, gl_state.lightmap_textures + lmtex);

			if (stage->colormap.enabled)
			qglDisable (GL_TEXTURE_2D);
			else if (stage->anim_count){
				GL_MBind (GL_TEXTURE0, RS_Animate(stage));
			}
			else
			{
		 		GL_MBind (GL_TEXTURE0, stage->texture->texnum);
			}			
		}
		else 
		{
			ToggleLightmap(false);
			qglShadeModel (GL_SMOOTH);

			if (stage->colormap.enabled)
				qglDisable (GL_TEXTURE_2D);
			else if (stage->anim_count)
			{
				GL_Bind (RS_Animate(stage));
			}
			else
			{
		 		GL_Bind (stage->texture->texnum);
			}			
		}
				
		if (stage->blendfunc.blend)
		{
			GL_BlendFunction(stage->blendfunc.source, stage->blendfunc.dest);
			GLSTATE_ENABLE_BLEND
		}
		else if (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66) && !stage->alphamask)
		{
			GL_BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GLSTATE_ENABLE_BLEND
		}
		else
		{
			GLSTATE_DISABLE_BLEND
		}

		// sane defaults
		alpha = 1.0f;

		if (stage->alphashift.min || stage->alphashift.speed)
		{
			if (!stage->alphashift.speed && stage->alphashift.min > 0)
			{
				alpha = stage->alphashift.min;
			}
			else if (stage->alphashift.speed)
			{
				alpha = sin (rs_realtime * stage->alphashift.speed);
				alpha = (alpha + 1)*0.5f;
				if (alpha > stage->alphashift.max)
					alpha = stage->alphashift.max;
				if (alpha < stage->alphashift.min)
					alpha = stage->alphashift.min;
			}
		}

		if (stage->scroll.speedX)
		{
			switch (stage->scroll.typeX)
			{
			case 0:	// static
				txm = rs_realtime*stage->scroll.speedX;
				break;
			case 1:	// sine
				txm = sin (rs_realtime*stage->scroll.speedX);
				break;
			case 2:	// cosine
				txm = cos (rs_realtime*stage->scroll.speedX);
				break;
			}
		}
		else
			txm=0;

		if (stage->scroll.speedY)
		{
			switch (stage->scroll.typeY)
			{
			case 0:	// static
				tym = rs_realtime*stage->scroll.speedY;
				break;
			case 1:	// sine
				tym = sin (rs_realtime*stage->scroll.speedY);
				break;
			case 2:	// cosine
				tym = cos (rs_realtime*stage->scroll.speedY);
				break;
			}
		} else
			tym=0;

		qglColor4f (1, 1, 1, alpha);

		if (stage->alphamask)
		{
			GLSTATE_ENABLE_ALPHATEST
		}
		else
		{
			GLSTATE_DISABLE_ALPHATEST
		}		
	
		if(stage->lightmap)
			R_InitVArrays (VERT_MULTI_TEXTURED);
		else
			R_InitVArrays (VERT_SINGLE_TEXTURED);

		VArray = &VArrayVerts[0];
		VertexCounter = 0;

		for (i = 0, v = p->verts[0]; i < nv; i++, v += VERTEXSIZE)
		{
			if (stage->envmap)
			{
				RS_SetEnvmap (v, &os, &ot);
				//move by normal & position
				os-=DotProduct (surf->plane->normal, vectors[1] ) + (r_newrefdef.vieworg[0]-r_newrefdef.vieworg[1]+r_newrefdef.vieworg[2])*0.0025;
				ot+=DotProduct (surf->plane->normal, vectors[2] ) + (-r_newrefdef.vieworg[0]+r_newrefdef.vieworg[1]-r_newrefdef.vieworg[2])*0.0025;

				if (surf->texinfo->flags & SURF_FLOWING)
					txm = tym = 0;
			}
			else 
			{
				os = v[3];
				ot = v[4];
			}

			RS_SetTexcoords (stage, &os, &ot, surf);
			{
				float red=255, green=255, blue=255;

				if (stage->colormap.enabled)
				{
					red *= stage->colormap.red/255.0f;
					green *= stage->colormap.green/255.0f;
					blue *= stage->colormap.blue/255.0f;
				}

				alpha = RS_AlphaFunc(stage->alphafunc, alpha, surf->plane->normal, v);
				if (red>1)red=1; if (red<0) red = 0;
				if (green>1)green=1; if (green<0) green = 0;
				if (blue>1)blue=1; if (blue<0) blue = 0;

				qglColor4f (red, green, blue, alpha);
			}

			// copy in vertex data
			VArray[0] = v[0];
			VArray[1] = v[1];
			VArray[2] = v[2];

			// world texture coords
			VArray[3] = os+txm;
			VArray[4] = ot+tym;

			// lightmap texture coords
			VArray[5] = v[5];
			VArray[6] = v[6];

			if(stage->lightmap)
				VArray += VertexSizes[VERT_MULTI_TEXTURED];
			else
				VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			VertexCounter++;		
		}

		R_DrawVarrays(GL_POLYGON, 0, VertexCounter, false);
					
		qglColor4f(1,1,1,1);
		if (stage->colormap.enabled)
			qglEnable (GL_TEXTURE_2D);

	} while ( (stage = stage->next) );	
	
	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	
	R_KillVArrays();		
	
	ToggleLightmap(true);

	GL_EnableMultitexture( false );

	// restore the original blend mode
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_TEXGEN
}

rscript_t *rs_caustics;
rscript_t *rs_glass;

void RS_LoadSpecialScripts (void) //the special cases of glass and water caustics
{
	rs_caustics = RS_FindScript("caustics");
	if(rs_caustics)
		RS_ReadyScript(rs_caustics);
	rs_glass = RS_FindScript("glass");
	if(rs_glass)
		RS_ReadyScript(rs_glass);
}

void RS_Surface (msurface_t *surf)
{
	rscript_t *rs_shader;

	//Underwater Caustics
	if(rs_caustics)
		if (surf->iflags & ISURF_UNDERWATER )
				RS_DrawSurfaceTexture(surf, rs_caustics);

	//all other textures shaders
	rs_shader = (rscript_t *)surf->texinfo->image->script;
	if(rs_shader)
		RS_DrawSurfaceTexture(surf, rs_shader);
}


