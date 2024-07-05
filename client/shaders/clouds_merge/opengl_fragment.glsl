#define cloudsTexture texture0
#define sceneTexture texture1

uniform sampler2D cloudsTexture;
uniform sampler2D sceneTexture;
uniform vec2 texelSize0;

uniform vec3 dayLight;

varying vec2 screenspaceCoordinate;

vec4 sampleClouds(vec2 uv) {
	vec4 cloudsKey = texture2D(cloudsTexture, uv);

	const vec3 darkColor = vec3(0.05, 0.1, 0.2);
	const vec3 auroraDark = vec3(0., 0.5, 0.5);
	const vec3 auroraBright = vec3(0., 0.5, .0);

	return vec4(
		mix(auroraDark, auroraBright, cloudsKey.b) * cloudsKey.b * max(0., 1. - cloudsKey.r) + 
		cloudsKey.r * (darkColor * max(0., 1. - cloudsKey.g) + dayLight * cloudsKey.g),
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
	vec4 cloudsColor = getClouds(screenspaceCoordinate * 0.5 + 0.5);
	vec4 sceneColor = texture2D(sceneTexture, screenspaceCoordinate * 0.5 + 0.5);

	gl_FragColor = vec4(sceneColor.rgb * (1. - cloudsColor.a) + cloudsColor.rgb, 1.);
}
