//copied from vkguide.dev for hello triangle


//we will be using glsl version 4.5 syntax
#version 450




vec2 positions[3] = vec2[](
    vec2(0.0, -1),
    vec2(1, 1),
    vec2(-1, 1)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}



//void main()
//{
	//const array of positions for the triangle
	//const vec3 positions[3] = vec3[3](
	//	vec3(-1.0f,-1.0f, 0.0f),
	//	vec3(1.0f,-1.0f, 0.0f),
	//	vec3(0.f,1.0f, 0.0f)
	//);
	



	//output the position of each vertex
	//gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
	//gl_Position = vec4(positions, 1.0f);
//}
