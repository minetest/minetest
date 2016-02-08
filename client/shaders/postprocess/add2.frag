uniform sampler2D Render;
uniform sampler2D Tex0;

const float A = 0.22;
const float B = 0.30;
const float C = 0.10;
const float D = 0.20;
const float E = 0.01;
const float F = 0.30;
const float W = 11.2;

vec3 lerp(vec3 a, vec3 b, float w)
{
	return a + w *(b - a);
}

vec3 uncharted2Tonemap(vec3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec4 applyToneMapping(vec4 color, vec2 uv)
{
	color = vec4(pow(color.rgb, vec3(2.2)), color.a);
	vec4 bloom = texture2D(Render, uv);
	color.rgb += 0.5 * bloom.rgb;
	const float gamma = 1.8;
	float exposureBias = 4.0;
	color.rgb = uncharted2Tonemap(exposureBias * color.rgb);
	vec3 whiteScale = 1.0 / uncharted2Tonemap(vec3(W, W, W));
	color.rgb *= whiteScale;
	return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

void main()
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(Tex0, uv);
	gl_FragColor = applyToneMapping(color, uv);
}
