uniform mat4 mWorld;
// Color of the light emitted by the sun.
uniform vec3 dayLight;

// The cameraOffset is the current center of the visible world.
uniform highp vec3 cameraOffset;
uniform float animationTimer;

varying vec3 vNormal;
varying vec3 vPosition;
// World position in the visible world (i.e. relative to the cameraOffset.)
// This can be used for many shader effects without loss of precision.
// If the absolute position is required it can be calculated with
// cameraOffset + worldPosition (for large coordinates the limits of float
// precision must be considered).
varying vec3 worldPosition;

varying lowp vec3 artificialColor;
varying lowp vec3 naturalColor;
// The centroid keyword ensures that after interpolation the texture coordinates
// lie within the same bounds when MSAA is en- and disabled.
// This fixes the stripes problem with nearest-neighbor textures and MSAA.
#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif
#ifdef ENABLE_DYNAMIC_SHADOWS
	// shadow uniforms
	uniform vec3 v_LightDirection;
	uniform float f_textureresolution;
	uniform mat4 m_ShadowViewProj;
	uniform float f_shadowfar;
	uniform float f_shadow_strength;
	uniform float f_timeofday;
	uniform vec4 CameraPos;

	varying float cosLight;
	varying float normalOffsetScale;
	varying float directLightIntensity;
	varying float f_normal_length;
	varying vec3 shadow_position;
	varying float perspective_factor;
	varying lowp vec3 directNaturalLight;
#endif

varying float area_enable_parallax;

varying highp vec3 eyeVec;
varying float nightRatio;
// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
const float e = 2.718281828459;
const float BS = 10.0;
uniform float xyPerspectiveBias0;
uniform float xyPerspectiveBias1;
uniform float zPerspectiveBias;

#ifdef ENABLE_DYNAMIC_SHADOWS

vec4 getRelativePosition(in vec4 position)
{
	vec2 l = position.xy - CameraPos.xy;
	vec2 s = l / abs(l);
	s = (1.0 - s * CameraPos.xy);
	l /= s;
	return vec4(l, s);
}

float getPerspectiveFactor(in vec4 relativePosition)
{
	float pDistance = length(relativePosition.xy);
	float pFactor = pDistance * xyPerspectiveBias0 + xyPerspectiveBias1;
	return pFactor;
}

vec4 applyPerspectiveDistortion(in vec4 position)
{
	vec4 l = getRelativePosition(position);
	float pFactor = getPerspectiveFactor(l);
	l.xy /= pFactor;
	position.xy = l.xy * l.zw + CameraPos.xy;
	position.z *= zPerspectiveBias;
	return position;
}

// custom smoothstep implementation because it's not defined in glsl1.2
// https://docs.gl/sl4/smoothstep
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

vec3 getDirectLightScatteringAtGround(vec3 dayLight, vec3 v_LightDirection)
{
	// Based on talk at 2002 Game Developers Conference by Naty Hoffman and Arcot J. Preetham
	const float beta_r0 = 1e-5; // Rayleigh scattering beta

	// These factors are calculated based on expected value of scattering factor of 1e-5
	// for Nitrogen at 532nm (green), 2e25 molecules/m3 in atmosphere
	const vec3 beta_r0_l = vec3(3.3362176e-01, 8.75378289198826e-01, 1.95342379700656) * beta_r0; // wavelength-dependent scattering

	const float atmosphere_height = 15000.; // height of the atmosphere in meters
	// sun/moon light at the ground level, after going through the atmosphere
	return exp(-beta_r0_l * atmosphere_height / (1e-5 - dot(v_LightDirection, vec3(0., 1., 0.))));
}

// calculates light intensity from a sun-like or moon-like sky body
float getLightIntensity(vec3 direction)
{
	const float distance_to_size_ratio = 107.143; // ~= 150million km / 1.4 million km

	return clamp(distance_to_size_ratio * dot(vec3(0., 1., 0.), -v_LightDirection), 0., 1.);
}
#endif


vec3 getArtificialLightTint(float intensity)
{
	float temperature = 4250. + 3250. * (0.0 + 1.0 * pow(intensity, 1.0));
	// Aproximation of RGB values for specific color temperature in range of 2500K to 7500K
	// Based on the table at https://andi-siess.de/rgb-to-color-temperature/
	vec3 color = min(temperature * vec3(0., 9.11765e-05, 1.77451e-04) + vec3(1., 0.403431, -0.161275),
			temperature * vec3(-7.84314e-05, -6.27451e-05, 7.84314e-06) + vec3(1.50980, 1.40392, 0.941176));
	// color /= dot(color, vec3(0.213, 0.715, 0.072));
	return color;
}

vec3 getNaturalLightTint(float brightness)
{
	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp final_color_blend()
	float b = max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);
	return vec3(0.0, 0.0, b);
}

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
float water_level(vec3 p)
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

// Always return a constant value to make the surface flat
return 0.05;

}

#endif

void main(void)
{
	varTexCoord = inTexCoord0.st;

	float disp_x;
	float disp_z;
// OpenGL < 4.3 does not support continued preprocessor lines
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES) || (MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS)
	vec4 pos2 = mWorld * inVertexPosition;
	float tOffset = (pos2.x + pos2.y) * 0.001 + pos2.z * 0.002;
	disp_x = (smoothTriangleWave(animationTimer * 23.0 + tOffset) +
		smoothTriangleWave(animationTimer * 11.0 + tOffset)) * 0.4;
	disp_z = (smoothTriangleWave(animationTimer * 31.0 + tOffset) +
		smoothTriangleWave(animationTimer * 29.0 + tOffset) +
		smoothTriangleWave(animationTimer * 13.0 + tOffset)) * 0.5;
#endif

	vec4 pos = inVertexPosition;
// OpenGL < 4.3 does not support continued preprocessor lines
#if (MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_OPAQUE || MATERIAL_TYPE == TILE_MATERIAL_WAVING_LIQUID_BASIC) && ENABLE_WAVING_WATER// &&  !ENABLE_WATER_REFLECTIONS
	// Generate waves with Perlin-type noise.
	// The constants are calibrated such that they roughly
	// correspond to the old sine waves.
	#if ENABLE_LIQUID_REFLECTIONS
	vec3 wavePos = (mWorld * pos).xyz + cameraOffset;
	pos.y += (water_level(wavePos) - 1.0) * WATER_WAVE_HEIGHT * 2.0;
	#else
	vec3 wavePos = (mWorld * pos).xyz + cameraOffset;
	// The waves are slightly compressed along the z-axis to get
	// wave-fronts along the x-axis.
	wavePos.x /= WATER_WAVE_LENGTH * 3.0;
	wavePos.z /= WATER_WAVE_LENGTH * 2.0;
	wavePos.z += animationTimer * WATER_WAVE_SPEED * 10.0;
	pos.y += (snoise(wavePos) - 1.0) * WATER_WAVE_HEIGHT * 5.0;
	#endif

#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_LEAVES && ENABLE_WAVING_LEAVES
	pos.x += disp_x;
	pos.y += disp_z * 0.1;
	pos.z += disp_z;
#elif MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
	if (varTexCoord.y < 0.05) {
		pos.x += disp_x;
		pos.z += disp_z;vec3 a = floor(p);
	}
#endif
	worldPosition = (mWorld * pos).xyz;
	gl_Position = mWorldViewProj * pos;

	vPosition = gl_Position.xyz;
	eyeVec = -(mWorldView * pos).xyz;
#ifdef SECONDSTAGE
	normalPass = normalize((inVertexNormal+1)/2);
#endif
	vNormal = inVertexNormal;

	// Calculate color.
	// Red, green and blue components are pre-multiplied with
	// the brightness, so now we have to multiply these
	// colors with the color of the incoming light.
	// The pre-baked colors are halved to prevent overflow.
#ifdef GL_ES
	vec4 color = inVertexColor.bgra;
#else
	vec4 color = inVertexColor;
#endif
	// The alpha gives the ratio of sunlight in the incoming light.
	nightRatio = 1.0 - color.a;

	// turns out that nightRatio falls off much faster than
	// actual brightness of artificial light in relation to natual light.
	// Power ratio was measured on torches in MTG (brightness = 14).
	float adjusted_night_ratio = pow(max(0.0, nightRatio), 0.6);
	
	artificialColor = color.rgb * adjusted_night_ratio * 2.0;
	artificialColor *= getArtificialLightTint(dot(artificialColor, vec3(0.33)));

	naturalColor = color.rgb * (1.0 - adjusted_night_ratio) * 2.0;
	naturalColor += getNaturalLightTint(dot(naturalColor, vec3(0.33)));

#ifdef ENABLE_DYNAMIC_SHADOWS
	directNaturalLight = getDirectLightScatteringAtGround(vec3(1.0), v_LightDirection) * getLightIntensity(v_LightDirection);
	
	if (f_shadow_strength > 0.0) {
#if MATERIAL_TYPE == TILE_MATERIAL_WAVING_PLANTS && ENABLE_WAVING_PLANTS
		// The shadow shaders don't apply waving when creating the shadow-map.
		// We are using the not waved inVertexPosition to avoid ugly self-shadowing.
		vec4 shadow_pos = inVertexPosition;
#else
		vec4 shadow_pos = pos;
#endif
		vec3 nNormal;
		f_normal_length = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(m_ShadowViewProj * mWorld * shadow_pos));
		if (f_normal_length > 0.0) {
			nNormal = normalize(vNormal);
			cosLight = max(1e-5, dot(nNormal, -v_LightDirection));
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 2.0 * pFactor * pFactor * sinLight * min(f_shadowfar, 500.0) /
					xyPerspectiveBias1 / f_textureresolution;
			z_bias = 1.0 * sinLight / cosLight;
		}
		else {
			nNormal = vec3(0.0);
			cosLight = clamp(dot(v_LightDirection, normalize(vec3(v_LightDirection.x, 0.0, v_LightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / cosLight;
		}
		z_bias *= pFactor * pFactor / f_textureresolution / f_shadowfar;

		shadow_position = applyPerspectiveDistortion(m_ShadowViewProj * mWorld * (shadow_pos + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
		shadow_position.z -= z_bias;
		perspective_factor = pFactor;
	}
#endif
}
