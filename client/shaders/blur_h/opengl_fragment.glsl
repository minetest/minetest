#define rendered texture0

uniform sampler2D rendered;
uniform vec2 texelSize0;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

void main(void)
{
	// kernel distance and linear size
	const lowp float d = 3.;
	const lowp float n = 2. * d + 1.;

	vec2 uv = varTexCoord.st - vec2(d * texelSize0.x, 0.);
	vec4 color = vec4(0.);
	for (lowp float i = 0.; i < n; i++) {
		color += texture2D(rendered, uv).rgba * ((d + 1.) - abs(i - d));
		uv += vec2(texelSize0.x, 0.);
	}
	color /= (d + 1.) * (d + 1.);
	gl_FragColor = vec4(color.rgb, 1.0); // force full alpha to avoid holes in the image.
}
