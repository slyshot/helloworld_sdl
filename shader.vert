//copied from vkguide.dev for hello triangle


//we will be using glsl version 4.5 syntax
#version 450
layout(location=0) out vec3 fragColor;



vec3 positions[3] = vec3[](
    vec3(0.0, -1, 0),
    vec3(1, 1, 0),
    vec3(-1, 1, 0)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    fragColor = colors[gl_VertexIndex];
}


