uniform mat4 mWorldViewProj;

void main(void)
{
	gl_Position = mWorldViewProj * gl_Vertex;
}