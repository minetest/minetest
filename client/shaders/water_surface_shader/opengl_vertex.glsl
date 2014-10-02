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

const float e = 2.718281828459;
const float BS = 10.0;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
#if (MATERIAL_TYPE == TILE_MATERIAL_LIQUID_TRANSPARENT || MATERIAL_TYPE == TILE_MATERIAL_LIQUID_OPAQUE) && ENABLE_WAVING_WATER
	vec4 pos = gl_Vertex;
	pos.y -= 2.0;
	pos.y -= sin (pos.z/WATER_WAVE_LENGTH + animationTimer * WATER_WAVE_SPEED * WATER_WAVE_LENGTH) * WATER_WAVE_HEIGHT
		+ sin ((pos.z/WATER_WAVE_LENGTH + animationTimer * WATER_WAVE_SPEED * WATER_WAVE_LENGTH) / 7.0) * WATER_WAVE_HEIGHT;
	gl_Position = mWorldViewProj * pos;
#else
	gl_Position = mWorldViewProj * gl_Vertex;
#endif

	vPosition = gl_Position.xyz;
	worldPosition = (mWorld * gl_Vertex).xyz;
	eyeVec = (gl_ModelViewMatrix * gl_Vertex).xyz;

	gl_FrontColor = gl_BackColor = gl_Color;
}
