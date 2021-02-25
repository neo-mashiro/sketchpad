#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 _normal;
out vec3 _position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    _normal = normal;
    _position = position;
    
    mat4 mvp = projection * view * model;
    gl_Position = mvp * vec4(position, 1.0f);
}