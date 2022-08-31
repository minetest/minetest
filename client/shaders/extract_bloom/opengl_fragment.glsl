#define rendered texture0

uniform sampler2D rendered;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

const float bloomLuminanceThreshold = 0.7;

void main(void)
{
	vec2 uv = varTexCoord.st;
	vec4 color = texture2D(rendered, uv).rgba;
	float luminance = dot(color.rgb, vec3(0.213, 0.715, 0.072)) + 1e-4;
	color.rgb *= max(0., 1. - bloomLuminanceThreshold / luminance);
	gl_FragColor = vec4(color.rgb, 1.0); // force full alpha to avoid holes in the image.
}
