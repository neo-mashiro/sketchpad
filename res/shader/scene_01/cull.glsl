#version 460 core

// Takahiro Harada et al. 2015, Forward+: Bringing Deferred Lighting to the Next Level
// reference:
// https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf
// https://www.3dgep.com/forward-plus/
// https://github.com/bcrusco/Forward-Plus-Renderer

layout(std140, binding = 0) uniform Camera {
    vec4 position;
    vec4 direction;
    mat4 view;
    mat4 projection;
} camera;

#include "../core/renderer_input.glsl"

////////////////////////////////////////////////////////////////////////////////

#ifdef compute_shader

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer Color    { vec4  pl_color[];    };
layout(std430, binding = 1) readonly buffer Position { vec4  pl_position[]; };
layout(std430, binding = 2) readonly buffer Range    { float pl_range[];    };
layout(std430, binding = 3) writeonly buffer Index   { int   pl_index[];    };

layout(location = 0) uniform uint n_lights;
layout(binding = 0) uniform sampler2D depth_texture;

// shared local storage within the current tile (local work group)
shared uint min_depth;
shared uint max_depth;
shared uint n_visible_lights;
shared vec4 frustum_planes[6];
shared int local_indices[28];  // indices of visible lights in the current tile

void main() {
    // step 0: initialize shared local data in the first local invocation (first thread)
    if (gl_LocalInvocationIndex == 0) {
        min_depth = 0xFFFFFFFF;  // max value of unsigned int
        max_depth = 0;
        n_visible_lights = 0;
    }

    barrier();  // synchronize shared data among all local threads within the work group

    float near = rdr_in.near_clip;
    float far = rdr_in.far_clip;

    // step 1: find the min/max depth value for the current tile (current work group)
    vec2 uv = vec2(gl_GlobalInvocationID.xy) / rdr_in.resolution;
    float depth = texture(depth_texture, uv).r;
    depth = depth * 2.0 - 1.0;  // convert to clip space
    depth = (2.0 * near * far) / (far + near - depth * (far - near));  // linearize to [near, far]

    // atomically compare depth values across local threads and update the min/max depth in the tile
    uint depth_uint = floatBitsToUint(depth);  // convert to uint so as to use atomic operations
    atomicMin(min_depth, depth_uint);
    atomicMax(max_depth, depth_uint);

    // if the pixel is off-screen, it won't have any visible light, to make this happen, we can set
    // the min depth to "max" value, thus the light-frustum intersection test is guaranteed to fail
    // there's no other way to return early in the compute shader when we have many `barrier` calls
    if (uv.x > 1.0 || uv.y > 1.0) {
        // min_depth = 0xFFFFFFFF;
        min_depth = 0x00000FFF;  // prevent overflow when later converting to floats
    }

    barrier();  // synchronize local threads

    // step 2: one thread should calculate the frustum planes to be used for this tile
    if (gl_LocalInvocationIndex == 0) {  // use the first thread
        // convert the min/max depth in this tile back to float
        float min_depth_float = uintBitsToFloat(min_depth);
        float max_depth_float = uintBitsToFloat(max_depth);

        // the current tile's range in screen space (4 corner points)
        vec2 lower = 2.0 * vec2(gl_WorkGroupID.xy) / vec2(gl_NumWorkGroups.xy);
        vec2 upper = 2.0 * vec2(gl_WorkGroupID.xy + uvec2(1)) / vec2(gl_NumWorkGroups.xy);

        // every plane is defined by a normal vector along with a screen space distance to the eye?
        // frustum_planes[i].xyz;  -> normal vector
        // frustum_planes[i].w;    -> screen space distance

        frustum_planes[0] = vec4(+1.0, +0.0, +0.0, +1.0 - lower.x);    // left
        frustum_planes[1] = vec4(-1.0, +0.0, +0.0, -1.0 + upper.x);    // right
        frustum_planes[2] = vec4(+0.0, +1.0, +0.0, +1.0 - lower.y);    // bottom
        frustum_planes[3] = vec4(+0.0, -1.0, +0.0, -1.0 + upper.y);    // top
        frustum_planes[4] = vec4(+0.0, +0.0, -1.0, -min_depth_float);  // near
        frustum_planes[5] = vec4(+0.0, +0.0, +1.0, +max_depth_float);  // far

        // convert the 4 side planes from screen space to view space
        for (uint i = 0; i < 4; i++) {
            frustum_planes[i] *= (camera.projection * camera.view);
            frustum_planes[i] /= length(frustum_planes[i].xyz);  // normalize
        }

        // convert the near and far plane from screen space to view space
        for (uint i = 4; i < 6; i++) {
            frustum_planes[i] *= camera.view;
            frustum_planes[i] /= length(frustum_planes[i].xyz);  // normalize
        }
    }

    barrier();  // other local threads must wait for the first thread

    // step 3: use all 256 threads for light culling (check if a light intersects the tile)
    // if the scene contains more than 256 lights, additional passes are necessary
    uint n_pass = (n_lights + 255) / 256;

    for (uint i = 0; i < n_pass; i++) {
        uint light_index = i * 256 + gl_LocalInvocationIndex;
        if (light_index >= n_lights) {
            break;
        }

        vec4 position = pl_position[light_index];
        float range = pl_range[light_index];

        // check if the light contributes to this tile (i.e. intersects every frustum plane)
        float signed_distance = 0.0;
        for (uint k = 0; k < 6; ++k) {
            signed_distance = dot(position, frustum_planes[k]) + range;
            if (signed_distance <= 0.0) {
                break;  // no intersection with the frustum
            }
        }

        // this light is visible, add its index to the local indices array
        // atomic operations always return the original value (before change)
        if (signed_distance > 0.0) {
            uint offset = atomicAdd(n_visible_lights, 1);
            local_indices[offset] = int(light_index);
        }
    }

    barrier();

    // step 4: one thread should push the local indices array to the global `Index` SSBO
    uint tile_id = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;  // the i-th tile
    if (gl_LocalInvocationIndex == 0) {
        uint offset = tile_id * n_lights;  // starting index of this tile in the global `Index` SSBO
        for (uint i = 0; i < n_visible_lights; ++i) {
            pl_index[offset + i] = local_indices[i];
        }

        // mark the end of array for this tile with -1, so that readers of SSBO can stop earlier
        if (n_visible_lights != n_lights) {
            pl_index[offset + n_visible_lights] = -1;
        }
    }
}

#endif
