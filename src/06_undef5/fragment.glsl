#version 440

smooth in vec4 _color;  // user-defined input, passed from the previous shader stage, prefixed by _

layout(location = 0) out vec4 Color;  // user-defined output to be consumed by the current stage, named as CamelCase

void main() {
    Color = _color;
}