#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 _position;
out vec3 _normal;

uniform mat4 mvp;

void main() {
    _position = position;
    _normal = normal;
    
    gl_Position = mvp * vec4(position, 1.0f);
}