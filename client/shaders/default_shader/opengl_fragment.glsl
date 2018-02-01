float linearizeDepth(float depth)
{
	float NEAR = 1.0;
	float FAR = 20000.0;
	depth = depth * 2.0 - 1.0;
	return  (-NEAR * FAR) / (depth * (FAR - NEAR) - FAR) / 2450.0;
}

void main(void)
{
	gl_FragData[0] = gl_Color;
	gl_FragData[1].r = linearizeDepth(gl_FragCoord.z);
}
