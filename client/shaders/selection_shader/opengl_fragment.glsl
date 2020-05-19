uniform sampler2D baseTexture;

varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 color = texture2D(baseTexture, uv);
	color.rgb *= varColor.rgb;
#ifdef SECONDSTAGE
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(0,0,0, DRAW_TYPE);
	gl_FragData[2] = vec4(1,0,0,0);
#else
	gl_FragColor = color;
#endif
}
