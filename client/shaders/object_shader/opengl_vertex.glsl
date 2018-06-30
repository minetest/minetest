uniform mat4 mWorldViewProj;
uniform mat4 mWorld;

uniform vec3 eyePosition;
uniform float animationTimer;

varying vec3 vNormal;
varying vec3 vPosition;
varying vec3 worldPosition;
varying vec4 varColor;

varying vec3 eyeVec;
varying float vIDiff;

const float e = 2.718281828459;
const float BS = 10.0;

float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0.0)
		return dot(v, vec3(0.670820, 0.447213, 0.836660));

	return dot(v, vec3(0.670820, 1.000000, 0.836660));
}

void main(void)
{
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;

	vPosition = gl_Position.xyz;
	vNormal = gl_Normal;
	worldPosition = (mWorld * gl_Vertex).xyz;
	eyeVec = -(gl_ModelViewMatrix * gl_Vertex).xyz;

#if (MATERIAL_TYPE == TILE_MATERIAL_PLAIN) || (MATERIAL_TYPE == TILE_MATERIAL_PLAIN_ALPHA)
	vIDiff = 1.0;
#else
	// This is intentional comparison with zero without any margin.
	// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
	vIDiff = length(gl_Normal) == 0.0
		? 1.0
		: directional_ambient(normalize(gl_Normal));
#endif

	varColor = gl_Color;
}
