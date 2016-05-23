uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;
uniform mat4 mWorld;

uniform float dayNightRatio;
uniform vec3 eyePosition;
uniform float animationTimer;

varying vec3 vPosition;
varying vec3 worldPosition;

varying vec3 eyeVec;
varying vec3 lightVec;
varying vec3 tsEyeVec;
varying vec3 tsLightVec;

const float e = 2.718281828459;
const float BS = 10.0;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;

	vPosition = gl_Position.xyz;
	worldPosition = (mWorld * gl_Vertex).xyz;

	vec3 sunPosition = vec3 (0.0, eyePosition.y * BS + 900.0, 0.0);

	lightVec = sunPosition - worldPosition;
	eyeVec = -(gl_ModelViewMatrix * gl_Vertex).xyz;

	gl_FrontColor = gl_BackColor = gl_Color;
}
