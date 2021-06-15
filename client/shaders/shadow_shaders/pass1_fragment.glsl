uniform sampler2D ColorMapSampler;
varying vec4 tPos;

void main()
{
	if(tPos.x<-1.0 || tPos.x>1.0 || 
		tPos.y<-1.0 ||  tPos.y>1.0)
			discard;
			
	vec4 col = texture2D(ColorMapSampler, gl_TexCoord[0].st);

	if (col.a < 0.70)
		discard;
	

	float depth = tPos.z* 0.5 + 0.5 ;
	depth-=5e-7;
	gl_FragColor = vec4(depth, 0.0, 0.0, 1.0);
}
