uniform sampler2D baseTexture;

varying vec4 varColor;
varying vec2 varTexCoord;

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 color = texture2D(baseTexture, uv);
	color.rgb *= varColor.rgb;
	gl_FragColor = color;
}
