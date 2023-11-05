uniform mat4 LightMVP; // world matrix
uniform float zPerspectiveBias;

varying vec4 tPos;


vec4 applyPerspectiveDistortion(in vec4 position)
{
	position /= position.w;
	position.z *= zPerspectiveBias;
	return position;
}

void main()
{
	vec4 pos = LightMVP * gl_Vertex;

	tPos = applyPerspectiveDistortion(pos);

	gl_Position = vec4(tPos.xyz, 1.0);
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
