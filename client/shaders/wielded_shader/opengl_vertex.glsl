uniform mat4 mWorldViewProj;
uniform mat4 mWorld;

// Directional lighting information
uniform vec4 lightColor;
uniform vec3 lightDirection;

uniform vec3 dayLight;
uniform vec3 eyePosition;
uniform float animationTimer;

varying vec3 vPosition;
varying vec3 worldPosition;

varying vec3 lightVec;
varying vec3 eyeVec;

// Color of the light emitted by the light sources.
const vec3 artificialLight = vec3(1.04, 1.04, 1.04);
const vec3 artificialLightDirection = normalize(vec3(0.2, 1.0, -0.5));

const float e = 2.718281828459;
const float BS = 10.0;

// These methods apply a gamma value to approximately convert a value from/to
// sRGB colourspace
float from_sRGB(float x)
{
	if (x < 0.0 || x > 1.0)
		return x;
	return pow(x, 2.2);
}
float to_sRGB(float x)
{
	if (x < 0.0 || x > 1.0)
		return x;
	return pow(x, 1.0 / 2.2);
}
vec3 from_sRGB_vec(vec3 v)
{
	return vec3(from_sRGB(v.r), from_sRGB(v.g), from_sRGB(v.b));
}
vec3 to_sRGB_vec(vec3 v)
{
	return vec3(to_sRGB(v.r), to_sRGB(v.g), to_sRGB(v.b));
}

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = mWorldViewProj * gl_Vertex;

	vPosition = gl_Position.xyz;
	worldPosition = (mWorld * gl_Vertex).xyz;

	vec3 sunPosition = vec3 (0.0, eyePosition.y * BS + 900.0, 0.0);

	lightVec = sunPosition - worldPosition;

	eyeVec = -(gl_ModelViewMatrix * gl_Vertex).xyz;

	// Calculate color.
	// Red, green and blue components are pre-multiplied with
	// the brightness, so now we have to multiply these
	// colors with the color of the incoming light.
	// The pre-baked colors are halved to prevent overflow.
	vec4 color;
	// The alpha gives the ratio of sunlight in the incoming light.
	float outdoorsRatio = 1.0 - gl_Color.a;
	color.a = 1.0;
	color.rgb = gl_Color.rgb * (outdoorsRatio * dayLight.rgb +
		gl_Color.a * artificialLight.rgb);

#if defined(ENABLE_DIRECTIONAL_SHADING) && !LIGHT_EMISSIVE
	vec3 norm = normalize((mWorld * vec4(gl_Normal, 1.0)).xyz);

	vec3 fakeLightDirection = lightDirection;
	fakeLightDirection.y *= mix(0.5, 3, clamp(dot(lightDirection, vec3(0.0, 1.0, 0.0)) * 3, 0, 1));
	fakeLightDirection = normalize(fakeLightDirection);

	vec3 dir = fakeLightDirection;

	float factor = clamp((dot(lightDirection, vec3(0.0, -1.0, 0.0)) - 0.3) * 5, 0, 1);
	dir = mix(dir, vec3(0, 1, 0), factor);

	// Lighting color
	vec3 resultLightColor = ((lightColor.rgb * gl_Color.a) + clamp(outdoorsRatio, 0.4f * factor,1.0f));
	resultLightColor = from_sRGB_vec(resultLightColor);

	float ambient_light = 0.3;
	float directional_boost = 0.5 - abs(dot(norm, vec3(1,0,0))) * 0.5;
	float directional_light = dot(norm, dir);

	directional_light = max(directional_light + directional_boost, 0.0);
	directional_light *= (1.0 - ambient_light) / (1 + directional_boost);
	resultLightColor = resultLightColor * directional_light + ambient_light;

	directional_light = dot(norm, artificialLightDirection);
	directional_light = max(directional_light + 0.5, 0.0);
	directional_light *= (1.0 - ambient_light) / 1.5;
	float artificialLightShading = directional_light + ambient_light;

	color.rgb *= to_sRGB_vec(max(resultLightColor,
			from_sRGB_vec(artificialLight.rgb) * artificialLightShading * outdoorsRatio));
#endif

        // Emphase blue a bit in darker places
        // See C++ implementation in mapblock_mesh.cpp finalColorBlend()
        float brightness = (color.r + color.g + color.b) / 3;
	color.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	gl_FrontColor = gl_BackColor = clamp(color, 0.0, 1.0);
}
