uniform sampler2D ColorMapSampler;
uniform int Cascade;

varying vec4 tPos;

void main()
{
	vec4 col = texture2D(ColorMapSampler, gl_TexCoord[0].st);

	if (Cascade < 2 && col.a < 0.70)
		discard;

	float depth = 0.5 + tPos.z * 0.5;
	gl_FragColor = vec4(depth, 0.0, 0.0, 1.0);
}
