uniform sampler2D Render;
uniform sampler2D Tex0; //depth

void main()
{
    vec4 color= texture2D(Render, gl_TexCoord[0].xy);
    vec4 fogColor= vec4(0.15, 0.15, 0.15, 1.0);
    float depth= texture2D(Tex0, gl_TexCoord[0].xy).r;

    if (depth < 1.0)
    {
        depth= max(depth-0.2, 0.0);
        color= mix(color, fogColor, depth);
    }

    gl_FragColor= color;
}
