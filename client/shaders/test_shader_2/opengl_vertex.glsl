
uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;

varying vec3 vPosition;

void main(void)
{
	vec4 pos = gl_Vertex;
	pos.y -= 2.0;
	gl_Position = mWorldViewProj * pos;

	vPosition = (mWorldViewProj * gl_Vertex).xyz;

	gl_FrontColor = gl_BackColor = gl_Color;
	//gl_FrontColor = gl_BackColor = vec4(1.0, 1.0, 1.0, 1.0);

	gl_TexCoord[0] = gl_MultiTexCoord0;
}
