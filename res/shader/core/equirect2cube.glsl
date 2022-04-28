#version 460 core

// convert a (2:1) equirectangle texture to a (1:1 x 6) cubemap texture

#ifdef compute_shader

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(binding = 0) uniform sampler2D equirectangle;
layout(binding = 0, rgba16f) restrict writeonly uniform imageCube cubemap;

#include "../utils/projection.glsl"

void main() {
    vec2 resolution = vec2(imageSize(cubemap));
    ivec3 ils_coordinate = ivec3(gl_GlobalInvocationID);

    vec3 v = ILS2Cartesian(ils_coordinate, resolution);

    vec2 sample_vec = Cartesian2Spherical(v);
    sample_vec = Spherical2Equirect(sample_vec);
    vec4 color = texture(equirectangle, sample_vec);

    imageStore(cubemap, ils_coordinate, color);
}

#endif
