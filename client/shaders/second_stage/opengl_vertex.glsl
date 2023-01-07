#define exposureMap texture2

uniform sampler2D exposureMap;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

varying float exposure;

void main(void)
{
#ifdef ENABLE_AUTO_EXPOSURE
	exposure = texture2D(exposureMap, vec2(0.5)).r;
	exposure = pow(2., exposure);
#else
	exposure = 1.0;
#endif

	varTexCoord.st = inTexCoord0.st;
	gl_Position = inVertexPosition;
}
