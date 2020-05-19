uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

#define rendered baseTexture
#define normalmap normalTexture
#define depthmap textureFlags

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, uv).rgba;
	vec4 normal_and_depth = texture2D(normalmap, uv);
	vec3 normal = normal_and_depth.rgb;
	float custom = normal_and_depth.a;
	float depth = texture2D(depthmap, uv).r;
	if (uv.x < 0.5 && uv.y < 0.5)
		gl_FragColor = color;
	else if (uv.y < 0.5)
		gl_FragColor = vec4(depth, depth, depth, 1);
	else if (uv.x < 0.5)
		gl_FragColor = vec4(normal, 1);
	else
		gl_FragColor = vec4(custom, custom, custom, 1);
}
