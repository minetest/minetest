uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform vec3 yawVec;
uniform float mapSize;

varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main (void)
{
	vec2 uv = varTexCoord;

	//texture sampling rate
	float step = 1.0 / mapSize;
	float tl = texture2D(normalTexture, vec2(uv.x - step, uv.y + step)).r;
	float t  = texture2D(normalTexture, vec2(uv.x,        uv.y + step)).r;
	float tr = texture2D(normalTexture, vec2(uv.x + step, uv.y + step)).r;
	float r  = texture2D(normalTexture, vec2(uv.x + step, uv.y       )).r;
	float br = texture2D(normalTexture, vec2(uv.x + step, uv.y - step)).r;
	float b  = texture2D(normalTexture, vec2(uv.x,        uv.y - step)).r;
	float bl = texture2D(normalTexture, vec2(uv.x - step, uv.y - step)).r;
	float l  = texture2D(normalTexture, vec2(uv.x - step, uv.y       )).r;
	float c  = texture2D(normalTexture, vec2(uv.x       , uv.y       )).r;
	float AO = 50.0 * (clamp(t - c, -0.001, 0.001) + clamp(b - c, -0.001, 0.001) + clamp(r - c, -0.001, 0.001) + clamp(l - c, -0.001, 0.001));
	float dX = 4.0 * (l - r);
	float dY = 4.0 * (t - b);
	vec3 bump = normalize(vec3 (dX, dY, 0.1));
	float height = 2.0 * texture2D(normalTexture, vec2(uv.x, uv.y)).r - 1.0;
	vec4 base = texture2D(baseTexture, uv).rgba;
	vec3 L = normalize(vec3(0.0, 0.0, 1.0));
	float specular = pow(clamp(dot(reflect(L, bump.xyz), yawVec), 0.0, 1.0), 1.0);
	float diffuse = dot(yawVec, bump);

	vec3 color = (1.1 * diffuse + 0.05 * height + 0.5 * specular + AO) * base.rgb;
	vec4 col = vec4(color, base.a);
	col *= varColor;
	gl_FragColor = vec4(col.rgb, base.a);
}
