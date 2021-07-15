uniform sampler2D ColorMapSampler;
varying vec4 tPos;

#ifdef COLORED_SHADOWS
// c_precision of 128 fits within 7 base-10 digits
const float c_precision = 128.0;
const float c_precisionp1 = c_precision + 1.0;

float packColor(vec3 color)
{
	return floor(color.b * c_precision + 0.5)
		+ floor(color.g * c_precision + 0.5) * c_precisionp1
		+ floor(color.r * c_precision + 0.5) * c_precisionp1 * c_precisionp1;
}

const vec3 black = vec3(0.0);
#endif

void main()
{
	vec4 col = texture2D(ColorMapSampler, gl_TexCoord[0].st);
#ifndef COLORED_SHADOWS
	if (col.a < 0.5)
		discard;
#endif

	float depth = 0.5 + tPos.z * 0.5;
	// ToDo: Liso: Apply movement on waving plants
	// depth in [0, 1] for texture

	//col.rgb = col.a == 1.0 ? vec3(1.0) : col.rgb;
#ifdef COLORED_SHADOWS
	float packedColor = packColor(mix(col.rgb, black, col.a));
	gl_FragColor = vec4(depth, packedColor, 0.0,1.0);
#else
	gl_FragColor = vec4(depth, 0.0, 0.0, 1.0);
#endif
}
