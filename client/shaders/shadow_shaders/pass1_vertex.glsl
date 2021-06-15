uniform mat4 LightMVP; // world matrix
varying vec4 tPos;

const float bias0 = 0.9;
const float zPersFactor = 1.0/4.0;
const float bias1 = 1.0 - bias0 + 1e-6;

vec4 getPerspectiveFactor(in vec4 shadowPosition)
{   
	float lnz = sqrt(shadowPosition.x*shadowPosition.x+shadowPosition.y*shadowPosition.y);

	float pf=mix(1.0, lnz * 1.165, bias0);
	
	float pFactor =1.0/pf;
	shadowPosition.xyz *= vec3(vec2(pFactor), zPersFactor);

	return shadowPosition;
}

void main()
{
	vec4 pos = LightMVP * gl_Vertex;

	tPos = getPerspectiveFactor(pos);

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;
}
