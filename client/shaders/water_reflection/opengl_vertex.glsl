#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

void main(void)
{
	varTexCoord.st = inTexCoord0.st;
	gl_Position = inVertexPosition;
}
