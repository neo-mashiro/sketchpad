#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 _position;
out vec3 _normal;

uniform mat4 u_MVP;

void main() {
    _position = position;
    _normal = normal;
    
    gl_Position = u_MVP * vec4(position, 1.0);
}