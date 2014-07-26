uniform sampler2D baseTexture;
uniform sampler2D normalTexture;
uniform sampler2D useNormalmap;

uniform vec4 skyBgColor;
uniform float fogDistance;
uniform vec3 eyePosition;
uniform float animationTimer;
uniform float timeOfDay;

varying vec3 vPosition;
varying vec3 eyeVec;
varying vec3 worldPosition;

vec2 windDir = vec2(0.5, -0.8); //wind direction XY
float windSpeed = 0.5; //wind speed
float scale = 1.0; //overall wave scale
vec2 bigWaves = vec2(0.3, 0.3); //strength of big waves
vec2 midWaves = vec2(0.3, 0.15); //strength of middle sized waves
vec2 smallWaves = vec2(0.15, 0.1); //strength of small waves
float choppy = 0.01; //wave choppyness
vec4 tangent = vec4(1.0, 0.0, 0.0, 0.0);
vec4 normal = vec4(0.0, 1.0, 0.0, 0.0);
vec4 binormal = vec4(0.0, 0.0, 1.0, 0.0);
	
void main (void)
{
	vec2 uv = worldPosition.xz;
	float timer = animationTimer * 75.0;
   	//normal map
	vec2 nCoord = vec2(0.0, 0.0);
	nCoord = uv * (scale * 0.05) + windDir * timer * (windSpeed*0.04);
	vec3 normal0 = 2.0 * texture2D(normalTexture, nCoord + vec2(-timer*0.015,-timer*0.005)).rgb - 1.0;
	nCoord = uv * (scale * 0.1) + windDir * timer * (windSpeed*0.08)-(normal0.xy/normal0.zz)*choppy;
	vec3 normal1 = 2.0 * texture2D(normalTexture, nCoord + vec2(+timer*0.020,+timer*0.015)).rgb - 1.0;
	nCoord = uv * (scale * 0.25) + windDir * timer * (windSpeed*0.07)-(normal1.xy/normal1.zz)*choppy;
	vec3 normal2 = 2.0 * texture2D(normalTexture, nCoord + vec2(-timer*0.04,-timer*0.03)).rgb - 1.0;
	nCoord = uv * (scale * 0.5) + windDir * timer * (windSpeed*0.09)-(normal2.xy/normal2.z)*choppy;
	vec3 normal3 = 2.0 * texture2D(normalTexture, nCoord + vec2(+timer*0.03,+timer*0.04)).rgb - 1.0;
	nCoord = uv * (scale* 1.0) + windDir * timer * (windSpeed*0.4)-(normal3.xy/normal3.zz)*choppy;
	vec3 normal4 = 2.0 * texture2D(normalTexture, nCoord + vec2(-timer*0.02,+timer*0.1)).rgb - 1.0;  
	nCoord = uv * (scale * 2.0) + windDir * timer * (windSpeed*0.7)-(normal4.xy/normal4.zz)*choppy;
	vec3 normal5 = 2.0 * texture2D(normalTexture, nCoord + vec2(+timer*0.1,-timer*0.06)).rgb - 1.0;

	vec4 nVec = vec4(normalize(normal0 * bigWaves.x + normal1 * bigWaves.y +
							normal2 * midWaves.x + normal3 * midWaves.y +
							normal4 * smallWaves.x + normal5 * smallWaves.y), 0.9);
	vec4 vDir = normalize(vec4(eyeVec, 1.0));
    vec4 vVec = normalize(vec4(dot(vDir, tangent),	dot(vDir, binormal), dot(vDir, normal), 1.0));
	float fresnelTerm = dot(nVec, normalize(reflect(-vVec, nVec)));
	vec3 reflectiveColor = vec3(0.7, 0.7, 0.7);
	vec3 refractiveColor = vec3(0.1, 0.3, 0.5);
	vec3 color = mix (reflectiveColor, refractiveColor, fresnelTerm);
	float alpha = clamp((color.r + color.b + color.g)*1.5, 0.3, 0.5);

	vec4 col = vec4(color.rgb, alpha);
	col *= gl_Color;
	if(fogDistance != 0.0){
		float d = max(0.0, min(vPosition.z / fogDistance * 1.5 - 0.6, 1.0));
		alpha = mix(alpha, 0.0, d);
	}
    gl_FragColor = vec4(col.rgb, alpha);
}
