#version 440

smooth in vec4 _color;  // user-defined input, passed from the previous shader stage

// if there's only 1 output variable, it will be output to location 0 by default
layout(location = 0) out vec4 Color;

void main() {
    Color = _color;
}