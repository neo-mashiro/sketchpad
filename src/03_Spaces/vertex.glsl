#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

smooth out vec4 _color;

uniform mat4 MVP;

void main() {
    _color = color;
    gl_Position = MVP * vec4(position, 1.0f);
}