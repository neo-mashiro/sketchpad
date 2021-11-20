#version 460 core

// convert a (2:1) equirectangle texture to a (1:1 x 6) cubemap texture.

// references:
// https://en.wikipedia.org/wiki/Cube_mapping#Memory_addressing
// http://alinloghin.com/articles/compute_ibl.html

#ifdef compute_shader

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(binding = 0) uniform sampler2D equirectangle;
layout(binding = 0, rgba16f) restrict writeonly uniform imageCube cubemap;

#include projection.glsl

void main() {
    vec2 resolution = vec2(imageSize(cubemap));
    vec2 st = gl_GlobalInvocationID.xy / resolution;     // tex coordinates in [0,1] range
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - vec2(1.0);  // convert to [-1,-1] and invert y

    vec3 v = vec3(0.0);  // sample vector in world space

    switch (gl_GlobalInvocationID.z) {
        case 0: v = vec3( +1.0,  uv.y, -uv.x); break;  // posx
        case 1: v = vec3( -1.0,  uv.y,  uv.x); break;  // negx
        case 2: v = vec3( uv.x,  +1.0, -uv.y); break;  // posy
        case 3: v = vec3( uv.x,  -1.0,  uv.y); break;  // negy
        case 4: v = vec3( uv.x,  uv.y,  +1.0); break;  // posz
        case 5: v = vec3(-uv.x,  uv.y,  -1.0); break;  // negz
    }

    v = normalize(v);

    vec2 sample_vec = Cartesian2Spherical(v);
    sample_vec = Spherical2Equirect(sample_vec);
	vec4 color = texture(equirectangle, sample_vec);

	imageStore(cubemap, ivec3(gl_GlobalInvocationID), color);
}

#endif
