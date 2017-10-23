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
	vec3 norm = normalize((mWorld * vec4(gl_Normal, 0.0)).xyz);

	// Lighting color
	vec3 resultLightColor = ((lightColor.rgb * gl_Color.a) + outdoorsRatio);

	// ((resultLightColor * ((max(dot(normal, lightDirection), -0.2) + 0.2) / 1.2)* 0.6)) + 0.4;
	resultLightColor = (resultLightColor * ((max(dot(norm, lightDirection), -0.2) + 0.2) * 0.5)) + 0.4;

	float artificialLightShading = ((dot(norm, artificialLightDirection) + 1.0) * 0.25) + 0.5;

	color.rgb *= mix(gl_Color.rgb * artificialLightShading, resultLightColor, outdoorsRatio);
#endif

        // Emphase blue a bit in darker places
        // See C++ implementation in mapblock_mesh.cpp finalColorBlend()
        float brightness = (color.r + color.g + color.b) / 3;
	color.b += max(0.0, 0.021 - abs(0.2 * brightness - 0.021) +
		0.07 * brightness);

	gl_FrontColor = gl_BackColor = clamp(color, 0.0, 1.0);
}
