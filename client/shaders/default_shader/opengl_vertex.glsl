varying lowp vec4 varColor;

void main(void)
{
	gl_Position = mWorldViewProj * inVertexPosition;
	varColor = inVertexColor;
}
