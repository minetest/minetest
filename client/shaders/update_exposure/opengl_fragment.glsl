#define exposure texture0
#define screen texture1

struct ExposureParams {
	float luminanceMin;
	float luminanceMax;
	float exposureCorrection;
	float luminanceKey;
	float speedDarkBright;
	float speedBrightDark;
	float centerWeightPower;
	float compensationFactor;
};

uniform sampler2D exposure;
uniform sampler2D screen;

#ifdef ENABLE_BLOOM
uniform float bloomStrength;
#else
const float bloomStrength = 1.0;
#endif
uniform ExposureParams exposureParams;
uniform float animationTimerDelta;


const vec3 luminanceFactors = vec3(0.213, 0.715, 0.072);

float getLuminance(vec3 color)
{
	return dot(color, luminanceFactors);
}

void main(void)
{
	float previousExposure = texture2D(exposure, vec2(0.5, 0.5)).r;

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
			n += 4.;
		}
	}

	float luminance = getLuminance(averageColor);
	luminance /= n;

	luminance /= pow(2., previousExposure) * bloomStrength * exposureParams.compensationFactor; // compensate for the configurable factors

	luminance = clamp(luminance, exposureParams.luminanceMin, exposureParams.luminanceMax);

	// From https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v2.pdf
	// 1. EV100 = log2(luminance * S / K) where S = 100, K = 0.125 = log2(luminance) + 3
	// 2. Lmax = 1.2 * 2 ^ (EV100 - EC)
	//    => Lmax = 1.2 * 2^3 * luminance / 2^EC = 9.6 * luminance / 2^EC
	// 3. exposure = 1 / Lmax
	//    => exposure = 2^EC / (9.6 * luminance)
	float wantedExposure = exposureParams.exposureCorrection - log(luminance)/0.693147180559945 - 3.263034405833794;

	if (wantedExposure < previousExposure)
		wantedExposure = mix(wantedExposure, previousExposure, exp(-animationTimerDelta * exposureParams.speedDarkBright)); // dark -> bright
	else
		wantedExposure = mix(wantedExposure, previousExposure, exp(-animationTimerDelta * exposureParams.speedBrightDark)); // bright -> dark

	gl_FragColor = vec4(vec3(wantedExposure), 1.);
}
