#define exposure texture0
#define screen texture1

uniform sampler2D exposure;
uniform sampler2D screen;

#ifdef ENABLE_BLOOM
uniform float bloomStrength;
#else
const float bloomStrength = 1.0;
#endif
uniform mediump float exposureFactor;
uniform float animationTimerDelta;

struct ExposureParams {
	float luminanceMin;
	float luminanceMax;
	float luminanceBias;
	float luminanceKey;
	float speedDarkBright;
	float speedBrightDark;
	float centerWeightPower;
} exposureParams = ExposureParams(0.02, 10.0, 0.0, 0.5, 2.0, 0.2, 2.0);

const vec3 luminanceFactors = vec3(0.213, 0.715, 0.072);

float getLuminance(vec3 color)
{
	return dot(color, luminanceFactors);
}

void main(void)
{
	float previousExposure = texture2D(exposure, vec2(0.5, 0.5)).r;

	previousExposure = clamp(previousExposure, 1e-10, 1e5);

	vec3 averageColor = vec3(0.);
	float n = 0.;

	// Scan the screen with center-weighting and sample average color
	for (float _x = 0.1; _x < 0.9; _x += 0.17) {
		float x = pow(_x, exposureParams.centerWeightPower);
		for (float _y = 0.1; _y < 0.9; _y += 0.17) {
			float y = pow(_y, exposureParams.centerWeightPower);
			averageColor += texture2D(screen, vec2(0.5 + 0.5 * x, 0.5 + 0.5 * y)).rgb;
			averageColor += texture2D(screen, vec2(0.5 + 0.5 * x, 0.5 - 0.5 * y)).rgb;
			averageColor += texture2D(screen, vec2(0.5 - 0.5 * x, 0.5 + 0.5 * y)).rgb;
			averageColor += texture2D(screen, vec2(0.5 - 0.5 * x, 0.5 - 0.5 * y)).rgb;
			n += 4;
		}
	}

	float luminance = getLuminance(averageColor);
	luminance /= n;

	luminance /= bloomStrength * exposureFactor; // compensate for the configurable factors

	// Equations borrowed from https://knarkowicz.wordpress.com/2016/01/09/automatic-exposure/
	float wantedExposure = exposureParams.luminanceKey / max(1e-10, clamp(luminance, exposureParams.luminanceMin, exposureParams.luminanceMax) - exposureParams.luminanceBias);

	if (wantedExposure < previousExposure)
		wantedExposure = mix(previousExposure, wantedExposure, 1. - exp(-animationTimerDelta * 100. * exposureParams.speedDarkBright)); // dark -> bright
	else
		wantedExposure = mix(previousExposure, wantedExposure, 1. - exp(-animationTimerDelta * 100. * exposureParams.speedBrightDark)); // bright -> dark

	gl_FragColor = vec4(vec3(wantedExposure), 1.);
}
