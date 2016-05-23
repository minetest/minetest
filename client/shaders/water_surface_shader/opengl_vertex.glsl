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

float smoothCurve(float x)
{
	return x * x * (3.0 - 2.0 * x);
}
float triangleWave(float x)
{
	return abs(fract( x + 0.5 ) * 2.0 - 1.0);
}
float smoothTriangleWave(float x)
{
	return smoothCurve(triangleWave( x )) * 2.0 - 1.0;
}

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;

#if (MATERIAL_TYPE == TILE_MATERIAL_LIQUID_TRANSPARENT || MATERIAL_TYPE == TILE_MATERIAL_LIQUID_OPAQUE) && ENABLE_WAVING_WATER
	vec4 pos = gl_Vertex;
	pos.y -= 2.0;

	float posYbuf = (pos.z / WATER_WAVE_LENGTH + animationTimer * WATER_WAVE_SPEED * WATER_WAVE_LENGTH);

	pos.y -= sin(posYbuf) * WATER_WAVE_HEIGHT + sin(posYbuf / 7.0) * WATER_WAVE_HEIGHT;
	gl_Position = mWorldViewProj * pos;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES
	vec4 pos = gl_Vertex;
	vec4 pos2 = mWorld * gl_Vertex;
	/*
	 * Mathematic optimization: pos2.x * A + pos2.z * A (2 multiplications + 1 addition)
	 * replaced with: (pos2.x + pos2.z) * A (1 addition + 1 multiplication)
	 * And bufferize calcul to a float
	 */
	float pos2XpZ = pos2.x + pos2.z;
	pos.x += (smoothTriangleWave(animationTimer*10.0 + pos2XpZ * 0.01) * 2.0 - 1.0) * 0.4;
	pos.y += (smoothTriangleWave(animationTimer*15.0 + pos2XpZ * -0.01) * 2.0 - 1.0) * 0.2;
	pos.z += (smoothTriangleWave(animationTimer*10.0 + pos2XpZ * -0.01) * 2.0 - 1.0) * 0.4;
	gl_Position = mWorldViewProj * pos;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
	vec4 pos = gl_Vertex;
	vec4 pos2 = mWorld * gl_Vertex;
	if (gl_TexCoord[0].y < 0.05) {
		/*
		 * Mathematic optimization: pos2.x * A + pos2.z * A (2 multiplications + 1 addition)
		 * replaced with: (pos2.x + pos2.z) * A (1 addition + 1 multiplication)
		 * And bufferize calcul to a float
		 */
		float pos2XpZ = pos2.x + pos2.z;
		pos.x += (smoothTriangleWave(animationTimer * 20.0 + pos2XpZ * 0.1) * 2.0 - 1.0) * 0.8;
		pos.y -= (smoothTriangleWave(animationTimer * 10.0 + pos2XpZ * -0.5) * 2.0 - 1.0) * 0.4;
	}
	gl_Position = mWorldViewProj * pos;
#else
	gl_Position = mWorldViewProj * gl_Vertex;
#endif

	vPosition = gl_Position.xyz;
	worldPosition = (mWorld * gl_Vertex).xyz;
	vec3 sunPosition = vec3 (0.0, eyePosition.y * BS + 900.0, 0.0);

	vec3 normal, tangent, binormal;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	if (gl_Normal.x > 0.5) {
		//  1.0,  0.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 0.0,  0.0, -1.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	} else if (gl_Normal.x < -0.5) {
		// -1.0,  0.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 0.0,  0.0,  1.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	} else if (gl_Normal.y > 0.5) {
		//  0.0,  1.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0,  0.0,  1.0));
	} else if (gl_Normal.y < -0.5) {
		//  0.0, -1.0,  0.0
		tangent  = normalize(gl_NormalMatrix * vec3( 1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0,  0.0,  1.0));
	} else if (gl_Normal.z > 0.5) {
		//  0.0,  0.0,  1.0
		tangent  = normalize(gl_NormalMatrix * vec3( 1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	} else if (gl_Normal.z < -0.5) {
		//  0.0,  0.0, -1.0
		tangent  = normalize(gl_NormalMatrix * vec3(-1.0,  0.0,  0.0));
		binormal = normalize(gl_NormalMatrix * vec3( 0.0, -1.0,  0.0));
	}
	mat3 tbnMatrix = mat3(tangent.x, binormal.x, normal.x,
							tangent.y, binormal.y, normal.y,
							tangent.z, binormal.z, normal.z);

	lightVec = sunPosition - worldPosition;
	tsLightVec = lightVec * tbnMatrix;
	eyeVec = (gl_ModelViewMatrix * gl_Vertex).xyz;
	tsEyeVec = eyeVec * tbnMatrix;

	vec4 color;
	float day = gl_Color.r;
	float night = gl_Color.g;
	float light_source = gl_Color.b;

	float rg = mix(night, day, dayNightRatio);
	rg += light_source * 2.5; // Make light sources brighter
	float b = rg;

	// Moonlight is blue
	b += (day - night) / 13.0;
	rg -= (day - night) / 23.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	b += max(0.0, (1.0 - abs(b - 0.13)/0.17) * 0.025);

	// Artificial light is yellow-ish
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	rg += max(0.0, (1.0 - abs(rg - 0.85)/0.15) * 0.065);

	color.r = rg;
	color.g = rg;
	color.b = b;

	color.a = gl_Color.a;
	gl_FrontColor = gl_BackColor = clamp(color,0.0,1.0);
}
