uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

#define ENABLE_DOF 1

#if ENABLE_DOF
uniform vec2 pixelSize;
vec2 off = 1.5 * pixelSize;

vec4 blur(sampler2D image, vec2 position, float sharpness)
{
	float bc0 = 1.0;
	float c = bc0;

	float bc1 = exp(-1.0 * sharpness);
	float bc2 = exp(-2.0 * sharpness);
	c += 4.0 * bc1 + 4.0 * bc2;

#ifdef EXTENDED_DOF_LIMIT
	float bc4 = exp(-4.0 * sharpness);
	float bc5 = exp(-5.0 * sharpness);
	c += 4.0 * bc4 + 8.0 * bc5;
#endif

#ifdef INSANE_DOF_LIMIT
	float bc8 = exp(-8.0 * sharpness);
	float bc9 = exp(-9.0 * sharpness);
	float bc10 = exp(-10.0 * sharpness);
	c += 4.0 * bc8 + 4.0 * bc9 + 8.0 * bc10;
#endif

	bc0 /= c;
	bc1 /= c;
	bc2 /= c;

#ifdef EXTENDED_DOF_LIMIT
	bc4 /= c;
	bc5 /= c;
#endif

#ifdef INSANE_DOF_LIMIT
	bc8 /= c;
	bc9 /= c;
	bc10 /= c;
#endif

	vec4 color = vec4(0.0);
	vec2 pos = position - vec2(pixelSize.x * 0.5, pixelSize.y * 0.5);

	color += texture2D(image, pos + vec2(off.x *  0.0, off.y *  0.0)) * bc0;

	color += texture2D(image, pos + vec2(off.x * -1.0, off.y *  0.0)) * bc1;
	color += texture2D(image, pos + vec2(off.x * +1.0, off.y *  0.0)) * bc1;
	color += texture2D(image, pos + vec2(off.x *  0.0, off.y * -1.0)) * bc1;
	color += texture2D(image, pos + vec2(off.x *  0.0, off.y * +1.0)) * bc1;

	color += texture2D(image, pos + vec2(off.x * -1.0, off.y * -1.0)) * bc2;
	color += texture2D(image, pos + vec2(off.x * -1.0, off.y * +1.0)) * bc2;
	color += texture2D(image, pos + vec2(off.x * +1.0, off.y * -1.0)) * bc2;
	color += texture2D(image, pos + vec2(off.x * +1.0, off.y * +1.0)) * bc2;

#ifdef EXTENDED_DOF_LIMIT
	color += texture2D(image, pos + vec2(off.x * -2.0, off.y *  0.0)) * bc4;
	color += texture2D(image, pos + vec2(off.x *  0.0, off.y * -2.0)) * bc4;
	color += texture2D(image, pos + vec2(off.x *  0.0, off.y * +2.0)) * bc4;
	color += texture2D(image, pos + vec2(off.x * +2.0, off.y *  0.0)) * bc4;

	color += texture2D(image, pos + vec2(off.x * -2.0, off.y * -1.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * -2.0, off.y * +1.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * -1.0, off.y * -2.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * -1.0, off.y * +2.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * +1.0, off.y * -2.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * +1.0, off.y * +2.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * +2.0, off.y * -1.0)) * bc5;
	color += texture2D(image, pos + vec2(off.x * +2.0, off.y * +1.0)) * bc5;
#endif
	return color;
}

uniform float dofstrength = 0.1;

const float fNear = 2.5e-4;
const float fFar = 1.0e-1;
#endif

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color;
#if ENABLE_DOF
	float depth = texture2D(textureFlags, uv).r;
	float fDepth = texture2D(textureFlags, vec2(0.5, 0.5)).x;
	float b = log(clamp(depth, fNear, fFar)) - log(clamp(fDepth, fNear, fFar));
	color = blur(baseTexture, uv, 0.5 / (dofstrength * b * b));
#else
	color = texture2D(baseTexture, uv).rgba;
#endif
	gl_FragColor = color;
}