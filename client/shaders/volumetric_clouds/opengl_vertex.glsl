uniform mat4 mCameraProjInv;
uniform mat4 mCameraView;
uniform float f_timeofday;

varying vec3 relativePosition;
varying vec3 viewDirection;

varying vec2 screenspaceCoordinate;
varying float sunStrength;

float mtsmoothstep(in float edge0, in float edge1, in float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

void main(void)
{
	screenspaceCoordinate = inVertexPosition.xy;
	vec4 p = mCameraProjInv * inVertexPosition;
	viewDirection = p.xyz / p.w;
	relativePosition = (p.xyz / p.w) * mat3(mCameraView);

	if (f_timeofday < 0.21) {
		sunStrength = 
			(1.0 - mtsmoothstep(0.18, 0.21, f_timeofday));
	} else if (f_timeofday >= 0.793) {
		sunStrength = 
			mtsmoothstep(0.793, 0.823, f_timeofday);
	} else {
		sunStrength = 
			mtsmoothstep(0.21, 0.26, f_timeofday) *
			(1.0 - mtsmoothstep(0.743, 0.793, f_timeofday));	
	}

	gl_Position = inVertexPosition;
}
