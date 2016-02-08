uniform sampler2D Tex0;

varying vec2 TexCoord;

void main()
{
    gl_FragColor = texture2D(Tex0, TexCoord);
}
