uniform mat4 LightMVP; // world matrix
varying vec4 tPos;

void main()
{
	tPos = LightMVP * gl_Vertex;
	gl_Position = tPos;
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;
}
