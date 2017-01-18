uniform mat4 mWorldViewProj;
uniform mat4 mWorld;

// Color of the light emitted by the sun.
uniform vec3 dayLight;
uniform vec3 eyePosition;
uniform float animationTimer;

varying vec3 vPosition;
varying vec3 worldPosition;

varying vec3 eyeVec;
varying vec3 lightVec;
varying vec3 tsEyeVec;
varying vec3 tsLightVec;

// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
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

	// Calculate color.
	// Red, green and blue components are pre-multiplied with
	// the brightness, so now we have to multiply these
	// colors with the color of the incoming light.
	// The pre-baked colors are halved to prevent overflow.
	vec4 color;
	// The alpha gives the ratio of sunlight in the incoming light.
	float nightRatio = 1 - gl_Color.a;
	color.rgb = gl_Color.rgb * (gl_Color.a * dayLight.rgb + 
		nightRatio * artificialLight.rgb) * 2;
	color.a = 1;
	
	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	float brightness = (color.r + color.g + color.b) / 3;
	color.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);
	
	gl_FrontColor = gl_BackColor = clamp(color, 0.0, 1.0);
}
