#ifdef ENABLE_AUTO_EXPOSURE
#define exposureMap texture2

uniform sampler2D exposureMap;

varying float exposure;
varying float ll;
varying float ev;
varying float targetEv;
#endif

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

void main(void)
{
#ifdef ENABLE_AUTO_EXPOSURE
	// value in the texture is on a logarithtmic scale
	vec4 sample = texture2D(exposureMap, vec2(0.5));
	ev = sample.r;
	targetEv = sample.g;
	ll = sample.b;
	exposure = pow(2., ev);
#endif

	varTexCoord.st = inTexCoord0.st;
	gl_Position = inVertexPosition;
}
