uniform mat3 tangentSpaceTransform;
uniform vec3 Eye;
uniform vec3 lightPosition;
uniform vec3 worldlightPosition;
uniform vec3 lightColour;
uniform float lightCutoffSquared;
uniform int FOG;

varying vec3 EyeDir;
varying vec3 LightDir;
varying vec3 varyingLightColour;
varying float varyingLightCutoffSquared;
varying vec4 ShadowCoord;
varying float fog;

void main( void )
{ 
	ShadowCoord = gl_TextureMatrix[7] * gl_Vertex;

    gl_Position = ftransform(); 
    
    gl_FrontColor = gl_Color;
          
	EyeDir = tangentSpaceTransform * ( Eye - gl_Vertex.xyz );
	LightDir = tangentSpaceTransform * (lightPosition - gl_Vertex.xyz);

	// pass any active texunits through 
    gl_TexCoord[0] = gl_MultiTexCoord0; 
    gl_TexCoord[1] = gl_MultiTexCoord1; 

	varyingLightColour = lightColour;
	varyingLightCutoffSquared = lightCutoffSquared;

    //fog
    if(FOG > 0){
		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
		fog = clamp(fog, 0.0, 1.0);
   	}
}
