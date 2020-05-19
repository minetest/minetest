varying lowp vec4 varColor;

void main(void)
{
#ifdef SECONDSTAGE
	gl_FragData[0] = varColor;
	gl_FragData[1] = vec4(0,0,0, DRAW_TYPE);
	gl_FragData[2] = vec4(1,0,0,0);
#else
	gl_FragColor = varColor;
#endif
}
