uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D textureFlags;

uniform float zNear = 0.1;
uniform float zFar = 500.0;

#define rendered baseTexture
#define normalmap normalTexture
#define depthmap textureFlags

float resolution = 1024;
vec2 dirs[]=vec2[8] (vec2(1.0f/resolution,0.0f),vec2(-1.0f/resolution,0.0f),vec2(0.0f,1.0f/resolution),vec2(0.0f,-1.0f/resolution),vec2(1.0f/resolution,1.0f/resolution),vec2(-1.0f/resolution,-1.0f/resolution),vec2(-1.0f/resolution,1.0f/resolution),vec2(1.0f/resolution,-1.0f/resolution));

float true_depth(float d) {
	return d;
	/*float z_n = 2.0 * d - 1.0;
    return (2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear)));*/
}
/*
Just for fun : 
Edgefinder, depthmap based
Usage : 
radius - Blur radius in px
step - Blur stepsize in px
tc - starting uv coordinates
*/
float edge(float radius, float step)
{
	vec2 tc = gl_TexCoord[0].st;
	float sd=true_depth(texture2D(normalmap,tc).w);
	if (sd==1.0f) {
		return 0.0f;
	}
	float edge=1.0f;
	for (float i=0; i < radius; i+=step) {
		for (int j=0; j < 8; j++) {
			float w=abs(1-abs(true_depth(texture2D(normalmap, tc+i*dirs[j]).w)-sd));
			edge*=w;
		}
	}
	return edge;
}

float normalEdge(float radius, float step)
{
	vec2 tc = gl_TexCoord[0].st;
	vec3 sd=(texture2D(normalmap,tc).xyz-0.5)*2;
	float edge=1.0f;
	for (float i=0; i < radius; i+=step) {
		for (int j=0; j < 8; j++) {
			vec3 normal = (texture2D(normalmap, tc+i*dirs[j]).xyz - 0.5)*2;
			float diff = length(sd-normal);
			diff = 1-(dot(sd, normal)+1)/2;
			float w=0.5+abs(1-diff);
			edge*=clamp(w, 0, 1);
		}
	}
	return edge;
}

vec4 blur(float radius, float step)
{
	vec2 tc = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, tc);
	float steps = 0;
	for (float x=0; x < radius; x+=step) {
		for (float y=0; y < radius; y+=step) {
			vec4 other_color = texture2D(rendered, tc+vec2(x/resolution,y/resolution));
			color += other_color;
			steps++;
		}
	}
	if (steps == 0)
		return color;
	return color/steps;
}

void main(void)
{
	vec2 uv = gl_TexCoord[0].st;
	vec4 color = texture2D(rendered, uv).rgba;
	vec4 normal_and_depth = texture2D(normalmap, uv);
	vec3 normal = normal_and_depth.rgb;
	float custom = normal_and_depth.a;
	float depth = texture2D(depthmap, uv).r;
	/*float ed = normalEdge(2.1, 1);
	float de = edge(2.1, 1);
	float aoc = (1-ed) * de;
	aoc = 1-aoc;
	gl_FragColor = vec4(aoc,aoc,aoc, 1);*/
	if (uv.x < 0.5 && uv.y < 0.5)
		gl_FragColor = color;
	else if (uv.y < 0.5)
		gl_FragColor = vec4(depth, depth, depth, 1);
	else if (uv.x < 0.5)
		gl_FragColor = vec4(normal, 1);
	else
		gl_FragColor = vec4(custom, custom, custom, 1);
	//float centerDepth = texture2D(depthmap, vec2(0.5,0.5)).r;
	//float depthDiff = abs(centerDepth - depth);
	//depthDiff *= depthDiff;
	//vec3 focus =blur(clamp(3*depthDiff, 0, 3), 1).rgb;
	// float depth = 
	// depth *= 0.1;
	//gl_FragColor = vec4(normal, 1);
	//gl_FragColor = blur(clamp(3*depthDiff, 0, 3), 1);
	/*float ed = edge(4, 1);
	float depth = true_depth(color.w);
	gl_FragColor = vec4(depth, depth, depth, 1.0);*///vec4(floor(color.xyz * 32) / 32 * min(1, 0.5+ed), 1.0);
}
