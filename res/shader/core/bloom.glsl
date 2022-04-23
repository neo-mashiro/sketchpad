#version 460 core

// simple bloom effect in compute shader (1 downsample + 6 Gaussian blur passes + 1 upsample)

// for image load store operations, reading from any texel outside the boundaries will return 0
// and writing to any texel outside the boundaries will do nothing, so we can safely ignore all
// boundary checks. This shader can be slightly improved by using shared local storage to cache
// the fetched pixels, but the performance gain is small since imageLoad() is already very fast

// caution: please do not blur the alpha channel!

#ifdef compute_shader

#define k11x11
#include "../utils/gaussian.glsl"

layout(local_size_x = 32, local_size_y = 18, local_size_z = 1) in;  // fit 16:9 aspect ratio

layout(binding = 0, rgba16f) uniform image2D ping;
layout(binding = 1, rgba16f) uniform image2D pong;

layout(location = 0) uniform bool horizontal;

void GaussianBlurH(const ivec2 coord) {
    vec3 color = imageLoad(ping, coord).rgb * weight[0];
    for (int i = 1; i < 6; i++) {
        ivec2 offset = ivec2(i, 0);
        color += imageLoad(ping, coord + offset).rgb * weight[i];
        color += imageLoad(ping, coord - offset).rgb * weight[i];
    }
    imageStore(pong, coord, vec4(color, 1.0));
}

void GaussianBlurV(const ivec2 coord) {
    vec3 color = imageLoad(pong, coord).rgb * weight[0];
    for (int i = 1; i < 6; i++) {
        ivec2 offset = ivec2(0, i);
        color += imageLoad(pong, coord + offset).rgb * weight[i];
        color += imageLoad(pong, coord - offset).rgb * weight[i];
    }
    imageStore(ping, coord, vec4(color, 1.0));
}

void main() {
    ivec2 ils_coord = ivec2(gl_GlobalInvocationID.xy);

    if (horizontal) {
        GaussianBlurH(ils_coord);  // horizontal
    }
    else {
        GaussianBlurV(ils_coord);  // vertical
    }
}

#endif