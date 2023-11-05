uniform sampler2D ShadowMapClientMap;
#ifdef COLORED_SHADOWS
uniform sampler2D ShadowMapClientMapTraslucent;
#endif
uniform sampler2D ShadowMapSamplerdynamic;

void main() {

	float first_depth = texture2D(ShadowMapClientMap, gl_TexCoord[0].st).r;
	if (gl_TexCoord[2].s < 2./3.) {
		float depth_splitdynamics = texture2D(ShadowMapSamplerdynamic, gl_TexCoord[2].st * vec2(1.5, 1.)).r;
		first_depth = min(first_depth, depth_splitdynamics);
	}
#ifdef COLORED_SHADOWS
	vec2 depth_color = texture2D(ShadowMapClientMapTraslucent, gl_TexCoord[1].st).rg;
	gl_FragColor = vec4(first_depth, depth_color.r, depth_color.g, 0.);
#else
	gl_FragColor = vec4(first_depth, 0.0, 0.0, 1.0);
#endif

}
