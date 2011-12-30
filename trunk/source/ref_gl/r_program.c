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
PFNGLVERTEXATTRIBPOINTERARBPROC     glVertexAttribPointerARB	= NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC		glBindAttribLocationARB		= NULL;

GLhandleARB g_programObj;
GLhandleARB g_waterprogramObj;
GLhandleARB g_meshprogramObj;
GLhandleARB g_fbprogramObj;
GLhandleARB g_blurprogramObj;
GLhandleARB g_rblurprogramObj;
GLhandleARB g_dropletsprogramObj;

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
GLuint	    g_location_lightPosition;
GLuint		g_location_staticLightPosition;
GLuint		g_location_lightColour;
GLuint	    g_location_lightCutoffSquared;

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
GLuint		g_location_minLight;
GLuint		g_location_meshNormal;
GLuint		g_location_meshTime;
GLuint		g_location_meshFog;
GLuint		g_location_useFX;
GLuint		g_location_useGlow;

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
static char bsp_vertex_program[] =
"uniform mat3 tangentSpaceTransform;\n"
"uniform vec3 Eye;\n"
"uniform vec3 lightPosition;\n"
"uniform vec3 staticLightPosition;\n"
"uniform int FOG;\n"

"varying vec3 EyeDir;\n"
"varying vec3 LightDir;\n"
"varying vec3 StaticLightDir;\n"
"varying vec4 sPos;\n"
"varying float fog;\n"

"void main( void )\n"
"{\n"
"   sPos = gl_Vertex;\n"

"   gl_Position = ftransform();\n"

"   gl_FrontColor = gl_Color;\n"

"   EyeDir = tangentSpaceTransform * ( Eye - gl_Vertex.xyz );\n"
"   LightDir = tangentSpaceTransform * (lightPosition - gl_Vertex.xyz);\n"
"   StaticLightDir = tangentSpaceTransform * (staticLightPosition - gl_Vertex.xyz);\n"

"   // pass any active texunits through\n"
"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"    gl_TexCoord[1] = gl_MultiTexCoord1;\n"

"    //fog\n"
"    if(FOG > 0){\n"
"      fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);\n"
"      fog = clamp(fog, 0.0, 1.0);\n"
"    }\n"
"}\n";

static char bsp_fragment_program[] =
"uniform sampler2D surfTexture;\n"
"uniform sampler2D HeightTexture;\n"
"uniform sampler2D NormalTexture;\n"
"uniform sampler2D lmTexture;\n"
"uniform sampler2D ShadowMap;\n"
"uniform sampler2D StatShadowMap;\n"
"uniform vec3 lightColour;\n"
"uniform float lightCutoffSquared;\n"
"uniform int FOG;\n"
"uniform int PARALLAX;\n"
"uniform int DYNAMIC;\n"
"uniform int STATSHADOW;\n"
"uniform int SHADOWMAP;\n"

"varying vec4 sPos;\n"
"varying vec3 EyeDir;\n"
"varying vec3 LightDir;\n"
"varying vec3 StaticLightDir;\n"
"varying float fog;\n"

"float lookupDynshadow( void )\n"
"{\n"
"	vec4 ShadowCoord;\n"
"	vec4 shadowCoordinateWdivide;\n"
"	float distanceFromLight;\n"
"    	vec4 tempShadowCoord;\n"
"    	float shadow1 = 1.0;\n"
"    	float shadow2 = 1.0;\n"
"    	float shadow3 = 1.0;\n"
"    	float shadows = 1.0;\n"

"	if(SHADOWMAP > 0) {\n"

"	   ShadowCoord = gl_TextureMatrix[7] * sPos;\n"

"      shadowCoordinateWdivide = ShadowCoord / ShadowCoord.w ;\n"
"      // Used to lower moir pattern and self-shadowing\n"
"      shadowCoordinateWdivide.z += 0.0005;\n"

"      distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.xy).z;\n"

"      if (ShadowCoord.w > 0.0)\n"
"		shadow1 = distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0 ;\n"

"	   //Blur shadows a bit\n"
"      tempShadowCoord = ShadowCoord + vec4(.2, 0, 0, 0);\n"
"      shadowCoordinateWdivide = tempShadowCoord / tempShadowCoord.w ;\n"
"      shadowCoordinateWdivide.z += 0.0005;\n"

"      distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.xy).z;\n"

"      if (ShadowCoord.w > 0.0)\n"
"          shadow2 = distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0 ;\n"

"      tempShadowCoord = ShadowCoord + vec4(0, .2, 0, 0);\n"
"      shadowCoordinateWdivide = tempShadowCoord / tempShadowCoord.w ;\n"
"      shadowCoordinateWdivide.z += 0.0005;\n"

"      distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.xy).z;\n"

"      if (ShadowCoord.w > 0.0)\n"
"          shadow3 = distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0 ;\n"

"      shadows = 0.33 * (shadow1 + shadow2 + shadow3);\n"

"    }\n"

"    return shadows;\n"
"}\n"

"float lookupStatshadow( void )\n"
"{\n"
"	vec4 ShadowCoord;\n"
"	vec4 shadowCoordinateWdivide;\n"
"	float distanceFromLight;\n"
"    	vec4 tempShadowCoord;\n"
"    	float shadow1 = 1.0;\n"
"    	float shadow2 = 1.0;\n"
"    	float shadow3 = 1.0;\n"
"    	float shadows = 1.0;\n"

"	if(SHADOWMAP > 0) {\n"

"      ShadowCoord = gl_TextureMatrix[6] * sPos;\n"

"      shadowCoordinateWdivide = ShadowCoord / ShadowCoord.w ;\n"
"      // Used to lower moir pattern and self-shadowing\n"
"      shadowCoordinateWdivide.z += 0.0005;\n"

"      distanceFromLight = texture2D(StatShadowMap,shadowCoordinateWdivide.xy).z;\n"

"      if (ShadowCoord.w > 0.0)\n"
"		shadow1 = distanceFromLight < shadowCoordinateWdivide.z ? 0.5 : 1.0 ;\n"

"	   //Blur shadows a bit\n"
"      tempShadowCoord = ShadowCoord + vec4(.2, 0, 0, 0);\n"
"      shadowCoordinateWdivide = tempShadowCoord / tempShadowCoord.w ;\n"
"      shadowCoordinateWdivide.z += 0.0005;\n"

"      distanceFromLight = texture2D(StatShadowMap,shadowCoordinateWdivide.xy).z;\n"

"      if (ShadowCoord.w > 0.0)\n"
"          shadow2 = distanceFromLight < shadowCoordinateWdivide.z ? 0.5 : 1.0 ;\n"

"      tempShadowCoord = ShadowCoord + vec4(0, .2, 0, 0);\n"
"      shadowCoordinateWdivide = tempShadowCoord / tempShadowCoord.w ;\n"
"      shadowCoordinateWdivide.z += 0.0005;\n"

"      distanceFromLight = texture2D(StatShadowMap,shadowCoordinateWdivide.xy).z;\n"

"      if (ShadowCoord.w > 0.0)\n"
"          shadow3 = distanceFromLight < shadowCoordinateWdivide.z ? 0.5 : 1.0 ;\n"

"      shadows = 0.33 * (shadow1 + shadow2 + shadow3);\n"

"    }\n"

"    return shadows;\n"
"}\n"

"void main( void )\n"
"{\n"
"   vec4 diffuse;\n"
"   vec4 lightmap;\n"
"	vec4 alphamask;\n"
"   float distanceSquared;\n"
"   vec3 relativeLightDirection;\n"
"   float diffuseTerm;\n"
"   vec3 colour;\n"
"   vec3 halfAngleVector;\n"
"   float specularTerm;\n"
"   float swamp;\n"
"   float attenuation;\n"
"   vec4 litColour;\n"
"   vec3 varyingLightColour;\n"
"   float varyingLightCutoffSquared;\n"
"   float dynshadowval;\n"
"	float statshadowval;\n"

"   varyingLightColour = lightColour;\n"
"   varyingLightCutoffSquared = lightCutoffSquared;\n"

"   vec3 relativeEyeDirection = normalize( EyeDir );\n"
"   vec3 normal = 2.0 * ( texture2D( NormalTexture, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );\n"
"   vec3 textureColour = texture2D( surfTexture, gl_TexCoord[0].xy ).rgb;\n"

"   lightmap = texture2D( lmTexture, gl_TexCoord[1].st );\n"
"	alphamask = texture2D( surfTexture, gl_TexCoord[0].xy );\n"

"   //shadows\n"
"	if(DYNAMIC > 0)\n"
"		dynshadowval = lookupDynshadow();\n"
"	else\n"
"		dynshadowval = 0.0;\n"

"	if(STATSHADOW > 0)\n"
"		statshadowval = lookupStatshadow();\n"
"	else\n"
"		statshadowval = 1.0;\n"

"   if(PARALLAX > 0) {\n"
"      //do the parallax mapping\n"
"      vec4 Offset = texture2D( HeightTexture,gl_TexCoord[0].xy );\n"
"      Offset = Offset * 0.04 - 0.02;\n"
"      vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + gl_TexCoord[0].xy;\n"

"      diffuse = texture2D( surfTexture, TexCoords );\n"

"      distanceSquared = dot( StaticLightDir, StaticLightDir );\n"
"      relativeLightDirection = StaticLightDir / sqrt( distanceSquared );\n"

"      diffuseTerm = clamp( dot( normal, relativeLightDirection ), 0.0, 1.0 );\n"
"      colour = vec3( 0.0, 0.0, 0.0 );\n"

"	   if( diffuseTerm > 0.0 )\n"
"	   {\n"
"		 halfAngleVector = normalize( relativeLightDirection + relativeEyeDirection );\n"

"        specularTerm = clamp( dot( normal, halfAngleVector ), 0.0, 1.0 );\n"
"        specularTerm = pow( specularTerm, 32.0 );\n"

"        colour = specularTerm * vec3( 1.0, 1.0, 1.0 ) / 2.0;\n"
"      }\n"
"      attenuation = 0.25;\n"
"      swamp = attenuation;\n"
"      swamp *= swamp;\n"
"      swamp *= swamp;\n"
"      swamp *= swamp;\n"

"      colour += ( ( ( 0.5 - swamp ) * diffuseTerm ) + swamp ) * textureColour * 4.0;\n"

"      litColour = vec4( attenuation * colour, 1.0 );\n"
"      litColour = litColour * lightmap * 6.0;\n"
"      gl_FragColor = max(litColour, diffuse * lightmap * 2.0);\n"
"	   gl_FragColor = (gl_FragColor * statshadowval);\n"
"   }\n"
"   else {\n"
"      diffuse = texture2D(surfTexture, gl_TexCoord[0].xy);\n"
"      gl_FragColor = (diffuse * lightmap * 2.0);\n"
"	   gl_FragColor = (gl_FragColor * statshadowval);\n"
"   }\n"

"   if(DYNAMIC > 0) {\n"

"	  lightmap = texture2D(lmTexture, gl_TexCoord[1].st);\n"

"      //now do the dynamic lighting\n"
"      distanceSquared = dot( LightDir, LightDir );\n"
"      relativeLightDirection = LightDir / sqrt( distanceSquared );\n"

"      diffuseTerm = clamp( dot( normal, relativeLightDirection ), 0.0, 1.0 );\n"
"      colour = vec3( 0.0, 0.0, 0.0 );\n"

"      if( diffuseTerm > 0.0 )\n"
"      {\n"
"         halfAngleVector = normalize( relativeLightDirection + relativeEyeDirection );\n"

"         float specularTerm = clamp( dot( normal, halfAngleVector ), 0.0, 1.0 );\n"
"         specularTerm = pow( specularTerm, 32.0 );\n"

"         colour = specularTerm * vec3( 1.0, 1.0, 1.0 ) / 2.0;\n"
"      }\n"

"      attenuation = clamp( 1.0 - ( distanceSquared / varyingLightCutoffSquared ), 0.0, 1.0 );\n"

"      swamp = attenuation;\n"
"      swamp *= swamp;\n"
"      swamp *= swamp;\n"
"      swamp *= swamp;\n"

"      colour += ( ( ( 0.5 - swamp ) * diffuseTerm ) + swamp ) * textureColour * 3.0;\n"

"      vec4 dynamicColour = vec4( attenuation * colour * dynshadowval * varyingLightColour, 1.0 );\n"
"      if(PARALLAX > 0) {\n"
"         dynamicColour = max(dynamicColour, gl_FragColor);\n"
"      }\n"
"      else {\n"
"         dynamicColour = max(dynamicColour, vec4(textureColour, 1.0) * lightmap * 2.0);\n"
"      }\n"
"      gl_FragColor = dynamicColour;\n"
"   }\n"
"	gl_FragColor = mix(vec4(0.0, 0.0, 0.0, alphamask.a), gl_FragColor, alphamask.a);\n"
"   if(FOG > 0)\n"
"      gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);\n"
"}\n";

//MESHES
static char mesh_vertex_program[] =
"uniform vec3 lightPos;\n"
"uniform float time;\n"
"uniform int FOG;\n"

"attribute vec4 tangent;\n"

"varying vec3 LightDir;\n"
"varying vec3 EyeDir;\n"
"varying float fog;\n"

"void main()\n"
"{\n"
"	vec3 t;\n"
"	vec3 b;\n"

"	gl_Position = ftransform();\n"

"	EyeDir = vec3(gl_ModelViewMatrix * gl_Vertex);\n"
"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"

"	vec3 n = normalize(gl_NormalMatrix * gl_Normal);\n"

"	vec3 tangent3 = vec3(tangent);\n"
"	t = normalize(gl_NormalMatrix * tangent3);\n"
"	b = tangent[3] * cross(n, t);\n"

"	vec3 v;\n"
"	v.x = dot(lightPos, t);\n"
"	v.y = dot(lightPos, b);\n"
"	v.z = dot(lightPos, n);\n"
"	LightDir = normalize(v);\n"

"	v.x = dot(EyeDir, t);\n"
"	v.y = dot(EyeDir, b);\n"
"	v.z = dot(EyeDir, n);\n"
"	EyeDir = normalize(v);\n"

"	//for scrolling fx\n"
"	vec4 texco = gl_MultiTexCoord0;\n"
"	texco.s = texco.s + time*1.0;\n"
"	texco.t = texco.t + time*1.0;\n"
"	gl_TexCoord[1] = texco;\n"

"	//fog\n"
"   if(FOG > 0){\n"
"		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);\n"
"		fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear\n"
"   }\n"
"}\n";

static char mesh_fragment_program[] =
"uniform sampler2D baseTex;\n"
"uniform sampler2D normalTex;\n"
"uniform sampler2D fxTex;\n"
"uniform vec3 baseColor;\n"
"uniform int FOG;\n"
"uniform int useFX;\n"
"uniform int useGlow;\n"
"uniform float minLight;\n"
"const float SpecularFactor = 0.5;\n"

"varying vec3 LightDir;\n"
"varying vec3 EyeDir;\n"
"varying float fog;\n"

"void main()\n"
"{\n"
"	vec3 litColor;\n"
"	vec4 fx;\n"
"	vec4 glow;\n"

"   vec3 textureColour = texture2D( baseTex, gl_TexCoord[0].xy ).rgb;\n"
"   vec3 normal = 2.0 * ( texture2D( normalTex, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );\n"

"	vec4 alphamask = texture2D( baseTex, gl_TexCoord[0].xy);\n"
"	vec4 specmask = texture2D( normalTex, gl_TexCoord[0].xy);\n"

"	//moving fx texture\n"
"	if(useFX > 0)\n"
"		fx = texture2D( fxTex, gl_TexCoord[1].xy );\n"
"	else\n"
"		fx = vec4(0.0, 0.0, 0.0, 0.0);\n"

"	//glowing fx texture\n"
"	if(useGlow > 0)\n"
"		glow = texture2D(fxTex, gl_TexCoord[0].xy );\n"

"	litColor = textureColour * max(dot(normal, LightDir), 0.0);\n"
"	vec3 reflectDir = reflect(LightDir, normal);\n"

"	float spec = max(dot(EyeDir, reflectDir), 0.0);\n"
"	spec = pow(spec, 6.0);\n"
"	spec *= (SpecularFactor*specmask.a);\n"
"	litColor = min(litColor + spec, vec3(1.0));\n"

"	//keep shadows from making meshes completely black\n"
"	litColor = max(litColor, (textureColour * vec3(minLight)));\n"

"	gl_FragColor = vec4(litColor * baseColor, 1.0);\n"

"	gl_FragColor = mix(fx, gl_FragColor, alphamask.a);\n"

"	if(useGlow > 0)\n"
"		gl_FragColor = mix(gl_FragColor, glow, glow.a);\n"

"	if(FOG > 0)\n"
"		gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);\n"
"}\n";

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
		glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)qwglGetProcAddress("glVertexAttribPointerARB");
		glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glEnableVertexAttribArrayARB");
		glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glDisableVertexAttribArrayARB");
		glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)qwglGetProcAddress("glBindAttribLocationARB");

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
		    !glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
		    !glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
		    !glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform1iARB || !glUniform1fARB || !glUniformMatrix3fvARB ||
				!glVertexAttribPointerARB || !glEnableVertexAttribArrayARB ||
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
			Com_Printf("...Linking Error\n");
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
		g_location_lightPosition = glGetUniformLocationARB( g_programObj, "lightPosition" );
		g_location_staticLightPosition = glGetUniformLocationARB( g_programObj, "staticLightPosition" );
		g_location_lightColour = glGetUniformLocationARB( g_programObj, "lightColour" );
		g_location_lightCutoffSquared = glGetUniformLocationARB( g_programObj, "lightCutoffSquared" );

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
			Com_Printf("...Linking Error\n");
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
		glLinkProgramARB( g_meshprogramObj );
		glGetObjectParameterivARB( g_meshprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_meshprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_meshlightPosition = glGetUniformLocationARB( g_meshprogramObj, "lightPos" );
		g_location_baseTex = glGetUniformLocationARB( g_meshprogramObj, "baseTex" );
		g_location_normTex = glGetUniformLocationARB( g_meshprogramObj, "normalTex" );
		g_location_fxTex = glGetUniformLocationARB( g_meshprogramObj, "fxTex" );
		g_location_color = glGetUniformLocationARB(	g_meshprogramObj, "baseColor" );
		g_location_minLight = glGetUniformLocationARB( g_meshprogramObj, "minLight" );
		g_location_meshNormal = glGetUniformLocationARB( g_meshprogramObj, "meshNormal" );
		g_location_meshTime = glGetUniformLocationARB( g_meshprogramObj, "time" );
		g_location_meshFog = glGetUniformLocationARB( g_meshprogramObj, "FOG" );
		g_location_useFX = glGetUniformLocationARB( g_meshprogramObj, "useFX" );
		g_location_useGlow = glGetUniformLocationARB( g_meshprogramObj, "useGlow");

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
			Com_Printf("...Linking Error\n");
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
			Com_Printf("...Linking Error\n");
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
			Com_Printf("...Linking Error\n");
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
			Com_Printf("...Linking Error\n");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_drSource = glGetUniformLocationARB( g_dropletsprogramObj, "drSource" );
		g_location_drTex = glGetUniformLocationARB( g_dropletsprogramObj, "drTex");
		g_location_drParams = glGetUniformLocationARB( g_dropletsprogramObj, "drParams" );
		g_location_drTime = glGetUniformLocationARB( g_dropletsprogramObj, "drTime" );

	}
	else
	{
		gl_glsl_shaders = Cvar_Get("gl_glsl_shaders", "0", CVAR_ARCHIVE);
		gl_dynamic = Cvar_Get ("gl_dynamic", "0", CVAR_ARCHIVE);
	}
}
