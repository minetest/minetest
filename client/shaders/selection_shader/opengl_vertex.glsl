varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main(void)
{
	varTexCoord = inTexCoord0.st;
	gl_Position = mProj * mView * mWorld * inVertexPosition;

	varColor = inVertexColor;
}
