
void main()
{
	vec4 uv = vec4(gl_Vertex.xyz, 1.0) * 0.5 + 0.5;
	gl_TexCoord[0] = uv;
	gl_TexCoord[1] = uv;
	gl_TexCoord[2] = uv;
	gl_Position = vec4(gl_Vertex.xyz, 1.0);
}
