#version 440

smooth in vec4 s_color;  // user-defined input, passed from the previous shader stage

// if there's only one output variable, it will be output to location 0 by default
layout(location = 0) out vec4 o_color;

void main() {
    o_color = s_color;
}