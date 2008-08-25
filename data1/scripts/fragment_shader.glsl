uniform sampler2D testTexture;
uniform sampler2D HeightTexture;
uniform sampler2D NormalTexture;
uniform sampler2D lmTexture;
uniform int FOG;
uniform int PARALLAX;
uniform int DYNAMIC;

varying vec3 EyeDir;
varying vec3 LightDir;
varying vec3 varyingLightColour;
varying float varyingLightCutoffSquared;
varying float fog;

void main( void )
{
	vec4 diffuse;
	vec4 lightmap;
	
	if(PARALLAX > 0) {
		//do the parallax mapping
		vec3 EyeT = normalize(EyeDir);
		vec4 Offset = texture2D(HeightTexture,gl_TexCoord[0].xy);
		Offset = Offset * 0.04 - 0.02;
		vec2 TexCoords = Offset.xy * EyeT.xy + gl_TexCoord[0].xy;

   		diffuse = texture2D(testTexture, TexCoords);
		lightmap = texture2D(lmTexture, gl_TexCoord[1].st);

		gl_FragColor = (diffuse * lightmap * 2.0); 	
	}
	else {
		diffuse = texture2D(testTexture, gl_TexCoord[0].xy);
		lightmap = texture2D(lmTexture, gl_TexCoord[1].st);
		gl_FragColor = (diffuse * lightmap * 2.0); 
	}

	if(DYNAMIC > 0) {
		//now do the dynamic lighting
		vec3 normal = 2.0 * ( texture2D( NormalTexture, gl_TexCoord[0].xy).xyz - vec3( 0.5, 0.5, 0.5 ) );
		float distanceSquared = dot( LightDir, LightDir );
		vec3 relativeLightDirection = LightDir / sqrt( distanceSquared );

		float diffuseTerm = clamp( dot( normal, relativeLightDirection ), 0.0, 1.0 );
		vec3 colour = vec3( 0.0, 0.0, 0.0 );

		if( diffuseTerm > 0.0 )
		{
			vec3 relativeEyeDirection = normalize( EyeDir );
			vec3 halfAngleVector = normalize( relativeLightDirection + relativeEyeDirection );
	
			float specularTerm = clamp( dot( normal, halfAngleVector ), 0.0, 1.0 );
			specularTerm = pow( specularTerm, 32.0 );

			colour = specularTerm * vec3( 1.0, 1.0, 1.0 ) / 2.0;
		}

		float attenuation = clamp( 1.0 - ( distanceSquared / varyingLightCutoffSquared ), 0.0, 1.0 );

		float swamp = attenuation;
		swamp *= swamp;
		swamp *= swamp;
		swamp *= swamp;
		swamp /= 1.0;

		vec3 textureColour = texture2D( testTexture, gl_TexCoord[0].xy ).rgb;
		colour += ( ( ( 0.5 - swamp ) * diffuseTerm ) + swamp ) * textureColour * 5.0;
		
		vec4 dynamicColour = vec4( attenuation * colour * varyingLightColour, 1.0 );
		if(PARALLAX > 0) {
			dynamicColour = max(dynamicColour, diffuse * lightmap * 2.0);
		}
		else {
			dynamicColour = max(dynamicColour, vec4(textureColour, 1.0) * lightmap * 2.0);
		}
    	gl_FragColor = dynamicColour;
	}
	if(FOG > 0)
		gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}
