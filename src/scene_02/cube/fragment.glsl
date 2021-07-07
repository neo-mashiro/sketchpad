#version 460

in vec3 _position;
in vec3 _normal;

layout(location = 0) out vec4 o_color;

uniform vec3 camera_pos;
uniform samplerCube skybox;
uniform mat4 u_MVP;
uniform mat4 u_M;

const vec4 extra_tint = vec4(0.08, 0.05, 0.07, 0);

void main() {
    // compute the real position and normal after transformation
    vec3 position = (u_M * vec4(_position, 1)).xyz;
    vec3 normal = (u_M * vec4(_normal, 0)).xyz;

    vec3 I = normalize(position - camera_pos);  // view direction vector
    vec3 R = reflect(I, normalize(normal));     // reflection vector

    vec4 reflection_color = vec4(texture(skybox, R).rgb, 1.0);
    o_color = clamp(reflection_color + extra_tint, vec4(0), vec4(1));
}
