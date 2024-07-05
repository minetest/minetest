uniform mat4 mCameraProjInv;
uniform mat4 mCameraView;
uniform vec3 eyePosition;

varying vec3 relativePosition;
varying vec3 viewDirection;

varying vec2 screenspaceCoordinate;

void main(void)
{
	screenspaceCoordinate = inVertexPosition.xy;
	vec4 p = mCameraProjInv * inVertexPosition;
	viewDirection = p.xyz / p.w;
	relativePosition = (p.xyz / p.w) * mat3(mCameraView);
	gl_Position = inVertexPosition;
}
