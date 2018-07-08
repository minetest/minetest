uniform sampler2D baseTexture;

varying vec4 varColor;

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(baseTexture, uv);
	color.rgb *= varColor.rgb;
	gl_FragColor = color;
}
