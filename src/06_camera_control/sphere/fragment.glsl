#version 460

in vec2 _uv;

layout(location = 0) out vec4 o_color;

uniform sampler2D albedo;
uniform sampler2D normal;

void main() {
    o_color = mix(texture(albedo, _uv * 4), texture(normal, _uv * 4), 0.8);
}