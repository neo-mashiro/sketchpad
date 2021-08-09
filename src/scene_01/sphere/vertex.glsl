#version 460

layout(location = 0) in vec3 position;  // in local model space
layout(location = 1) in vec3 normal;    // in local model space

out vec3 _position;  // in world space
out vec3 _normal;    // in world space

uniform mat4 u_M;
uniform mat4 u_MVP;

void main() {
    // model space -> world space
    _position = vec3(u_M * vec4(position, 1.0));
    _normal = normalize(vec3(u_M * vec4(normal, 0.0)));
    // we don't need to compute like this if the transform is orthogonal
    // _normal = normalize(mat3(transpose(inverse(u_M))) * normal);

    gl_Position = u_MVP * vec4(position, 1.0);
}
