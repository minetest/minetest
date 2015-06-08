uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D useNormalmap;

uniform vec4 skyBgColor;
uniform float fogDistance;
uniform vec3 eyePosition;

varying vec3 vPosition;
varying vec3 worldPosition;
varying float generate_heightmaps;

varying vec3 eyeVec;
varying vec3 tsEyeVec;
varying vec3 lightVec;
varying vec3 tsLightVec;

bool normalTexturePresent = false; 

const float e = 2.718281828459;
const float BS = 10.0;
 
float intensity (vec3 color)
{
	return (color.r + color.g + color.b) / 3.0;
}

float get_rgb_height (vec2 uv)
{
	return intensity(texture2D(baseTexture,uv).rgb);
}

vec4 get_normal_map(vec2 uv)
{
	vec4 bump = texture2D(normalTexture, uv).rgba;
	bump.xyz = normalize(bump.xyz * 2.0 - 1.0);
	return bump;
}

float find_intersection(vec2 dp, vec2 ds)
{
	const int linear_steps = 10;
	const int binary_steps = 5;
	const float depth_step = 1.0 / linear_steps;
	float size = depth_step;
	float depth = 1.0;
	float best_depth = 1.0;
	for (int i = 0 ; i < linear_steps - 1 ; ++i) {
		vec4 t = texture2D(normalTexture, dp + ds * depth);
		if (best_depth > 0.05)
			if (depth >= t.a)
				best_depth = depth;
		depth -= size;
	}
	depth = best_depth - size;
	for (int i = 0 ; i < binary_steps ; ++i) {
		size *= 0.5;
		vec4 t = texture2D(normalTexture, dp + ds * depth);
		if (depth >= t.a) {
			best_depth = depth;
			depth -= 2 * size;
		}
		depth += size;
	}
	return best_depth;
}

float find_intersectionRGB(vec2 dp, vec2 ds) {
	const float iterations = 24;
	const float depth_step = 1.0 / iterations;
	float depth = 1.0;
	for (int i = 0 ; i < iterations ; i++) {
		float h = get_rgb_height(dp + ds * depth);
		if (h >= depth)
			break;
		depth -= depth_step;
	}
	return depth;
}

void main (void)
{
	vec3 color;
	vec4 bump;
	vec2 uv = gl_TexCoord[0].st;
	bool use_normalmap = false;

#ifdef USE_NORMALMAPS
	if (texture2D(useNormalmap,vec2(1.0, 1.0)).r > 0.0) {
		normalTexturePresent = true;
	}
#endif

#ifdef ENABLE_PARALLAX_OCCLUSION
	vec3 eyeRay = normalize(tsEyeVec);
#if PARALLAX_OCCLUSION_MODE == 0
	// Parallax occlusion with slope information
	if (normalTexturePresent) {
		const float scale = PARALLAX_OCCLUSION_SCALE / PARALLAX_OCCLUSION_ITERATIONS;
		const float bias = PARALLAX_OCCLUSION_BIAS / PARALLAX_OCCLUSION_ITERATIONS;
		for(int i = 0; i < PARALLAX_OCCLUSION_ITERATIONS; i++) {
			vec4 normal = texture2D(normalTexture, uv.xy);
			float h = normal.a * scale - bias;
			uv += h * normal.z * eyeRay.xy;
		}
#endif
#if PARALLAX_OCCLUSION_MODE == 1
	// Relief mapping
	if (normalTexturePresent) {
		vec2 ds = eyeRay.xy * PARALLAX_OCCLUSION_SCALE;
		float dist = find_intersection(uv, ds);
		uv += dist * ds;
#endif
	} else if (generate_heightmaps > 0.0) {
		vec2 ds = eyeRay.xy * PARALLAX_OCCLUSION_SCALE;
		float dist = find_intersectionRGB(uv, ds);
		uv += dist * ds;
	}
#endif

#ifdef USE_NORMALMAPS
	if (normalTexturePresent) {
		bump = get_normal_map(uv);
		use_normalmap = true;
	} 
#endif

	if (GENERATE_NORMALMAPS == 1 && use_normalmap == false) {
		float tl = get_rgb_height(vec2(uv.x - SAMPLE_STEP, uv.y + SAMPLE_STEP));
		float t  = get_rgb_height(vec2(uv.x - SAMPLE_STEP, uv.y - SAMPLE_STEP));
		float tr = get_rgb_height(vec2(uv.x + SAMPLE_STEP, uv.y + SAMPLE_STEP));
		float r  = get_rgb_height(vec2(uv.x + SAMPLE_STEP, uv.y));
		float br = get_rgb_height(vec2(uv.x + SAMPLE_STEP, uv.y - SAMPLE_STEP));
		float b  = get_rgb_height(vec2(uv.x, uv.y - SAMPLE_STEP));
		float bl = get_rgb_height(vec2(uv.x -SAMPLE_STEP, uv.y - SAMPLE_STEP));
		float l  = get_rgb_height(vec2(uv.x - SAMPLE_STEP, uv.y));
		float dX = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
		float dY = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);
		bump = vec4(normalize(vec3 (-dX, -dY, NORMALMAPS_STRENGTH)), 1.0);
		use_normalmap = true;
	}

	vec4 base = texture2D(baseTexture, uv).rgba;

#ifdef ENABLE_BUMPMAPPING
	if (use_normalmap) {
		vec3 L = normalize(lightVec);
		vec3 E = normalize(-eyeVec);
		float specular = pow(clamp(dot(reflect(L, bump.xyz), -E), 0.0, 1.0), 1.0);
		float diffuse = dot(E,bump.xyz);
		color = (diffuse + 0.1 * specular) * base.rgb;
	} else {
		color = base.rgb;
	}
#else
	color = base.rgb;
#endif

#if MATERIAL_TYPE == TILE_MATERIAL_LIQUID_TRANSPARENT || MATERIAL_TYPE == TILE_MATERIAL_LIQUID_OPAQUE
	float alpha = gl_Color.a;
	vec4 col = vec4(color.rgb, alpha);
	col *= gl_Color;
	if (fogDistance != 0.0) {
		float d = max(0.0, min(vPosition.z / fogDistance * 1.5 - 0.6, 1.0));
		alpha = mix(alpha, 0.0, d);
	}
	gl_FragColor = vec4(col.rgb, alpha);
#else
	vec4 col = vec4(color.rgb, base.a);
	col *= gl_Color;
	if (fogDistance != 0.0) {
		float d = max(0.0, min(vPosition.z / fogDistance * 1.5 - 0.6, 1.0));
		col = mix(col, skyBgColor, d);
	}
	gl_FragColor = vec4(col.rgb, base.a);
#endif
}
