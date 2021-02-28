#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

smooth out vec4 _color;  // user-defined output, to be used by later shader stages

uniform float elapsed_time;      // uninitialized, to be queried and set by OpenGL code
uniform float loop_time = 4.0f;  // initialized, users don't have to set value from OpenGL

void main() {
    _color = color;

    float time_scale = 3.141592653f * 2.0f / loop_time;

    float tick = mod(elapsed_time, loop_time);  // modulo operation
    vec2 offset = vec2(cos(tick * time_scale) * 0.5f, sin(tick * time_scale) * 0.5f);

    gl_Position = position + vec4(offset, 0.0, 0.0);
}