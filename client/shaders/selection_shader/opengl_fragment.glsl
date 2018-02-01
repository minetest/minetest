uniform sampler2D baseTexture;

float linearizeDepth(float depth)
{
	float NEAR = 1.0;
	float FAR = 20000.0;
	depth = depth * 2.0 - 1.0;
	return  (-NEAR * FAR) / (depth * (FAR - NEAR) - FAR) / 2450.0;
}

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(baseTexture, uv);
	color.rgb *= gl_Color.rgb;
	gl_FragData[0] = color;
	gl_FragData[1].r = linearizeDepth(gl_FragCoord.z);
}
