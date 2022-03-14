uniform mat4 mWorld;
uniform vec3 dayLight;
uniform vec3 eyePosition;
uniform float animationTimer;
uniform vec4 emissiveColor;
uniform vec3 cameraOffset;


varying vec3 vNormal;
varying vec3 vPosition;
varying vec3 worldPosition;
varying lowp vec4 varColor;
#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

#ifdef ENABLE_DYNAMIC_SHADOWS
	// shadow uniforms
	uniform vec3 v_LightDirection;
	uniform float f_textureresolution;
	uniform mat4 m_ShadowViewProj;
	uniform float f_shadowfar;
	uniform float f_shadow_strength;
	uniform float f_timeofday;
	varying float cosLight;
	varying float normalOffsetScale;
	varying float adj_shadow_strength;
	varying float f_normal_length;
#endif

varying vec3 eyeVec;
varying float nightRatio;
// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
varying float vIDiff;
const float e = 2.718281828459;
const float BS = 10.0;
uniform float xyPerspectiveBias0;
uniform float xyPerspectiveBias1;

#ifdef ENABLE_DYNAMIC_SHADOWS
// custom smoothstep implementation because it's not defined in glsl1.2
// https://docs.gl/sl4/smoothstep
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}
#endif


float directional_ambient(vec3 normal)
{
	vec3 v = normal * normal;

	if (normal.y < 0.0)
		return dot(v, vec3(0.670820, 0.447213, 0.836660));

	return dot(v, vec3(0.670820, 1.000000, 0.836660));
}

void main(void)
{
	varTexCoord = (mTexture * inTexCoord0).st;
	gl_Position = mWorldViewProj * inVertexPosition;

	vPosition = gl_Position.xyz;
	vNormal = (mWorld * vec4(inVertexNormal, 0.0)).xyz;
	worldPosition = (mWorld * inVertexPosition).xyz;
	eyeVec = -(mWorldView * inVertexPosition).xyz;

#if (MATERIAL_TYPE == TILE_MATERIAL_PLAIN) || (MATERIAL_TYPE == TILE_MATERIAL_PLAIN_ALPHA)
	vIDiff = 1.0;
#else
	// This is intentional comparison with zero without any margin.
	// If normal is not equal to zero exactly, then we assume it's a valid, just not normalized vector
	vIDiff = length(inVertexNormal) == 0.0
		? 1.0
		: directional_ambient(normalize(inVertexNormal));
#endif

#ifdef GL_ES
	vec4 color = inVertexColor.bgra;
#else
	vec4 color = inVertexColor;
#endif

	color *= emissiveColor;

	// The alpha gives the ratio of sunlight in the incoming light.
	nightRatio = 1.0 - color.a;
	color.rgb = color.rgb * (color.a * dayLight.rgb +
		nightRatio * artificialLight.rgb) * 2.0;
	color.a = 1.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp final_color_blend()
	float brightness = (color.r + color.g + color.b) / 3.0;
	color.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	varColor = clamp(color, 0.0, 1.0);


#ifdef ENABLE_DYNAMIC_SHADOWS
	if (f_shadow_strength > 0.0) {
		vec3 nNormal = normalize(vNormal);
		cosLight = dot(nNormal, -v_LightDirection);

		// Calculate normal offset scale based on the texel size adjusted for 
		// curvature of the SM texture. This code must be change together with
		// getPerspectiveFactor or any light-space transformation.
		vec3 eyeToVertex = worldPosition - eyePosition + cameraOffset;
		// Distance from the vertex to the player
		float distanceToPlayer = length(eyeToVertex - v_LightDirection * dot(eyeToVertex, v_LightDirection)) / f_shadowfar;
		// perspective factor estimation according to the
		float perspectiveFactor = distanceToPlayer * xyPerspectiveBias0 + xyPerspectiveBias1;
		float texelSize = f_shadowfar * perspectiveFactor * perspectiveFactor /
				(f_textureresolution * xyPerspectiveBias1  - perspectiveFactor * xyPerspectiveBias0);
		float slopeScale = clamp(pow(1.0 - cosLight*cosLight, 0.5), 0.0, 1.0);
		normalOffsetScale = texelSize * slopeScale;

		if (f_timeofday < 0.2) {
			adj_shadow_strength = f_shadow_strength * 0.5 *
				(1.0 - mtsmoothstep(0.18, 0.2, f_timeofday));
		} else if (f_timeofday >= 0.8) {
			adj_shadow_strength = f_shadow_strength * 0.5 *
				mtsmoothstep(0.8, 0.83, f_timeofday);
		} else {
			adj_shadow_strength = f_shadow_strength *
				mtsmoothstep(0.20, 0.25, f_timeofday) *
				(1.0 - mtsmoothstep(0.7, 0.8, f_timeofday));
		}
		f_normal_length = length(vNormal);
	}
#endif
}
