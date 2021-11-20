#version 460 core
#pragma optimize(off)

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 projection;
} camera;

layout(location = 0) uniform bool depth_prepass;
layout(location = 1) uniform mat4 transform;
layout(location = 2) uniform int material_id;

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

out vec3 _position;
out vec3 _normal;

void main() {
    _position = vec3(transform * vec4(position, 1.0));
    _normal = normalize(vec3(transform * vec4(normal, 0.0)));

    gl_Position = camera.projection * camera.view * transform * vec4(position, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

in vec3 _position;
in vec3 _normal;

layout(location = 0) out vec4 irradiance;

layout(binding = 0) uniform samplerCube irradiance_map;

void main() {
    vec3 I = normalize(_position - camera.position);
    vec3 R = reflect(I, normalize(_normal));

    //vec3 N = normalize(_normal);

    vec4 reflection_color = vec4(texture(irradiance_map, R).rgb, 1.0);
    //vec4 reflection_color = vec4(texture(irradiance_map, N).rgb, 1.0);
    irradiance = clamp(reflection_color, vec4(0), vec4(1));
}

#endif
