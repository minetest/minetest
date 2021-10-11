uniform mat4 LightMVP; // world matrix
varying vec4 tPos;
#ifdef COLORED_SHADOWS
varying vec3 varColor;
#endif

const float bias0 = 0.9;
const float zPersFactor = 0.5;
const float bias1 = 1.0 - bias0 + 1e-6;

vec4 getPerspectiveFactor(in vec4 shadowPosition)
{
	float pDistance = length(shadowPosition.xy);
	float pFactor = pDistance * bias0 + bias1;
	shadowPosition.xyz *= vec3(vec2(1.0 / pFactor), zPersFactor);

	return shadowPosition;
}


void main()
{
	vec4 pos = LightMVP * gl_Vertex;

	tPos = getPerspectiveFactor(LightMVP * gl_Vertex);

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

#ifdef COLORED_SHADOWS
	varColor = gl_Color.rgb;
#endif
}
