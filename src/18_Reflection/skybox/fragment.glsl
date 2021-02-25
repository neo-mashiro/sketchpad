#version 440

in vec3 _tex_coords;

layout(location = 0) out vec4 Color;

uniform samplerCube skybox;

void main() {
    Color = texture(skybox, _tex_coords);
}