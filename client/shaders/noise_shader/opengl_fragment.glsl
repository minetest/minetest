// Pseudorandom number generator
float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

// More random pseudorandom number generator;
float noise(vec2 p){
	vec2 p2 = p + vec2(rand(p), rand(p.yx));
	return rand(p2);
}

void main(void)
{
	gl_FragColor = vec4(vec3(noise(gl_FragCoord.xy)), 1.);
}
