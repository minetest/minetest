uniform sampler2D baseTexture;

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	gl_FragColor = texture2D(baseTexture, uv).rgba;
}