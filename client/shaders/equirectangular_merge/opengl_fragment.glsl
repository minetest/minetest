#define PI 3.1415926535897932384626433832795

uniform sampler2D baseTexture;

void main(void)
{
	vec2 screen = gl_TexCoord[0].st;
	float phi = screen.t * PI;
	float theta = screen.s * 2 * PI;
	float x = sin(theta) * sin(phi);
	float y = cos(phi);
	float z = cos(theta) * sin(phi);
	float a = max(max(abs(x), abs(y)), abs(z));
	vec2 atlas = vec2(1, 1);
	int face = -1;
	if (a == x) { // X -
		face = 1;
		atlas.x = (1.0 - tan(atan(z / x))) / 2.0;
		atlas.y = (1.0 + tan(atan(y / x))) / 2.0;
	} else if (a == -x) { // X+
		face = 0;
		atlas.x = (1.0 - tan(atan(z / x))) / 2.0;
		atlas.y = (1.0 - tan(atan(y / x))) / 2.0;
	} else if (a == y) { // Y+
		face = 3;
		atlas.x = (1.0 + tan(atan(x / y))) / 2.0;
		atlas.y = (1.0 - tan(atan(z / y))) / 2.0;
	} else if (a == -y) { // Y-
		face = 2;
		atlas.x = (1.0 - tan(atan(x / y))) / 2.0;
		atlas.y = (1.0 - tan(atan(z / y))) / 2.0;
	} else if (a == z) { // Z-
		face = 5;
		atlas.x = (1.0 + tan(atan(x / z))) / 2.0;
		atlas.y = (1.0 + tan(atan(y / z))) / 2.0;
	} else if (a == -z) { // Z+
		face = 4;
		atlas.x = (1.0 + tan(atan(x / z))) / 2.0;
		atlas.y = (1.0 - tan(atan(y / z))) / 2.0;
	}
	atlas.x = min(atlas.x, 1);
	atlas.y = min(atlas.y, 1);
	if (face == 2 || face == 3) {
		atlas.xy = atlas.yx;
		atlas.x = 1 - atlas.x;
	}
	atlas.x /= 3.0;
	atlas.y /= 2.0;
	atlas.x += mod(face, 3) / 3.0;
	atlas.y = 1 - (atlas.y + floor(face / 3) / 2.0);
	vec4 color = texture2D(baseTexture, atlas).rgba;
	gl_FragColor = color;
}
