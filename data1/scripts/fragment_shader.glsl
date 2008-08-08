uniform sampler2D testTexture;
uniform sampler2D HeightTexture;
uniform sampler2D lmTexture;
varying vec2 v_texCoord;

varying vec3 EyeDir;

void main( void )
{
     	vec3 EyeT = normalize(EyeDir);
    	vec4 Offset = texture2D(HeightTexture,gl_TexCoord[0].xy);
    	Offset = Offset * 0.04 - 0.02;
    	vec2 TexCoords = Offset.xy * EyeT.xy + gl_TexCoord[0].xy;

   	vec4 diffuse = texture2D(testTexture, TexCoords);
	
	
	vec4 lightmap = texture2D(lmTexture, gl_TexCoord[1].st);

    	gl_FragColor = (diffuse * lightmap * 2.0); 

}
