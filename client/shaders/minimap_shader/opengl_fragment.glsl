uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform vec3 yawVec;

varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

const mat3 RGBtoOpponentMat = mat3(0.2814, -0.0971, -0.0930, 0.6938, 0.1458,-0.2529, 0.0638, -0.0250, 0.4665);
const mat3 OpponentToRGBMat = mat3(1.1677, 0.9014, 0.7214, -6.4315, 2.5970, 0.1257, -0.5044, 0.0159, 2.0517);

const int NONE = 0;
const int PROTANOPIA = 1;
const int DEUTERANOPIA = 2;
const int TRITANOPIA = 3;

const int blindnessType = NONE; //example

void blindnessFilter( out vec4 myoutput, in vec4 myinput )
{
	if (blindnessType == PROTANOPIA) {
			vec3 opponentColor = RGBtoOpponentMat * vec3(myinput.r, myinput.g, myinput.b);
			opponentColor.x -= opponentColor.y * 1.5; // reds (y <= 0) become lighter, greens (y >= 0) become darker
			vec3 rgbColor = OpponentToRGBMat * opponentColor;
			myoutput = vec4(rgbColor.r, rgbColor.g, rgbColor.b, myinput.a);
	} else if (blindnessType == DEUTERANOPIA) {
			vec3 opponentColor = RGBtoOpponentMat * vec3(myinput.r, myinput.g, myinput.b);
			opponentColor.x -= opponentColor.y * 1.5; // reds (y <= 0) become lighter, greens (y >= 0) become darker
			vec3 rgbColor = OpponentToRGBMat * opponentColor;
			myoutput = vec4(rgbColor.r, rgbColor.g, rgbColor.b, myinput.a);
	} else if (blindnessType == TRITANOPIA) {
			vec3 opponentColor = RGBtoOpponentMat * vec3(myinput.r, myinput.g, myinput.b);
			opponentColor.x -= ((3.0 * opponentColor.z) - opponentColor.y) * 0.25;
			vec3 rgbColor = OpponentToRGBMat * opponentColor;
			myoutput = vec4(rgbColor.r, rgbColor.g, rgbColor.b, myinput.a);
    } else {
			myoutput = myinput;
	}	
}

void blindnessVision( out vec4 myoutput, in vec4 myinput )
{
	vec4 blindVisionR;
	vec4 blindVisionG;
	vec4 blindVisionB;
	if (blindnessType == PROTANOPIA) {
			blindVisionR = vec4( 0.20,  0.99, -0.19, 0.0);
			blindVisionG = vec4( 0.16,  0.79,  0.04, 0.0);
			blindVisionB = vec4( 0.01, -0.01,  1.00, 0.0);
	} else if (blindnessType == DEUTERANOPIA) {
			blindVisionR = vec4( 0.43,  0.72, -0.15, 0.0 );
			blindVisionG = vec4( 0.34,  0.57,  0.09, 0.0 );
			blindVisionB = vec4(-0.02,  0.03,  1.00, 0.0 );		
	} else if (blindnessType == TRITANOPIA) {
			blindVisionR = vec4( 0.97,  0.11, -0.08, 0.0 );
			blindVisionG = vec4( 0.02,  0.82,  0.16, 0.0 );
			blindVisionB = vec4(-0.06,  0.88,  0.18, 0.0 );
	} else {
        	blindVisionR = vec4(1.0,  0.0,  0.0, 0.0 );
        	blindVisionG = vec4(0.0,  1.0,  0.0, 0.0 );
        	blindVisionB = vec4(0.0,  0.0,  1.0, 0.0 );			
	}
	myoutput = vec4(dot(myinput, blindVisionR), dot(myinput, blindVisionG), dot(myinput, blindVisionB), myinput.a);	
}

void main (void)
{
	vec2 uv = varTexCoord.st;

	//texture sampling rate
	const float step = 1.0 / 256.0;
	float tl = texture2D(normalTexture, vec2(uv.x - step, uv.y + step)).r;
	float t  = texture2D(normalTexture, vec2(uv.x - step, uv.y - step)).r;
	float tr = texture2D(normalTexture, vec2(uv.x + step, uv.y + step)).r;
	float r  = texture2D(normalTexture, vec2(uv.x + step, uv.y)).r;
	float br = texture2D(normalTexture, vec2(uv.x + step, uv.y - step)).r;
	float b  = texture2D(normalTexture, vec2(uv.x, uv.y - step)).r;
	float bl = texture2D(normalTexture, vec2(uv.x - step, uv.y - step)).r;
	float l  = texture2D(normalTexture, vec2(uv.x - step, uv.y)).r;
	float dX = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
	float dY = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);
	vec4 bump = vec4 (normalize(vec3 (dX, dY, 0.1)),1.0);
	float height = 2.0 * texture2D(normalTexture, vec2(uv.x, uv.y)).r - 1.0;
	vec4 base = texture2D(baseTexture, uv).rgba;
	vec3 L = normalize(vec3(0.0, 0.75, 1.0));
	float specular = pow(clamp(dot(reflect(L, bump.xyz), yawVec), 0.0, 1.0), 1.0);
	float diffuse = dot(yawVec, bump.xyz);

	vec3 color = (1.1 * diffuse + 0.05 * height + 0.5 * specular) * base.rgb;
	vec4 col = vec4(color.rgb, base.a);
	col *= varColor;
	col = vec4(col.rgb, base.a);
	vec4 tmp;
    blindnessFilter(tmp, col);    
    blindnessVision(col, tmp);
	gl_FragColor = col;
}
