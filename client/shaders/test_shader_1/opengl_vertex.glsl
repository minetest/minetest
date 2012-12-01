
uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;

varying vec3 vPosition;

void main(void)
{
	gl_Position = mWorldViewProj * gl_Vertex;

	vPosition = (mWorldViewProj * gl_Vertex).xyz;

	if(gl_Normal.y > 0.5)
		gl_FrontColor = gl_BackColor = gl_Color;
	else
		gl_FrontColor = gl_BackColor = gl_Color * 0.7;

	/*if(gl_Normal.y > 0.5)
		gl_FrontColor = gl_BackColor = vec4(1.0, 1.0, 1.0, 1.0);
	else
		gl_FrontColor = gl_BackColor = vec4(1.0, 1.0, 1.0, 1.0) * 0.7;*/

	gl_TexCoord[0] = gl_MultiTexCoord0;
}
