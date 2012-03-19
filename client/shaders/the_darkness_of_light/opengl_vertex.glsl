
uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;

void main(void)
{
	gl_Position = mWorldViewProj * gl_Vertex;
	
	if(gl_Normal.y > 0.5)
		gl_FrontColor = gl_BackColor = gl_Color;
	else
		gl_FrontColor = gl_BackColor = gl_Color * 0.5;

	gl_TexCoord[0] = gl_MultiTexCoord0;
}
