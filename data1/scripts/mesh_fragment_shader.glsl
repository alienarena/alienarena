uniform sampler2D baseTex;
uniform sampler2D normalTex;
uniform sampler2D fxTex;
uniform vec3 baseColor;
uniform int FOG;
uniform int useFX;
uniform int useGlow;
uniform float minLight; 
const vec3 specularMaterial = vec3(1.0, 1.0, 1.0);
const float SpecularFactor = 0.5;

varying vec3 LightDir;
varying vec3 EyeDir;
varying float fog;

void main()
{
	vec3 litColor;
	vec4 fx;
	vec4 glow;
	
    vec3 textureColour = texture2D( baseTex, gl_TexCoord[0].xy ).rgb;
    vec3 normal = 2.0 * ( texture2D( normalTex, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );
	
	vec4 alphamask = texture2D( baseTex, gl_TexCoord[0].xy);
	vec4 specmask = texture2D( normalTex, gl_TexCoord[0].xy);
	
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
	spec = pow(spec, 6.0);
	spec *= (SpecularFactor*specmask.a);
	litColor = min(litColor + spec, vec3(1.0));
	
	//keep shadows from making meshes completely black
	litColor = max(litColor, (textureColour * vec3(minLight)));
	
	gl_FragColor = vec4(litColor * baseColor, 1.0);
	
	gl_FragColor = mix(fx, gl_FragColor, alphamask.a);

	if(useGlow > 0)
		gl_FragColor = mix(gl_FragColor, glow, glow.a);

	if(FOG > 0)
		gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}
