#version 460

smooth in vec4 _color;

layout(location = 0) out vec4 o_color;

void main() {
    o_color = clamp(_color, vec4(0.1), vec4(0.9));
}
