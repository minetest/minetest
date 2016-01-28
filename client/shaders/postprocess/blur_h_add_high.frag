precision mediump float;

uniform sampler2D Render;
uniform sampler2D Tex0;

uniform vec2 PixelSize;

varying vec2 TexCoord;


void main()
{
    vec4 blur = vec4(0.0);
    vec2 tc = TexCoord + vec2(PixelSize.x * 0.5, PixelSize.x * 0.5);

    blur += texture2D(Render, tc - vec2(PixelSize.x * 10.0, 0.0)) * 0.00292;
    blur += texture2D(Render, tc - vec2(PixelSize.x * 8.0, 0.0)) * 0.01611;
    blur += texture2D(Render, tc - vec2(PixelSize.x * 6.0, 0.0)) * 0.05371;
    blur += texture2D(Render, tc - vec2(PixelSize.x * 4.0, 0.0)) * 0.12084;
    blur += texture2D(Render, tc - vec2(PixelSize.x * 2.0, 0.0)) * 0.19335;
    blur += texture2D(Render, tc) * 0.22558;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 2.0, 0.0)) * 0.19335;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 4.0, 0.0)) * 0.12084;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 6.0, 0.0)) * 0.05371;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 8.0, 0.0)) * 0.01611;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 10.0, 0.0)) * 0.00292;

    gl_FragColor = blur + texture2D(Tex0, TexCoord);
}
