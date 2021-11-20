#version 460 core

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 projection;
} camera;

// location 0, 1, 2 are always reserved even if we don't use PBR shaders
layout(location = 1) uniform mat4 transform;

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;

void main() {
    gl_Position = camera.projection * camera.view * transform * vec4(position, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) out vec4 irradiance;
layout(location = 1) out vec4 bloom;

layout(location = 3) uniform vec3 light_color;
layout(location = 4) uniform float light_intensity;
layout(location = 5) uniform float bloom_multiplier = 1.0;

float Luminance(vec3 irradiance) {
    return dot(irradiance, vec3(0.2126, 0.7152, 0.0722));
}

// light sources are often rendered with bloom effect to simulate light rays bleeding.
// since we know this is a light source, we can skip the luminance threshold check and
// always write to the second render target for the bloom effect. The bloom multiplier
// uniform can be used to amplify (> 1) or reduce (< 1) the bloom effect (saturation).

// it's ok to write to multiple output variables even if there's only one render target
// in the current framebuffer, in that case, `layout(location = 1) out vec4 bloom` will
// write to `GL_NONE`, the written value is discarded, but it's not considered an error.

void main() {
    irradiance = vec4(light_color * light_intensity, 1.0);
    bloom = vec4(irradiance.rgb * bloom_multiplier, 1.0);
}

#endif
