uniform mat4 mWorld;

varying vec4 varColor;
varying vec2 varTexCoord;

void main(void)
{
	varTexCoord = inTexCoord0.st;
	gl_Position = mWorldViewProj * inVertexPosition;
	varColor = inVertexColor;
}
