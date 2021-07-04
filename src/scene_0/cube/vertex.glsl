#version 460

layout(location = 0) in vec3 position;

smooth out vec4 _color;

uniform mat4 u_MVP;
uniform vec4 u_color;

void main() {
    _color = u_color * 0.8 + vec4(position, 1.0) * 0.5;
    gl_Position = u_MVP * vec4(position, 1.0);
}
