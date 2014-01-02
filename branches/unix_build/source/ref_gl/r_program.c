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
PFNGLVERTEXATTRIBPOINTERARBPROC	 glVertexAttribPointerARB	= NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC		glBindAttribLocationARB		= NULL;
PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog			= NULL;

#define STRINGIFY(...) #__VA_ARGS__

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
	uniform int SHINY;

	const float Eta = 0.66;
	const float FresnelPower = 5.0;
	const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));

	varying float FresRatio;
	varying vec3 EyeDir;
	varying vec3 LightDir;
	varying vec3 StaticLightDir;
	varying vec4 sPos;
	varying float fog;

	void main( void )
	{
		sPos = gl_Vertex;

		gl_Position = ftransform();

		if(SHINY > 0)
		{
			vec3 norm = vec3(0, 0, 1); 

			vec4 neyeDir = gl_ModelViewMatrix * gl_Vertex;
			vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
			refeyeDir = normalize(refeyeDir);

			FresRatio = F + (1.0-F) * pow((1.0-dot(refeyeDir,norm)),FresnelPower);
		}

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
	uniform sampler2D chromeTex;
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
	uniform int SHINY;
	uniform float rsTime;
	uniform float xPixelOffset;
	uniform float yPixelOffset;

	varying float FresRatio;
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

	   if(SHINY > 0)
	   {
		   vec3 reflection = reflect(relativeEyeDirection, normal);
		   vec3 refraction = refract(relativeEyeDirection, normal, 0.66);

		   vec4 Tl = texture2DProj(chromeTex, vec4(reflection.xy, 1.0, 1.0) );
		   vec4 Tr = texture2DProj(chromeTex, vec4(refraction.xy, 1.0, 1.0) );

		   vec4 cubemap = mix(Tl,Tr,FresRatio);  

		   gl_FragColor = max(gl_FragColor, (cubemap * 0.05 * alphamask.a));
	   }

	   if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

static char bsp_fragment_program_ATI[] = STRINGIFY (
	uniform sampler2D surfTexture;
	uniform sampler2D HeightTexture;
	uniform sampler2D NormalTexture;
	uniform sampler2D lmTexture;
	uniform sampler2D liquidTexture;
	uniform sampler2D liquidNormTex;
	uniform sampler2D chromeTex;
	uniform sampler2D ShadowMap;
	uniform sampler2D StatShadowMap;
	uniform vec3 lightColour;
	uniform float lightCutoffSquared;
	uniform int FOG;
	uniform int PARALLAX;
	uniform int DYNAMIC;
	uniform int STATSHADOW;
	uniform int SHADOWMAP;
	uniform int LIQUID;
	uniform int SHINY;
	uniform float rsTime;
	uniform float xPixelOffset;
	uniform float yPixelOffset;

	varying float FresRatio;
	varying vec4 sPos;
	varying vec3 EyeDir;
	varying vec3 LightDir;
	varying vec3 StaticLightDir;
	varying float fog;

	vec4 ShadowCoord;

	float lookup( vec2 offSet, sampler2D Map)
	{

		float shadow = 1.0;

		vec4 shadowCoordinateWdivide = (ShadowCoord + vec4(offSet.x * xPixelOffset * ShadowCoord.w, offSet.y * yPixelOffset * ShadowCoord.w, 0.0, 0.0)) / ShadowCoord.w ;
		// Used to lower moir pattern and self-shadowing
		shadowCoordinateWdivide.z += 0.0005;

		float distanceFromLight = texture2D(Map, shadowCoordinateWdivide.xy).z;

		if (ShadowCoord.w > 0.0)
			shadow = distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0 ;

		return shadow;
	}
	
	float lookupShadow( sampler2D Map )
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

	   if(SHINY > 0)
	   {
		   vec3 reflection = reflect(relativeEyeDirection, normal);
		   vec3 refraction = refract(relativeEyeDirection, normal, 0.66);

		   vec4 Tl = texture2DProj(chromeTex, vec4(reflection.xy, 1.0, 1.0) );
		   vec4 Tr = texture2DProj(chromeTex, vec4(refraction.xy, 1.0, 1.0) );

		   vec4 cubemap = mix(Tl,Tr,FresRatio);

		   gl_FragColor = max(gl_FragColor, (cubemap * 0.05 * alphamask.a));
	   }

	   if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);


//SHADOWS
static char shadow_vertex_program[] = STRINGIFY (		
	varying vec4 ShadowCoord;

	void main( void )
	{
		ShadowCoord = gl_TextureMatrix[6] * gl_Vertex;

		gl_Position = ftransform();

		gl_Position.z -= 0.05; //eliminate z-fighting on some drivers
	}
);

static char shadow_fragment_program[] = STRINGIFY (

	uniform sampler2DShadow StatShadowMap;
	uniform float fadeShadow;
	uniform float xPixelOffset;
	uniform float yPixelOffset;

	varying vec4 ShadowCoord;
	
	float lookup( vec2 offSet)
	{
		return shadow2DProj(StatShadowMap, ShadowCoord + vec4(offSet.x * xPixelOffset * ShadowCoord.w, offSet.y * yPixelOffset * ShadowCoord.w, 0.05, 0.0) ).w;
	}

	float lookupStatshadow( void )
	{
		float shadow = 1.0;

		if (ShadowCoord.w > 1.0)
		{
			vec2 o = mod(floor(gl_FragCoord.xy), 2.0);
				
			shadow += lookup(vec2(-1.5, 1.5) + o);
			shadow += lookup(vec2( 0.5, 1.5) + o);
			shadow += lookup(vec2(-1.5, -0.5) + o);
			shadow += lookup(vec2( 0.5, -0.5) + o);
			shadow *= 0.25 ;
		}
		shadow += 0.3;
		if(shadow > 1.0)
			shadow = 1.0;
	
		return shadow;
	}		

	void main( void )
	{
		float statshadowval = 1/fadeShadow * lookupStatshadow();	  

		gl_FragColor = vec4(1.0);
		gl_FragColor = (gl_FragColor * statshadowval);	   
	}
);

static char shadow_fragment_program_ATI[] = STRINGIFY (

	uniform sampler2D StatShadowMap;
	uniform float fadeShadow;
	uniform float xPixelOffset;
	uniform float yPixelOffset;

	varying vec4 ShadowCoord;

	float lookup( vec2 offSet)
	{
		float shadow = 1.0;

		vec4 shadowCoordinateWdivide = (ShadowCoord + vec4(offSet.x * xPixelOffset * ShadowCoord.w, offSet.y * yPixelOffset * ShadowCoord.w, 0.0, 0.0)) / ShadowCoord.w ;
		// Used to lower moir pattern and self-shadowing
		shadowCoordinateWdivide.z += 0.0005;

		float distanceFromLight = texture2D(StatShadowMap,shadowCoordinateWdivide.xy).z;

		if (ShadowCoord.w > 0.0)
			shadow = distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0 ;

		return shadow;
	}

	float lookupStatshadow( void )
	{
		float shadow = 1.0;

		if (ShadowCoord.w > 1.0)
		{
			vec2 o = mod(floor(gl_FragCoord.xy), 2.0);

			shadow += lookup(vec2(-1.5, 1.5) + o);
			shadow += lookup(vec2( 0.5, 1.5) + o);
			shadow += lookup(vec2(-1.5, -0.5) + o);
			shadow += lookup(vec2( 0.5, -0.5) + o);
			shadow *= 0.25 ;
		}
		if(shadow > 1.0)
			shadow = 1.0;

		return shadow;
	}

	void main( void )
	{
		float statshadowval = 1/fadeShadow * lookupStatshadow();

		gl_FragColor = vec4(1.0);
		gl_FragColor = (gl_FragColor * statshadowval);
	}
);


#define USE_MESH_ANIM_LIBRARY "/*USE_MESH_ANIM_LIBRARY*/"
static char mesh_anim_library[] = STRINGIFY (
	
	uniform mat3x4 bonemats[70];
	uniform int GPUANIM; // 0 for none, 1 for IQM skeletal, 2 for MD2 lerp
	
	// MD2 only
	uniform float lerp; // 1.0 = all new vertex, 0.0 = all old vertex

	// IQM and MD2
	attribute vec4 tangent;
	
	// IQM only
	attribute vec4 weights;
	attribute vec4 bones;
	
	// MD2 only
	attribute vec3 oldvertex;
	attribute vec3 oldnormal;
	attribute vec4 oldtangent;
	
	// anim_compute () output
	vec4 anim_vertex;
	vec3 anim_normal;
	vec3 anim_tangent;
	vec3 anim_tangent_w;
	
	// if dotangent is true, compute anim_tangent and anim_tangent_w
	// if donormal is true, compute anim_normal
	// hopefully the if statements for these booleans will get optimized out
	void anim_compute (bool dotangent, bool donormal)
	{
		if (GPUANIM == 1)
		{
			mat3x4 m = bonemats[int(bones.x)] * weights.x;
			m += bonemats[int(bones.y)] * weights.y;
			m += bonemats[int(bones.z)] * weights.z;
			m += bonemats[int(bones.w)] * weights.w;
			
			anim_vertex = vec4 (gl_Vertex * m, gl_Vertex.w);
			if (donormal)
				anim_normal = vec4 (gl_Normal, 0.0) * m;
			if (dotangent)
			{
				anim_tangent = vec4 (tangent.xyz, 0.0) * m;
				anim_tangent_w = vec4 (tangent.w, 0.0, 0.0, 0.0) * m;
			}
		}
		else if (GPUANIM == 2)
		{
			anim_vertex = mix (vec4 (oldvertex, 1), gl_Vertex, lerp);
			if (donormal)
				anim_normal = normalize (mix (oldnormal, gl_Normal, lerp));
			if (dotangent)
			{
				vec4 tmptan = mix (oldtangent, tangent, lerp);
				anim_tangent = tmptan.xyz;
				anim_tangent_w = vec3 (tmptan.w);
			}
		}
		else
		{
			anim_vertex = gl_Vertex;
			if (donormal)
				anim_normal = gl_Normal;
			if (dotangent)
			{
				anim_tangent = tangent.xyz;
				anim_tangent_w = vec3 (tangent.w);
			}
		}
	}
);

//MESHES
static char mesh_vertex_program[] = USE_MESH_ANIM_LIBRARY STRINGIFY (
	uniform vec3 lightPos;
	uniform float time;
	uniform int FOG;
	uniform float useShell; // doubles as shell scale
	uniform int useCube;
	uniform vec3 baseColor;
	
	const float Eta = 0.66;
	const float FresnelPower = 5.0;
	const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));

	varying vec3 LightDir;
	varying vec3 EyeDir;
	varying float fog;
	varying float FresRatio;
	varying vec3 vertPos, lightVec;
	varying vec3 worldNormal;

	void subScatterVS(in vec4 ecVert)
	{
		if(useShell == 0 && useCube == 0)
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
		vec4 neyeDir;
		
		anim_compute (true, true);
		
		if (useShell > 0)
			anim_vertex += normalize (vec4 (anim_normal, 0)) * useShell;
			
		gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;
		subScatterVS (gl_Position);
		
		n = normalize (gl_NormalMatrix * anim_normal);
		t = normalize (gl_NormalMatrix * anim_tangent);
		b = normalize (gl_NormalMatrix * anim_tangent_w) * cross (n, t);
		
		EyeDir = vec3(gl_ModelViewMatrix * anim_vertex);
		neyeDir = gl_ModelViewMatrix * anim_vertex;
		
		worldNormal = n;

		vec3 v;
		v.x = dot(lightPos, t);
		v.y = dot(lightPos, b);
		v.z = dot(lightPos, n);
		LightDir = normalize(v);

		v.x = dot(EyeDir, t);
		v.y = dot(EyeDir, b);
		v.z = dot(EyeDir, n);
		EyeDir = normalize(v);

		if(useShell > 0)
		{
			gl_TexCoord[0] = vec4 ((anim_vertex[1]+anim_vertex[0])/40.0, anim_vertex[2]/40.0 - time, 0.0, 1.0);
		}
		else
		{
			gl_TexCoord[0] = gl_MultiTexCoord0;
			//for scrolling fx
			vec4 texco = gl_MultiTexCoord0;
			texco.s = texco.s + time*1.0;
			texco.t = texco.t + time*2.0;
			gl_TexCoord[1] = texco;
		}

		if(useCube > 0)
		{
			vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
			refeyeDir = normalize(refeyeDir);

			FresRatio = F + (1.0-F) * pow((1.0-dot(refeyeDir, n)), FresnelPower);
		}

		//fog
		if(FOG > 0) 
		{
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
		}
		
		// vertexOnly is defined as const, so this branch should get optimized
		// out.
		if (vertexOnly == 1)
		{
			// We try to bias toward light instead of shadow, but then make
			// the contrast between light and shadow more pronounced.
			float lightness = max (dot (worldNormal, LightDir), 0.0) + 0.25;
			lightness = lightness * lightness + 0.25;
			gl_FrontColor = gl_BackColor = vec4 (baseColor * lightness, 1.0);
		}
	}
);

static char mesh_fragment_program[] = STRINGIFY (
	uniform sampler2D baseTex;
	uniform sampler2D normalTex;
	uniform sampler2D fxTex;
	uniform sampler2D fx2Tex;
	uniform vec3 baseColor;
	uniform int GPUANIM; // 0 for none, 1 for IQM skeletal, 2 for MD2 lerp
	uniform int FOG;
	uniform int useFX;
	uniform int useCube;
	uniform int useGlow;
	uniform float useShell;
	uniform int fromView;
	uniform vec3 lightPos;
	const float SpecularFactor = 0.5;
	//next group could be made uniforms if we want to control this 
	const float MaterialThickness = 2.0; //this val seems good for now
	const vec3 ExtinctionCoefficient = vec3(0.80, 0.12, 0.20); //controls subsurface value
	const float RimScalar = 10.0; //intensity of the rim effect

	varying vec3 LightDir;
	varying vec3 EyeDir;
	varying float fog;
	varying float FresRatio;
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

		if(useShell == 0 && useCube == 0 && specmask.a < 1.0)
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
		if(GPUANIM == 1)
			spec = pow(spec, 4.0);
		else
			spec = pow(spec, 6.0);
		spec *= (SpecularFactor*specmask.a);
		litColor = min(litColor + spec, vec3(1.0));

		//keep shadows from making meshes completely black
		litColor = max(litColor, (textureColour * vec3(0.15)));

		gl_FragColor = vec4(litColor * baseColor, 1.0);		

		gl_FragColor = mix(fx, gl_FragColor + scatterCol, alphamask.a);

		if(useCube > 0 && specmask.a < 1.0)
		{			
			vec3 relEyeDir;
			
			if(fromView > 0)
				relEyeDir = normalize(LightDir);
			else
				relEyeDir = normalize(EyeDir);
					
			vec3 reflection = reflect(relEyeDir, normal);
			vec3 refraction = refract(relEyeDir, normal, 0.66);

			vec4 Tl = texture2D(fx2Tex, reflection.xy );
			vec4 Tr = texture2D(fx2Tex, refraction.xy );

			vec4 cubemap = mix(Tl,Tr,FresRatio);
			
			cubemap.rgb = max(gl_FragColor.rgb, cubemap.rgb * litColor);

			gl_FragColor = mix(cubemap, gl_FragColor, specmask.a);
		}
		
		if(useGlow > 0)
			gl_FragColor = mix(gl_FragColor, glow, glow.a);

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

//GLASS 
static char glass_vertex_program[] = USE_MESH_ANIM_LIBRARY STRINGIFY (

	uniform int FOG;

	varying vec3 r;
	varying float fog;
	varying vec3 orig_normal, normal, vert;

	void main(void)
	{
		anim_compute (false, true);

		gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;

		vec3 u = normalize( vec3(gl_ModelViewMatrix * anim_vertex) ); 	
		vec3 n = normalize(gl_NormalMatrix * anim_normal); 

		r = reflect( u, n );

		normal = n;
		vert = vec3( gl_ModelViewMatrix * anim_vertex );

		orig_normal = anim_normal;

		//fog
	   if(FOG > 0) 
	   {
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
	   }
	   
	   // for mirroring
	   gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char glass_fragment_program[] = STRINGIFY (

	uniform vec3 LightPos;
	uniform vec3 left;
	uniform vec3 up;
	uniform sampler2D refTexture;
	uniform sampler2D mirTexture;
	uniform int FOG;
	uniform int type; // 1 means mirror only, 2 means glass only, 3 means both

	varying vec3 r;
	varying float fog;
	varying vec3 orig_normal, normal, vert;

	void main (void)
	{
		vec3 light_dir = normalize( LightPos - vert );  	
		vec3 eye_dir = normalize( -vert.xyz );  	
		vec3 ref = normalize( -reflect( light_dir, normal ) );  
	
		float ld = max( dot(normal, light_dir), 0.0 ); 	
		float ls = 0.75 * pow( max( dot(ref, eye_dir), 0.0 ), 0.70 ); //0.75 specular, .7 shininess

		float m = -1.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
		
		vec3 n_orig_normal = normalize (orig_normal);
		vec2 coord_offset = vec2 (dot (n_orig_normal, left), dot (n_orig_normal, up));
		vec2 glass_coord = -vec2 (r.x/m + 0.5, r.y/m + 0.5);
		vec2 mirror_coord = vec2 (-gl_TexCoord[0].s, gl_TexCoord[0].t);

		vec4 mirror_color, glass_color;
		if (type == 1)
			mirror_color = texture2D(mirTexture, mirror_coord.st);
		else if (type == 3)
			mirror_color = texture2D(mirTexture, mirror_coord.st + coord_offset.st);
		if (type != 1)
			glass_color = texture2D(refTexture, glass_coord.st + coord_offset.st/2.0);
		
		if (type == 3)
			gl_FragColor = 0.3 * glass_color + 0.3 * mirror_color * vec4 (ld + ls + 0.35);
		else if (type == 2)
			gl_FragColor = glass_color/2.0;
		else if (type == 1)
			gl_FragColor = mirror_color;

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

//NO TEXTURE 
static char blankmesh_vertex_program[] = USE_MESH_ANIM_LIBRARY STRINGIFY (
	void main(void)
	{
		anim_compute (false, false);
		gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;
	}
);

static char blankmesh_fragment_program[] = STRINGIFY (
	void main (void)
	{		
		gl_FragColor = vec4(1.0);
	}
);


static char water_vertex_program[] = STRINGIFY (
	uniform mat3 tangentSpaceTransform;
	uniform vec3 Eye; 
	uniform vec3 LightPos;
	uniform float time;
	uniform int	REFLECT;
	uniform int FOG;

	const float Eta = 0.66;
	const float FresnelPower = 2.5;
	const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));

	varying float FresRatio;
	varying vec3 lightDir;
	varying vec3 eyeDir;
	varying float fog;

	void main(void)
	{
		gl_Position = ftransform();

		lightDir = tangentSpaceTransform * (LightPos - gl_Vertex.xyz);

		vec4 neyeDir = gl_ModelViewMatrix * gl_Vertex;
		vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
		refeyeDir = normalize(refeyeDir);

		// The normal is always 0, 0, 1 because water is always up. Thus, 
		// dot (refeyeDir,norm) is always refeyeDir.z
		FresRatio = F + (1.0-F) * pow((1.0-refeyeDir.z),FresnelPower);

		eyeDir = tangentSpaceTransform * ( Eye - gl_Vertex.xyz );

		vec4 texco = gl_MultiTexCoord0;
		if(REFLECT > 0) 
		{
			texco.s = texco.s - LightPos.x/256.0;
			texco.t = texco.t + LightPos.y/256.0;
		}
		gl_TexCoord[0] = texco;

		texco = gl_MultiTexCoord0;
		texco.s = texco.s + time*0.05;
		texco.t = texco.t + time*0.05;
		gl_TexCoord[1] = texco;

		texco = gl_MultiTexCoord0;
		texco.s = texco.s - time*0.05;
		texco.t = texco.t - time*0.05;
		gl_TexCoord[2] = texco;

		//fog
	   if(FOG > 0)
	   {
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 1.0);
	  	}
	}
);

static char water_fragment_program[] = STRINGIFY (
	varying vec3 lightDir;
	varying vec3 eyeDir;
	varying float FresRatio;

	varying float fog;

	uniform sampler2D refTexture;
	uniform sampler2D normalMap;
	uniform sampler2D baseTexture;

	uniform float TRANSPARENT;
	uniform int FOG;
	
	void main (void)
	{
		vec4 refColor;

		vec3 vVec = normalize(eyeDir);

		refColor = texture2D(refTexture, gl_TexCoord[0].xy);

		vec3 bump = normalize( texture2D(normalMap, gl_TexCoord[1].xy).xyz - 0.5);
		vec3 secbump = normalize( texture2D(normalMap, gl_TexCoord[2].xy).xyz - 0.5);
		vec3 modbump = mix(secbump,bump,0.5);

		vec3 reflection = reflect(vVec,modbump);
		vec3 refraction = refract(vVec,modbump,0.66);

		vec4 Tl = texture2D(baseTexture, reflection.xy);
		vec4 Tr = texture2D(baseTexture, refraction.xy);

		vec4 cubemap = mix(Tl,Tr,FresRatio);

		gl_FragColor = mix(cubemap,refColor,0.5);

		gl_FragColor.a = TRANSPARENT;

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);

	}
);

//FRAMEBUFFER DISTORTION EFFECTS
static char fb_vertex_program[] = STRINGIFY (
	void main( void )
	{
		gl_Position = ftransform();

		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char fb_fragment_program[] = STRINGIFY (
	uniform sampler2D fbtexture;
	uniform sampler2D distortiontexture;
	uniform vec2 dParams;
	uniform vec2 fxPos;

	void main(void)
	{
		vec3 noiseVec;
		vec2 displacement;
		float wScissor;
		float hScissor;

	   displacement = gl_TexCoord[0].st;

		displacement.x -= fxPos.x;
		displacement.y -= fxPos.y;

		noiseVec = normalize(texture2D(distortiontexture, displacement.xy)).xyz;
		noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

		//clamp edges to prevent artifacts

		wScissor = dParams.x - 0.008;
		hScissor = dParams.y - 0.008;

		if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < wScissor)
			displacement.x = gl_TexCoord[0].s + noiseVec.x;
		else
			displacement.x = gl_TexCoord[0].s;

		if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < hScissor) 
			displacement.y = gl_TexCoord[0].t + noiseVec.y;
		else
			displacement.y = gl_TexCoord[0].t;

		gl_FragColor = texture2D(fbtexture, displacement.xy);
	}
);

//GAUSSIAN BLUR EFFECTS
static char blur_vertex_program[] = STRINGIFY (
	varying vec2	texcoord1, texcoord2, texcoord3,
					texcoord4, texcoord5, texcoord6,
					texcoord7, texcoord8, texcoord9;
	uniform vec2	ScaleU;
	
	void main()
	{
		gl_Position = ftransform();
		
		// If we do all this math here, and let GLSL do its built-in
		// interpolation of varying variables, the math still comes out right,
		// but it's faster.
		texcoord1 = gl_MultiTexCoord0.xy-4.0*ScaleU;
		texcoord2 = gl_MultiTexCoord0.xy-3.0*ScaleU;
		texcoord3 = gl_MultiTexCoord0.xy-2.0*ScaleU;
		texcoord4 = gl_MultiTexCoord0.xy-ScaleU;
		texcoord5 = gl_MultiTexCoord0.xy;
		texcoord6 = gl_MultiTexCoord0.xy+ScaleU;
		texcoord7 = gl_MultiTexCoord0.xy+2.0*ScaleU;
		texcoord8 = gl_MultiTexCoord0.xy+3.0*ScaleU;
		texcoord9 = gl_MultiTexCoord0.xy+4.0*ScaleU;
	}
);

static char blur_fragment_program[] = STRINGIFY (
	varying vec2	texcoord1, texcoord2, texcoord3,
					texcoord4, texcoord5, texcoord6,
					texcoord7, texcoord8, texcoord9;
	uniform sampler2D textureSource;

	void main()
	{
	   vec4 sum = vec4(0.0);

	   // take nine samples
	   sum += texture2D(textureSource, texcoord1) * 0.05;
	   sum += texture2D(textureSource, texcoord2) * 0.09;
	   sum += texture2D(textureSource, texcoord3) * 0.12;
	   sum += texture2D(textureSource, texcoord4) * 0.15;
	   sum += texture2D(textureSource, texcoord5) * 0.16;
	   sum += texture2D(textureSource, texcoord6) * 0.15;
	   sum += texture2D(textureSource, texcoord7) * 0.12;
	   sum += texture2D(textureSource, texcoord8) * 0.09;
	   sum += texture2D(textureSource, texcoord9) * 0.05;

	   gl_FragColor = sum;
	}
);

//RADIAL BLUR EFFECTS // xy = radial center screen space position, z = radius attenuation, w = blur strength
static char rblur_vertex_program[] = STRINGIFY (
	void main()
	{
		gl_Position = ftransform();
		gl_TexCoord[0] =  gl_MultiTexCoord0;
	}
);

static char rblur_fragment_program[] = STRINGIFY (
	uniform sampler2D rtextureSource;
	uniform vec3 radialBlurParams;
	uniform vec3 rblurScale;

	void main(void)
	{
		float wScissor;
		float hScissor;

		vec2 dir = vec2(radialBlurParams.x - gl_TexCoord[0].x, radialBlurParams.x - gl_TexCoord[0].x);
		float dist = sqrt(dir.x*dir.x + dir.y*dir.y); 
	  	dir = dir/dist;
		vec4 color = texture2D(rtextureSource,gl_TexCoord[0].xy); 
		vec4 sum = color;

		float strength = radialBlurParams.z;
		vec2 pDir = vec2(rblurScale.y * 0.5 - gl_TexCoord[0].s, rblurScale.z * 0.5 - gl_TexCoord[0].t);
		float pDist = sqrt(pDir.x*pDir.x + pDir.y*pDir.y);
		clamp(pDist, 0.0, 1.0);

		 //the following ugliness is due to ATI's drivers inablity to handle a simple for-loop!
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.06 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.05 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.03 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.02 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * -0.01 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.01 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.02 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.03 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.05 * strength * pDist );
		sum += texture2D( rtextureSource, gl_TexCoord[0].xy + dir * 0.06 * strength * pDist );

		sum *= 1.0/11.0;
 
		float t = dist * rblurScale.x;
		t = clamp( t ,0.0,1.0);

		vec4 final = mix( color, sum, t );

		//clamp edges to prevent artifacts
		wScissor = rblurScale.y - 0.008;
		hScissor = rblurScale.z - 0.008;

		if(gl_TexCoord[0].s > 0.008 && gl_TexCoord[0].s < wScissor && gl_TexCoord[0].t > 0.008 && gl_TexCoord[0].t < hScissor)
			gl_FragColor = final;
		else
			gl_FragColor = color;
	}
);

//WATER DROPLETS
static char droplets_vertex_program[] = STRINGIFY (
	uniform float drTime;

	void main( void )
	{
		gl_Position = ftransform();

		 //for vertical scrolling
		 vec4 texco = gl_MultiTexCoord0;
		 texco.t = texco.t + drTime*1.0;
		 gl_TexCoord[1] = texco;

		 texco = gl_MultiTexCoord0;
		 texco.t = texco.t + drTime*0.8;
		 gl_TexCoord[2] = texco;

		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char droplets_fragment_program[] = STRINGIFY (
	uniform sampler2D drSource;
	uniform sampler2D drTex;
	uniform vec2 drParams;

	void main(void)
	{
		vec3 noiseVec;
		vec3 noiseVec2;
		vec2 displacement;
		float wScissor;
		float hScissor;

	   displacement = gl_TexCoord[1].st;

		noiseVec = normalize(texture2D(drTex, displacement.xy)).xyz;
		noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

		displacement = gl_TexCoord[2].st;

		noiseVec2 = normalize(texture2D(drTex, displacement.xy)).xyz;
		noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;

		//clamp edges to prevent artifacts
		wScissor = drParams.x - 0.008;
		hScissor = drParams.y - 0.028;

		if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < wScissor)
			displacement.x = gl_TexCoord[0].s + noiseVec.x + noiseVec2.x;
		else
			displacement.x = gl_TexCoord[0].s;

		if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < hScissor) 
			displacement.y = gl_TexCoord[0].t + noiseVec.y + noiseVec2.y;
		else
			displacement.y = gl_TexCoord[0].t;

		gl_FragColor = texture2D(drSource, displacement.xy);
	}
);

static char rgodrays_vertex_program[] = STRINGIFY (
	void main()
	{
		gl_TexCoord[0] =  gl_MultiTexCoord0;
		gl_Position = ftransform();
	}
);

static char rgodrays_fragment_program[] = STRINGIFY (
	uniform vec2 lightPositionOnScreen;
	uniform sampler2D sunTexture;
	uniform float aspectRatio; //width/height
	uniform float sunRadius;

	//note - these could be made uniforms to control externally
	const float exposure = 0.0034;
	const float decay = 1.0;
	const float density = 0.84;
	const float weight = 5.65;
	const int NUM_SAMPLES = 75;

	void main()
	{
		vec2 deltaTextCoord = vec2( gl_TexCoord[0].st - lightPositionOnScreen.xy );
		vec2 textCoo = gl_TexCoord[0].st;
		float adjustedLength = length (vec2 (deltaTextCoord.x*aspectRatio, deltaTextCoord.y));
		deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;
		float illuminationDecay = 1.0;

		int lim = NUM_SAMPLES;

		if (adjustedLength > sunRadius)
		{		
			//first simulate the part of the loop for which we won't get any
			//samples anyway
			float ratio = (adjustedLength-sunRadius)/adjustedLength;
			lim = int (float(lim)*ratio);

			textCoo -= deltaTextCoord*lim;
			illuminationDecay *= pow (decay, lim);

			//next set up the following loop so it gets the correct number of
			//samples.
			lim = NUM_SAMPLES-lim;
		}

		gl_FragColor = vec4(0.0);
		for(int i = 0; i < lim; i++)
		{
			textCoo -= deltaTextCoord;
			
			vec4 sample = texture2D(sunTexture, textCoo );

			sample *= illuminationDecay * weight;

			gl_FragColor += sample;

			illuminationDecay *= decay;
		}
		gl_FragColor *= exposure;
	}
);

typedef struct {
	const char	*name;
	int			index;
} vertex_attribute_t;

// add new vertex attributes here
#define NO_ATTRIBUTES			0
const vertex_attribute_t standard_attributes[] = 
{
	#define	ATTRIBUTE_TANGENT	(1<<0)
	{"tangent",		ATTR_TANGENT_IDX},
	#define	ATTRIBUTE_WEIGHTS	(1<<1)
	{"weights",		ATTR_WEIGHTS_IDX},
	#define ATTRIBUTE_BONES		(1<<2)
	{"bones",		ATTR_BONES_IDX},
	#define ATTRIBUTE_OLDVTX	(1<<3)
	{"oldvertex",	ATTR_OLDVTX_IDX},
	#define ATTRIBUTE_OLDNORM	(1<<4)
	{"oldnormal",	ATTR_OLDNORM_IDX},
	#define ATTRIBUTE_OLDTAN	(1<<5)
	{"oldtangent",	ATTR_OLDTAN_IDX}
};
const int num_standard_attributes = sizeof(standard_attributes)/sizeof(vertex_attribute_t);
	
void R_LoadGLSLProgram (const char *name, char *vertex, char *fragment, int attributes, GLhandleARB *program)
{
	char		str[4096];
	const char	*shaderStrings[4];
	int			nResult;
	int			i;
	
	shaderStrings[0] = "#version 120\n";
	
	*program = glCreateProgramObjectARB();
	
	g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
	
	if (strstr (vertex, USE_MESH_ANIM_LIBRARY))
		shaderStrings[1] = mesh_anim_library;
	else
		shaderStrings[1] = "";
	
	if (fragment == NULL)
		shaderStrings[2] = "const int vertexOnly = 1;";
	else
		shaderStrings[2] = "const int vertexOnly = 0;";
	
	shaderStrings[3] = vertex;
	glShaderSourceARB( g_vertexShader, 4, shaderStrings, NULL );
	glCompileShaderARB( g_vertexShader);
	glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

	if( nResult )
	{
		glAttachObjectARB( *program, g_vertexShader );
	}
	else
	{
		Com_Printf("...%s Vertex Shader Compile Error\n", name);
		if (glGetShaderInfoLog != NULL)
		{
			glGetShaderInfoLog (g_vertexShader, sizeof(str), NULL, str);
			Com_Printf ("%s\n", str);
		}
	}
	
	if (fragment != NULL)
	{
		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		shaderStrings[1] = fragment;
	
		glShaderSourceARB( g_fragmentShader, 2, shaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if( nResult )
		{
			glAttachObjectARB( *program, g_fragmentShader );
		}
		else
		{
			Com_Printf("...%s Fragment Shader Compile Error\n", name);
			if (glGetShaderInfoLog != NULL)
			{
				glGetShaderInfoLog (g_fragmentShader, sizeof(str), NULL, str);
				Com_Printf ("%s\n", str);
			}
		}
	}
	
	for (i = 0; i < num_standard_attributes; i++)
	{
		if (attributes & (1<<i))
			glBindAttribLocationARB(*program, standard_attributes[i].index, standard_attributes[i].name);
	}

	glLinkProgramARB( *program );
	glGetObjectParameterivARB( *program, GL_OBJECT_LINK_STATUS_ARB, &nResult );

	glGetInfoLogARB( *program, sizeof(str), NULL, str );
	if( !nResult )
		Com_Printf("...%s Shader Linking Error\n%s\n", name, str);
}

void R_LoadGLSLPrograms(void)
{
	//load glsl (to do - move to own file)
	if (strstr(gl_config.extensions_string,  "GL_ARB_shader_objects" ))
	{
		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB		 = (PFNGLDELETEOBJECTARBPROC)qwglGetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB	 = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)qwglGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB		 = (PFNGLSHADERSOURCEARBPROC)qwglGetProcAddress("glShaderSourceARB");
		glCompileShaderARB		= (PFNGLCOMPILESHADERARBPROC)qwglGetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)qwglGetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB		 = (PFNGLATTACHOBJECTARBPROC)qwglGetProcAddress("glAttachObjectARB");
		glGetInfoLogARB		   = (PFNGLGETINFOLOGARBPROC)qwglGetProcAddress("glGetInfoLogARB");
		glLinkProgramARB		  = (PFNGLLINKPROGRAMARBPROC)qwglGetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)qwglGetProcAddress("glGetUniformLocationARB");
		glUniform3fARB			= (PFNGLUNIFORM3FARBPROC)qwglGetProcAddress("glUniform3fARB");
		glUniform2fARB			= (PFNGLUNIFORM2FARBPROC)qwglGetProcAddress("glUniform2fARB");
		glUniform1iARB			= (PFNGLUNIFORM1IARBPROC)qwglGetProcAddress("glUniform1iARB");
		glUniform1fARB		  = (PFNGLUNIFORM1FARBPROC)qwglGetProcAddress("glUniform1fARB");
		glUniformMatrix3fvARB	  = (PFNGLUNIFORMMATRIX3FVARBPROC)qwglGetProcAddress("glUniformMatrix3fvARB");
		glUniformMatrix3x4fvARB	  = (PFNGLUNIFORMMATRIX3X4FVARBPROC)qwglGetProcAddress("glUniformMatrix3x4fv");
		glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)qwglGetProcAddress("glVertexAttribPointerARB");
		glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glEnableVertexAttribArrayARB");
		glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glDisableVertexAttribArrayARB");
		glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)qwglGetProcAddress("glBindAttribLocationARB");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)qwglGetProcAddress("glGetShaderInfoLog");

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
			!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
			!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
			!glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform1iARB || !glUniform1fARB || !glUniformMatrix3fvARB ||
				!glUniformMatrix3x4fvARB || !glVertexAttribPointerARB 
				|| !glEnableVertexAttribArrayARB ||
				!glBindAttribLocationARB)
		{
			Com_Error (ERR_FATAL, "...One or more GL_ARB_shader_objects functions were not found\n");
		}
	}
	else
	{
		Com_Error (ERR_FATAL, "...One or more GL_ARB_shader_objects functions were not found\n");
	}

	gl_dynamic = Cvar_Get ("gl_dynamic", "1", CVAR_ARCHIVE);

	//standard bsp surfaces
	if(gl_state.ati)
		R_LoadGLSLProgram ("BSP", (char*)bsp_vertex_program, (char*)bsp_fragment_program_ATI, NO_ATTRIBUTES, &g_programObj);
	else
		R_LoadGLSLProgram ("BSP", (char*)bsp_vertex_program, (char*)bsp_fragment_program, NO_ATTRIBUTES, &g_programObj);

	// Locate some parameters by name so we can set them later...
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
	g_location_shiny = glGetUniformLocationARB( g_programObj, "SHINY" );
	g_location_rsTime = glGetUniformLocationARB( g_programObj, "rsTime" );
	g_location_liquidTexture = glGetUniformLocationARB( g_programObj, "liquidTexture" );
	g_location_liquidNormTex = glGetUniformLocationARB( g_programObj, "liquidNormTex" );
	g_location_chromeTex = glGetUniformLocationARB( g_programObj, "chromeTex" );

	//shadowed white bsp surfaces
	if(gl_state.ati)
		R_LoadGLSLProgram ("Shadow", (char*)shadow_vertex_program, (char*)shadow_fragment_program_ATI, NO_ATTRIBUTES, &g_shadowprogramObj);
	else
		R_LoadGLSLProgram ("Shadow", (char*)shadow_vertex_program, (char*)shadow_fragment_program, NO_ATTRIBUTES, &g_shadowprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_entShadow = glGetUniformLocationARB( g_shadowprogramObj, "StatShadowMap" );
	g_location_fadeShadow = glGetUniformLocationARB( g_shadowprogramObj, "fadeShadow" );
	g_location_xOffset = glGetUniformLocationARB( g_shadowprogramObj, "xPixelOffset" );
	g_location_yOffset = glGetUniformLocationARB( g_shadowprogramObj, "yPixelOffset" );

	//warp(water) bsp surfaces
	R_LoadGLSLProgram ("Water", (char*)water_vertex_program, (char*)water_fragment_program, NO_ATTRIBUTES, &g_waterprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_baseTexture = glGetUniformLocationARB( g_waterprogramObj, "baseTexture" );
	g_location_normTexture = glGetUniformLocationARB( g_waterprogramObj, "normalMap" );
	g_location_refTexture = glGetUniformLocationARB( g_waterprogramObj, "refTexture" );
	g_location_waterEyePos = glGetUniformLocationARB( g_waterprogramObj, "Eye" );
	g_location_tangentSpaceTransform = glGetUniformLocationARB( g_waterprogramObj, "tangentSpaceTransform");
	g_location_time = glGetUniformLocationARB( g_waterprogramObj, "time" );
	g_location_lightPos = glGetUniformLocationARB( g_waterprogramObj, "LightPos" );
	g_location_reflect = glGetUniformLocationARB( g_waterprogramObj, "REFLECT" );
	g_location_trans = glGetUniformLocationARB( g_waterprogramObj, "TRANSPARENT" );
	g_location_fogamount = glGetUniformLocationARB( g_waterprogramObj, "FOG" );

	//meshes
	R_LoadGLSLProgram ("Mesh", (char*)mesh_vertex_program, (char*)mesh_fragment_program, ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN, &g_meshprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_meshlightPosition = glGetUniformLocationARB( g_meshprogramObj, "lightPos" );
	g_location_baseTex = glGetUniformLocationARB( g_meshprogramObj, "baseTex" );
	g_location_normTex = glGetUniformLocationARB( g_meshprogramObj, "normalTex" );
	g_location_fxTex = glGetUniformLocationARB( g_meshprogramObj, "fxTex" );
	g_location_fx2Tex = glGetUniformLocationARB( g_meshprogramObj, "fx2Tex" );
	g_location_color = glGetUniformLocationARB(	g_meshprogramObj, "baseColor" );
	g_location_meshTime = glGetUniformLocationARB( g_meshprogramObj, "time" );
	g_location_meshFog = glGetUniformLocationARB( g_meshprogramObj, "FOG" );
	g_location_useFX = glGetUniformLocationARB( g_meshprogramObj, "useFX" );
	g_location_useGlow = glGetUniformLocationARB( g_meshprogramObj, "useGlow" );
	g_location_useShell = glGetUniformLocationARB( g_meshprogramObj, "useShell" );
	g_location_useCube = glGetUniformLocationARB( g_meshprogramObj, "useCube" );
	g_location_useGPUanim = glGetUniformLocationARB( g_meshprogramObj, "GPUANIM");
	g_location_outframe = glGetUniformLocationARB( g_meshprogramObj, "bonemats");
	g_location_fromView = glGetUniformLocationARB( g_meshprogramObj, "fromView");
	g_location_lerp = glGetUniformLocationARB( g_meshprogramObj, "lerp");
	
	//vertex-only meshes
	R_LoadGLSLProgram ("VertexOnly_Mesh", (char*)mesh_vertex_program, NULL, ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN, &g_vertexonlymeshprogramObj);
	
	// Locate some parameters by name so we can set them later...
	g_location_vo_meshlightPosition = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "lightPos" );
	g_location_vo_baseTex = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "baseTex" );
	g_location_vo_normTex = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "normalTex" );
	g_location_vo_fxTex = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "fxTex" );
	g_location_vo_fx2Tex = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "fx2Tex" );
	g_location_vo_color = glGetUniformLocationARB(	g_vertexonlymeshprogramObj, "baseColor" );
	g_location_vo_meshTime = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "time" );
	g_location_vo_meshFog = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "FOG" );
	g_location_vo_useFX = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "useFX" );
	g_location_vo_useGlow = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "useGlow" );
	g_location_vo_useShell = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "useShell" );
	g_location_vo_useCube = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "useCube" );
	g_location_vo_useGPUanim = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "GPUANIM");
	g_location_vo_outframe = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "bonemats");
	g_location_vo_fromView = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "fromView");
	g_location_vo_lerp = glGetUniformLocationARB( g_vertexonlymeshprogramObj, "lerp");
	
	//Glass
	R_LoadGLSLProgram ("Glass", (char*)glass_vertex_program, (char*)glass_fragment_program, ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM, &g_glassprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_g_outframe = glGetUniformLocationARB( g_glassprogramObj, "bonemats" );
	g_location_g_fog = glGetUniformLocationARB( g_glassprogramObj, "FOG" );
	g_location_g_type = glGetUniformLocationARB( g_glassprogramObj, "type" );
	g_location_g_left = glGetUniformLocationARB( g_glassprogramObj, "left" );
	g_location_g_up = glGetUniformLocationARB( g_glassprogramObj, "up" );
	g_location_g_lightPos = glGetUniformLocationARB( g_glassprogramObj, "LightPos" );
	g_location_g_mirTexture = glGetUniformLocationARB( g_glassprogramObj, "mirTexture" );
	g_location_g_refTexture = glGetUniformLocationARB( g_glassprogramObj, "refTexture" );
	g_location_g_useGPUanim = glGetUniformLocationARB( g_glassprogramObj, "GPUANIM");
	g_location_g_lerp = glGetUniformLocationARB( g_glassprogramObj, "lerp");

	//Blank mesh (for shadowmapping efficiently)
	R_LoadGLSLProgram ("Blankmesh", (char*)blankmesh_vertex_program, (char*)blankmesh_fragment_program, ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX, &g_blankmeshprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_bm_outframe = glGetUniformLocationARB( g_blankmeshprogramObj, "bonemats" );
	g_location_bm_useGPUanim = glGetUniformLocationARB( g_blankmeshprogramObj, "GPUANIM" );
	g_location_bm_lerp = glGetUniformLocationARB( g_blankmeshprogramObj, "lerp" );
	
	//fullscreen distortion effects
	R_LoadGLSLProgram ("Framebuffer Distort", (char*)fb_vertex_program, (char*)fb_fragment_program, NO_ATTRIBUTES, &g_fbprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_framebuffTex = glGetUniformLocationARB( g_fbprogramObj, "fbtexture" );
	g_location_distortTex = glGetUniformLocationARB( g_fbprogramObj, "distorttexture");
	g_location_dParams = glGetUniformLocationARB( g_fbprogramObj, "dParams" );
	g_location_fxPos = glGetUniformLocationARB( g_fbprogramObj, "fxPos" );

	//gaussian blur
	R_LoadGLSLProgram ("Framebuffer Blur", (char*)blur_vertex_program, (char*)blur_fragment_program, NO_ATTRIBUTES, &g_blurprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
	g_location_source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");

	//radial blur
	R_LoadGLSLProgram ("Framebuffer Radial Blur", (char*)rblur_vertex_program, (char*)rblur_fragment_program, NO_ATTRIBUTES, &g_rblurprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_rscale = glGetUniformLocationARB( g_rblurprogramObj, "rblurScale" );
	g_location_rsource = glGetUniformLocationARB( g_rblurprogramObj, "rtextureSource");
	g_location_rparams = glGetUniformLocationARB( g_rblurprogramObj, "radialBlurParams");

	//water droplets
	R_LoadGLSLProgram ("Framebuffer Droplets", (char*)droplets_vertex_program, (char*)droplets_fragment_program, NO_ATTRIBUTES, &g_dropletsprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_drSource = glGetUniformLocationARB( g_dropletsprogramObj, "drSource" );
	g_location_drTex = glGetUniformLocationARB( g_dropletsprogramObj, "drTex");
	g_location_drParams = glGetUniformLocationARB( g_dropletsprogramObj, "drParams" );
	g_location_drTime = glGetUniformLocationARB( g_dropletsprogramObj, "drTime" );

	//god rays
	R_LoadGLSLProgram ("God Rays", (char*)rgodrays_vertex_program, (char*)rgodrays_fragment_program, NO_ATTRIBUTES, &g_godraysprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_lightPositionOnScreen = glGetUniformLocationARB( g_godraysprogramObj, "lightPositionOnScreen" );
	g_location_sunTex = glGetUniformLocationARB( g_godraysprogramObj, "sunTexture");
	g_location_godrayScreenAspect = glGetUniformLocationARB( g_godraysprogramObj, "aspectRatio");
	g_location_sunRadius = glGetUniformLocationARB( g_godraysprogramObj, "sunRadius");
}
