const float Eta = 0.66;
const float FresnelPower = 5.0; 

const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));
varying float FresRatio; 

varying vec3 lightDir; 
varying vec3 eyeDir;
varying vec3 reflectDir;
varying vec3 refractDir; 

varying float fog;

uniform vec3 stangent;
uniform vec3 LightPos;
uniform float time; 
uniform int	REFLECT;
uniform int FOG;

void main(void)
{ 

	vec4 v = vec4(gl_Vertex);
	gl_Position = ftransform();
	
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	vec3 tang = normalize(gl_NormalMatrix * stangent);
	vec3 binorm = cross(norm,tang);

	eyeDir = vec3(gl_ModelViewMatrix * v); 
	
	//for refraction
	vec4 neyeDir = gl_ModelViewMatrix * v;
	vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
	refeyeDir = normalize(refeyeDir); 
	
	refeyeDir.x *= -1.0;
	refeyeDir.y *= -1.0;
	refeyeDir.z *= -1.0;

	reflectDir = reflect(eyeDir,norm);
	refractDir = refract(eyeDir,norm,Eta);
	refractDir = vec3(gl_TextureMatrix[0] * vec4(refractDir,1.0));
	FresRatio = F + (1.0-F) * pow((1.0-dot(-refeyeDir,norm)),FresnelPower);

	vec3 tmp;
	tmp.x = dot(LightPos,tang);
	tmp.y = dot(LightPos,binorm);
	tmp.z = dot(LightPos,norm);
	lightDir = normalize(tmp); 

	tmp.x = dot(eyeDir,tang);
	tmp.y = dot(eyeDir,binorm);
	tmp.z = dot(eyeDir,norm);
	eyeDir = normalize(tmp);

	vec4 texco = gl_MultiTexCoord0; 
	if(REFLECT > 0) {
		texco.s = texco.s - LightPos.x/256.0;
		texco.t = texco.t + LightPos.y/256.0; 
	}

	gl_TexCoord[0] = texco; 

	texco = gl_MultiTexCoord0;
	texco.s = texco.s + time*0.05;
	texco.t = texco.t + time*0.05; 

	gl_TexCoord[1] = texco; 

	texco = gl_MultiTexCoord0;
	texco.s = texco.s + -time*0.05;
	texco.t = texco.t + -time*0.05; 
	gl_TexCoord[2] = texco;
	
	//fog
    if(FOG > 0){
		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
		fog = clamp(fog, 0.0, 1.0);
   	}
}

