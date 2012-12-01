
uniform sampler2D myTexture;
uniform vec4 skyBgColor;
uniform float fogDistance;

varying vec3 vPosition;

void main (void)
{
    //vec4 col = vec4(1.0, 0.0, 0.0, 1.0);
    vec4 col = texture2D(myTexture, vec2(gl_TexCoord[0]));
	float a = col.a;
    col *= gl_Color;
	col = col * col; // SRGB -> Linear
	col *= 1.8;
	col.r = 1.0 - exp(1.0 - col.r) / exp(1.0);
	col.g = 1.0 - exp(1.0 - col.g) / exp(1.0);
	col.b = 1.0 - exp(1.0 - col.b) / exp(1.0);
	col = sqrt(col); // Linear -> SRGB
	if(fogDistance != 0.0){
		float d = max(0.0, min(vPosition.z / fogDistance * 1.5 - 0.6, 1.0));
		col = mix(col, skyBgColor, d);
	}
    gl_FragColor = vec4(col.r, col.g, col.b, a);
}
