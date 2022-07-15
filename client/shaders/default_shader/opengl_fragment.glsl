varying lowp vec4 varColor;

void main(void)
{
	gl_FragData[0] = varColor;
	gl_FragData[1] = vec4(0,0,0, DRAW_TYPE / 256.);
	gl_FragData[2] = vec4(1,0,0,0);
}
