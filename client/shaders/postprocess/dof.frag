uniform sampler2D Render;
uniform sampler2D Tex0;
uniform sampler2D Tex1;

uniform vec2 PixelSize;

float Far = 20000.0;
float fLength = 12.0/Far;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    float fDepth = texture2D(Tex1, vec2(0.5, 0.5)).x;
    float depth = texture2D(Tex1, uv).x;
    vec3 color = texture2D(Tex0, uv).rgb;
    vec3 colorBlur = texture2D(Render, uv).rgb;
    float blur = 0.0;
    float nearBoundDOF = clamp(fDepth - fLength, 0.0, fDepth);
    float farBoundDOF = clamp(fDepth + fLength, fDepth, 1.0);
    if (depth > farBoundDOF)
		blur = (depth - farBoundDOF)/farBoundDOF;
	else if (depth < nearBoundDOF)
		blur = (nearBoundDOF - depth)/nearBoundDOF;
	blur = clamp(blur, 0.0, 1.0);
	gl_FragData[0].rgba = vec4(mix(color, colorBlur, blur), 1.0);
}
