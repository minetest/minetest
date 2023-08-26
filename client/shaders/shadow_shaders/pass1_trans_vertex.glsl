uniform mat4 LightMVP; // world matrix
uniform float zPerspectiveBias;

varying vec4 tPos;
#ifdef COLORED_SHADOWS
varying vec3 varColor;
#endif


vec4 applyPerspectiveDistortion(in vec4 position)
{
	position /= position.w;
	position.z *= zPerspectiveBias;
	return position;
}

void main()
{
	vec4 pos = LightMVP * gl_Vertex;

	tPos = applyPerspectiveDistortion(LightMVP * gl_Vertex);

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

#ifdef COLORED_SHADOWS
	varColor = gl_Color.rgb;
#endif
}
