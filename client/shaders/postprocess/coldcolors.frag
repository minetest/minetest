uniform sampler2D Render;
void main()
{
    vec4 color= texture2D(Render, gl_TexCoord[0].xy);
    color.r*= 0.85;
    color.g*= 0.95;
    color.b*= 1.25;

    gl_FragColor= color;
}
