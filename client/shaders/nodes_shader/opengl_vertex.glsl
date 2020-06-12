uniform mat4 mWorldViewProj;
uniform mat4 mWorld;

// Color of the light emitted by the sun.
uniform vec3 dayLight;
uniform vec3 eyePosition;

// The cameraOffset is the current center of the visible world.
uniform vec3 cameraOffset;
uniform float animationTimer;

varying vec3 vPosition;
// World position in the visible world (i.e. relative to the cameraOffset.)
// This can be used for many shader effects without loss of precision.
// If the absolute position is required it can be calculated with
// cameraOffset + worldPosition (for large coordinates the limits of float
// precision must be considered).
varying vec3 worldPosition;

varying vec3 eyeVec;
varying vec3 lightVec;
varying vec3 tsEyeVec;
varying vec3 tsLightVec;
varying float area_enable_parallax;

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
	return abs(fract(x + 0.5) * 2.0 - 1.0);
}


float smoothTriangleWave(float x)
{
	return smoothCurve(triangleWave(x)) * 2.0 - 1.0;
}

// OpenGL < 4.3 does not support continued preprocessor lines
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_OPAQUE || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_BASIC) && ENABLE_WAVING_WATER

//
// Simple, fast noise function.
// See: https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
//
vec4 perm(vec4 x)
{
	return mod(((x * 34.0) + 1.0) * x, 289.0);
}

float snoise(vec3 p)
{
	vec3 a = floor(p);
	vec3 d = p - a;
	d = d * d * (3.0 - 2.0 * d);

	vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
	vec4 k1 = perm(b.xyxy);
	vec4 k2 = perm(k1.xyxy + b.zzww);

	vec4 c = k2 + a.zzzz;
	vec4 k3 = perm(c);
	vec4 k4 = perm(c + 1.0);

	vec4 o1 = fract(k3 * (1.0 / 41.0));
	vec4 o2 = fract(k4 * (1.0 / 41.0));

	vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
	vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

	return o4.y * d.y + o4.x * (1.0 - d.y);
}

#endif

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	//TODO: make offset depending on view angle and parallax uv displacement
	//thats for textures that doesnt align vertically, like dirt with grass
	//gl_TexCoord[0].y += 0.008;

	//Allow parallax/relief mapping only for certain kind of nodes
	//Variable is also used to control area of the effect
#if (DRAW_TYPE == NDT_NORMAL || DRAW_TYPE == NDT_LIQUID || DRAW_TYPE == NDT_FLOWINGLIQUID)
	area_enable_parallax = 1.0;
#else
	area_enable_parallax = 0.0;
#endif


float disp_x;
float disp_z;
// OpenGL < 4.3 does not support continued preprocessor lines
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES) || (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS)
	vec4 pos2 = mWorld * gl_Vertex;
	float tOffset = (pos2.x + pos2.y) * 0.001 + pos2.z * 0.002;
	disp_x = (smoothTriangleWave(animationTimer * 23.0 + tOffset) +
		smoothTriangleWave(animationTimer * 11.0 + tOffset)) * 0.4;
	disp_z = (smoothTriangleWave(animationTimer * 31.0 + tOffset) +
		smoothTriangleWave(animationTimer * 29.0 + tOffset) +
		smoothTriangleWave(animationTimer * 13.0 + tOffset)) * 0.5;
#endif

	worldPosition = (mWorld * gl_Vertex).xyz;

// OpenGL < 4.3 does not support continued preprocessor lines
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_OPAQUE || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_BASIC) && ENABLE_WAVING_WATER
	// Generate waves with Perlin-type noise.
	// The constants are calibrated such that they roughly
	// correspond to the old sine waves.
	vec4 pos = gl_Vertex;
	vec3 wavePos = worldPosition + cameraOffset;
	// The waves are slightly compressed along the z-axis to get
	// wave-fronts along the x-axis.
	wavePos.x /= WATER_WAVE_LENGTH * 3;
	wavePos.z /= WATER_WAVE_LENGTH * 2;
	wavePos.z += animationTimer * WATER_WAVE_SPEED * 10;
	pos.y += (snoise(wavePos) - 1) * WATER_WAVE_HEIGHT * 5;
	gl_Position = mWorldViewProj * pos;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES
	vec4 pos = gl_Vertex;
	pos.x += disp_x;
	pos.y += disp_z * 0.1;
	pos.z += disp_z;
	gl_Position = mWorldViewProj * pos;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
	vec4 pos = gl_Vertex;
	if (gl_TexCoord[0].y < 0.05) {
		pos.x += disp_x;
		pos.z += disp_z;
	}
	gl_Position = mWorldViewProj * pos;
#else
	gl_Position = mWorldViewProj * gl_Vertex;
#endif


	vPosition = gl_Position.xyz;

	// Don't generate heightmaps when too far from the eye
	float dist = distance (vec3(0.0, 0.0, 0.0), vPosition);
	if (dist > 150.0) {
		area_enable_parallax = 0.0;
	}

	vec3 sunPosition = vec3 (0.0, eyePosition.y * BS + 900.0, 0.0);

	vec3 normal, tangent, binormal;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	tangent = normalize(gl_NormalMatrix * gl_MultiTexCoord1.xyz);
	binormal = normalize(gl_NormalMatrix * gl_MultiTexCoord2.xyz);

	vec3 v;

	lightVec = sunPosition - worldPosition;
	v.x = dot(lightVec, tangent);
	v.y = dot(lightVec, binormal);
	v.z = dot(lightVec, normal);
	tsLightVec = normalize (v);

	eyeVec = -(gl_ModelViewMatrix * gl_Vertex).xyz;
	v.x = dot(eyeVec, tangent);
	v.y = dot(eyeVec, binormal);
	v.z = dot(eyeVec, normal);
	tsEyeVec = normalize (v);

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
	// See C++ implementation in mapblock_mesh.cpp final_color_blend()
	float brightness = (color.r + color.g + color.b) / 3;
	color.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	gl_FrontColor = gl_BackColor = clamp(color, 0.0, 1.0);
}
