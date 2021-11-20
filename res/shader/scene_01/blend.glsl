#version 460 core

#ifdef vertex_shader

layout(location = 0) out vec2 uv;

void main() {
    vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;
    uv = (position + 1) * 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D hdr_map;
layout(binding = 1) uniform sampler2D bloom_map;

layout(location = 0) uniform int tone_mapping_mode;

#include tone_map.glsl

void main() {
    vec3 irradiance = texture(hdr_map, uv).rgb;
    vec3 ldr_color;

    // the extended Reinhard luminance tone mapping operator tends to tone down or
    // completely wash out the bloom effect, the color correction is too strong in
    // bright areas but I'm really clueless about why this happens......

    if (tone_mapping_mode == 0) {
        ldr_color = Reinhard(irradiance);
    }
    // else if (tone_mapping_mode == 1) {
    //     ldr_color = ReinhardLuminance(irradiance, 9914.8);
    // }
    else if (tone_mapping_mode == 1) {
        ldr_color = ReinhardJodie(irradiance);
    }
    else if (tone_mapping_mode == 2) {
        ldr_color = Uncharted2Filmic(irradiance);
    }
    else if (tone_mapping_mode == 3) {
        ldr_color = ApproxACES(irradiance);
    }

    // typically we don't need to apply tone mapping to the bloom effect, because the
    // highlights are meant to be blurred anyway and they don't contain details, just
    // add the bloom to the tone-mapped fragment color and let clipping does its work

    ldr_color += texture(bloom_map, uv).rgb;
    ldr_color = pow(ldr_color, vec3(1.0 / 2.2));
    color = vec4(ldr_color, 1.0);
}

#endif
