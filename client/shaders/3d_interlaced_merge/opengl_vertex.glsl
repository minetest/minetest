varying mediump vec2 varTexCoord;

void main(void)
{
	varTexCoord = inTexCoord0;
	gl_Position = inVertexPosition;
}
