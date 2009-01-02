varying vec3 lightDir; 
varying vec3 eyeDir;
varying float FresRatio;

varying float fog;

uniform sampler2D baseTexture;
uniform sampler2D normalMap;
uniform samplerCube cubeMap;

uniform int FOG;

void main (void)
{ 

	float distSqr = dot(lightDir, lightDir);
	float att = clamp(1.0 - 0.0 * sqrt(distSqr), 0.0, 1.0);
	vec3 lVec = lightDir * inversesqrt(distSqr); 

	vec3 vVec = normalize(eyeDir); 

	vec4 base = vec4(0.15,0.67,0.93,1.0); //base water color
	vec4 baseColor = mix(base, texture2D(baseTexture, gl_TexCoord[0].xy), 1.0);

	vec3 bump = normalize( texture2D(normalMap, gl_TexCoord[1].xy).xyz * 2.0 - 1.0);
	vec3 secbump = normalize( texture2D(normalMap, gl_TexCoord[2].xy).xyz * 2.0 - 1.0);
	vec3 modbump = mix(secbump,bump,0.5);

	vec3 reflection = vec3(textureCube(cubeMap,reflect(eyeDir,modbump)));
	vec3 refraction = vec3(textureCube(cubeMap,refract(eyeDir,modbump,0.66)));
	vec3 cubemap = mix(reflection,refraction,FresRatio);

	gl_FragColor = mix(vec4(cubemap,1.0),baseColor,0.5) ; 
	gl_FragColor.a = 0.5;
	
	if(FOG > 0)
		gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);	
	
}

