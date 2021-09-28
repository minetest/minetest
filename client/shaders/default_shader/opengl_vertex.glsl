varying lowp vec4 varColor;

void main(void)
{
	gl_Position = mWorldViewProj * inVertexPosition;
#ifdef GL_ES
	varColor = inVertexColor.bgra;
#else
	varColor = inVertexColor;
#endif
}
