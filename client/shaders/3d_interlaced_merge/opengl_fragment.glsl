uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

#define leftImage baseTexture
#define rightImage normalTexture
#define maskImage textureFlags

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 left = texture2D(leftImage, uv).rgba;
	vec4 right = texture2D(rightImage, uv).rgba;
	vec4 mask = texture2D(maskImage, uv).rgba;
	vec4 color;
	if (mask.r > 0.5)
		color = right;
	else
		color = left;
	gl_FragColor = color;
}
