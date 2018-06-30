uniform mat4 mWorldViewProj;

varying vec4 varColor;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;

	varColor = gl_Color;
}
