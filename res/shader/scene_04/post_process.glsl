#version 460 core

#ifdef vertex_shader

layout(location = 0) out vec2 _uv;

void main() {
    vec2 position = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1;
    _uv = (position + 1) * 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec2 _uv;
layout(location = 0) out vec4 color;

layout(location = 0) uniform int operator;  // tone-mapping operator
layout(binding = 0) uniform sampler2D hdr_map;

#include "../utils/postprocess.glsl"

void main() {
    vec3 radiance = texture(hdr_map, _uv).rgb;
    vec3 ldr_color;

    switch (operator) {
        case 0: ldr_color = Reinhard(radiance);         break;
        case 1: ldr_color = ReinhardJodie(radiance);    break;
        case 2: ldr_color = Uncharted2Filmic(radiance); break;
        case 3: ldr_color = ApproxACES(radiance);       break;
    }

    color = vec4(Linear2Gamma(ldr_color), 1.0);
}

#endif
