uniform mat4 mWorldViewProj;
uniform mat4 mWorld;

uniform vec3 eyePosition;
uniform float animationTimer;

varying vec3 vNormal;
varying vec3 vPosition;
varying vec3 worldPosition;

varying vec3 eyeVec;
varying vec3 lightVec;
varying float vIDiff;

const float e = 2.718281828459;
const float BS = 10.0;

float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0)
		return dot(v, vec3(0.670820f, 0.447213f, 0.836660f));

	return dot(v, vec3(0.670820f, 1.000000f, 0.836660f));
}

void main(void)
{
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;

	vPosition = gl_Position.xyz;
	vNormal = gl_Normal;
	worldPosition = (mWorld * gl_Vertex).xyz;

	vec3 sunPosition = vec3 (0.0, eyePosition.y * BS + 900.0, 0.0);

	lightVec = sunPosition - worldPosition;
	eyeVec = -(gl_ModelViewMatrix * gl_Vertex).xyz;
	vIDiff = directional_ambient(normalize(gl_Normal));

	gl_FrontColor = gl_BackColor = gl_Color;
}
