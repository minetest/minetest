uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D ShadowMapSampler;

#define rendered baseTexture
#define normalmap normalTexture
#define depthmap ShadowMapSampler

const float far = 20000.;
const float near = 1.;
float mapDepth(float depth)
{
	depth = 2. * near / (far + near - depth * (far - near));
	return clamp(pow(depth, 1./2.5), 0.0, 1.0);
}

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, uv).rgba;
	vec4 normal_and_depth = texture2D(normalmap, uv);
	vec3 normal = normal_and_depth.rgb;
	float draw_type = normal_and_depth.a * 256. / 25.;
	float depth = mapDepth(texture2D(depthmap, uv).r);

	if (uv.x < 0.5 && uv.y < 0.5)
		gl_FragColor = color;
	else if (uv.y < 0.5)
		gl_FragColor = vec4(depth, depth, depth, 1);
	else if (uv.x < 0.5)
		gl_FragColor = vec4(normal, 1);
	else
		gl_FragColor = vec4(draw_type, draw_type, draw_type, 1);
}
