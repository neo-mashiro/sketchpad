#version 460

////////////////////////////////////////////////////////////////////////////////

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

layout(location = 0) out vec4 blurred_color;

layout(location = 0) uniform bool first_iteration;
layout(location = 1) uniform bool ping;

layout(binding = 0) uniform sampler2D source_texture;
layout(binding = 1) uniform sampler2D ping_texture;  // 1/4 size mipmap
layout(binding = 2) uniform sampler2D pong_texture;  // 1/4 size mipmap

#define kernel11x11
#include gaussian_kernel.glsl

void main() {
    float dx = 1.0 / (textureSize(ping_texture, 0)).x;
    float dy = 1.0 / (textureSize(ping_texture, 0)).y;

    // caution: do not blur the alpha channel!

    // read from the source texture, write to pong
    if (first_iteration) {
        float sdx = 1.0 / (textureSize(source_texture, 0)).x;
        vec3 color = texture(source_texture, uv).rgb * weight[0];

        for (int i = 1; i < 6; i++) {
            vec2 offset = vec2(sdx * i, 0.0);
            color += texture(source_texture, uv + offset).rgb * weight[i];
            color += texture(source_texture, uv - offset).rgb * weight[i];
        }

        blurred_color = vec4(color, 1.0);  // write to pong
    }

    // read from ping, write to pong
    else if (ping) {
        vec3 color = texture(ping_texture, uv).rgb * weight[0];
        for (int i = 1; i < 6; i++) {
            vec2 offset = vec2(dx * i, 0.0);
            color += texture(ping_texture, uv + offset).rgb * weight[i];
            color += texture(ping_texture, uv - offset).rgb * weight[i];
        }

        blurred_color = vec4(color, 1.0);  // write to pong
    }

    // read from pong, write to ping
    else {
        vec3 color = texture(pong_texture, uv).rgb * weight[0];
        for (int i = 1; i < 6; i++) {
            vec2 offset = vec2(0.0, dy * i);
            color += texture(pong_texture, uv + offset).rgb * weight[i];
            color += texture(pong_texture, uv - offset).rgb * weight[i];
        }

        blurred_color = vec4(color, 1.0);  // write to ping
    }
}

#endif
