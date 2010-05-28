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

GLhandleARB g_programObj;
GLhandleARB g_waterprogramObj;
GLhandleARB g_meshprogramObj;
GLhandleARB g_fbprogramObj;
GLhandleARB g_blurprogramObj;

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
GLuint		g_location_fog;
GLuint	    g_location_parallax;
GLuint		g_location_dynamic;
GLuint		g_location_shadowmap;
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
GLuint		g_location_meshnormal;
GLuint		g_location_baseTex;
GLuint		g_location_normTex;
GLuint		g_location_fxTex;
GLuint		g_location_color;
GLuint		g_location_minLight;
GLuint		g_location_meshNormal;
GLuint		g_location_meshTangent;
GLuint		g_location_meshTime;
GLuint		g_location_meshFog;
GLuint		g_location_useFX;
GLuint		g_location_useGlow;

//fullscreen distortion effects
GLuint		g_location_framebuffTex;
GLuint		g_location_distortTex;
GLuint		g_location_frametime;
GLuint		g_location_fxType;
GLuint		g_location_fxPos;
GLuint		g_location_fxColor;
GLuint		g_location_fbSampleSize;

//blur
GLuint		g_location_scale;
GLuint		g_location_source;

void R_LoadARBPrograms(void)
{
	int len;

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
		qglProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, 1.0f, 0.1f, 0.6f, 0.5f); // jitest
		len = FS_LoadFile("scripts/water1.arbf", &fragment_program_text);

		if (len > 0) {
			qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, len, fragment_program_text);
			FS_FreeFile(fragment_program_text);
		}
		else
			Com_Printf("Unable to find scripts/water1.arbf\n");

		
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

void R_LoadGLSLPrograms(void)
{
	const char *shaderStrings[1];
	unsigned char *shader_assembly;
    int		nResult;
    char	str[4096];
	int		len;

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

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
		    !glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB || 
		    !glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB || 
		    !glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform1iARB || !glUniform1fARB || !glUniformMatrix3fvARB)
		{
			Com_Printf("...One or more GL_ARB_shader_objects functions were not found");
			gl_state.glsl_shaders = false;
		}
	}
	else
	{            
		Com_Printf("...One or more GL_ARB_shader_objects functions were not found");
		gl_state.glsl_shaders = false;
	}

	if(gl_state.glsl_shaders) {

		//we have them, set defaults accordingly
		gl_glsl_shaders = Cvar_Get("gl_glsl_shaders", "1", CVAR_ARCHIVE); 
		gl_dynamic = Cvar_Get ("gl_dynamic", "1", CVAR_ARCHIVE);

		//standard bsp surfaces

		g_programObj = glCreateProgramObjectARB();
	
		//
		// Vertex shader
		//

		len = FS_LoadFile("scripts/vertex_shader.glsl", &shader_assembly);

		if (len > 0) {
			g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_vertexShader);
			glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Vertex Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_programObj, g_vertexShader );
		else
		{
			Com_Printf("...Vertex Shader Compile Error");
		}

		//
		// Fragment shader
		//
		len = FS_LoadFile("scripts/fragment_shader.glsl", &shader_assembly);
		
		if(len > 0) {
			g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_fragmentShader );
			glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Fragment Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_programObj, g_fragmentShader );
		else
		{
			Com_Printf("...Fragment Shader Compile Error");
		}

		glLinkProgramARB( g_programObj );
		glGetObjectParameterivARB( g_programObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_programObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error");
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
		g_location_fog = glGetUniformLocationARB( g_programObj, "FOG" );
		g_location_parallax = glGetUniformLocationARB( g_programObj, "PARALLAX" );
		g_location_dynamic = glGetUniformLocationARB( g_programObj, "DYNAMIC" );
		g_location_shadowmap = glGetUniformLocationARB( g_programObj, "SHADOWMAP" );
		g_location_lightPosition = glGetUniformLocationARB( g_programObj, "lightPosition" );
		g_location_staticLightPosition = glGetUniformLocationARB( g_programObj, "staticLightPosition" );
		g_location_lightColour = glGetUniformLocationARB( g_programObj, "lightColour" );
		g_location_lightCutoffSquared = glGetUniformLocationARB( g_programObj, "lightCutoffSquared" );

		//warp(water) bsp surfaces

		g_waterprogramObj = glCreateProgramObjectARB();
	
		//
		// Vertex shader
		//

		len = FS_LoadFile("scripts/water_vertex_shader.glsl", &shader_assembly);

		if (len > 0) {
			g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_vertexShader);
			glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Water Vertex Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_waterprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Water Vertex Shader Compile Error");
		}

		//
		// Fragment shader
		//
		len = FS_LoadFile("scripts/water_fragment_shader.glsl", &shader_assembly);
		
		if(len > 0) {
			g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_fragmentShader );
			glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Water Fragment Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_waterprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Water Fragment Shader Compile Error");
		}

		glLinkProgramARB( g_waterprogramObj );
		glGetObjectParameterivARB( g_waterprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_waterprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error");
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

		len = FS_LoadFile("scripts/mesh_vertex_shader.glsl", &shader_assembly);

		if (len > 0) {
			g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_vertexShader);
			glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Mesh Vertex Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_meshprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Mesh Vertex Shader Compile Error");
		}

		//
		// Fragment shader
		//
		len = FS_LoadFile("scripts/mesh_fragment_shader.glsl", &shader_assembly);
		
		if(len > 0) {
			g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_fragmentShader );
			glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Mesh Fragment Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_meshprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Mesh Fragment Shader Compile Error");
		}

		glLinkProgramARB( g_meshprogramObj );
		glGetObjectParameterivARB( g_meshprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_meshprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error");
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
		g_location_meshTangent = glGetUniformLocationARB( g_meshprogramObj, "meshTangent" );
		g_location_meshTime = glGetUniformLocationARB( g_meshprogramObj, "time" );
		g_location_meshFog = glGetUniformLocationARB( g_meshprogramObj, "FOG" );
		g_location_useFX = glGetUniformLocationARB( g_meshprogramObj, "useFX" );
		g_location_useGlow = glGetUniformLocationARB( g_meshprogramObj, "useGlow");

		//fullscreen distortion effects

		g_fbprogramObj = glCreateProgramObjectARB();
	
		//
		// Vertex shader
		//

		len = FS_LoadFile("scripts/fb_vertex_shader.glsl", &shader_assembly);

		if (len > 0) {
			g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_vertexShader);
			glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Framebuffer Vertex Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_fbprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Framebuffer Vertex Shader Compile Error");
		}

		//
		// Fragment shader
		//
		len = FS_LoadFile("scripts/fb_fragment_shader.glsl", &shader_assembly);
		
		if(len > 0) {
			g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_fragmentShader );
			glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Framebuffer Fragment Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_fbprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Framebuffer Fragment Shader Compile Error");
		}

		glLinkProgramARB( g_fbprogramObj );
		glGetObjectParameterivARB( g_fbprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_fbprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_framebuffTex = glGetUniformLocationARB( g_fbprogramObj, "fbtexture" );
		g_location_distortTex = glGetUniformLocationARB( g_fbprogramObj, "distorttexture");
		g_location_frametime = glGetUniformLocationARB( g_fbprogramObj, "frametime" );
		g_location_fxType = glGetUniformLocationARB( g_fbprogramObj, "fxType" );
		g_location_fxPos = glGetUniformLocationARB( g_fbprogramObj, "fxPos" ); 
		g_location_fxColor = glGetUniformLocationARB( g_fbprogramObj, "fxColor" );
		g_location_fbSampleSize = glGetUniformLocationARB( g_fbprogramObj, "fbSampleSize" );

		//blur

		g_blurprogramObj = glCreateProgramObjectARB();
	
		//
		// Vertex shader
		//

		len = FS_LoadFile("scripts/blur_vertex_shader.glsl", &shader_assembly);

		if (len > 0) {
			g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_vertexShader);
			glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Blur Vertex Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_blurprogramObj, g_vertexShader );
		else
		{
			Com_Printf("...Blur Vertex Shader Compile Error");
		}

		//
		// Fragment shader
		//
		len = FS_LoadFile("scripts/blur_fragment_shader.glsl", &shader_assembly);
		
		if(len > 0) {
			g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_fragmentShader );
			glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Blur Fragment Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_blurprogramObj, g_fragmentShader );
		else
		{
			Com_Printf("...Framebuffer Blur Shader Compile Error");
		}

		glLinkProgramARB( g_blurprogramObj );
		glGetObjectParameterivARB( g_blurprogramObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_blurprogramObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
		g_location_source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");
	
	}
	else {
		gl_glsl_shaders = Cvar_Get("gl_glsl_shaders", "0", CVAR_ARCHIVE); 
		gl_dynamic = Cvar_Get ("gl_dynamic", "0", CVAR_ARCHIVE);
	}
}