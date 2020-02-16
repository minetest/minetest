uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

uniform vec4 emissiveColor;
uniform vec4 skyBgColor;
uniform float fogDistance;
uniform vec3 eyePosition;

varying vec3 vNormal;
varying vec3 vPosition;
varying vec3 worldPosition;

varying vec3 eyeVec;
varying vec3 lightVec;
varying float vIDiff;

bool normalTexturePresent = false;
bool texTileableHorizontal = false;
bool texTileableVertical = false;
bool texSeamless = false;

const float e = 2.718281828459;
const float BS = 10.0;
const float fogStart = FOG_START;
const float fogShadingParameter = 1 / ( 1 - fogStart);

void get_texture_flags()
{
	vec4 flags = texture2D(textureFlags, vec2(0.0, 0.0));
	if (flags.r > 0.5) {
		normalTexturePresent = true;
	}
	if (flags.g > 0.5) {
		texTileableHorizontal = true;
	}
	if (flags.b > 0.5) {
		texTileableVertical = true;
	}
	if (texTileableHorizontal && texTileableVertical) {
		texSeamless = true;
	}
}

float intensity(vec3 color)
{
	return (color.r + color.g + color.b) / 3.0;
}

float get_rgb_height(vec2 uv)
{
	if (texSeamless) {
		return intensity(texture2D(baseTexture, uv).rgb);
	} else {
		return intensity(texture2D(baseTexture, clamp(uv, 0.0, 0.999)).rgb);
	}
}

vec4 get_normal_map(vec2 uv)
{
	vec4 bump = texture2D(normalTexture, uv).rgba;
	bump.xyz = normalize(bump.xyz * 2.0 - 1.0);
	return bump;
}

void main(void)
{
	vec3 color;
	vec4 bump;
	vec2 uv = gl_TexCoord[0].st;
	bool use_normalmap = false;
	get_texture_flags();

#if USE_NORMALMAPS == 1
	if (normalTexturePresent) {
		bump = get_normal_map(uv);
		use_normalmap = true;
	}
#endif

#if GENERATE_NORMALMAPS == 1
	if (normalTexturePresent == false) {
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
		bump = vec4(normalize(vec3 (dX, dY, NORMALMAPS_STRENGTH)), 1.0);
		use_normalmap = true;
	}
#endif

	vec4 base = texture2D(baseTexture, uv).rgba;

#ifdef ENABLE_BUMPMAPPING
	if (use_normalmap) {
		vec3 L = normalize(lightVec);
		vec3 E = normalize(eyeVec);
		float specular = pow(clamp(dot(reflect(L, bump.xyz), E), 0.0, 1.0), 1.0);
		float diffuse = dot(-E,bump.xyz);
		color = (diffuse + 0.1 * specular) * base.rgb;
	} else {
		color = base.rgb;
	}
#else
	color = base.rgb;
#endif

	vec4 col = vec4(color.rgb, base.a);

	col.rgb *= emissiveColor.rgb * vIDiff;
	// Due to a bug in some (older ?) graphics stacks (possibly in the glsl compiler ?),
	// the fog will only be rendered correctly if the last operation before the
	// clamp() is an addition. Else, the clamp() seems to be ignored.
	// E.g. the following won't work:
	//      float clarity = clamp(fogShadingParameter
	//		* (fogDistance - length(eyeVec)) / fogDistance), 0.0, 1.0);
	// As additions usually come for free following a multiplication, the new formula
	// should be more efficient as well.
	// Note: clarity = (1 - fogginess)
	float clarity = clamp(fogShadingParameter
		- fogShadingParameter * length(eyeVec) / fogDistance, 0.0, 1.0);
	col = mix(skyBgColor, col, clarity);

	gl_FragColor = vec4(col.rgb, base.a);
}
