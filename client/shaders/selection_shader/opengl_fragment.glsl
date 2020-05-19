uniform sampler2D baseTexture;

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(baseTexture, uv);
	color.rgb *= gl_Color.rgb;
#ifdef SECONDSTAGE
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(0,0,0, DRAW_TYPE);
	gl_FragData[2] = vec4(1,0,0,0);
#else
	gl_FragColor = color;
#endif
}
