uniform mat3 tangentSpaceTransform;
uniform vec3 Eye;
uniform int FOG;



varying vec3 EyeDir;
varying float fog;

void main( void )
{ 
     	gl_Position = ftransform(); 
          
	EyeDir = tangentSpaceTransform * ( Eye - gl_Vertex.xyz );

	// pass any active texunits through 
    	gl_TexCoord[0] = gl_MultiTexCoord0; 
          
    	gl_TexCoord[1] = gl_MultiTexCoord1; 

    	//fog
    	if(FOG > 0){
		
		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
		
    		fog = clamp(fog, 0.0, 1.0);
	
   	}

}
