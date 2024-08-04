uniform mat4 mWorld;
uniform vec3 dayLight;
uniform float animationTimer;
uniform lowp vec4 emissiveColor;

varying vec3 vNormal;
varying vec3 vPosition;
varying vec3 worldPosition;
varying lowp vec3 artificialColor;
varying lowp vec3 naturalColor;
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
	varying float f_normal_length;
	varying vec3 shadow_position;
	varying float perspective_factor;
	varying lowp vec3 directNaturalLight;
#endif

varying highp vec3 eyeVec;
varying float nightRatio;
// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
varying float vIDiff;
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

float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0.0)
		return dot(v, vec3(0.670820, 0.447213, 0.836660));

	return dot(v, vec3(0.670820, 1.000000, 0.836660));
}

void main(void)
{
	varTexCoord = (mTexture * inTexCoord0).st;
	gl_Position = mWorldViewProj * inVertexPosition;

	vPosition = gl_Position.xyz;
	vNormal = (mWorld * vec4(inVertexNormal, 0.0)).xyz;
	worldPosition = (mWorld * inVertexPosition).xyz;
	eyeVec = -(mWorldView * inVertexPosition).xyz;

#if (MATERIAL_TYPE == TILE_MATERIAL_PLAIN) || (MATERIAL_TYPE == TILE_MATERIAL_PLAIN_ALPHA)
	vIDiff = 1.0;
#else
	// This is intentional comparison with zero without any margin.
	// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
	vIDiff = length(inVertexNormal) == 0.0
		? 1.0
		: directional_ambient(normalize(inVertexNormal));
#endif

#ifdef GL_ES
	vec4 color = inVertexColor.bgra;
#else
	vec4 color = inVertexColor;
#endif

	color *= emissiveColor;

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
		vec3 nNormal = normalize(vNormal);
		f_normal_length = length(vNormal);

		/* normalOffsetScale is in world coordinates (1/10th of a meter)
		z_bias is in light space coordinates */
		float normalOffsetScale, z_bias;
		float pFactor = getPerspectiveFactor(getRelativePosition(m_ShadowViewProj * mWorld * inVertexPosition));
		if (f_normal_length > 0.0) {
			nNormal = normalize(vNormal);
			cosLight = max(1e-5, dot(nNormal, -v_LightDirection));
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 0.1 * pFactor * pFactor * sinLight * min(f_shadowfar, 500.0) /
					xyPerspectiveBias1 / f_textureresolution;
			z_bias = 1e3 * sinLight / cosLight * (0.5 + f_textureresolution / 1024.0);
		}
		else {
			nNormal = vec3(0.0);
			cosLight = clamp(dot(v_LightDirection, normalize(vec3(v_LightDirection.x, 0.0, v_LightDirection.z))), 1e-2, 1.0);
			float sinLight = pow(1.0 - pow(cosLight, 2.0), 0.5);
			normalOffsetScale = 0.0;
			z_bias = 3.6e3 * sinLight / cosLight;
		}
		z_bias *= pFactor * pFactor / f_textureresolution / f_shadowfar;

		shadow_position = applyPerspectiveDistortion(m_ShadowViewProj * mWorld * (inVertexPosition + vec4(normalOffsetScale * nNormal, 0.0))).xyz;
		shadow_position.z -= z_bias;
		perspective_factor = pFactor;

	}
#endif
}
