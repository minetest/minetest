#define rendered texture0
#define bloom texture1

uniform sampler2D rendered;
uniform sampler2D bloom;
uniform mediump float exposureFactor = 2.5;
uniform lowp float bloomIntensity = 1.0;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

#if ENABLE_BLOOM

vec4 applyBloom(vec4 color, vec2 uv)
{
	color.rgb = mix(color.rgb, texture2D(bloom, uv).rgb, bloomIntensity);
	return color;
}

#endif

#if ENABLE_TONE_MAPPING

/* Hable's UC2 Tone mapping parameters
	A = 0.22;
	B = 0.30;
	C = 0.10;
	D = 0.20;
	E = 0.01;
	F = 0.30;
	W = 11.2;
	equation used:  ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F
*/

vec3 uncharted2Tonemap(vec3 x)
{
	return ((x * (0.22 * x + 0.03) + 0.002) / (x * (0.22 * x + 0.3) + 0.06)) - 0.03333;
}

vec4 applyToneMapping(vec4 color)
{
	const float exposureBias = 2.0;
	color.rgb = uncharted2Tonemap(exposureBias * color.rgb);
	// Precalculated white_scale from
	//vec3 whiteScale = 1.0 / uncharted2Tonemap(vec3(W));
	vec3 whiteScale = vec3(1.036015346);
	color.rgb *= whiteScale;
	return color;
}
#endif

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 color = texture2D(rendered, uv).rgba;
	// translate to linear colorspace (approximate) and apply exposure
	color.rgb = pow(color.rgb, vec3(2.2)) * exposureFactor;

#if ENABLE_BLOOM
	color = applyBloom(color, uv);
#endif

#if ENABLE_TONE_MAPPING
	color = applyToneMapping(color);
#else
	color.rgb /= 2.5; // default exposure factor, see also RenderingEngine::DEFAULT_EXPOSURE_FACTOR;
#endif

	// return to sRGB colorspace (approximate)
	color = vec4(pow(color.rgb, vec3(1.0 / 2.2)), color.a);

	gl_FragColor = vec4(color.rgb, 1.0); // force full alpha to avoid holes in the image.
}
