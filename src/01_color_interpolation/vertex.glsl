#version 440

layout(location = 0) in vec4 i_position;  // position attribute index = 0
layout(location = 1) in vec4 i_color;     // color attribute index = 1

smooth out vec4 s_color;  // user-defined output variables, used by later shader stages
                          // the smooth keyword is optional (the default)

void main() {
    s_color = i_color;
    gl_Position = i_position;  // built-in output variables used by OpenGL start with gl_xxx
}