uniform mat4 LightMVP; // world matrix
varying vec4 tPos;

uniform float xyPerspectiveBias0;
uniform float xyPerspectiveBias1;
uniform float zPerspectiveBias;

vec4 getPerspectiveFactor(in vec4 shadowPosition)
{
	float pDistance = length(shadowPosition.xy);
	float pFactor = pDistance * xyPerspectiveBias0 + xyPerspectiveBias1;
	shadowPosition.xyz *= vec3(vec2(1.0 / pFactor), zPerspectiveBias);

	return shadowPosition;
}


void main()
{
	vec4 pos = LightMVP * gl_Vertex;

	tPos = getPerspectiveFactor(pos);

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
