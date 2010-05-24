uniform sampler2D surfTexture;
uniform sampler2D HeightTexture;
uniform sampler2D NormalTexture;
uniform sampler2D lmTexture;
uniform sampler2D ShadowMap;
uniform vec3 lightColour;
uniform float lightCutoffSquared;
uniform int FOG;
uniform int PARALLAX;
uniform int DYNAMIC;
uniform int SHADOWMAP;

varying vec4 sPos;
varying vec3 EyeDir;
varying vec3 LightDir;
varying vec3 StaticLightDir;
varying float fog;

float lookupDynshadow( void )
{
	vec4 ShadowCoord;
	vec4 shadowCoordinateWdivide;
	float distanceFromLight;
    	vec4 tempShadowCoord;
    	float shadow1 = 1.0;
    	float shadow2 = 1.0;
    	float shadow3 = 1.0;
    	float shadows = 1.0;
    
	if(SHADOWMAP > 0 && DYNAMIC > 0) {

	ShadowCoord = gl_TextureMatrix[7] * sPos;		
		
      shadowCoordinateWdivide = ShadowCoord / ShadowCoord.w ;
      // Used to lower moiré pattern and self-shadowing
      shadowCoordinateWdivide.z += 0.0005;   
      
      distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.xy).z;
	            
      if (ShadowCoord.w > 0.0)
		shadows = distanceFromLight < shadowCoordinateWdivide.z ? 0.6 : 1.0 ;

	//Blur shadows a bit
      tempShadowCoord = ShadowCoord + vec4(.2, 0, 0, 0);
      shadowCoordinateWdivide = tempShadowCoord / tempShadowCoord.w ;
      shadowCoordinateWdivide.z += 0.0005;

      distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.xy).z;

      if (ShadowCoord.w > 0.0)
          shadow2 = distanceFromLight < shadowCoordinateWdivide.z ? 0.7 : 1.0 ;

      tempShadowCoord = ShadowCoord + vec4(0, .2, 0, 0);
      shadowCoordinateWdivide = tempShadowCoord / tempShadowCoord.w ;
      shadowCoordinateWdivide.z += 0.0005;

      distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.xy).z;

      if (ShadowCoord.w > 0.0)
          shadow3 = distanceFromLight < shadowCoordinateWdivide.z ? 0.7 : 1.0 ;

      shadows = 0.33 * (shadow1 + shadow2 + shadow3);

    }    
    
   return shadows;
}

void main( void )
{
   vec4 diffuse;
   vec4 lightmap;
   float distanceSquared;
   vec3 relativeLightDirection;
   float diffuseTerm;
   vec3 colour;
   vec3 halfAngleVector;
   float specularTerm;
   float swamp;
   float attenuation;
   vec4 litColour;
   vec3 varyingLightColour;
   float varyingLightCutoffSquared;
   float dynshadowval;
   
   varyingLightColour = lightColour;
   varyingLightCutoffSquared = lightCutoffSquared;
   
   vec3 relativeEyeDirection = normalize( EyeDir );
   vec3 normal = 2.0 * ( texture2D( NormalTexture, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );
   vec3 textureColour = texture2D( surfTexture, gl_TexCoord[0].xy ).rgb;
   
   lightmap = texture2D(lmTexture, gl_TexCoord[1].st); 
   
   //shadows
   dynshadowval = lookupDynshadow();
  
   if(PARALLAX > 0) {
      //do the parallax mapping
      vec4 Offset = texture2D(HeightTexture,gl_TexCoord[0].xy);
      Offset = Offset * 0.04 - 0.02;
      vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + gl_TexCoord[0].xy;

      diffuse = texture2D(surfTexture, TexCoords);
          
      distanceSquared = dot( StaticLightDir, StaticLightDir );
      relativeLightDirection = StaticLightDir / sqrt( distanceSquared );

      diffuseTerm = clamp( dot( normal, relativeLightDirection ), 0.0, 1.0 );
      colour = vec3( 0.0, 0.0, 0.0 );

	  if( diffuseTerm > 0.0 )
	  {
		halfAngleVector = normalize( relativeLightDirection + relativeEyeDirection );
         
        specularTerm = clamp( dot( normal, halfAngleVector ), 0.0, 1.0 );
        specularTerm = pow( specularTerm, 32.0 );

        colour = specularTerm * vec3( 1.0, 1.0, 1.0 ) / 2.0;
      }
      attenuation = 0.25;
      swamp = attenuation;
      swamp *= swamp;
      swamp *= swamp;
      swamp *= swamp;
      swamp /= 1.0;

      colour += ( ( ( 0.5 - swamp ) * diffuseTerm ) + swamp ) * textureColour * 4.0;
            
      litColour = vec4( attenuation * colour, 1.0 );
      litColour = litColour * lightmap * 6.0;
      gl_FragColor = max(litColour, diffuse * lightmap * 2.0);
   }
   else {
      diffuse = texture2D(surfTexture, gl_TexCoord[0].xy);     
      gl_FragColor = (diffuse * lightmap * 2.0);
   }      
   
   if(DYNAMIC > 0) {

	  lightmap = texture2D(lmTexture, gl_TexCoord[1].st);
	  
      //now do the dynamic lighting
      distanceSquared = dot( LightDir, LightDir );
      relativeLightDirection = LightDir / sqrt( distanceSquared );

      diffuseTerm = clamp( dot( normal, relativeLightDirection ), 0.0, 1.0 );
      colour = vec3( 0.0, 0.0, 0.0 );

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
      swamp /= 1.0;

      colour += ( ( ( 0.5 - swamp ) * diffuseTerm ) + swamp ) * textureColour * 3.0;
      
      vec4 dynamicColour = vec4( attenuation * colour * dynshadowval * varyingLightColour, 1.0 );
      if(PARALLAX > 0) {
         dynamicColour = max(dynamicColour, gl_FragColor);
      }
      else {
         dynamicColour = max(dynamicColour, vec4(textureColour, 1.0) * lightmap * 2.0);
      }
      gl_FragColor = dynamicColour; 
   }
   if(FOG > 0)
      gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}
