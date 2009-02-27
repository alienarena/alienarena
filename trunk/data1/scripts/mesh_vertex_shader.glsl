uniform vec3 lightPos;
uniform vec3 meshTangent;
uniform int FOG;

varying vec3 LightDir;
varying vec3 EyeDir;
varying float fog;

void main()
{
	gl_Position = ftransform();

	EyeDir = vec3(gl_ModelViewMatrix * gl_Vertex);
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 t = normalize(gl_NormalMatrix * meshTangent);
	vec3 b = cross(n, t);
	
	vec3 v;
	v.x = dot(lightPos, t);
	v.y = dot(lightPos, b);
	v.z = dot(lightPos, n);
	LightDir = normalize(v);
	
	v.x = dot(EyeDir, t);
	v.y = dot(EyeDir, b);
	v.z = dot(EyeDir, n);
	EyeDir = normalize(v);
	
	//fog
    if(FOG > 0){
		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
		fog = clamp(fog, 0.0, 1.0);
   	}
}