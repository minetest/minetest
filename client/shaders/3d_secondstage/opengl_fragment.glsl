uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

#define rendered baseTexture
#define normalmap normalTexture
#define depthmap textureFlags

float resolution = 1024;
vec4 blur(float radius, float step)
{
	vec2 tc = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, tc);
	float steps = 0;
	for (float x=0; x < radius; x+=step) {
		for (float y=0; y < radius; y+=step) {
			vec4 other_color = texture2D(rendered, tc+vec2(x/resolution,y/resolution));
			color += other_color;
			steps++;
		}
	}
	if (steps == 0)
		return color;
	return color/steps;
}

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, uv).rgba;
	vec4 normal_and_depth = texture2D(normalmap, uv);
	vec3 normal = normal_and_depth.rgb;
	float custom = normal_and_depth.a;
	float depth = texture2D(depthmap, uv).r;
	float div = 0;
	float focused_depth = 0;
	for (float x = 0.4; x <= 0.6; x+=0.05) {
		for (float y = 0.4; y <= 0.6; y+=0.05) {
			focused_depth += texture2D(depthmap, vec2(x,y)).r;
			div++;
		}
	}
	focused_depth /= div;

	/*if (uv.x < 0.5 && uv.y < 0.5)
		gl_FragColor = color;
	else if (uv.y < 0.5)
		gl_FragColor = vec4(depth, depth, depth, 1);
	else if (uv.x < 0.5)
		gl_FragColor = vec4(normal, 1);
	else
		gl_FragColor = vec4(custom, custom, custom, 1);
	*/
	/*float green = length(color) / 4 + depth;
	green = clamp(green - length(uv - 0.5)*4, 0, 1);*/

	gl_FragColor = vec4(blur(2*pow(abs(depth-focused_depth), 2), 0.5).rgb,1);
}
