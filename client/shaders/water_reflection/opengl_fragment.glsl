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

#if ENABLE_LIQUID_REFLECTIONS && ENABLE_WAVING_WATER

vec4 permute(vec4 x)
{
    return mod(((x * 34.0) + 1.0) * x, 289.0);
}

float snoise(vec3 p)
{
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = permute(b.xyxy);
    vec4 k2 = permute(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = permute(c);
    vec4 k4 = permute(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = mix(o1, o2, d.z);
    vec2 o4 = mix(o3.xz, o3.yw, d.x);

    return mix(o4.x, o4.y, d.y);
}


vec2 projectPos(vec3 pos) {
	vec4 projected = mCameraViewProj * vec4(pos, 0.0);
	return (projected.xy / projected.w) * 0.5 + 0.5;
}

vec3 worldPos(vec2 pos) {
	vec4 position = mCameraViewProjInv * vec4((pos - 0.5) * 2.0, texture2D(depthmap, pos).x, 1.0);
	return position.xyz / position.w;
}

const float _MaxDistance = 100000.0;
const float _Step = 100; 
const float _Thickness = 0.05;

vec4 raycast(vec3 position, vec3 direction, float limit, vec4 fallback) {
    float ray_length = length(position) * 0.05;
 	float ray_length_inv = length(position * _Thickness) * 20000 + _Step * 0.0001;
    float start_depth = texture2D(depthmap, uv).x;

    float stp = _Step;
    vec3 march_position = position;
    vec3 march_position_inv = position;

    vec2 sample_uv;
    vec2 sample_uv_inv;
    float screen_depth, target_depth, target_depth_inv;

    while (ray_length < limit) {
        march_position = position + direction * ray_length;
        sample_uv = projectPos(march_position);
        march_position_inv = position - direction - ray_length_inv;
        sample_uv_inv = projectPos(march_position_inv);

        if (sample_uv.x > 1.0 || sample_uv.x < 0.0 || sample_uv.y > 1.0 || sample_uv.y < 0.0) {
            break;
        }

        screen_depth = worldPos(sample_uv).z;
        target_depth = march_position.z;
        target_depth_inv = march_position_inv.y;


    	if ((screen_depth - target_depth) < 0.0005) {
       		if (texture2D(depthmap, sample_uv).x > start_depth && march_position.z / screen_depth > 1.01) {
            	return texture2D(rendered, sample_uv);
        	}
    	}
        ray_length += stp;
        stp *= 1.01;
}

   return texture2D(rendered, sample_uv_inv, target_depth_inv);
}

const float _WindSpeed = 100.0;
const vec3 _WindDir = normalize(vec3(1.0, 1.0, 1.0));

#if ENABLE_WAVING_WATER
const float _WaveWidth = 30.0;
const float _WaveHeight = 0.04;

const float _RippleWidth = 15.0;
const float _RippleHeight = 0.02;
#else
const float _WaveWidth = 0.0;
const float _WaveHeight = 0.0;

const float _RippleWidth = 0.0;
const float _RippleHeight = 0.0;
#endif

const float _ReflectFactor = 0.5;
#endif


void main(void) {
	vec4 mask = texture2D(water_mask, uv);
    vec4 base_color = texture2D(rendered, uv);
#if ENABLE_LIQUID_REFLECTIONS && ENABLE_WAVING_WATER
    if (mask == vec4(1.0, 0.0, 1.0, 1.0)) {
        vec3 position = worldPos(uv);
        vec3 normal = normalize(mat3(mCameraView) * texture2D(normalmap, uv).xyz);

        // Waves and ripples
        vec3 real_position = position * mat3(mCameraView) + cameraPosition;

        float wave_noise = snoise((real_position / _WaveWidth) + (animationTimer * _WindSpeed * 0.5 * _WindDir));
        float ripple_noise = snoise((real_position / _RippleWidth) + (animationTimer * _WindSpeed * 2.0 * _WindDir));

        normal.z += ((wave_noise - 0.5) * _WaveHeight) + ((ripple_noise - 0.5) * _RippleHeight);

        // Reflection color
        vec3 camera_dir = normalize(position);
        vec3 direction = reflect(camera_dir, normal);
        vec4 reflect_color = raycast(position, direction, _MaxDistance, skyBgColor);

        // Fresnel
        float factor = dot(-camera_dir, normal);
        factor = pow(factor, _ReflectFactor);

        gl_FragColor = mix(reflect_color, base_color, factor);
        return;
    }

    gl_FragColor = base_color;
    #else
    gl_FragColor = base_color;
    #endif
}
