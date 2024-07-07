#define rendered texture0
#define depthmap texture1
#define normalmap texture2
#define water_mask texture3

#define uv varTexCoord.st

uniform sampler2D rendered;
uniform sampler2D depthmap;
uniform sampler2D normalmap;
uniform sampler2D water_mask;

uniform vec4 skyBgColor;

uniform mat4 mCameraView;
uniform mat4 mCameraViewInv;
uniform mat4 mCameraViewProj;
uniform mat4 mCameraViewProjInv;

uniform vec3 cameraPosition;
uniform vec3 cameraOffset;

uniform float animationTimer;

#ifdef GL_ES
varying mediump vec2 varTexCoord;
#else
centroid varying vec2 varTexCoord;
#endif

float fastNoise(vec3 uvd) {
	return fract(sin(dot(uvd, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

vec2 projectPos(vec3 pos) {
	vec4 projected = mCameraViewProj * vec4(pos, 1.0);
	return (projected.xy / projected.w) * 0.5 + 0.5;
}

vec3 worldPos(vec2 pos) {
	vec4 position = mCameraViewProjInv * vec4((pos - 0.5) * 2.0, texture2D(depthmap, pos).x, 1.0);
	return position.xyz / position.w;
}

const float _MaxDistance = 100000.0;
const float _Step = 100; // Increased step size slightly
const float _Thickness = 0.05;

vec4 raycast(vec3 position, vec3 direction, float limit, vec4 fallback) {
	float ray_length = length(position) * 0.05;
	float start_depth = texture2D(depthmap, uv).x;

	float stp = _Step;
	vec3 march_position = position;

	vec2 sample_uv;
	float screen_depth, target_depth;

	vec4 reflect_color = vec4(0.0);
	float total_weight = 0.0;
	float weight;

	while (ray_length < limit) {
		march_position = position + direction * ray_length;
		sample_uv = projectPos(march_position);

		if (sample_uv.x > 1.0 || sample_uv.x < 0.0 || sample_uv.y > 1.0 || sample_uv.y < 0.0) {
			break;
		}

		screen_depth = worldPos(sample_uv).z;
		target_depth = march_position.z;

		if (texture2D(depthmap, sample_uv).x > start_depth && march_position.z / screen_depth > 1.01) {
			weight = 1.0 / (1.0 + ray_length * ray_length);
			reflect_color += texture2D(rendered, sample_uv) * weight;
			total_weight += weight;
		}

		ray_length += stp;
		stp *= 2.05; // Reduced step increment rate to increase sample density
	}

	if (total_weight > 0.0) {
		reflect_color /= total_weight;
	} else {
		reflect_color = fallback;
	}

	return reflect_color;
}

const float _WindSpeed = 100.0;
const vec3 _WindDir = normalize(vec3(1.0, 1.0, 1.0));

const float _WaveWidth = 16.0;
const float _WaveHeight = 0.0;

const float _RippleWidth = 8.0;
const float _RippleHeight = 0.0;

const float _ReflectFactor = 0.5;

void main(void) {
	vec4 mask = texture2D(water_mask, uv);
	vec4 base_color = texture2D(rendered, uv);

	if (mask == vec4(1.0, 0.0, 1.0, 1.0)) {
		vec3 position = worldPos(uv);
		vec3 normal = normalize(mat3(mCameraView) * texture2D(normalmap, uv).xyz);

		vec3 real_position = position * mat3(mCameraView) + cameraPosition;

		float wave_noise = fastNoise((real_position / _WaveWidth) + (animationTimer * _WindSpeed * 0.5 * _WindDir));
		float ripple_noise = fastNoise((real_position / _RippleWidth) + (animationTimer * _WindSpeed * 2.0 * _WindDir));

		normal.z += ((wave_noise - 0.5) * _WaveHeight) + ((ripple_noise - 0.5) * _RippleHeight);

		vec3 camera_dir = normalize(position);
		vec3 direction = reflect(camera_dir, normal);
		vec4 reflect_color = raycast(position, direction, _MaxDistance, skyBgColor);

		float factor = dot(-camera_dir, normal);
		factor = pow(factor, _ReflectFactor);

		gl_FragColor = mix(reflect_color, base_color, factor);
		return;
	}

	gl_FragColor = base_color;
}
