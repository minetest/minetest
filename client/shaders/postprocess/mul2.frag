uniform sampler2D Render;
uniform sampler2D Tex0;

void main()
{
	vec2 uv = gl_TexCoord[0].st;
	gl_FragColor = texture2D(Render, uv) * texture2D(Tex0, uv);
}
