uniform sampler2D Render;
uniform vec2 PixelSize;

void main()
{
	vec2 uv = gl_TexCoord[0].xy;

	vec4 blur = vec4(0.0);
	vec2 tc = uv - vec2(PixelSize.x * 0.5, PixelSize.y * 0.5);

	blur += texture2D(Render, tc - vec2(0.0, PixelSize.y * 10.0)) * 0.00292;
	blur += texture2D(Render, tc - vec2(0.0, PixelSize.y * 8.0)) * 0.01611;
	blur += texture2D(Render, tc - vec2(0.0, PixelSize.y * 6.0)) * 0.05371;
	blur += texture2D(Render, tc - vec2(0.0, PixelSize.y * 4.0)) * 0.12084;
	blur += texture2D(Render, tc - vec2(0.0, PixelSize.y * 2.0)) * 0.19335;
	blur += texture2D(Render, tc) * 0.22558;
	blur += texture2D(Render, tc + vec2(0.0, PixelSize.y * 2.0)) * 0.19335;
	blur += texture2D(Render, tc + vec2(0.0, PixelSize.y * 4.0)) * 0.12084;
	blur += texture2D(Render, tc + vec2(0.0, PixelSize.y * 6.0)) * 0.05371;
	blur += texture2D(Render, tc + vec2(0.0, PixelSize.y * 8.0)) * 0.01611;
	blur += texture2D(Render, tc + vec2(0.0, PixelSize.y * 10.0)) * 0.00292;

	gl_FragColor = blur;
}
	
