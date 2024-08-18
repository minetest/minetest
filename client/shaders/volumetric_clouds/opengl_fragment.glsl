#define depthmap texture0
#define noiseTexture texture1
#define noiseTextureCoarse texture2

#define PROBING_ITERATIONS 30
#define ITERATIONS 50
#define LIGHT_ITERATIONS 3
#define AURORA_ITERATIONS 80

// See clouds.cpp
#define CLOUD_SIZE 640.0

uniform sampler2D depthmap;
uniform sampler2D noiseTexture;
uniform sampler2D noiseTextureCoarse;

uniform vec2 texelSize0;

uniform float cloudHeight;
uniform float cloudThickness;
uniform float cloudDensity;

varying vec3 relativePosition;
varying vec3 viewDirection;
uniform vec3 cameraOffset;
uniform vec3 cameraPosition;

uniform float cameraNear;
uniform float cameraFar;

uniform vec2 cloudOffset;
uniform float cloudRadius;

varying vec2 screenspaceCoordinate;
varying float sunStrength;

uniform float fogDistance;
uniform float fogShadingParameter;

uniform vec3 v_LightDirection;

uniform float animationTimer;

// Derived From http://alex.vlachos.com/graphics/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
// and https://www.shadertoy.com/view/MslGR8 (5th one starting from the bottom)
// NOTE: `frag_coord` is in pixels (i.e. not normalized UV).
float screenSpaceDither(highp vec2 frag_coord) 
{
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

float toLinearDepth(float depth) 
{
	return cameraNear * cameraFar / (cameraFar + depth * (cameraNear - cameraFar));
}

float getDepth(vec2 screenspacePosition) 
{
	return texture2D(depthmap, screenspacePosition * 0.5 + 0.5).r;
}

float noise(vec3 p) 
{
    float y = floor(p.y);
    float f1 = texture2D(noiseTexture, p.xz / 256. + y * 0.2).r;
    float f2 = texture2D(noiseTexture, p.xz / 256. + y * 0.2 + 0.2).r;
    return mix(f1, f2, fract(p.y));
}

float fnoise(vec3 p) 
{
    return noise(p * 4.) * 0.5 + noise(p * 8.) * 0.25;
}

float fnoise3(vec3 p) 
{
    return noise(p * 4.) * 0.5 + noise(p * 8.) * 0.25 + noise(p * 16.) * 0.125;
}

float getAuroraDensity(vec3 position) 
{
	float density = pow(max(0., 1. - 10. * abs(fnoise3(vec3(position.x * 0.25, animationTimer, position.z * 0.25)) - 0.5)), 4.);
	return 0.7 * density * mtsmoothstep(0.0, 0.05, position.y - 1.) * pow(1. - mtsmoothstep(0.05, 2.0, position.y - 1.), 4.);
}

float getCoarse(vec3 position) {
	return texture2D(noiseTextureCoarse, (position.xz - cloudOffset) * 0.5 / CLOUD_SIZE / cloudRadius).r;
}

float getDensity(vec3 position) 
{
	float density = texture2D(noiseTextureCoarse, (position.xz - cloudOffset) * 0.5 / CLOUD_SIZE / cloudRadius).r *
		mtsmoothstep(0.0, cloudThickness * 0.2, position.y - cloudHeight) *
		(1.0 - mtsmoothstep(cloudThickness * 0.5, cloudThickness, position.y - cloudHeight));

	density = max(0., density - 0.5 * fnoise(position * 0.005));

	return 0.04 * density;
}

float getBrightness(vec3 position, float lightDistance) 
{
	float density = 0.;
	for (int i = 1; i <= LIGHT_ITERATIONS; i++) {
		vec3 rayPosition = position - v_LightDirection * lightDistance * float(i) / float(LIGHT_ITERATIONS);

		density += getDensity(rayPosition) * float(lightDistance) / float(LIGHT_ITERATIONS);
	}
	return exp(-density);
}

float blend(float A, float B, float alphaA, float alphaB) 
{
    float alphaC = alphaA + (1. - alphaA) * alphaB;
    return (alphaA * A + (1. - alphaA) * alphaB * B) / alphaC;
}

void main(void)
{
	vec3 viewVec = normalize(relativePosition);

	vec3 position = cameraOffset + cameraPosition;

	float depth = toLinearDepth(getDepth(screenspaceCoordinate)) / normalize(viewDirection).z;
	float bottomPlaneIntersect = clamp((cloudHeight - cameraPosition.y) / viewVec.y, 0., 4. * fogDistance);
	float topPlaneIntersect = clamp((cloudHeight + cloudThickness - cameraPosition.y) / viewVec.y, 0., 4. * fogDistance);

#if (VOLUMETRICS_UNDERSAMPLING <= 1)
	bottomPlaneIntersect = min(depth, bottomPlaneIntersect);
	topPlaneIntersect = min(depth, topPlaneIntersect);
#else
	if ((bottomPlaneIntersect > depth + 5.0) != (topPlaneIntersect > depth + 5.0)) {
		bottomPlaneIntersect = min(depth, bottomPlaneIntersect);
		topPlaneIntersect = min(depth, topPlaneIntersect);
	}
#endif

	float startDepth = min(bottomPlaneIntersect, topPlaneIntersect);
	float endDepth = max(bottomPlaneIntersect, topPlaneIntersect);

	float bias = screenSpaceDither(gl_FragCoord.xy + animationTimer * 2400.0);

	vec3 color = vec3(0.);

	float density = 0.;

	float auroraStartDepth = min(max(0., 1.0 / viewVec.y), 8.);
	float auroraEndDepth = min(max(0., 3.0 / viewVec.y), 8.);
	float rawDepth = getDepth(screenspaceCoordinate);

	if (auroraEndDepth - auroraStartDepth > 0.1 && rawDepth >= 1.0) {
		for (int i = 0; i < AURORA_ITERATIONS; i++) {
			vec3 rayPosition = viewVec * (auroraStartDepth + (auroraEndDepth - auroraStartDepth) * (float(i) + bias) / float(AURORA_ITERATIONS));

			float localDensity = getAuroraDensity(rayPosition);

			localDensity *= 1.0 - mtsmoothstep(4.0, 8.0, length(rayPosition));

			density += localDensity;
		}
	}

	color.b = density * (auroraEndDepth - auroraStartDepth) / float(AURORA_ITERATIONS);
	
	float sunlightContribution = 0.;
	float alpha = 0.;
	float outScatter = 2. * (dot(v_LightDirection, viewVec) * 0.5 + 0.5);
	float forwardScatter = 1. + 2. * pow(min(dot(v_LightDirection, viewVec), 0.), 4.);
	density = 0.;

	float fogDepth = min(4. * fogDistance, startDepth + 2000.);
	endDepth = min(endDepth, fogDepth);

	float dx = (endDepth - startDepth) / float(ITERATIONS);
	float lightDistance = cloudThickness * 0.5;

	if (endDepth - startDepth > 0.1) {
		for (int i = 0; i < ITERATIONS; i++) {
			vec3 rayPosition = cameraPosition + viewVec * (startDepth + (endDepth - startDepth) * (float(i) + bias) / float(ITERATIONS));

			float localDensity = getDensity(rayPosition) * dx;

			if (localDensity < 0.0001) continue;

			float clarity = clamp(fogShadingParameter - fogShadingParameter * length(rayPosition - cameraPosition) / (fogDepth), 0.0, 1.0);
			float outScatterContribution = exp(-0.5 * outScatter * localDensity);
			float brightness = getBrightness(rayPosition, lightDistance) * forwardScatter * outScatterContribution * sunStrength + (1. - outScatterContribution);
			sunlightContribution = blend(sunlightContribution, brightness, 1. - exp(-density), 1. - exp(-localDensity));
			alpha = blend(alpha, clarity, 1. - exp(-density), 1. - exp(-localDensity));

			density += localDensity;

			if (density > 10.0) break;
		}
	}

	color.r = (1. - exp(-density)) * alpha;
	color.g = sunlightContribution;
	color.b *= exp(-density);

	gl_FragColor = vec4(color, 1.0);
}
