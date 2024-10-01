uniform lowp vec4 fogColor;
uniform float fogDistance;
uniform float fogShadingParameter;
varying highp vec3 eyeVec;

varying lowp vec4 varColor;
varying lowp vec3 normal;
uniform vec3 v_LightDirection;

void main(void)
{
	float brightness = max(dot(-v_LightDirection, normal), 0.0);
	vec4 col = varColor;

	col.rgb *= brightness;

	float clarity = clamp(fogShadingParameter
		- fogShadingParameter * length(eyeVec) / fogDistance, 0.0, 1.0);
	col.rgb = mix(fogColor.rgb, col.rgb, clarity);

	gl_FragColor = col;
}
