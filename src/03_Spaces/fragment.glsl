#version 440

smooth in vec4 _color;

layout(location = 0) out vec4 Color;

void main() {
    Color = _color;
}