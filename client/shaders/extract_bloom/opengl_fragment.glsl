#define rendered texture0

uniform sampler2D rendered;
uniform mediump float exposureFactor;
uniform mediump float bloomStrength;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif


void main(void)
{
	vec2 uv = varTexCoord.st;
	vec3 color = texture2D(rendered, uv).rgb;
	// translate to linear colorspace (approximate)
	color = pow(color, vec3(2.2));

	// Scale colors by luminance to amplify bright colors
	// in SDR textures.
	float luminance = dot(color, vec3(0.213, 0.515, 0.072));
	luminance *= luminance;
	color *= luminance * exposureFactor * bloomStrength;
	gl_FragColor = vec4(color, 1.0); // force full alpha to avoid holes in the image.
}
