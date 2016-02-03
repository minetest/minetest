uniform sampler2D Render;
uniform sampler2D Tex0;
uniform sampler2D Tex1;

uniform vec2 PixelSize;

float pixelOffsetX = 1.2;
float pixelOffsetY = 1.2;
vec2 ps = PixelSize / 1.0;

float fetchDepth(vec2 uv, float x, float y) {
    vec3 d1 = 4.0 * (1.0 - texture2D(Tex0, uv + vec2(x*ps.x*pixelOffsetX, y*ps.y*pixelOffsetY)).rgb);
	vec3 d2 = 1.0 * (1.0 - texture2D(Tex1, uv + vec2(x*ps.x*pixelOffsetX, y*ps.y*pixelOffsetY)).rgb);
	return length(d1 *d2);
}

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    mat3 depthMatrix;
    depthMatrix[0][0] = fetchDepth(uv, -1.0, -1.0);
    depthMatrix[1][0] = fetchDepth(uv, 0.0, -1.0);
    depthMatrix[2][0] = fetchDepth(uv, 1.0, -1.0);
    depthMatrix[0][1] = fetchDepth(uv, -1.0, 0.0);
    depthMatrix[2][1] = fetchDepth(uv, 1.0, 0.0);
    depthMatrix[0][2] = fetchDepth(uv, -1.0, 1.0);
    depthMatrix[1][2] = fetchDepth(uv, 0.0, 1.0);
    depthMatrix[2][2] = fetchDepth(uv, 1.0, 1.0);

    float gx, gy;
    gx = -1.0*depthMatrix[0][0]-2.0*depthMatrix[1][0]-1.0*depthMatrix[2][0]+1.0*depthMatrix[0][2]+2.0*depthMatrix[1][2]+1.0*depthMatrix[2][2];
    gy = -1.0*depthMatrix[0][0]-2.0*depthMatrix[0][1]-1.0*depthMatrix[0][2]+1.0*depthMatrix[2][0]+2.0*depthMatrix[2][1]+1.0*depthMatrix[2][2];

    float result = 1.0 - sqrt(gx*gx + gy*gy);
    gl_FragData[0].rgba = vec4(result, result, result, 1.0);
}
