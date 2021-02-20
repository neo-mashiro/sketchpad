#version 440

smooth in vec4 _color;  // user-defined input, passed from the previous shader stage

layout(location = 0) out vec4 Color;

void main() {
    Color = _color;
}