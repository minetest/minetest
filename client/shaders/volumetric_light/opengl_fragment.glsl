#define rendered texture0
#define depthmap texture1

uniform sampler2D rendered;
uniform sampler2D depthmap;

uniform vec3 sunPositionScreen;
uniform float sunBrightness;
uniform vec3 moonPositionScreen;
uniform float moonBrightness;

uniform lowp float volumetricLightStrength;

uniform vec3 dayLight;
#ifdef ENABLE_DYNAMIC_SHADOWS
uniform vec3 v_LightDirection;
#else
const vec3 v_LightDirection = vec3(0.0, -1.0, 0.0);
#endif

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

uniform vec3 beta_r0_l;

const float far = 1000.;
float mapDepth(float depth)
{
	return min(1., 1. / (1.00001 - depth) / far);
}

float noise(vec3 uvd) {
	return fract(dot(sin(uvd * vec3(13041.19699, 27723.29171, 61029.77801)), vec3(73137.11101, 37312.92319, 10108.89991)));
}

float sampleVolumetricLight(vec2 uv, vec3 lightVec, float rawDepth)
{
	lightVec = 0.5 * lightVec / lightVec.z + 0.5;
	const float samples = 30.;
	float result = texture2D(depthmap, uv).r < 1. ? 0.0 : 1.0;
	float bias = noise(vec3(uv, rawDepth));
	vec2 samplepos;
	for (float i = 1.; i < samples; i++) {
		samplepos = mix(uv, lightVec.xy, (i + bias) / samples);
		if (min(samplepos.x, samplepos.y) > 0. && max(samplepos.x, samplepos.y) < 1.)
			result += texture2D(depthmap, samplepos).r < 1. ? 0.0 : 1.0;
	}

#ifdef VOLUMETRIC_DEPTH_ATTENUATION
	// We use the depth map to approximate the effect of depth on the light intensity.
	// The exponent was chosen based on aesthetic preference.
	// To make this phsyically accurate, the brightness here should scale linearly with depth,
	// but this would make the godrays either too faint or too strong in many cases.
	return result / samples * pow(texture2D(depthmap, uv).r, 128.0);
#else
	return result / samples;
#endif
}

vec3 getDirectLightScatteringAtGround(vec3 v_LightDirection)
{
	// Based on talk at 2002 Game Developers Conference by Naty Hoffman and Arcot J. Preetham
	const float beta_r0 = 1e-5; // Rayleigh scattering beta

	const float atmosphere_height = 15000.; // height of the atmosphere in meters
	// sun/moon light at the ground level, after going through the atmosphere

	return exp(-beta_r0_l * beta_r0 * atmosphere_height / (1e-5 - dot(v_LightDirection, vec3(0., 1., 0.))));
}

vec3 applyVolumetricLight(vec3 color, vec2 uv, float rawDepth)
{
	vec3 lookDirection = normalize(vec3(uv.x * 2. - 1., uv.y * 2. - 1., rawDepth));

	const float boost = 4.0;
	float brightness = 0.;
	vec3 sourcePosition = vec3(-1., -1., -1);

	if (sunPositionScreen.z > 0. && sunBrightness > 0.) {
		brightness = sunBrightness;
		sourcePosition = sunPositionScreen;
	}
	else if (moonPositionScreen.z > 0. && moonBrightness > 0.) {
		brightness = moonBrightness * 0.05;
		sourcePosition = moonPositionScreen;
	}

	float cameraDirectionFactor = pow(clamp(dot(sourcePosition, vec3(0., 0., 1.)), 0.0, 0.7), 2.5);
	float viewAngleFactor = pow(max(0., dot(sourcePosition, lookDirection)), 8.);

	float lightFactor = brightness * sampleVolumetricLight(uv, sourcePosition, rawDepth) *
			(0.05 * cameraDirectionFactor + 0.95 * viewAngleFactor);

	color = mix(color, boost * getDirectLightScatteringAtGround(v_LightDirection) * dayLight, lightFactor);

	// a factor of 5 tested well
	color *= volumetricLightStrength * 5.0;

	// if (sunPositionScreen.z < 0.)
	// 	color.rg += 1. - clamp(abs((2. * uv.xy - 1.) - sunPositionScreen.xy / sunPositionScreen.z) * 1000., 0., 1.);
	// if (moonPositionScreen.z < 0.)
	// 	color.rg += 1. - clamp(abs((2. * uv.xy - 1.) - moonPositionScreen.xy / moonPositionScreen.z) * 1000., 0., 1.);
	return color;
}

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec3 color = texture2D(rendered, uv).rgb;
	// translate to linear colorspace (approximate)
	color = pow(color, vec3(2.2));

	if (volumetricLightStrength > 0.0) {
		float rawDepth = texture2D(depthmap, uv).r;

		color = applyVolumetricLight(color, uv, rawDepth);
	}

	gl_FragColor = vec4(color, 1.0); // force full alpha to avoid holes in the image.
}
