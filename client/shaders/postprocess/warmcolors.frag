uniform sampler2D Render;

void main()
{
    vec4 color= texture2D(Render, gl_TexCoord[0].xy);
    color.r*= 1.20;
    color.g*= 1.15;
    color.b*= 0.90;
	color =clamp(color, 0.0, 1.0);
    gl_FragColor= color;
}

