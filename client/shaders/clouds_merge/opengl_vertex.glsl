uniform mat4 mCameraProjInv;
uniform mat4 mCameraView;
uniform vec3 v_LightDirection;
uniform float f_timeofday;

varying vec3 relativePosition;
varying vec3 viewDirection;
varying vec2 screenspaceCoordinate;
varying vec3 sunTint;
varying float auroraFactor;

vec3 getDirectLightScatteringAtGround(vec3 v_LightDirection)
{
	// Based on talk at 2002 Game Developers Conference by Naty Hoffman and Arcot J. Preetham
	const float beta_r0 = 1e-5; // Rayleigh scattering beta

	// These factors are calculated based on expected value of scattering factor of 1e-5
	// for Nitrogen at 532nm (green), 2e25 molecules/m3 in atmosphere
	const vec3 beta_r0_l = vec3(3.3362176e-01, 8.75378289198826e-01, 1.95342379700656) * beta_r0; // wavelength-dependent scattering

	const float atmosphere_height = 15000.; // height of the atmosphere in meters
	// sun/moon light at the ground level, after going through the atmosphere
	return exp(-beta_r0_l * atmosphere_height / (1e-5 - dot(v_LightDirection, vec3(0., 1., 0.))));
}

// custom smoothstep implementation because it's not defined in glsl1.2
// https://docs.gl/sl4/smoothstep
float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

void main(void)
{
	vec4 p = mCameraProjInv * inVertexPosition;
	viewDirection = p.xyz / p.w;
	relativePosition = (p.xyz / p.w) * mat3(mCameraView);
	screenspaceCoordinate = inVertexPosition.xy;

	auroraFactor = 1. - mtsmoothstep(0.13, 0.15, f_timeofday) * mtsmoothstep(0.87, 0.85, f_timeofday);

	float tintFactor = mtsmoothstep(0.21, 0.24, f_timeofday) * mtsmoothstep(0.793, 0.753, f_timeofday);

	sunTint = mix(vec3(1.0), getDirectLightScatteringAtGround(v_LightDirection), tintFactor);

	gl_Position = inVertexPosition;
}
