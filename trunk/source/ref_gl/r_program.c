/*
Copyright (C) 2009 COR Entertainment, LLC

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_program.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

//ARB fragment program extensions
PFNGLGENPROGRAMSARBPROC             qglGenProgramsARB            = NULL;
PFNGLDELETEPROGRAMSARBPROC          qglDeleteProgramsARB         = NULL;
PFNGLBINDPROGRAMARBPROC             qglBindProgramARB            = NULL;
PFNGLPROGRAMSTRINGARBPROC           qglProgramStringARB          = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC   qglProgramEnvParameter4fARB  = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;

unsigned int g_water_program_id;
char *fragment_program_text;

//GLSL
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObjectARB	= NULL;
PFNGLDELETEOBJECTARBPROC			glDeleteObjectARB			= NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObjectARB		= NULL;
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObjectARB		= NULL;
PFNGLSHADERSOURCEARBPROC			glShaderSourceARB			= NULL;
PFNGLCOMPILESHADERARBPROC			glCompileShaderARB			= NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameterivARB	= NULL;
PFNGLATTACHOBJECTARBPROC			glAttachObjectARB			= NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLogARB				= NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgramARB			= NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocationARB		= NULL;
PFNGLUNIFORM3FARBPROC				glUniform3fARB				= NULL;
PFNGLUNIFORM2FARBPROC				glUniform2fARB				= NULL;
PFNGLUNIFORM1IARBPROC				glUniform1iARB				= NULL;
PFNGLUNIFORM1FARBPROC				glUniform1fARB				= NULL;
PFNGLUNIFORMMATRIX3FVARBPROC		glUniformMatrix3fvARB		= NULL;
PFNGLUNIFORMMATRIX3X4FVARBPROC		glUniformMatrix3x4fvARB		= NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC     glVertexAttribPointerARB	= NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC		glBindAttribLocationARB		= NULL;

GLhandleARB g_programObj;
GLhandleARB g_shadowprogramObj;
GLhandleARB g_waterprogramObj;
GLhandleARB g_meshprogramObj;
GLhandleARB g_glassprogramObj;
GLhandleARB g_blankmeshprogramObj;
GLhandleARB g_fbprogramObj;
GLhandleARB g_blurprogramObj;
GLhandleARB g_rblurprogramObj;
GLhandleARB g_dropletsprogramObj;
GLhandleARB g_godraysprogramObj;

GLhandleARB g_vertexShader;
GLhandleARB g_fragmentShader;

//standard bsp
GLuint      g_location_surfTexture;
GLuint      g_location_eyePos;
GLuint		g_tangentSpaceTransform;
GLuint      g_location_heightTexture;
GLuint		g_location_lmTexture;
GLuint		g_location_normalTexture;
GLuint		g_location_bspShadowmapTexture;
GLuint		g_location_bspShadowmapTexture2;
GLuint		g_location_fog;
GLuint	    g_location_parallax;
GLuint		g_location_dynamic;
GLuint		g_location_shadowmap;
GLuint		g_Location_statshadow;
GLuint		g_location_xOffs;
GLuint		g_location_yOffs;
GLuint	    g_location_lightPosition;
GLuint		g_location_staticLightPosition;
GLuint		g_location_lightColour;
GLuint	    g_location_lightCutoffSquared;
GLuint		g_location_liquid;
GLuint		g_location_rsTime;
GLuint		g_location_liquidTexture;
GLuint		g_location_liquidNormTex;

//shadow on white bsp polys
GLuint		g_location_entShadow;
GLuint		g_location_fadeShadow;
GLuint		g_location_xOffset;
GLuint		g_location_yOffset;

//water
GLuint		g_location_baseTexture;
GLuint		g_location_normTexture;
GLuint		g_location_refTexture;
GLuint		g_location_tangent;
GLuint		g_location_time;
GLuint		g_location_lightPos;
GLuint		g_location_reflect;
GLuint		g_location_trans;
GLuint		g_location_fogamount;

//mesh
GLuint		g_location_meshlightPosition;
GLuint		g_location_baseTex;
GLuint		g_location_normTex;
GLuint		g_location_fxTex;
GLuint		g_location_color;
GLuint		g_location_meshNormal;
GLuint		g_location_meshTime;
GLuint		g_location_meshFog;
GLuint		g_location_useFX;
GLuint		g_location_useGlow;
GLuint		g_location_useShell;
GLuint		g_location_useGPUanim;
GLuint		g_location_outframe;

//glass
GLuint		g_location_gmirTexture;
GLuint		g_location_grefTexture;
GLuint		g_location_gLightPos;
GLuint		g_location_gFog;
GLuint		g_location_gOutframe;

//blank mesh
GLuint		g_location_bmOutframe;

//fullscreen distortion effects
GLuint		g_location_framebuffTex;
GLuint		g_location_distortTex;
GLuint		g_location_dParams;
GLuint		g_location_fxPos;

//gaussian blur
GLuint		g_location_scale;
GLuint		g_location_source;

//radial blur	
GLuint		g_location_rscale;
GLuint		g_location_rsource;
GLuint		g_location_rparams;

//water droplets
GLuint		g_location_drSource;
GLuint		g_location_drTex;
GLuint		g_location_drTime;
GLuint		g_location_drParams;

//god rays
GLuint g_location_lightPositionOnScreen;
GLuint g_location_sunTex;

//doesn't work with ARB shaders, unfortunately, due to the #comments
#define STRINGIFY(...) #__VA_ARGS__

static char water_ARB_program[] =
"!!ARBfp1.0\n"

"# Scroll and scale the distortion texture coordinates.\n"
"# Scroll coordinates are specified externally.\n"
"PARAM scroll1 = program.local[0];\n"
"PARAM scroll2 = program.local[1];\n"
"PARAM texScale1 = { 0.008, 0.008, 1.0, 1.0 };\n"
"PARAM texScale2 = { 0.007, 0.007, 1.0, 1.0 };\n"
"TEMP texCoord1;\n"
"TEMP texCoord2;\n"
"MUL texCoord1, fragment.texcoord[1], texScale1;\n"
"MUL texCoord2, fragment.texcoord[1], texScale2;\n"
"ADD texCoord1, texCoord1, scroll1;\n"
"ADD texCoord2, texCoord2, scroll2;\n"

"# Load the distortion textures and add them together.\n"
"TEMP distortColor;\n"
"TEMP distortColor2;\n"
"TXP distortColor, texCoord1, texture[0], 2D;\n"
"TXP distortColor2, texCoord2, texture[0], 2D;\n"
"ADD distortColor, distortColor, distortColor2;\n"

"# Subtract 1.0 and scale by 2.0.\n"
"# Textures will be distorted from -2.0 to 2.0 texels.\n"
"PARAM scaleFactor = { 2.0, 2.0, 2.0, 2.0 };\n"
"PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n"
"SUB distortColor, distortColor, one;\n"
"MUL distortColor, distortColor, scaleFactor;\n"

"# Apply distortion to reflection texture coordinates.\n"
"TEMP distortCoord;\n"
"TEMP endColor;\n"
"ADD distortCoord, distortColor, fragment.texcoord[0];\n"
"TXP endColor, distortCoord, texture, 2D;\n"

"# Get a vector from the surface to the view origin\n"
"PARAM vieworg = program.local[2];\n"
"TEMP eyeVec;\n"
"TEMP trans;\n"
"SUB eyeVec, vieworg, fragment.texcoord[1];\n"

"# Normalize the vector to the eye position\n"
"TEMP temp;\n"
"TEMP invLen;\n"
"DP3 temp, eyeVec, eyeVec;\n"
"RSQ invLen, temp.x;\n"
"MUL eyeVec, eyeVec, invLen;\n"
"ABS eyeVec.z, eyeVec.z; # so it works underwater, too\n"

"# Load the ripple normal map\n"
"TEMP normalColor;\n"
"TEMP normalColor2;\n"
"# Scale texture\n"
"MUL texCoord1, fragment.texcoord[2], texScale2;\n"
"MUL texCoord2, fragment.texcoord[2], texScale1;\n"
"# Scroll texture\n"
"ADD texCoord1, texCoord1, scroll1;\n"
"ADD texCoord2, texCoord2, scroll2;\n"
"# Get texel color\n"
"TXP normalColor, texCoord1, texture[1], 2D;\n"
"TXP normalColor2, texCoord2, texture[1], 2D;\n"
"# Combine normal maps\n"
"ADD normalColor, normalColor, normalColor2;\n"
"SUB normalColor, normalColor, 1.0;\n"

"# Normalize normal texture\n"
"DP3 temp, normalColor, normalColor;\n"
"RSQ invLen, temp.x;\n"
"MUL normalColor, invLen, normalColor;\n"

"# Fresenel approximation\n"
"DP3 trans.w, normalColor, eyeVec;\n"
"SUB endColor.w, 1.0, trans.w;\n"
"MAX endColor.w, endColor.w, 0.4; # MAX sets the min?  How odd.\n"
"MIN endColor.w, endColor.w, 0.9; # Leave a LITTLE bit of transparency always\n"

"# Put the color in the output (TODO: put this in final OP)\n"
"MOV result.color, endColor;\n"

"END\n";

void R_LoadARBPrograms(void)
{
	if (strstr(gl_config.extensions_string, "GL_ARB_fragment_program"))
	{
		gl_state.fragment_program = true;

		qglGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)qwglGetProcAddress("glGenProgramsARB");
		qglDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)qwglGetProcAddress("glDeleteProgramsARB");
		qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)qwglGetProcAddress("glBindProgramARB");
		qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)qwglGetProcAddress("glProgramStringARB");
		qglProgramEnvParameter4fARB =
			(PFNGLPROGRAMENVPARAMETER4FARBPROC)qwglGetProcAddress("glProgramEnvParameter4fARB");
		qglProgramLocalParameter4fARB =
			(PFNGLPROGRAMLOCALPARAMETER4FARBPROC)qwglGetProcAddress("glProgramLocalParameter4fARB");

		if (!(qglGenProgramsARB && qglDeleteProgramsARB && qglBindProgramARB &&
			qglProgramStringARB && qglProgramEnvParameter4fARB && qglProgramLocalParameter4fARB))
		{
			gl_state.fragment_program = false;
			Com_Printf("...GL_ARB_fragment_program not found\n");

		}
	}
	else
	{
		gl_state.fragment_program = false;
		Com_Printf("...GL_ARB_fragment_program not found\n");
	}

	if (gl_state.fragment_program)
	{
		gl_arb_fragment_program = Cvar_Get("gl_arb_fragment_program", "1", CVAR_ARCHIVE);

		qglGenProgramsARB(1, &g_water_program_id);
		qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, g_water_program_id);
		qglProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, 1.0f, 0.1f, 0.6f, 0.5f);
		qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(water_ARB_program), water_ARB_program);

		// Make sure the program loaded correctly
		{
			int err = 0;
			assert((err = qglGetError()) == GL_NO_ERROR);
			err = err; // for debugging only -- todo, remove
		}
	}
	else
		gl_arb_fragment_program = Cvar_Get("gl_arb_fragment_program", "0", CVAR_ARCHIVE);
}

//GLSL Programs

//BSP Surfaces
static char bsp_vertex_program[] = STRINGIFY (
	uniform mat3 tangentSpaceTransform;
	uniform vec3 Eye;
	uniform vec3 lightPosition;
	uniform vec3 staticLightPosition;
	uniform float rsTime;
	uniform int LIQUID;
	uniform int FOG;
	uniform int PARALLAX;
	uniform int DYNAMIC;

	varying vec3 EyeDir;
	varying vec3 LightDir;
	varying vec3 StaticLightDir;
	varying vec4 sPos;
	varying float fog;

	void main( void )
	{
		sPos = gl_Vertex;

		gl_Position = ftransform();

		gl_FrontColor = gl_Color;

		EyeDir = tangentSpaceTransform * ( Eye - gl_Vertex.xyz );
		if (DYNAMIC > 0)
		{
			LightDir = tangentSpaceTransform * (lightPosition - gl_Vertex.xyz);
		}
		if (PARALLAX > 0)
		{
			StaticLightDir = tangentSpaceTransform * (staticLightPosition - gl_Vertex.xyz);
		}

		// pass any active texunits through
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_TexCoord[1] = gl_MultiTexCoord1;

		//fog
		if(FOG > 0){
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 1.0);
		}
	}
);

static char bsp_fragment_program[] = STRINGIFY (
	uniform sampler2D surfTexture;
	uniform sampler2D HeightTexture;
	uniform sampler2D NormalTexture;
	uniform sampler2D lmTexture;
	uniform sampler2D liquidTexture;
	uniform sampler2D liquidNormTex;
	uniform sampler2DShadow ShadowMap;
	uniform sampler2DShadow StatShadowMap;
	uniform vec3 lightColour;
	uniform float lightCutoffSquared;
	uniform int FOG;
	uniform int PARALLAX;
	uniform int DYNAMIC;
	uniform int STATSHADOW;
	uniform int SHADOWMAP;
	uniform int LIQUID;
	uniform float rsTime;
	uniform float xPixelOffset;
	uniform float yPixelOffset;

	varying vec4 sPos;
	varying vec3 EyeDir;
	varying vec3 LightDir;
	varying vec3 StaticLightDir;
	varying float fog;

	vec4 ShadowCoord;
	
	float lookup( vec2 offSet, sampler2DShadow Map)
	{
		return shadow2DProj(Map, ShadowCoord + vec4(offSet.x * xPixelOffset * ShadowCoord.w, offSet.y * yPixelOffset * ShadowCoord.w, 0.05, 0.0) ).w;
	}

	float lookupShadow( sampler2DShadow Map )
	{
		float shadow = 1.0;

		if(SHADOWMAP > 0) 
		{			
			if (ShadowCoord.w > 1.0)
			{
				vec2 o = mod(floor(gl_FragCoord.xy), 2.0);
				
				shadow += lookup(vec2(-1.5, 1.5) + o, Map);
				shadow += lookup(vec2( 0.5, 1.5) + o, Map);
				shadow += lookup(vec2(-1.5, -0.5) + o, Map);
				shadow += lookup(vec2( 0.5, -0.5) + o, Map);
				shadow *= 0.25 ;
			}
	
			shadow += 0.2;
			if(shadow > 1.0)
				shadow = 1.0;
		}
		
		return shadow;
	}		

	void main( void )
	{
		vec4 diffuse;
		vec4 lightmap;
		vec4 alphamask;
		vec4 bloodColor;
		float distanceSquared;
		vec3 relativeLightDirection;
		float diffuseTerm;
		vec3 halfAngleVector;
		float specularTerm;
		float swamp;
		float attenuation;
		vec4 litColour;
		vec3 varyingLightColour;
		float varyingLightCutoffSquared;
		float dynshadowval;
		float statshadowval;
		vec2 displacement;
		vec2 displacement2;
		vec2 displacement3;
		vec2 displacement4;

		varyingLightColour = lightColour;
		varyingLightCutoffSquared = lightCutoffSquared;

		vec3 relativeEyeDirection = normalize( EyeDir );

		vec3 normal = 2.0 * ( texture2D( NormalTexture, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );
		vec3 textureColour = texture2D( surfTexture, gl_TexCoord[0].xy ).rgb;

		lightmap = texture2D( lmTexture, gl_TexCoord[1].st );
		alphamask = texture2D( surfTexture, gl_TexCoord[0].xy );

	   	//shadows
		if(DYNAMIC > 0)
		{
			ShadowCoord = gl_TextureMatrix[7] * sPos;
			dynshadowval = lookupShadow(ShadowMap);
		}
		else
			dynshadowval = 0.0;

		if(STATSHADOW > 0)
		{
			ShadowCoord = gl_TextureMatrix[6] * sPos;
			statshadowval = lookupShadow(StatShadowMap);
		}
		else
			statshadowval = 1.0;

		bloodColor = vec4(0.0, 0.0, 0.0, 0.0);
		displacement4 = vec2(0.0, 0.0);
		if(LIQUID > 0)
		{
			vec3 noiseVec;
			vec3 noiseVec2;
			vec3 noiseVec3;

			//for liquid fx scrolling
			vec4 texco = gl_TexCoord[0];
			texco.t = texco.t - rsTime*1.0/LIQUID;

			vec4 texco2 = gl_TexCoord[0];
			texco2.t = texco2.t - rsTime*0.9/LIQUID;
			//shift the horizontal here a bit
			texco2.s = texco2.s/1.5;

			vec4 texco3 = gl_TexCoord[0];
			texco3.t = texco3.t - rsTime*0.6/LIQUID;

			vec4 Offset = texture2D( HeightTexture,gl_TexCoord[0].xy );
			Offset = Offset * 0.04 - 0.02;
			vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + gl_TexCoord[0].xy;

			displacement = texco.st;

			noiseVec = normalize(texture2D(liquidNormTex, texco.st)).xyz;
			noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

			displacement2 = texco2.st;

			noiseVec2 = normalize(texture2D(liquidNormTex, displacement2.xy)).xyz;
			noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;

			if(LIQUID > 2)
			{
				displacement3 = texco3.st;

				noiseVec3 = normalize(texture2D(liquidNormTex, displacement3.xy)).xyz;
				noiseVec3 = (noiseVec3 * 2.0 - 0.635) * 0.035;
			}
			else
			{
				//used for water effect only
				displacement4.x = noiseVec.x + noiseVec2.x;
				displacement4.y = noiseVec.y + noiseVec2.y;
			}

			displacement.x = texco.s + noiseVec.x + TexCoords.x;
			displacement.y = texco.t + noiseVec.y + TexCoords.y;
			displacement2.x = texco2.s + noiseVec2.x + TexCoords.x;
			displacement2.y = texco2.t + noiseVec2.y + TexCoords.y;
			displacement3.x = texco3.s + noiseVec3.x + TexCoords.x;
			displacement3.y = texco3.t + noiseVec3.y + TexCoords.y;

			if(LIQUID > 2)
			{
				vec4 diffuse1 = texture2D(liquidTexture, texco.st + displacement.xy);
				vec4 diffuse2 = texture2D(liquidTexture, texco2.st + displacement2.xy);
				vec4 diffuse3 = texture2D(liquidTexture, texco3.st + displacement3.xy);
				vec4 diffuse4 = texture2D(liquidTexture, gl_TexCoord[0].st + displacement4.xy);
				bloodColor = max(diffuse1, diffuse2);
				bloodColor = max(bloodColor, diffuse3);
			}
		}

	   if(PARALLAX > 0) 
	   {
			//do the parallax mapping
			vec4 Offset = texture2D( HeightTexture,gl_TexCoord[0].xy );
			Offset = Offset * 0.04 - 0.02;
			vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + gl_TexCoord[0].xy + displacement4.xy;

			diffuse = texture2D( surfTexture, TexCoords );

			relativeLightDirection = normalize (StaticLightDir);

			diffuseTerm = dot( normal, relativeLightDirection  );

			if( diffuseTerm > 0.0 )
			{
				halfAngleVector = normalize( relativeLightDirection + relativeEyeDirection );

				specularTerm = clamp( dot( normal, halfAngleVector ), 0.0, 1.0 );
				specularTerm = pow( specularTerm, 32.0 );

				litColour = vec4 (specularTerm + ( 3.0 * diffuseTerm ) * textureColour, 6.0);
			}
			else
			{
				litColour = vec4( 0.0, 0.0, 0.0, 6.0 );
			}

			gl_FragColor = max(litColour, diffuse * 2.0);
			gl_FragColor = (gl_FragColor * lightmap) + bloodColor;
			gl_FragColor = (gl_FragColor * statshadowval);
	   }
	   else
	   {
			diffuse = texture2D(surfTexture, gl_TexCoord[0].xy);
			gl_FragColor = (diffuse * lightmap * 2.0);
			gl_FragColor = (gl_FragColor * statshadowval);
	   }

	   if(DYNAMIC > 0) 
	   {
			lightmap = texture2D(lmTexture, gl_TexCoord[1].st);

			//now do the dynamic lighting
			distanceSquared = dot( LightDir, LightDir );
			relativeLightDirection = LightDir / sqrt( distanceSquared );

			diffuseTerm = clamp( dot( normal, relativeLightDirection ), 0.0, 1.0 );
			vec3 colour = vec3( 0.0, 0.0, 0.0 );

			if( diffuseTerm > 0.0 )
			{
				halfAngleVector = normalize( relativeLightDirection + relativeEyeDirection );

				float specularTerm = clamp( dot( normal, halfAngleVector ), 0.0, 1.0 );
				specularTerm = pow( specularTerm, 32.0 );

				colour = specularTerm * vec3( 1.0, 1.0, 1.0 ) / 2.0;
			}

			attenuation = clamp( 1.0 - ( distanceSquared / varyingLightCutoffSquared ), 0.0, 1.0 );

			swamp = attenuation;
			swamp *= swamp;
			swamp *= swamp;
			swamp *= swamp;

			colour += ( ( ( 0.5 - swamp ) * diffuseTerm ) + swamp ) * textureColour * 3.0;

			vec4 dynamicColour = vec4( attenuation * colour * dynshadowval * varyingLightColour, 1.0 );
			if(PARALLAX > 0) 
			{
				dynamicColour = max(dynamicColour, gl_FragColor);
			}
			else 
			{
				dynamicColour = max(dynamicColour, vec4(textureColour, 1.0) * lightmap * 2.0);
			}
			gl_FragColor = dynamicColour;
	   }

		gl_FragColor = mix(vec4(0.0, 0.0, 0.0, alphamask.a), gl_FragColor, alphamask.a);
		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

static char shadow_vertex_program[] = STRINGIFY (
		
	varying vec4 sPos;

	void main( void )
	{
		sPos = gl_Vertex;

		gl_Position = ftransform();
	}
);

static char shadow_fragment_program[] = STRINGIFY (

	uniform sampler2DShadow StatShadowMap;
	uniform float fadeShadow;
	uniform float xPixelOffset;
	uniform float yPixelOffset;

	varying vec4 sPos;

	vec4 ShadowCoord;
	
	float lookup( vec2 offSet)
	{
		return shadow2DProj(StatShadowMap, ShadowCoord + vec4(offSet.x * xPixelOffset * ShadowCoord.w, offSet.y * yPixelOffset * ShadowCoord.w, 0.05, 0.0) ).w;
	}

	float lookupStatshadow( void )
	{
		float shadow;

		if (ShadowCoord.w > 1.0)
		{
			vec2 o = mod(floor(gl_FragCoord.xy), 2.0);
				
			shadow += lookup(vec2(-1.5, 1.5) + o);
			shadow += lookup(vec2( 0.5, 1.5) + o);
			shadow += lookup(vec2(-1.5, -0.5) + o);
			shadow += lookup(vec2( 0.5, -0.5) + o);
			shadow *= 0.25 ;
		}
	
		return shadow + 0.5;
	}

	void main( void )
	{
		ShadowCoord = gl_TextureMatrix[6] * sPos;

	    float statshadowval = 1/fadeShadow * lookupStatshadow();	  

		gl_FragColor = vec4(1.0);
		gl_FragColor = (gl_FragColor * statshadowval);	   
	}
);

//MESHES
static char mesh_vertex_program[] = STRINGIFY (
	uniform vec3 lightPos;
	uniform float time;
	uniform int FOG;
	uniform mat3x4 bonemats[70];
	uniform int GPUANIM;
	uniform int useShell;

	attribute vec4 tangent;
	attribute vec4 weights;
	attribute vec4 bones;

	varying vec3 LightDir;
	varying vec3 EyeDir;
	varying float fog;

	varying vec3 vertPos, lightVec;
	varying vec3 worldNormal;

	void subScatterVS(in vec4 ecVert)
	{
		if(useShell == 0)
		{
			lightVec = lightPos - ecVert.xyz;
			vertPos = ecVert.xyz;
		}
	}

	void main()
	{
		vec3 n;
		vec3 t;
		vec3 b;

		if(GPUANIM > 0)
		{
			mat3x4 m = bonemats[int(bones.x)] * weights.x;
			m += bonemats[int(bones.y)] * weights.y;
			m += bonemats[int(bones.z)] * weights.z;
			m += bonemats[int(bones.w)] * weights.w;
			vec4 mpos = vec4(gl_Vertex * m, gl_Vertex.w);
			gl_Position = gl_ModelViewProjectionMatrix * mpos;

			subScatterVS(gl_Position);

			n = normalize(gl_NormalMatrix * (vec4(gl_Normal, 0.0) * m));

			t = normalize(gl_NormalMatrix * (vec4(tangent.xyz, 0.0) * m));

			b = normalize(gl_NormalMatrix * (vec4(tangent.w, 0.0, 0.0, 0.0) * m)) * cross(n, t); 

			EyeDir = vec3(gl_ModelViewMatrix * mpos);
		}
		else
		{	
			subScatterVS(gl_ModelViewProjectionMatrix * gl_Vertex);

			gl_Position = ftransform();

			n = normalize(gl_NormalMatrix * gl_Normal);

			t = normalize(gl_NormalMatrix * tangent.xyz);

			b = tangent.w * cross(n, t);

			EyeDir = vec3(gl_ModelViewMatrix * gl_Vertex);
		}

		worldNormal = n;
		gl_TexCoord[0] = gl_MultiTexCoord0;

		vec3 v;
		v.x = dot(lightPos, t);
		v.y = dot(lightPos, b);
		v.z = dot(lightPos, n);
		LightDir = normalize(v);

		v.x = dot(EyeDir, t);
		v.y = dot(EyeDir, b);
		v.z = dot(EyeDir, n);
		EyeDir = normalize(v);

		//for scrolling fx
		vec4 texco = gl_MultiTexCoord0;
		texco.s = texco.s + time*1.0;
		texco.t = texco.t + time*2.0;
		gl_TexCoord[1] = texco;

		if(useShell)
			gl_TexCoord[0] = texco;

		//fog
	   if(FOG > 0) 
	   {
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
	   }
	}
);

static char mesh_fragment_program[] = STRINGIFY (
	uniform sampler2D baseTex;
	uniform sampler2D normalTex;
	uniform sampler2D fxTex;
	uniform vec3 baseColor;
	uniform int GPUANIM;
	uniform int FOG;
	uniform int useFX;
	uniform int useGlow;
	uniform int useShell;
	uniform vec3 lightPos;
	const float SpecularFactor = 0.5;

	varying vec3 LightDir;
	varying vec3 EyeDir;
	varying float fog;

	//next group could be made uniforms if we want to control this 
	const float MaterialThickness = 2.0; //this val seems good for now
	const vec3 ExtinctionCoefficient = vec3(0.80, 0.12, 0.20); //controls subsurface value
	const float RimScalar = 10.0; //intensity of the rim effect

	varying vec3 vertPos, lightVec, worldNormal; 

	float halfLambert(in vec3 vect1, in vec3 vect2)
	{
		float product = dot(vect1,vect2);
		return product * 0.5 + 0.5;
	}

	float blinnPhongSpecular(in vec3 normalVec, in vec3 lightVec, in float specPower)
	{
		vec3 halfAngle = normalize(normalVec + lightVec);
		return pow(clamp(0.0,1.0,dot(normalVec,halfAngle)),specPower);
	}

	void main()
	{
		vec3 litColor;
		vec4 fx;
		vec4 glow;
		vec4 scatterCol = vec4(0.0);

		vec3 textureColour = texture2D( baseTex, gl_TexCoord[0].xy ).rgb;
		vec3 normal = 2.0 * ( texture2D( normalTex, gl_TexCoord[0].xy).xyz - vec3( 0.5 ) );

		vec4 alphamask = texture2D( baseTex, gl_TexCoord[0].xy);
		vec4 specmask = texture2D( normalTex, gl_TexCoord[0].xy);

		if(useShell == 0)
		{
			vec4 SpecColor = vec4(baseColor, 1.0)/2.0;

			float attenuation = 2.0 * (1.0 / distance(lightPos, vertPos)); 
			vec3 wNorm = worldNormal;
			vec3 eVec = EyeDir;
			vec3 lVec = normalize(lightVec);

			vec4 dotLN = vec4(halfLambert(lVec, wNorm) * attenuation);

			vec3 indirectLightComponent = vec3(MaterialThickness * max(0.0,dot(-wNorm, lVec)));
			indirectLightComponent += MaterialThickness * halfLambert(-eVec, lVec);
			indirectLightComponent *= attenuation;
			indirectLightComponent.r *= ExtinctionCoefficient.r;
			indirectLightComponent.g *= ExtinctionCoefficient.g;
			indirectLightComponent.b *= ExtinctionCoefficient.b;

			vec3 rim = vec3(1.0 - max(0.0,dot(wNorm, eVec)));
			rim *= rim;
			rim *= max(0.0,dot(wNorm, lVec)) * SpecColor.rgb;

			scatterCol = dotLN + vec4(indirectLightComponent, 1.0);
			scatterCol.rgb += (rim * RimScalar * attenuation * scatterCol.a);
			scatterCol.rgb += vec3(blinnPhongSpecular(wNorm, lVec, SpecularFactor*2.0) * attenuation * SpecColor * scatterCol.a * 0.05);
			scatterCol.rgb *= baseColor;
			scatterCol.rgb /= (specmask.a*specmask.a);//we use the spec mask for scatter mask, presuming non-spec areas are always soft/skin
		}

		//moving fx texture
		if(useFX > 0)
			fx = texture2D( fxTex, gl_TexCoord[1].xy );
		else
			fx = vec4(0.0, 0.0, 0.0, 0.0);

		//glowing fx texture
		if(useGlow > 0)
			glow = texture2D(fxTex, gl_TexCoord[0].xy );

		litColor = textureColour * max(dot(normal, LightDir), 0.0);
		vec3 reflectDir = reflect(LightDir, normal);

		float spec = max(dot(EyeDir, reflectDir), 0.0);
		if(GPUANIM > 0)
			spec = pow(spec, 4.0);
		else
			spec = pow(spec, 6.0);
		spec *= (SpecularFactor*specmask.a);
		litColor = min(litColor + spec, vec3(1.0));

		//keep shadows from making meshes completely black
		litColor = max(litColor, (textureColour * vec3(0.15)));

		gl_FragColor = vec4(litColor * baseColor, 1.0);

	//	gl_FragColor = scatterCol;\n" //for testing the subsurface scattering effect alone
		gl_FragColor = mix(fx, gl_FragColor + scatterCol, alphamask.a);

		if(useGlow > 0)
			gl_FragColor = mix(gl_FragColor, glow, glow.a);

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

//GLASS 
static char glass_vertex_program[] = STRINGIFY (
	uniform mat3x4 bonemats[70];
	uniform int FOG;

	attribute vec4 weights;
	attribute vec4 bones;

	varying vec3 r;
	varying float fog;
	varying vec3 normal, vert;

	void main(void)
	{
		mat3x4 m = bonemats[int(bones.x)] * weights.x;
		m += bonemats[int(bones.y)] * weights.y;
		m += bonemats[int(bones.z)] * weights.z;
		m += bonemats[int(bones.w)] * weights.w;
		vec4 mpos = vec4(gl_Vertex * m, gl_Vertex.w);
		gl_Position = gl_ModelViewProjectionMatrix * mpos;

		vec3 u = normalize( vec3(gl_ModelViewMatrix * mpos) ); 	
		vec3 n = normalize(gl_NormalMatrix * (vec4(gl_Normal, 0.0) * m)); 

		r = reflect( u, n );

		normal = n;
		vert = vec3( gl_ModelViewMatrix * mpos );

		//fog
	   if(FOG > 0) 
	   {
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
	   }
	}
);

static char glass_fragment_program[] = STRINGIFY (
	uniform vec3 LightPos;
	uniform sampler2D refTexture;
	uniform sampler2D mirTexture;
	uniform int FOG;

	varying vec3 r;
	varying float fog;
	varying vec3 normal, vert;

	void main (void)
	{
		vec3 light_dir = normalize( LightPos - vert );  	
		vec3 eye_dir = normalize( -vert.xyz );  	
		vec3 ref = normalize( -reflect( light_dir, normal ) );  
	
		vec4 ld = max( dot(normal, light_dir), 0.0 ); 	
		vec4 ls = 0.75 * pow( max( dot(ref, eye_dir), 0.0 ), 0.70 ); //0.75 specular, .7 shininess

		float m = -1.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) ); 	
		vec2 coord = -vec2(r.x/m + 0.5, r.y/m + 0.5); 	

		vec4 t0 = texture2D(refTexture, coord.st);
		vec4 t1 = texture2D(mirTexture, coord.st);

		gl_FragColor = 0.3 * t0 + 0.3 * t1 * (ld + ls);

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

//NO TEXTURE 
static char blankmesh_vertex_program[] = STRINGIFY (
	uniform mat3x4 bonemats[70];

	attribute vec4 weights;
	attribute vec4 bones;

	void main(void)
	{
		mat3x4 m = bonemats[int(bones.x)] * weights.x;
		m += bonemats[int(bones.y)] * weights.y;
		m += bonemats[int(bones.z)] * weights.z;
		m += bonemats[int(bones.w)] * weights.w;
		vec4 mpos = vec4(gl_Vertex * m, gl_Vertex.w);
		gl_Position = gl_ModelViewProjectionMatrix * mpos;		
	}
);

static char blankmesh_fragment_program[] = STRINGIFY (
	void main (void)
	{		
		gl_FragColor = vec4(1.0);
	}
);


static char water_vertex_program[] =
"const float Eta = 0.66;\n"
"const float FresnelPower = 5.0;\n"

"const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));\n"
"varying float FresRatio;\n"

"varying vec3 lightDir;\n"
"varying vec3 eyeDir;\n"
"varying vec3 reflectDir;\n"
"varying vec3 refractDir;\n"

"varying float fog;\n"

"uniform vec3 stangent;\n"
"uniform vec3 LightPos;\n"
"uniform float time;\n"
"uniform int	REFLECT;\n"
"uniform int FOG;\n"

"void main(void)\n"
"{\n"

"	vec4 v = vec4(gl_Vertex);\n"
"	gl_Position = ftransform();\n"

"	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);\n"
"	vec3 tang = normalize(gl_NormalMatrix * stangent);\n"
"	vec3 binorm = cross(norm,tang);\n"

"	eyeDir = vec3(gl_ModelViewMatrix * v);\n"

"	//for refraction\n"
"	vec4 neyeDir = gl_ModelViewMatrix * v;\n"
"	vec3 refeyeDir = neyeDir.xyz / neyeDir.w;\n"
"	refeyeDir = normalize(refeyeDir);\n"

"	refeyeDir.x *= -1.0;\n"
"	refeyeDir.y *= -1.0;\n"
"	refeyeDir.z *= -1.0;\n"

"	reflectDir = reflect(eyeDir,norm);\n"
"	refractDir = refract(eyeDir,norm,Eta);\n"
"	refractDir = vec3(gl_TextureMatrix[0] * vec4(refractDir,1.0));\n"
"	FresRatio = F + (1.0-F) * pow((1.0-dot(-refeyeDir,norm)),FresnelPower);\n"

"	vec3 tmp;\n"
"	tmp.x = dot(LightPos,tang);\n"
"	tmp.y = dot(LightPos,binorm);\n"
"	tmp.z = dot(LightPos,norm);\n"
"	lightDir = normalize(tmp);\n"

"	tmp.x = dot(eyeDir,tang);\n"
"	tmp.y = dot(eyeDir,binorm);\n"
"	tmp.z = dot(eyeDir,norm);\n"
"	eyeDir = normalize(tmp);\n"

"	vec4 texco = gl_MultiTexCoord0;\n"
"	if(REFLECT > 0) {\n"
"		texco.s = texco.s - LightPos.x/256.0;\n"
"		texco.t = texco.t + LightPos.y/256.0;\n"
"	}\n"

"	gl_TexCoord[0] = texco;\n"

"	texco = gl_MultiTexCoord0;\n"
"	texco.s = texco.s + time*0.05;\n"
"	texco.t = texco.t + time*0.05;\n"

"	gl_TexCoord[1] = texco;\n"

"	texco = gl_MultiTexCoord0;\n"
"	texco.s = texco.s + -time*0.05;\n"
"	texco.t = texco.t + -time*0.05;\n"
"	gl_TexCoord[2] = texco;\n"

"	//fog\n"
"   if(FOG > 0){\n"
"		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);\n"
"		fog = clamp(fog, 0.0, 1.0);\n"
"  	}\n"
"}\n";

static char water_fragment_program[] =
"varying vec3 lightDir;\n"
"varying vec3 eyeDir;\n"
"varying float FresRatio;\n"

"varying float fog;\n"

"uniform sampler2D refTexture;\n"
"uniform sampler2D normalMap;\n"
"uniform sampler2D baseTexture;\n"

"uniform int REFLECT;\n"
"uniform int TRANSPARENT;\n"
"uniform int FOG;\n"

"void main (void)\n"
"{\n"
"	vec4 refColor;\n"

"	float distSqr = dot(lightDir, lightDir);\n"
"	float att = clamp(1.0 - 0.0 * sqrt(distSqr), 0.0, 1.0);\n"
"	vec3 lVec = lightDir * inversesqrt(distSqr);\n"

"	vec3 vVec = normalize(eyeDir);\n"

"	vec4 base = vec4(0.15,0.67,0.93,1.0); //base water color\n"
"	if(REFLECT > 0)\n"
"		refColor = mix(base, texture2D(refTexture, gl_TexCoord[0].xy), 1.0);\n"
"	else\n"
"		refColor = mix(base, texture2D(baseTexture, gl_TexCoord[0].xy), 1.0);\n"

"	vec3 bump = normalize( texture2D(normalMap, gl_TexCoord[1].xy).xyz * 2.0 - 1.0);\n"
"	vec3 secbump = normalize( texture2D(normalMap, gl_TexCoord[2].xy).xyz * 2.0 - 1.0);\n"
"	vec3 modbump = mix(secbump,bump,0.5);\n"

"	vec3 reflection = reflect(eyeDir,modbump);\n"
"	vec3 refraction = refract(eyeDir,modbump,0.66);\n"

"	vec4 Tl = texture2DProj(baseTexture, vec4(reflection.xy, 1.0, 1.0) );\n"
"   vec4 Tr = texture2DProj(baseTexture, vec4(refraction.xy, 1.0, 1.0) );\n"

"	vec4 cubemap = mix(Tl,Tr,FresRatio);\n"

"	gl_FragColor = mix(cubemap,refColor,0.5);\n"
"	if(TRANSPARENT > 0)\n"
"		gl_FragColor.a = 0.5;\n"

"	if(FOG > 0)\n"
"		gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);\n"

"}\n";

//FRAMEBUFFER DISTORTION EFFECTS
static char fb_vertex_program[] =
"void main( void )\n"
"{\n"
"    gl_Position = ftransform();\n"

"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"}\n";

static char fb_fragment_program[] =
"uniform sampler2D fbtexture;\n"
"uniform sampler2D distortiontexture;\n"
"uniform vec2 dParams;\n"
"uniform vec2 fxPos;\n"

"void main(void)\n"
"{\n"
"	vec3 noiseVec;\n"
"	vec2 displacement;\n"
"	float wScissor;\n"
"	float hScissor;\n"

"   displacement = gl_TexCoord[0].st;\n"

"	displacement.x -= fxPos.x;\n"
"	displacement.y -= fxPos.y;\n"

"	noiseVec = normalize(texture2D(distortiontexture, displacement.xy)).xyz;\n"
"	noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;\n"

"	//clamp edges to prevent artifacts\n"

"	wScissor = dParams.x - 0.008;\n"
"	hScissor = dParams.y - 0.008;\n"

"	if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < wScissor)\n"
"		displacement.x = gl_TexCoord[0].s + noiseVec.x;\n"
"	else\n"
"		displacement.x = gl_TexCoord[0].s;\n"

"	if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < hScissor) \n"
"		displacement.y = gl_TexCoord[0].t + noiseVec.y;\n"
"	else\n"
"		displacement.y = gl_TexCoord[0].t;\n"

"	gl_FragColor = texture2D(fbtexture, displacement.xy);\n"
"}\n";

//GAUSSIAN BLUR EFFECTS
static char blur_vertex_program[] =
"void main()\n"
"{\n"
"	gl_Position = ftransform();\n"
"	gl_TexCoord[0] =  gl_MultiTexCoord0;\n"
"}\n";

static char blur_fragment_program[] =
"uniform vec2 ScaleU;\n"
"uniform sampler2D textureSource;\n"

"void main()\n"
"{\n"
"   vec4 sum = vec4(0.0);\n"

"   // take nine samples\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - 4.0*ScaleU.x, gl_TexCoord[0].y - 4.0*ScaleU.y)) * 0.05;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - 3.0*ScaleU.x, gl_TexCoord[0].y - 3.0*ScaleU.y)) * 0.09;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - 2.0*ScaleU.x, gl_TexCoord[0].y - 2.0*ScaleU.y)) * 0.12;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x - ScaleU.x, gl_TexCoord[0].y - ScaleU.y)) * 0.15;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y)) * 0.16;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + ScaleU.x, gl_TexCoord[0].y + ScaleU.y)) * 0.15;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + 2.0*ScaleU.x, gl_TexCoord[0].y + 2.0*ScaleU.y)) * 0.12;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + 3.0*ScaleU.x, gl_TexCoord[0].y + 3.0*ScaleU.y)) * 0.09;\n"
"   sum += texture2D(textureSource, vec2(gl_TexCoord[0].x + 4.0*ScaleU.x, gl_TexCoord[0].y + 4.0*ScaleU.y)) * 0.05;\n"

"   gl_FragColor = sum;\n"
"}\n";

//RADIAL BLUR EFFECTS // xy = radial center screen space position, z = radius attenuation, w = blur strength
static char rblur_vertex_program[] =
"void main()\n"
"{\n"
"	gl_Position = ftransform();\n"
"	gl_TexCoord[0] =  gl_MultiTexCoord0;\n"
"}\n";

static char rblur_fragment_program[] =
"uniform sampler2D rtextureSource;\n"
"uniform vec3 radialBlurParams;\n" 
"uniform vec3 rblurScale;\n"

"void main(void)\n"
"{\n"
"	 float wScissor;\n"
"	 float hScissor;\n"

"    vec2 dir = vec2(radialBlurParams.x - gl_TexCoord[0].x, radialBlurParams.x - gl_TexCoord[0].x);\n" 
"    float dist = sqrt(dir.x*dir.x + dir.y*dir.y);\n" 
"  	 dir = dir/dist;\n" 
"    vec4 color = texture2D(rtextureSource,gl_TexCoord[0].xy);\n" 
"    vec4 sum = color;\n"

"	 float strength = radialBlurParams.z;\n"
"	 vec2 pDir = vec2(rblurScale.y * 0.5 - gl_TexCoord[0].s, rblurScale.z * 0.5 - gl_TexCoord[0].t);\n" 
"    float pDist = sqrt(pDir.x*pDir.x + pDir.y*pDir.y);\n"
"	 clamp(pDist, 0.0, 1.0);\n"

"	 //the following ugliness is due to ATI's drivers inablity to handle a simple for-loop!\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.06 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.05 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.03 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.02 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.01 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.01 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.02 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.03 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.05 * strength * pDist );\n"
"    sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.06 * strength * pDist );\n"

"    sum *= 1.0/11.0;\n"
 
"    float t = dist * rblurScale.x;\n"
"    t = clamp( t ,0.0,1.0);\n" 

"    vec4 final = mix( color, sum, t );\n"

"	//clamp edges to prevent artifacts\n"

"	wScissor = rblurScale.y - 0.008;\n"
"	hScissor = rblurScale.z - 0.008;\n"

"	if(gl_TexCoord[0].s > 0.008 && gl_TexCoord[0].s < wScissor && gl_TexCoord[0].t > 0.008 && gl_TexCoord[0].t < hScissor)\n"
"		gl_FragColor = final;\n"
"	else\n"
"		gl_FragColor = color;\n"
"}\n";

//WATER DROPLETS
static char droplets_vertex_program[] =
"uniform float drTime;\n"

"void main( void )\n"
"{\n"
"    gl_Position = ftransform();\n"

"	 //for vertical scrolling\n"
"	 vec4 texco = gl_MultiTexCoord0;\n"
"	 texco.t = texco.t + drTime*1.0;\n"
"	 gl_TexCoord[1] = texco;\n"

"	 texco = gl_MultiTexCoord0;\n"
"	 texco.t = texco.t + drTime*0.8;\n"
"	 gl_TexCoord[2] = texco;\n"

"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"}\n";

static char droplets_fragment_program[] =
"uniform sampler2D drSource;\n"
"uniform sampler2D drTex;\n"
"uniform vec2 drParams;\n"

"void main(void)\n"
"{\n"
"	vec3 noiseVec;\n"
"	vec3 noiseVec2;\n"
"	vec2 displacement;\n"
"	float wScissor;\n"
"	float hScissor;\n"

"   displacement = gl_TexCoord[1].st;\n"

"	noiseVec = normalize(texture2D(drTex, displacement.xy)).xyz;\n"
"	noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;\n"

"   displacement = gl_TexCoord[2].st;\n"

"	noiseVec2 = normalize(texture2D(drTex, displacement.xy)).xyz;\n"
"	noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;\n"

"	//clamp edges to prevent artifacts\n"

"	wScissor = drParams.x - 0.008;\n"
"	hScissor = drParams.y - 0.028;\n"

"	if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < wScissor)\n"
"		displacement.x = gl_TexCoord[0].s + noiseVec.x + noiseVec2.x;\n"
"	else\n"
"		displacement.x = gl_TexCoord[0].s;\n"

"	if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < hScissor) \n"
"		displacement.y = gl_TexCoord[0].t + noiseVec.y + noiseVec2.y;\n"
"	else\n"
"		displacement.y = gl_TexCoord[0].t;\n"

"	gl_FragColor = texture2D(drSource, displacement.xy);\n"
"}\n";

static char rgodrays_vertex_program[] =
"void main()\n"
"{\n"
"	gl_TexCoord[0] =  gl_MultiTexCoord0;\n"
"	gl_Position = ftransform();\n"
"}\n";

static char rgodrays_fragment_program[] =

"uniform vec2 lightPositionOnScreen;\n"
"uniform sampler2D sunTexture;\n"

//note - these could be made uniforms to control externally
"const float exposure = 0.0034;\n"
"const float decay = 1.0;\n"
"const float density = 0.84;\n"
"const float weight = 5.65;\n"
"const int NUM_SAMPLES = 75;\n" 

"void main()\n"
"{\n"	
"	vec2 deltaTextCoord = vec2( gl_TexCoord[0].st - lightPositionOnScreen.xy );\n"
"	vec2 textCoo = gl_TexCoord[0].st;\n"
"	deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;\n"
"	float illuminationDecay = 1.0;\n"

"	for(int i = 0; i < NUM_SAMPLES; i++)\n"
"	{\n"
"		textCoo -= deltaTextCoord;\n"
"		vec4 sample = texture2D(sunTexture, textCoo );\n"
			
"		sample *= illuminationDecay * weight;\n"
			
"		gl_FragColor += sample;\n"
			
"		illuminationDecay *= decay;\n"
"	}\n"
	
"	gl_FragColor *= exposure;\n"
"}\n";

void R_LoadGLSLPrograms(void)
{
	const char *shaderStrings[1];
    int		nResult;
    char	str[4096];

	//load glsl (to do - move to own file)
	if (strstr(gl_config.extensions_string,  "GL_ARB_shader_objects" ) && gl_state.fragment_program)
	{
		gl_state.glsl_shaders = true;

		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC)qwglGetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)qwglGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC)qwglGetProcAddress("glShaderSourceARB");
		glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC)qwglGetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)qwglGetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC)qwglGetProcAddress("glAttachObjectARB");
		glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC)qwglGetProcAddress("glGetInfoLogARB");
		glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC)qwglGetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)qwglGetProcAddress("glGetUniformLocationARB");
		glUniform3fARB            = (PFNGLUNIFORM3FARBPROC)qwglGetProcAddress("glUniform3fARB");
		glUniform2fARB            = (PFNGLUNIFORM2FARBPROC)qwglGetProcAddress("glUniform2fARB");
		glUniform1iARB            = (PFNGLUNIFORM1IARBPROC)qwglGetProcAddress("glUniform1iARB");
		glUniform1fARB		  = (PFNGLUNIFORM1FARBPROC)qwglGetProcAddress("glUniform1fARB");
		glUniformMatrix3fvARB	  = (PFNGLUNIFORMMATRIX3FVARBPROC)qwglGetProcAddress("glUniformMatrix3fvARB");
		glUniformMatrix3x4fvARB	  = (PFNGLUNIFORMMATRIX3X4FVARBPROC)qwglGetProcAddress("glUniformMatrix3x4fv");
		glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)qwglGetProcAddress("glVertexAttribPointerARB");
		glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glEnableVertexAttribArrayARB");
		glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glDisableVertexAttribArrayARB");
		glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)qwglGetProcAddress("glBindAttribLocationARB");

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
		    !glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
		    !glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
		    !glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform1iARB || !glUniform1fARB || !glUniformMatrix3fvARB ||
				!glUniformMatrix3x4fvARB || !glVertexAttribPointerARB 
				|| !glEnableVertexAttribArrayARB ||
				!glBindAttribLocationARB)
		{
			Com_Printf("...One or more GL_ARB_shader_objects functions were not found\n");
			gl_state.glsl_shaders = false;
		}
	}
	else
	{
		Com_Printf("...One or more GL_ARB_shader_objects functions were not found\n");
		gl_state.glsl_shaders = false;
	}

	if(gl_state.glsl_shaders)
	{
		//we have them, set defaults accordingly
		gl_glsl_shaders = Cvar_Get("gl_glsl_shaders", "1", CVAR_ARCHIVE);
		gl_dynamic = Cvar_Get ("gl_dynamic", "1", CVAR_ARCHIVE);

		//standard bsp surfaces

		g_programObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)bsp_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_programObj, g_vertexShader );
		else
		{
			Com_Printf("...Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)bsp_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_programObj, g_fragmentShader );
		else
		{
			Com_Printf("...Fragment Shader Compile Error\n");
		}

		glLinkProgramARB( g_programObj );
		glGetObjectParameterivARB( g_programObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_programObj, sizeof(str), NULL, str );
			Com_Printf("...BSP Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_surfTexture = glGetUniformLocationARB( g_programObj, "surfTexture" );
		g_location_eyePos = glGetUniformLocationARB( g_programObj, "Eye" );
		g_tangentSpaceTransform = glGetUniformLocationARB( g_programObj, "tangentSpaceTransform");
		g_location_heightTexture = glGetUniformLocationARB( g_programObj, "HeightTexture" );
		g_location_lmTexture = glGetUniformLocationARB( g_programObj, "lmTexture" );
		g_location_normalTexture = glGetUniformLocationARB( g_programObj, "NormalTexture" );
		g_location_bspShadowmapTexture = glGetUniformLocationARB( g_programObj, "ShadowMap" );
		g_location_bspShadowmapTexture2 = glGetUniformLocationARB( g_programObj, "StatShadowMap" );
		g_location_fog = glGetUniformLocationARB( g_programObj, "FOG" );
		g_location_parallax = glGetUniformLocationARB( g_programObj, "PARALLAX" );
		g_location_dynamic = glGetUniformLocationARB( g_programObj, "DYNAMIC" );
		g_location_shadowmap = glGetUniformLocationARB( g_programObj, "SHADOWMAP" );
		g_Location_statshadow = glGetUniformLocationARB( g_programObj, "STATSHADOW" );
		g_location_xOffs = glGetUniformLocationARB( g_programObj, "xPixelOffset" );
		g_location_yOffs = glGetUniformLocationARB( g_programObj, "yPixelOffset" );
		g_location_lightPosition = glGetUniformLocationARB( g_programObj, "lightPosition" );
		g_location_staticLightPosition = glGetUniformLocationARB( g_programObj, "staticLightPosition" );
		g_location_lightColour = glGetUniformLocationARB( g_programObj, "lightColour" );
		g_location_lightCutoffSquared = glGetUniformLocationARB( g_programObj, "lightCutoffSquared" );
		g_location_liquid = glGetUniformLocationARB( g_programObj, "LIQUID" );
		g_location_rsTime = glGetUniformLocationARB( g_programObj, "rsTime" );
		g_location_liquidTexture = glGetUniformLocationARB( g_programObj, "liquidTexture" );
		g_location_liquidNormTex = glGetUniformLocationARB( g_programObj, "liquidNormTex" );

		//shadowed white bsp surfaces

		g_shadowprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)shadow_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_shadowprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Shadow Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)shadow_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_shadowprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Shadow Fragment Shader Compile Error\n");
		}

		glLinkProgramARB( g_shadowprogramObj );
		glGetObjectParameterivARB( g_shadowprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_shadowprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Shadow Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_entShadow = glGetUniformLocationARB( g_shadowprogramObj, "StatShadowMap" );
		g_location_fadeShadow = glGetUniformLocationARB( g_shadowprogramObj, "fadeShadow" );
		g_location_xOffset = glGetUniformLocationARB( g_shadowprogramObj, "xPixelOffset" );
		g_location_yOffset = glGetUniformLocationARB( g_shadowprogramObj, "yPixelOffset" );

		//warp(water) bsp surfaces

		g_waterprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)water_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_waterprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Water Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)water_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_waterprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Water Fragment Shader Compile Error\n");
		}

		glLinkProgramARB( g_waterprogramObj );
		glGetObjectParameterivARB( g_waterprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_waterprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Water Surface Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_baseTexture = glGetUniformLocationARB( g_waterprogramObj, "baseTexture" );
		g_location_normTexture = glGetUniformLocationARB( g_waterprogramObj, "normalMap" );
		g_location_refTexture = glGetUniformLocationARB( g_waterprogramObj, "refTexture" );
		g_location_tangent = glGetUniformLocationARB( g_waterprogramObj, "stangent" );
		g_location_time = glGetUniformLocationARB( g_waterprogramObj, "time" );
		g_location_lightPos = glGetUniformLocationARB( g_waterprogramObj, "LightPos" );
		g_location_reflect = glGetUniformLocationARB( g_waterprogramObj, "REFLECT" );
		g_location_trans = glGetUniformLocationARB( g_waterprogramObj, "TRANSPARENT" );
		g_location_fogamount = glGetUniformLocationARB( g_waterprogramObj, "FOG" );

		//meshes

		g_meshprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)mesh_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_meshprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Mesh Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)mesh_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_meshprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Mesh Fragment Shader Compile Error\n");
		}

		glBindAttribLocationARB(g_meshprogramObj, 1, "tangent");
		glBindAttribLocationARB(g_meshprogramObj, 6, "weights");
		glBindAttribLocationARB(g_meshprogramObj, 7, "bones");
		glLinkProgramARB( g_meshprogramObj );
		glGetObjectParameterivARB( g_meshprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_meshprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Mesh Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_meshlightPosition = glGetUniformLocationARB( g_meshprogramObj, "lightPos" );
		g_location_baseTex = glGetUniformLocationARB( g_meshprogramObj, "baseTex" );
		g_location_normTex = glGetUniformLocationARB( g_meshprogramObj, "normalTex" );
		g_location_fxTex = glGetUniformLocationARB( g_meshprogramObj, "fxTex" );
		g_location_color = glGetUniformLocationARB(	g_meshprogramObj, "baseColor" );
		g_location_meshNormal = glGetUniformLocationARB( g_meshprogramObj, "meshNormal" );
		g_location_meshTime = glGetUniformLocationARB( g_meshprogramObj, "time" );
		g_location_meshFog = glGetUniformLocationARB( g_meshprogramObj, "FOG" );
		g_location_useFX = glGetUniformLocationARB( g_meshprogramObj, "useFX" );
		g_location_useGlow = glGetUniformLocationARB( g_meshprogramObj, "useGlow");
		g_location_useShell = glGetUniformLocationARB( g_meshprogramObj, "useShell");
		g_location_useGPUanim = glGetUniformLocationARB( g_meshprogramObj, "GPUANIM");
		g_location_outframe = glGetUniformLocationARB( g_meshprogramObj, "bonemats");

		//Glass

		g_glassprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)glass_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_glassprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Glass Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)glass_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_glassprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Glass Fragment Shader Compile Error\n");
		}

		glBindAttribLocationARB(g_glassprogramObj, 6, "weights");
		glBindAttribLocationARB(g_glassprogramObj, 7, "bones");
		glLinkProgramARB( g_glassprogramObj );
		glGetObjectParameterivARB( g_glassprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_glassprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Glass Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_gOutframe = glGetUniformLocationARB( g_glassprogramObj, "bonemats" );
		g_location_gFog = glGetUniformLocationARB( g_glassprogramObj, "FOG" );
		g_location_gLightPos = glGetUniformLocationARB( g_glassprogramObj, "LightPos" );
		g_location_gmirTexture = glGetUniformLocationARB( g_glassprogramObj, "mirTexture" );
		g_location_grefTexture = glGetUniformLocationARB( g_glassprogramObj, "refTexture" );

		//Blank mesh (for shadowmapping efficiently)

		g_blankmeshprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)blankmesh_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_blankmeshprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Blankmesh Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)blankmesh_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_blankmeshprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Blankmesh Fragment Shader Compile Error\n");
		}

		glBindAttribLocationARB(g_blankmeshprogramObj, 6, "weights");
		glBindAttribLocationARB(g_blankmeshprogramObj, 7, "bones");
		glLinkProgramARB( g_blankmeshprogramObj );
		glGetObjectParameterivARB( g_blankmeshprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_blankmeshprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Blankmesh Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_bmOutframe = glGetUniformLocationARB( g_blankmeshprogramObj, "bonemats" );
		
		//fullscreen distortion effects

		g_fbprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)fb_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_fbprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Framebuffer Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)fb_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_fbprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Framebuffer Fragment Shader Compile Error\n");
		}

		glLinkProgramARB( g_fbprogramObj );
		glGetObjectParameterivARB( g_fbprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_fbprogramObj, sizeof(str), NULL, str );
			Com_Printf("...FBO Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_framebuffTex = glGetUniformLocationARB( g_fbprogramObj, "fbtexture" );
		g_location_distortTex = glGetUniformLocationARB( g_fbprogramObj, "distorttexture");
		g_location_dParams = glGetUniformLocationARB( g_fbprogramObj, "dParams" );
		g_location_fxPos = glGetUniformLocationARB( g_fbprogramObj, "fxPos" );

		//gaussian blur

		g_blurprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)blur_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_blurprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Blur Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)blur_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_blurprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Framebuffer Blur Shader Compile Error\n");
		}

		glLinkProgramARB( g_blurprogramObj );
		glGetObjectParameterivARB( g_blurprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_blurprogramObj, sizeof(str), NULL, str );
			Com_Printf("...FBO Blur Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
		g_location_source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");

		//radial blur

		g_rblurprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)rblur_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_rblurprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Radial Blur Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)rblur_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_rblurprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Radial Fragment Blur Shader Compile Error\n");
		}

		glLinkProgramARB( g_rblurprogramObj );
		glGetObjectParameterivARB( g_rblurprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_rblurprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Radial Blur Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_rscale = glGetUniformLocationARB( g_rblurprogramObj, "rblurScale" );
		g_location_rsource = glGetUniformLocationARB( g_rblurprogramObj, "rtextureSource");
		g_location_rparams = glGetUniformLocationARB( g_rblurprogramObj, "radialBlurParams");

		//water droplets
		g_dropletsprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)droplets_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_dropletsprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Water Droplets Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)droplets_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_dropletsprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Water Droplets Fragment Shader Compile Error\n");
		}

		glLinkProgramARB( g_dropletsprogramObj );
		glGetObjectParameterivARB( g_dropletsprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_dropletsprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Liquid Droplet Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_drSource = glGetUniformLocationARB( g_dropletsprogramObj, "drSource" );
		g_location_drTex = glGetUniformLocationARB( g_dropletsprogramObj, "drTex");
		g_location_drParams = glGetUniformLocationARB( g_dropletsprogramObj, "drParams" );
		g_location_drTime = glGetUniformLocationARB( g_dropletsprogramObj, "drTime" );

		//god rays
		g_godraysprogramObj = glCreateProgramObjectARB();

		//
		// Vertex shader
		//

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		shaderStrings[0] = (char*)rgodrays_vertex_program;
		glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_godraysprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...God Rays Vertex Shader Compile Error\n");
		}

		//
		// Fragment shader
		//

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[0] = (char*)rgodrays_fragment_program;
		glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
			glAttachObjectARB( g_godraysprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...God Rays Fragment Shader Compile Error\n");
		}

		glLinkProgramARB( g_godraysprogramObj );
		glGetObjectParameterivARB( g_godraysprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_godraysprogramObj, sizeof(str), NULL, str );
			Com_Printf("...God Ray Shader Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_lightPositionOnScreen = glGetUniformLocationARB( g_godraysprogramObj, "lightPositionOnScreen" );
		g_location_sunTex = glGetUniformLocationARB( g_godraysprogramObj, "sunTexture");
	}
	else
	{
		gl_glsl_shaders = Cvar_Get("gl_glsl_shaders", "0", CVAR_ARCHIVE);
		gl_dynamic = Cvar_Get ("gl_dynamic", "0", CVAR_ARCHIVE);
	}
}
