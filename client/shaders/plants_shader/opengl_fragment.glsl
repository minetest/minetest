uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D useNormalmap;

uniform vec4 skyBgColor;
uniform float fogDistance;
uniform vec3 eyePosition;

varying vec3 vPosition;
varying vec3 eyeVec;

const float e = 2.718281828459;

void main (void)
{
	vec3 color;
	vec2 uv = gl_TexCoord[0].st;

#ifdef USE_NORMALMAPS
	float use_normalmap = texture2D(useNormalmap,vec2(1.0,1.0)).r;
#endif

#ifdef ENABLE_BUMPMAPPING
	if (use_normalmap > 0.0) {
		vec3 base = texture2D(baseTexture, uv).rgb;
		vec3 vVec = normalize(eyeVec);
		vec3 bump = normalize(texture2D(normalTexture, uv).xyz * 2.0 - 1.0);
		vec3 R = reflect(-vVec, bump);
		vec3 lVec = normalize(vVec);
		float diffuse = max(dot(lVec, bump), 0.0);
		float specular = pow(clamp(dot(R, lVec), 0.0, 1.0),1.0);
		color = mix (base,diffuse*base,1.0) + 0.1 * specular * diffuse;
	} else {
		color = texture2D(baseTexture, uv).rgb;
	}
#else
	color = texture2D(baseTexture, uv).rgb;
#endif

	float alpha = texture2D(baseTexture, uv).a;
	vec4 col = vec4(color.r, color.g, color.b, alpha);
	col *= gl_Color;
	col = col * col; // SRGB -> Linear
	col *= 1.8;
	col.r = 1.0 - exp(1.0 - col.r) / e;
	col.g = 1.0 - exp(1.0 - col.g) / e;
	col.b = 1.0 - exp(1.0 - col.b) / e;
	col = sqrt(col); // Linear -> SRGB
	if(fogDistance != 0.0){
		float d = max(0.0, min(vPosition.z / fogDistance * 1.5 - 0.6, 1.0));
		col = mix(col, skyBgColor, d);
	}
    gl_FragColor = vec4(col.r, col.g, col.b, alpha);
}
