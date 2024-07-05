varying vec2 screenspaceCoordinate;

void main(void)
{
	screenspaceCoordinate = inVertexPosition.xy;
	gl_Position = inVertexPosition;
}
