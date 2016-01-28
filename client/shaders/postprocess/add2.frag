precision mediump float;

uniform sampler2D Render;
uniform sampler2D Tex0;

varying vec2 TexCoord;

void main()
{
    gl_FragColor = texture2D(Render, TexCoord) + texture2D(Tex0, TexCoord);
}
