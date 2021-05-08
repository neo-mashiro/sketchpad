#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

out vec2 _uv;

uniform mat4 u_MVP;

void main() {
    _uv = uv;

    gl_Position = u_MVP * vec4(position, 1.0);
}
