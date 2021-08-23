#version 460

in vec3 _tex_coords;

layout(binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 color;

void main() {
    color = texture(skybox, _tex_coords);
}
