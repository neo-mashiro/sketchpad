#version 440

layout(location = 0) in vec3 i_position;  // in local model space
layout(location = 2) in vec3 i_normal;    // in local model space

out vec3 s_position;  // in world space
out vec3 s_normal;    // in world space

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    // model space -> world space
    s_position = vec3(u_model * vec4(i_position, 1.0));
    s_normal = normalize(mat3(transpose(inverse(u_model))) * i_normal);

    gl_Position = u_projection * u_view * u_model * vec4(i_position, 1.0);
}