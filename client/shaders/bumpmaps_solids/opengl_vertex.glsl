
uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;
uniform float dayNightRatio;

varying vec3 vPosition;
varying vec3 viewVec;

void main(void)
{
	gl_Position = mWorldViewProj * gl_Vertex;

	vPosition = (mWorldViewProj * gl_Vertex).xyz;
	
	vec3 tangent; 
	vec3 binormal; 

	vec3 c1 = cross( gl_Normal, vec3(0.0, 0.0, 1.0) ); 
	vec3 c2 = cross( gl_Normal, vec3(0.0, 1.0, 0.0) ); 

	if( length(c1)>length(c2) )
	{
		tangent = c1;	
	}
	else
	{
		tangent = c2;	
	}

	tangent = normalize(tangent);

//binormal = cross(gl_Normal, tangent); 
//binormal = normalize(binormal);

	vec4 color;
	//color = vec4(1.0, 1.0, 1.0, 1.0);

	float day = gl_Color.r;
	float night = gl_Color.g;
	float light_source = gl_Color.b;

	/*color.r = mix(night, day, dayNightRatio);
	color.g = color.r;
	color.b = color.r;*/

	float rg = mix(night, day, dayNightRatio);
	rg += light_source * 1.5; // Make light sources brighter
	float b = rg;

	// Moonlight is blue
	b += (day - night) / 13.0;
	rg -= (day - night) / 13.0;

	// Emphase blue a bit in darker places
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	b += max(0.0, (1.0 - abs(b - 0.13)/0.17) * 0.025);

	// Artificial light is yellow-ish
	// See C++ implementation in mapblock_mesh.cpp finalColorBlend()
	rg += max(0.0, (1.0 - abs(rg - 0.85)/0.15) * 0.065);

	color.r = rg;
	color.g = rg;
	color.b = b;

	// Make sides and bottom darker than the top
	color = color * color; // SRGB -> Linear
	if(gl_Normal.y <= 0.5)
		color *= 0.6;
		//color *= 0.7;
	color = sqrt(color); // Linear -> SRGB

	color.a = gl_Color.a;

	gl_FrontColor = gl_BackColor = color;

	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec3 n1 = normalize(gl_NormalMatrix * gl_Normal);
	vec4 tangent1 = vec4(tangent.x, tangent.y, tangent.z, 0);
	//vec3 t1 = normalize(gl_NormalMatrix * tangent1);
	//vec3 b1 = cross(n1, t1);

	vec3 v;
	vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	vec3 vVec = -vVertex;
	//v.x = dot(vVec, t1);
	//v.y = dot(vVec, b1);
	//v.z = dot(vVec, n1);
	//viewVec = vVec;
	viewVec = normalize(vec3(0.0, -0.4, 0.5));
	//Vector representing the 0th texture coordinate passed to fragment shader
//gl_TexCoord[0] = vec2(gl_MultiTexCoord0);

// Transform the current vertex
//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
