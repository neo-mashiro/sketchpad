#version 460 core

// Takahiro Harada et al. 2015, Forward+: Bringing Deferred Lighting to the Next Level
// reference:
// https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf
// https://www.3dgep.com/forward-plus/
// https://github.com/bcrusco/Forward-Plus-Renderer
// https://wickedengine.net/2018/01/10/optimizing-tile-based-light-culling/

layout(std140, binding = 0) uniform Camera {
    vec4 position;
    vec4 direction;
    mat4 view;
    mat4 projection;
} camera;

#include "../core/renderer_input.glsl"

////////////////////////////////////////////////////////////////////////////////

#ifdef compute_shader

#define TILE_SIZE 16
#define MAX_LIGHT 1024

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer Color    { vec4  pl_color[];    };
layout(std430, binding = 1) readonly buffer Position { vec4  pl_position[]; };
layout(std430, binding = 2) readonly buffer Range    { float pl_range[];    };
layout(std430, binding = 3) writeonly buffer Index   { int   pl_index[];    };

layout(location = 0) uniform uint n_lights;
layout(location = 1) uniform mat4 inverse_projection;
layout(binding = 0) uniform sampler2D depth_texture;

// shared local storage within the current tile (local work group)
shared uint min_depth;
shared uint max_depth;
shared uint n_visible_lights;
shared vec4 tile_corner[4];
shared vec4 tile_frustum[4];
shared int local_indices[MAX_LIGHT];  // indices of visible lights in the current tile

// linearize a depth in screen space to the [near, far] range in view space
float LinearizeDepth(float depth) {
    float near = rdr_in.near_clip;
    float far = rdr_in.far_clip;
    float ndc_depth = depth * 2.0 - 1.0;  // convert back to NDC space
    return (2.0 * near * far) / (far + near - ndc_depth * (far - near));
}

// convert a vertex in NDC space back to view/camera space
vec4 NDC2View(const vec4 v) {
    vec4 u = inverse_projection * v;
    u /= u.w;
    return u;
}

void main() {
    // step 0: initialize shared local data using the first thread
    if (gl_LocalInvocationIndex == 0) {
        min_depth = 0xFFFFFFFF;  // max value of unsigned int
        max_depth = 0;
        n_visible_lights = 0;
    }

    barrier();  // synchronize shared data among all local threads within the work group

    // step 1: find the min/max depth value for the current tile (current work group)
    vec2 uv = vec2(gl_GlobalInvocationID.xy) / rdr_in.resolution;
    float depth = LinearizeDepth(texture(depth_texture, uv).r);

    // atomically compare depth values across local threads and update the min/max depth in the tile
    uint depth_uint = floatBitsToUint(depth);  // convert to uint so as to use atomic operations
    atomicMin(min_depth, depth_uint);
    atomicMax(max_depth, depth_uint);

    // corner case: if the pixel is off-screen, we can set the min depth to max value to cut it off
    // this will bring the frustum to infinitely far away so the intersection is guaranteed to fail
    if (uv.x > 1.0 || uv.y > 1.0) {
        min_depth = 0x00000FFF;  // 0xFFFFFFFF will cause overflow when later converting to floats
    }

    barrier();  // synchronize local threads

    // convert min/max depth values back to float
    float min_depth_float = uintBitsToFloat(min_depth);
    float max_depth_float = uintBitsToFloat(max_depth);

    /* step 2: calculate the frustum planes of this tile using the first thread

       the view frustum has 1 near clip plane, 1 far clip plane + 4 tilted planes on the sides
       for the purpose of testing light-plane intersection, we only need to compute the 4 side
       planes (as for the near/far clip plane intersection, we only need to compare the depth)
       mathematically, every plane is uniquely defined by a surface normal and its distance to
       the origin, luckily for us, every side plane already passes through the camera position
       so the surface normal is all we need. To compute this normal for a side plane, first we
       need to find the 2 bottom vertices v1 and v2. Since the camera is at the origin (0,0,0)
       of the view space, v1 and v2 are also the edge vectors of the plane, thus the normal is
       as trivial as `cross(v1, v2)`.

       note that this is the easiest solution for testing sphere-frustum intersection, in some
       corner cases though, lights might not be properly culled as we are assuming infinitely
       large frustum planes. A more optimized solution is to use AABB bounding box for testing
       intersection: https://wickedengine.net/2018/01/10/optimizing-tile-based-light-culling/
    */

    if (gl_LocalInvocationIndex == 0) {
        // find the 4 corners of the tile in screen space
        vec2 lower = vec2(gl_WorkGroupID.xy) / vec2(gl_NumWorkGroups.xy);
        vec2 upper = vec2(gl_WorkGroupID.xy + uvec2(1)) / vec2(gl_NumWorkGroups.xy);

        /* convert back to NDC space and assign in this order
           3------2
           | tile |
           0------1
        */
        lower = lower * 2.0 - 1.0;
        upper = upper * 2.0 - 1.0;

        tile_corner[0] = vec4(lower, 1.0, 1.0);
        tile_corner[1] = vec4(upper.x, lower.y, 1.0, 1.0);
        tile_corner[2] = vec4(upper, 1.0, 1.0);
        tile_corner[3] = vec4(lower.x, upper.y, 1.0, 1.0);

        // convert back to camera/view space
        tile_corner[0] = NDC2View(tile_corner[0]);
        tile_corner[1] = NDC2View(tile_corner[1]);
        tile_corner[2] = NDC2View(tile_corner[2]);
        tile_corner[3] = NDC2View(tile_corner[3]);

        // build the side planes of the frustum, plane.xyz = surface normal, plane.w = distance to origin = 0
        tile_frustum[0] = vec4(normalize(cross(tile_corner[0].xyz, tile_corner[1].xyz)), 0.0);  // down
        tile_frustum[1] = vec4(normalize(cross(tile_corner[1].xyz, tile_corner[2].xyz)), 0.0);  // right
        tile_frustum[2] = vec4(normalize(cross(tile_corner[2].xyz, tile_corner[3].xyz)), 0.0);  // up
        tile_frustum[3] = vec4(normalize(cross(tile_corner[3].xyz, tile_corner[0].xyz)), 0.0);  // left
    }

    barrier();  // join threads again

    // step 3: use all 256 threads for light culling (light-plane or sphere-plane intersection check)
    // if the scene contains more than 256 lights, additional passes are necessary
    uint n_pass = (n_lights + 255) / 256;

    for (uint i = 0; i < n_pass; i++) {
        uint light_index = i * 256 + gl_LocalInvocationIndex;
        if (light_index >= n_lights) {
            break;
        }

        float radius = pl_range[light_index];  // the effective range of the light is a sphere
        vec4 position = camera.view * pl_position[light_index];  // light position in view space

        // if the light sphere is fully beind the far plane or in front of the near plane, it won't
        // intersect the frustum so we can early out, note that we are using a right-handed system
        // where the forward vector is -z, so we need to negate the z component to get the depth
        float light_depth = -position.z;
        if ((light_depth + radius < min_depth_float) || (light_depth - radius > max_depth_float)) {
            continue;
        }

        // compute the *signed* distance from the light to every side plane of the frustum
        float d0 = dot(position.xyz, tile_frustum[0].xyz);
        float d1 = dot(position.xyz, tile_frustum[1].xyz);
        float d2 = dot(position.xyz, tile_frustum[2].xyz);
        float d3 = dot(position.xyz, tile_frustum[3].xyz);

        // if the light sphere "intersects" every plane of the frustum, increment the visible lights
        // count and add its index to the shared local indices array, note that the distance can be
        // negative, in which case the light is inside the frustum so the test should still pass
        if ((d0 <= radius) && (d1 <= radius) && (d2 <= radius) && (d3 <= radius)) {
            uint offset = atomicAdd(n_visible_lights, 1);  // atomic operation returns the value before change
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
