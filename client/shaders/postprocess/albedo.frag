uniform sampler2D Render;

void main()
{
    gl_FragColor = texture2D(Render, vec2(gl_TexCoord[0]));
}

