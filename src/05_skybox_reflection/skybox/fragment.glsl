#version 440

in vec3 _uv;

layout(location = 0) out vec4 Color;

uniform samplerCube skybox;

void main() {
    Color = texture(skybox, _uv);
}