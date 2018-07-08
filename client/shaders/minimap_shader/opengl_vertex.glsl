uniform mat4 mWorldViewProj;
uniform mat4 mWorld;

varying vec4 varColor;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;
	varColor = gl_Color;
}
