
uniform sampler2D myTexture;
uniform sampler2D normalTexture;

uniform vec4 skyBgColor;
uniform float fogDistance;

varying vec3 vPosition;

varying vec3 viewVec;

void main (void)
{
	vec4 col = texture2D(myTexture, vec2(gl_TexCoord[0]));
	float alpha = col.a;
	vec2 uv = gl_TexCoord[0].st;
	vec4 base = texture2D(myTexture, uv);
	vec4 final_color = vec4(0.2, 0.2, 0.2, 1.0) * base;
	vec3 vVec = normalize(viewVec);
	vec3 bump = normalize(texture2D(normalTexture, uv).xyz * 2.0 - 1.0);
	vec3 R = reflect(-vVec, bump);
	vec3 lVec = normalize(vec3(0.0, -0.4, 0.5));
	float diffuse = max(dot(lVec, bump), 0.0);
  
	vec3 color = diffuse * texture2D(myTexture, gl_TexCoord[0].st).rgb;
	
	
	float specular = pow(clamp(dot(R, lVec), 0.0, 1.0),1.0);
    color += vec3(0.2*specular*diffuse);
 
  
	col = vec4(color.r, color.g, color.b, alpha);
	col *= gl_Color;
	col = col * col; // SRGB -> Linear
	col *= 1.8;
	col.r = 1.0 - exp(1.0 - col.r) / exp(1.0);
	col.g = 1.0 - exp(1.0 - col.g) / exp(1.0);
	col.b = 1.0 - exp(1.0 - col.b) / exp(1.0);
	col = sqrt(col); // Linear -> SRGB
	if(fogDistance != 0.0){
		float d = max(0.0, min(vPosition.z / fogDistance * 1.5 - 0.6, 1.0));
		alpha = mix(alpha, 0.0, d);
	}

    gl_FragColor = vec4(col.r, col.g, col.b, alpha);   
}
