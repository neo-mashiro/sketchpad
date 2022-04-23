#version 460 core

layout(std140, binding = 0) uniform Camera {
    vec4 position;
    vec4 direction;
    mat4 view;
    mat4 projection;
} camera;

#include "../core/renderer_input.glsl"

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;

void main() {
    gl_Position = camera.projection * camera.view * self.transform * vec4(position, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bloom;

layout(location = 3) uniform vec3  light_color;
layout(location = 4) uniform float light_intensity;
layout(location = 5) uniform float bloom_factor;

// light sources are often rendered with bloom effect to simulate light rays bleeding so
// we can always write to the second render target regardless of the luminance threshold
// check, the bloom factor controls the saturation of bloom, > 1 = amplify, < 1 = reduce

void main() {
    float fade_io = 0.3 + abs(cos(rdr_in.time));
    float intensity = light_intensity * fade_io;

    // if the 2nd MRT isn't enabled, bloom will write to GL_NONE and be discarded
    color = vec4(light_color * intensity, 1.0);
    bloom = intensity > 0.2 ? vec4(color.rgb * bloom_factor, 1.0) : vec4(0.0);
}

#endif
