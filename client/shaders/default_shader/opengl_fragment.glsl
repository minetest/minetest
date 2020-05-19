void main(void)
{
	gl_FragColor = gl_Color;
#ifdef SECONDSTAGE
	gl_FragData[0] = gl_Color;
	gl_FragData[1] = vec4(0,0,0, DRAW_TYPE);
	gl_FragData[2] = vec4(1,0,0,0);
#else
	gl_FragColor = gl_Color;
#endif
}
