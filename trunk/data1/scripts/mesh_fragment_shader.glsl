uniform sampler2D baseTex;
uniform sampler2D normalTex;
uniform vec3 baseColor;
uniform int FOG;
const vec3 specularMaterial = vec3(1.0, 1.0, 1.0);
const float SpecularFactor = 0.5; //remove

varying vec3 LightDir;
varying vec3 EyeDir;
varying float fog;

void main()
{
	vec3 litColor;
    vec3 textureColour = texture2D( baseTex, gl_TexCoord[0].xy ).rgb;
    vec3 normal = 2.0 * ( texture2D( normalTex, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );
	
	litColor = textureColour * max(dot(normal, LightDir), 0.0);
	vec3 reflectDir = reflect(LightDir, normal);
	
	float spec = max(dot(EyeDir, reflectDir), 0.0);
	spec = pow(spec, 6.0);
	spec *= SpecularFactor;
	litColor = min(litColor + spec, vec3(1.0));
	
	gl_FragColor = vec4(litColor * baseColor, 1.0);

	if(FOG > 0)
		gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}
