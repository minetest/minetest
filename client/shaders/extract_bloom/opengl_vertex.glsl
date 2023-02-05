#define exposureMap texture1

uniform sampler2D exposureMap;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

varying float exposure;

void main(void)
{
	exposure = texture2D(exposureMap, vec2(0.5)).r;

	varTexCoord.st = inTexCoord0.st;
	gl_Position = inVertexPosition;
}
