//taken from vkguide.dev for hello triangle
//glsl version 4.5
#version 450

//output write
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;
void main()
{
	//return red
	outColor = vec4(fragColor,1.0);
}
