#version 440

layout(location = 0) in vec3 i_position;  // value set by c++ code, in model space
layout(location = 1) in vec3 i_normal;    // value set by c++ code, in model space

// calculate each fragment's input in view space
out vec3 s_position;
out vec3 s_normal;
out vec3 s_light_position;

uniform vec3 u_light_position;  // value set by c++ code, already in world space

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    // model space -> world space -> view space
    s_position = vec3(u_view * u_model * vec4(i_position, 1.0));
    s_normal = mat3(transpose(inverse(u_view * u_model))) * i_normal;

    // world space -> view space (camera space)
    s_light_position = vec3(u_view * vec4(u_light_position, 1.0));

    gl_Position = u_projection * u_view * u_model * vec4(i_position, 1.0f);
}