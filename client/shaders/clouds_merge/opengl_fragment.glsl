#define cloudsTexture texture0
#define sceneTexture texture1
#define depthmap texture2

uniform sampler2D cloudsTexture;
uniform sampler2D sceneTexture;
uniform sampler2D depthmap;
uniform vec2 texelSize0;

uniform vec3 dayLight;
uniform vec3 cloudColor;

varying vec2 screenspaceCoordinate;
varying vec3 relativePosition;
varying vec3 viewDirection;
varying vec3 sunTint;
varying float auroraFactor;

uniform vec3 cameraOffset;
uniform vec3 cameraPosition;

uniform float cameraNear;
uniform float cameraFar;

uniform float cloudHeight;
uniform float cloudThickness;

float getDepth(vec2 screenspacePosition) {
	float depth = texture2D(depthmap, screenspacePosition * 0.5 + 0.5).r;
	return cameraNear * cameraFar / (cameraFar + depth * (cameraNear - cameraFar));
}

vec4 sampleClouds(vec2 uv) {
	vec4 cloudsKey = texture2D(cloudsTexture, uv);

	//const vec3 darkColor = vec3(0.05, 0.1, 0.2);
	vec3 darkColor = vec3(0.2, 0.4, 0.8) * dayLight;
	const vec3 auroraDark = vec3(0., 0.5, 0.5);
	const vec3 auroraBright = vec3(0., 0.5, .0);

	//const vec3 auroraDark = vec3(0.);
	//const vec3 auroraBright = vec3(0.);

	return vec4(
		mix(auroraDark, auroraBright, cloudsKey.b) * cloudsKey.b * auroraFactor + 
		cloudsKey.r * cloudColor * (darkColor * max(0., 1. - cloudsKey.g) + dayLight * sunTint * cloudsKey.g),
	cloudsKey.r);
}

vec4 getClouds(vec2 uv) {
	return
		sampleClouds(uv - texelSize0 * vec2(-1.0, -1.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2( 0.0, -1.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2( 1.0, -1.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2(-1.0,  0.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2( 0.0,  0.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2( 1.0,  0.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2(-1.0,  1.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2( 0.0,  1.0)) / 9.0 +
		sampleClouds(uv - texelSize0 * vec2( 1.0,  1.0)) / 9.0;
}

void main(void)
{
	vec3 viewVec = normalize(relativePosition);

	vec3 position = cameraOffset + cameraPosition;

	float depth = getDepth(screenspaceCoordinate) / normalize(viewDirection).z;
	float bottomPlaneIntersect = max((cloudHeight - cameraPosition.y) / viewVec.y, 0.);
	float topPlaneIntersect = max((cloudHeight + cloudThickness - cameraPosition.y) / viewVec.y, 0.);
	float minPlane = min(bottomPlaneIntersect, topPlaneIntersect);

	vec4 sceneColor = texture2D(sceneTexture, screenspaceCoordinate * 0.5 + 0.5);

	if (depth > minPlane) {
		vec4 finalColor = getClouds(screenspaceCoordinate * 0.5 + 0.5);

		gl_FragColor = vec4(sceneColor.rgb * (1. - finalColor.a) + finalColor.rgb, 1.);
	} else {
		gl_FragColor = sceneColor;
	}
}
