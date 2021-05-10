#version 460

layout(location = 0) in vec3 position;

smooth out vec4 _color;

uniform mat4 u_MVP;

void main() {
    _color = vec4(position * 0.5 + vec3(0.7), 1.0);
    gl_Position = u_MVP * vec4(position, 1.0);
}