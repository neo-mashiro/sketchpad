#version 440

in vec3 _normal;  // normal vector
in vec3 _position;

layout(location = 0) out vec4 Color;

uniform vec3 camera_pos;
uniform samplerCube skybox;

const vec4 extra_tint = vec4(0.25, 0.0, 0.2, 0);

void main() {
    vec3 I = normalize(_position - camera_pos);  // view direction vector
    vec3 R = reflect(I, normalize(_normal));     // reflection vector

    vec4 reflection_color = vec4(texture(skybox, R).rgb, 1.0);
    Color = clamp(reflection_color + extra_tint, vec4(0), vec4(1));
}