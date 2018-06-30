uniform mat4 mWorldViewProj;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;

	varColor = gl_Color;
}
