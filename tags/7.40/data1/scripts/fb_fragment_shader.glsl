uniform sampler2D fbtexture;
uniform sampler2D distortiontexture;
uniform float frametime;
uniform vec2 fxPos;
uniform int fxType;
uniform vec3 fxColor;
uniform int fbSampleSize;

void main(void)
{
	vec3 noiseVec;
	vec2 displacement;
	float wScissor;
	float hScissor;
    
    displacement = gl_TexCoord[0].st;

	displacement.x -= fxPos.x*0.002;
	displacement.y -= fxPos.y*0.002;

	noiseVec = normalize(texture2D(distortiontexture, displacement.xy)).xyz;
	noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

	//clamp edges to prevent artifacts
	
	//different sample sizes need different clamps
	if(fbSampleSize == 1) {
		wScissor = 0.9;
		hScissor = 0.6;
	} 
	else if(fbSampleSize == 2) {
		wScissor = 0.5;
		hScissor = 0.6;
	}
	else if(fbSampleSize == 3) {
		wScissor = 0.8;
		hScissor = 0.5;
	}
		
	if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < wScissor)
		displacement.x = gl_TexCoord[0].s + noiseVec.x;
	else
		displacement.x = gl_TexCoord[0].s;
		
	if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < hScissor) 
		displacement.y = gl_TexCoord[0].t + noiseVec.y;
	else
		displacement.y = gl_TexCoord[0].t;
		
	gl_FragColor = texture2D(fbtexture, displacement.xy);

	if(fxType == 2)
		gl_FragColor = mix(gl_FragColor, vec4(fxColor, 1.0), 0.3);

}