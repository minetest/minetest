varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main(void)
{
	varTexCoord = inTexCoord0.st;
	gl_Position = mWorldViewProj * inVertexPosition;

#ifdef GL_ES
	varColor = inVertexColor.bgra;
#else
	varColor = inVertexColor;
#endif
}
