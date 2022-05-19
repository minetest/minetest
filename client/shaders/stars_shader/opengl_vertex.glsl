#line 1

void main(void)
{
	gl_Position = mWorldViewProj * inVertexPosition;
}
