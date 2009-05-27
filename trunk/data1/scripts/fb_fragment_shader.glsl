uniform sampler2D fbtexture;
uniform sampler2D distortiontexture;
uniform float frametime;
uniform vec2 fxPos;
uniform int fxType;
uniform vec3 fxColor;
uniform float asRatio;

void main(void)
{
	vec3 noiseVec;
	vec2 displacement;
	float hRatio;

    displacement = gl_TexCoord[0].st;

	displacement.x -= fxPos.x*0.002;
	displacement.y -= fxPos.y*0.002;

	noiseVec = normalize(texture2D(distortiontexture, displacement.xy)).xyz;
	noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

	//clamp edges to prevent artifacts
	if(asRatio == 0.8) //this is hacky and needs to be addressed
		hRatio = 0.5;
	else
		hRatio = 0.6;
		
	if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < asRatio)
		displacement.x = gl_TexCoord[0].s + noiseVec.x;
	else
		displacement.x = gl_TexCoord[0].s;
		
	if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < hRatio) 
		displacement.y = gl_TexCoord[0].t + noiseVec.y;
	else
		displacement.y = gl_TexCoord[0].t;
		
	gl_FragColor = texture2D(fbtexture, displacement.xy);

	if(fxType == 2)
		gl_FragColor = mix(gl_FragColor, vec4(fxColor, 1.0), 0.3);

}