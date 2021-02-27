#version 440

layout(location = 0) in vec4 position;  // position attribute index = 0
layout(location = 1) in vec4 color;     // color attribute index = 1

smooth out vec4 _color;  // user-defined output variables, used by later shader stages
                         // the smooth keyword is optional because that's the default

void main() {
    _color = color;
    gl_Position = position;  // built-in output variables used by OpenGL start with gl_xxx
}