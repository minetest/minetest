#define THRESHOLD 0.5f

uniform sampler2D Render;

void main()
{
    vec2 uv = gl_TexCoord[0];
	//uv.y = 1.0 - uv.y;
    vec2 texcoord = vec2(uv);
    vec4 color = texture2D(Render, uv);
    float brightness = dot(color.rgb, vec3(0.5126, 0.7152, 0.0722));
    if(brightness > 0.5)
		gl_FragColor = vec4(color.rgb, 1.0);
	else 
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);

}	
