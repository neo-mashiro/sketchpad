#version 440

in vec2 _uv;
in vec3 _normal;

layout(location = 0) out vec4 Color;

uniform sampler2D base;
uniform sampler2D overlay;

void main() {
    Color = mix(texture(base, _uv), texture(overlay, _uv), 0.1);
}