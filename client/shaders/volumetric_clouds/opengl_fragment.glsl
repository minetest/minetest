#define depthmap texture0
#define noiseTexture texture1
#define noiseTextureCoarse texture2

#define ITERATIONS 50
#define LIGHT_ITERATIONS 10
#define LIGHT_DISTANCE 100.
#define AURORA_ITERATIONS 100

uniform sampler2D depthmap;
uniform sampler2D noiseTexture;
uniform sampler2D noiseTextureCoarse;

uniform vec2 texelSize0;

uniform float cloudHeight;
uniform float cloudThickness;
uniform float cloudDensity;

varying vec3 relativePosition;
varying vec3 viewDirection;
uniform vec3 eyePosition;
uniform vec3 cameraOffset;
uniform vec3 cameraPosition;

uniform mat4 mCameraView;
uniform mat4 mCameraProjInv;

uniform float cameraNear;
uniform float cameraFar;

varying vec2 screenspaceCoordinate;

uniform float fogDistance;
uniform float fogShadingParameter;

uniform vec3 v_LightDirection;

uniform float animationTimer;

// Derived From http://alex.vlachos.com/graphics/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
// and https://www.shadertoy.com/view/MslGR8 (5th one starting from the bottom)
// NOTE: `frag_coord` is in pixels (i.e. not normalized UV).
float screenSpaceDither(highp vec2 frag_coord) {
	// Iestyn's RGB dither (7 asm instructions) from Portal 2 X360, slightly modified for VR.
	highp float dither = dot(vec2(171.0, 231.0), frag_coord);
	dither = fract(dither / 103.0);

	return dither;
}

// custom smoothstep implementation because it's not defined in glsl1.2
// https://docs.gl/sl4/smoothstep
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

float getDepth(vec2 screenspacePosition) {
	float depth = texture2D(depthmap, screenspacePosition * 0.5 + 0.5).r;
	return cameraNear * cameraFar / (cameraFar + depth * (cameraNear - cameraFar));
}

float getRawDepth(vec2 screenspacePosition) {
	return texture2D(depthmap, screenspacePosition * 0.5 + 0.5).r;
}

float noise(vec3 p){
    //p.y *= 1.;
    float y = floor(p.y);
    float f1 = texture2D(noiseTexture, p.xz / 256. + y * 0.2).r;
    float f2 = texture2D(noiseTexture, p.xz / 256. + y * 0.2 + 0.2).r;
    return mix(f1, f2, fract(p.y));
}

float fnoise(vec3 p) {
    return noise(p * 4.) * 0.5 + noise(p * 8.) * 0.25;
}

float fnoise3(vec3 p) {
    return noise(p * 4.) * 0.5 + noise(p * 8.) * 0.25 + noise(p * 16.) * 0.125;
}

float getAuroraDensity(vec3 position) {
	float density = pow(max(0., 1. - 10. * abs(fnoise3(vec3(position.x * 0.25, animationTimer, position.z * 0.25)) - 0.5)), 4.);
	return 1.0 * density * mtsmoothstep(0.0, 0.05, position.y - 1.) * pow(1. - mtsmoothstep(0.05, 2.0, position.y - 1.), 4.);
}

float getDensity(vec3 position) {
	float density = texture2D(noiseTextureCoarse, position.xz / 2560. / 16.).r *
		mtsmoothstep(0.0, cloudThickness * 0.2, position.y - cloudHeight) *
		(1.0 - mtsmoothstep(cloudThickness * 0.5, cloudThickness, position.y - cloudHeight));

	density = max(0., density - 0.5 * fnoise(position * 0.005));

	return 0.04 * density;
}

float getBrightness(vec3 position, float bias) {
	float density = 0.;
	for (int i = 0; i < LIGHT_ITERATIONS; i++) {
		vec3 rayPosition = position - v_LightDirection * LIGHT_DISTANCE * (float(i) + bias) / float(LIGHT_ITERATIONS);

		density += getDensity(rayPosition) * float(LIGHT_DISTANCE) / float(LIGHT_ITERATIONS);
	}
	return exp(-density);
}

float blend(float A, float B, float alphaA, float alphaB) {
    float alphaC = alphaA + (1. - alphaA) * alphaB;
    return (alphaA * A + (1. - alphaA) * alphaB * B) / alphaC;
}

void main(void)
{
	vec3 viewVec = normalize(relativePosition);

	vec3 position = cameraOffset + eyePosition;

	float depth = getDepth(screenspaceCoordinate) / normalize(viewDirection).z;
	float bottomPlaneIntersect = clamp(min((cloudHeight - eyePosition.y) / viewVec.y, depth), 0., 4. * fogDistance);
	float topPlaneIntersect = clamp(min((cloudHeight + cloudThickness - eyePosition.y) / viewVec.y, depth), 0., 4. * fogDistance);

	float startDepth = min(bottomPlaneIntersect, topPlaneIntersect);
	float endDepth = max(bottomPlaneIntersect, topPlaneIntersect);

	float bias = screenSpaceDither(gl_FragCoord.xy + animationTimer * 2400.0);

	vec3 color = vec3(0.);

	float dx = (endDepth - startDepth) / float(ITERATIONS);

	float density = 0.;

	float auroraStartDepth = min(max(0., 1.0 / viewVec.y), 8.);
	float auroraEndDepth = min(max(0., 3.0 / viewVec.y), 8.);
	float rawDepth = getRawDepth(screenspaceCoordinate);

	if (auroraEndDepth - auroraStartDepth > 0.1 && rawDepth >= 1.0) {
		for (int i = 0; i < ITERATIONS; i++) {
			vec3 rayPosition = viewVec * (auroraStartDepth + (auroraEndDepth - auroraStartDepth) * (float(i) + bias) / float(ITERATIONS));

			float localDensity = getAuroraDensity(rayPosition);

			localDensity *= 1.0 - mtsmoothstep(4.0, 8.0, length(rayPosition));

			density += localDensity;
		}
	}

	color.b = density * (auroraEndDepth - auroraStartDepth) / float(AURORA_ITERATIONS);

	float sunlightContribution = 0.;
	float alpha = 0.;
	float outScatter = 2. * (dot(v_LightDirection, viewVec) * 0.5 + 0.5);
	density = 0.;

	for (int i = 0; i < ITERATIONS; i++) {
		vec3 rayPosition = eyePosition + viewVec * (startDepth + (endDepth - startDepth) * (float(i) + bias) / float(ITERATIONS));

		float localDensity = getDensity(rayPosition) * dx;

		if (localDensity < 0.0001) continue;

		float clarity = clamp(fogShadingParameter - fogShadingParameter * length(rayPosition - eyePosition) / (4. * fogDistance), 0.0, 1.0);
		float brightness = getBrightness(rayPosition, bias) * exp(-outScatter * localDensity);
		sunlightContribution = blend(sunlightContribution, brightness, 1. - exp(-density), 1. - exp(-localDensity));
		alpha = blend(alpha, clarity, 1. - exp(-density), 1. - exp(-localDensity));

		density += localDensity;

		if (density > 10.0) break;
	}

	float forwardScatter = 1. + 4. * pow(min(dot(v_LightDirection, viewVec), 0.), 4.);

	color.r = (1. - exp(-density)) * alpha;
	color.g = sunlightContribution * forwardScatter;

	gl_FragColor = vec4(color, 1.0);
}
