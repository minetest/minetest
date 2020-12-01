varying lowp vec4 varColor;

void main(void)
{
	gl_Position = mProj * mView * mWorld * inVertexPosition;
	varColor = inVertexColor;
}
