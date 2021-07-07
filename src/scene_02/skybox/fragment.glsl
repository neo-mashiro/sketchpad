#version 460

in vec3 _uvw;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform samplerCube skybox;

void main() {
    o_color = texture(skybox, _uvw);
}
