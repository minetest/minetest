#define rendered texture0

uniform sampler2D rendered;
uniform mediump float exposureFactor;
uniform float bloomLuminanceThreshold;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif


void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 color = texture2D(rendered, uv).rgba;
	// translate to linear colorspace (approximate)
	color.rgb = pow(color.rgb, vec3(2.2)) * exposureFactor;
	gl_FragColor = vec4(color.rgb, 1.0); // force full alpha to avoid holes in the image.
}
