#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

out vec2 _uv;
out vec3 _normal;

uniform mat4 mvp;

void main() {
    _uv = uv;
    _normal = normal;
    
    gl_Position = mvp * vec4(position, 1.0f);
}