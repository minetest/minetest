uniform mat4 LightMVP; // world matrix
varying vec4 tPos;
#ifdef COLORED_SHADOWS
varying vec3 varColor;
#endif


void main()
{
	tPos = LightMVP * gl_Vertex;

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

#ifdef COLORED_SHADOWS
	varColor = gl_Color.rgb;
#endif
}
