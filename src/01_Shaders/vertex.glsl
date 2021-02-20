#version 440

layout(location = 0) in vec4 position;  // position attribute index = 0
layout(location = 1) in vec4 color;     // color attribute index = 1

smooth out vec4 _color;  // user-defined output variables, used by later shader stages

void main() {
    gl_Position = position;  // built-in output variables used by OpenGL start with gl_xxx
    _color = color;
}