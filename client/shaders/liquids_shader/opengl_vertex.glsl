uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;
uniform float dayNightRatio;
uniform float animationTimer;

uniform vec3 eyePosition;

varying vec3 vPosition;
varying vec3 eyeVec;

void main(void)
{
#ifdef ENABLE_WAVING_WATER
	vec4 pos2 = gl_Vertex;
	pos2.y -= 2.0;
	pos2.y -= sin (pos2.z/WATER_WAVE_LENGTH + animationTimer * WATER_WAVE_SPEED * WATER_WAVE_LENGTH) * WATER_WAVE_HEIGHT
		+ sin ((pos2.z/WATER_WAVE_LENGTH + animationTimer * WATER_WAVE_SPEED * WATER_WAVE_LENGTH) / 7.0) * WATER_WAVE_HEIGHT;
	gl_Position = mWorldViewProj * pos2;
#else
	gl_Position = mWorldViewProj * gl_Vertex;
#endif

	eyeVec = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vPosition = (mWorldViewProj * gl_Vertex).xyz;

	vec4 color;
	//color = vec4(1.0, 1.0, 1.0, 1.0);

	float day = gl_Color.r;
	float night = gl_Color.g;
	float light_source = gl_Color.b;

	/*color.r = mix(night, day, dayNightRatio);
	color.g = color.r;
	color.b = color.r;*/

	float rg = mix(night, day, dayNightRatio);
	rg += light_source * 2.5; // Make light sources brighter
	float b = rg;

	// Moonlight is blue
	b += (day - night) / 13.0;
	rg -= (day - night) / 13.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	b += max(0.0, (1.0 - abs(b - 0.13)/0.17) * 0.025);

	// Artificial light is yellow-ish
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	rg += max(0.0, (1.0 - abs(rg - 0.85)/0.15) * 0.065);

	color.r = clamp(rg,0.0,1.0);
	color.g = clamp(rg,0.0,1.0);
	color.b = clamp(b,0.0,1.0);
	color.a = gl_Color.a;

	gl_FrontColor = gl_BackColor = color;

	gl_TexCoord[0] = gl_MultiTexCoord0;

}
