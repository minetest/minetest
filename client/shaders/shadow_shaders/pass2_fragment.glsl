uniform sampler2D ShadowMapClientMap;
#ifdef COLORED_SHADOWS
uniform sampler2D ShadowMapClientMapTraslucent;
#endif
uniform sampler2D ShadowMapSamplerdynamic;

void main() {

#ifdef COLORED_SHADOWS
	vec2 first_depth = texture2D(ShadowMapClientMap, gl_TexCoord[0].st).rg;
	vec2 depth_splitdynamics = vec2(texture2D(ShadowMapSamplerdynamic, gl_TexCoord[2].st).r, 0.0);
	if (first_depth.r > depth_splitdynamics.r)
		first_depth = depth_splitdynamics;
	vec2 depth_color = texture2D(ShadowMapClientMapTraslucent, gl_TexCoord[1].st).rg;
	gl_FragColor = vec4(first_depth.r, first_depth.g, depth_color.r, depth_color.g);
#else
	float first_depth = texture2D(ShadowMapClientMap, gl_TexCoord[0].st).r;
	float depth_splitdynamics = texture2D(ShadowMapSamplerdynamic, gl_TexCoord[2].st).r;
	first_depth = min(first_depth, depth_splitdynamics);
	gl_FragColor = vec4(first_depth, 0.0, 0.0, 1.0);
#endif

}
