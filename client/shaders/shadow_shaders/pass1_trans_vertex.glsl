uniform mat4 LightMVP; // world matrix
uniform vec4 CameraPos;
varying vec4 tPos;
#ifdef COLORED_SHADOWS
varying vec3 varColor;
#endif

uniform float xyPerspectiveBias0;
uniform float xyPerspectiveBias1;
uniform float zPerspectiveBias;

vec4 getPerspectiveFactor(in vec4 shadowPosition)
{
	vec2 s = vec2(shadowPosition.x > CameraPos.x ? 1.0 : -1.0, shadowPosition.y > CameraPos.y ? 1.0 : -1.0);
	vec2 l = s * (shadowPosition.xy - CameraPos.xy) / (1.0 - s * CameraPos.xy);
	float pDistance = length(l);
	float pFactor = pDistance * xyPerspectiveBias0 + xyPerspectiveBias1;
	l /= pFactor;
	shadowPosition.xy = CameraPos.xy * (1.0 - l) + s * l;
	shadowPosition.z *= zPerspectiveBias;
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
