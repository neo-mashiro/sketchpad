#version 460

in vec2 _uv;

layout(location = 0) out vec4 o_color;

uniform sampler2D albedo;

void main() {
    o_color = texture(albedo, _uv * 1);
}
