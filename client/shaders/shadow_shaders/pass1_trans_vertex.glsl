uniform mat4 LightMVP; // world matrix
uniform vec4 CameraPos;
varying vec4 tPos;
#ifdef COLORED_SHADOWS
varying vec3 varColor;
#endif

uniform float xyPerspectiveBias0;
uniform float xyPerspectiveBias1;
uniform float zPerspectiveBias;
float scale;

vec4 getPerspectiveFactor(in vec4 shadowPosition)
{
	shadowPosition.xy = (shadowPosition.xy - CameraPos.xy) / scale;
	float pDistance = length(shadowPosition.xy);
	float pFactor = pDistance * xyPerspectiveBias0 + xyPerspectiveBias1;
	shadowPosition.xyz *= vec3(vec2(1.0 / pFactor), zPerspectiveBias);
	shadowPosition.xy = scale * shadowPosition.xy + CameraPos.xy;	

	return shadowPosition;
}


void main()
{
	vec4 pos = LightMVP * gl_Vertex;
	scale = 0.8 + pow(length(CameraPos.xy), 2.0);

	tPos = getPerspectiveFactor(LightMVP * gl_Vertex);

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

#ifdef COLORED_SHADOWS
	varColor = gl_Color.rgb;
#endif
}
