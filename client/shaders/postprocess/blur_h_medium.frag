precision mediump float;

uniform sampler2D Render;

uniform vec2 PixelSize;

varying vec2 TexCoord;


void main()
{
    vec4 blur = vec4(0.0);
    vec2 tc = TexCoord + vec2(PixelSize.x*0.5, PixelSize.x*0.5);

    blur += texture2D(Render, tc - vec2(PixelSize.x * 6.0, 0.0)) * 0.015625;
    blur += texture2D(Render, tc - vec2(PixelSize.x * 4.0, 0.0)) * 0.09375;
    blur += texture2D(Render, tc - vec2(PixelSize.x * 2.0, 0.0)) * 0.234375;
    blur += texture2D(Render, tc) * 0.3125;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 2.0, 0.0)) * 0.234375;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 4.0, 0.0)) * 0.09375;
    blur += texture2D(Render, tc + vec2(PixelSize.x * 6.0, 0.0)) * 0.015625;

    gl_FragColor = blur;
}
