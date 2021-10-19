#version 460

// this compute shader is used by the light culling pass in forward+ rendering
// references:
// --- https://github.com/bcrusco/Forward-Plus-Renderer
// --- https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf
// --- https://www.3dgep.com/forward-plus/

// currently we only support light culling on point lights but it's more than enough.
// it is rare that users have thousands of directional and area lights or spotlights.

////////////////////////////////////////////////////////////////////////////////

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 projection;
} camera;

#ifdef compute_shader

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer Color {
	vec4 data[];
} light_colors;

layout(std430, binding = 1) readonly buffer Position {
	vec4 data[];
} light_positions;

layout(std430, binding = 2) readonly buffer Range {
	float data[];
} light_ranges;

layout(std430, binding = 3) writeonly buffer Index {
	int data[];
} light_indices;

layout(location = 0) uniform uint n_lights;
layout(binding = 0) uniform sampler2D depth_texture;

// constants
const ivec2 window_size = ivec2(1600, 900);
const float near = 0.1;
const float far = 100.0;

// shared local data within the current tile (work group)
shared uint min_depth;
shared uint max_depth;
shared uint n_visible_lights;
shared vec4 frustum_planes[6];
shared int local_indices[28];  // indices of visible lights in the current tile

void main() {
    // step 0: initialize shared data in the first local invocation (first thread)
	if (gl_LocalInvocationIndex == 0) {
		min_depth = 0xFFFFFFFF;  // max value of unsigned int
		max_depth = 0;
		n_visible_lights = 0;
	}

	barrier();  // synchronize shared data among all local threads within the work group

	// step 1: find the min/max depth value for the current tile (current work group)
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / window_size;
    float depth = texture(depth_texture, uv).r;
    depth = depth * 2.0 - 1.0;  // convert to the clip space
    depth = (2.0 * near * far) / (far + near - depth * (far - near));  // linearize to [near, far]

	// atomically compare depth values across local threads and update the min/max depth in the tile
	uint depth_uint = floatBitsToUint(depth);  // convert to uint so as to use atomic operations
	atomicMin(min_depth, depth_uint);
	atomicMax(max_depth, depth_uint);

    // if the pixel is off-screen, set the min depth to "max" value, thus it is guaranteed that the
    // light-frustum intersection test would fail so this off-screen pixel has no visible lights.
    // there's no other way to return early in the compute shader when you have many `barrier` calls
    if (uv.x > 1.0 || uv.y > 1.0) {
        // min_depth = 0xFFFFFFFF;
        min_depth = 0x00000FFF;  // prevent overflow when later converting to floats
    }

	barrier();  // synchronize local threads

	// step 2: one thread should calculate the frustum planes to be used for this tile
	if (gl_LocalInvocationIndex == 0) {  // the first thread
		// convert the min/max depth in this tile back to float
		float min_depth_float = uintBitsToFloat(min_depth);
		float max_depth_float = uintBitsToFloat(max_depth);

        // the current tile's range in screen space (4 corner points)
		vec2 lower = 2.0 * vec2(gl_WorkGroupID.xy) / vec2(gl_NumWorkGroups.xy);
		vec2 upper = 2.0 * vec2(gl_WorkGroupID.xy + uvec2(1)) / vec2(gl_NumWorkGroups.xy);

        // every plane is defined by a normal vector along with a screen space distance to the eye?
        //     frustum_planes[i].xyz;  -> normal vector
        //     frustum_planes[i].w;    -> screen space distance

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

		vec4 position = light_positions.data[light_index];
		float range = light_ranges.data[light_index];

		// check if the light contributes to this tile (intersects every frustum plane)
		float signed_distance = 0.0;
		for (uint k = 0; k < 6; k++) {
			signed_distance = dot(position, frustum_planes[k]) + range;
			if (signed_distance <= 0.0) {
				break;  // no intersection with the frustum
			}
		}

        // this light is visible, add its index to the local light indices array
        // recall that atomic operations always return the original value (before change)
		if (signed_distance > 0.0) {
			uint offset = atomicAdd(n_visible_lights, 1);
			local_indices[offset] = int(light_index);
		}
	}

	barrier();

	// step 4: one thread should push the local light indices array to the global SSBO buffer
    uint tile_id = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;  // the i-th tile
	if (gl_LocalInvocationIndex == 0) {
		uint offset = tile_id * n_lights;  // starting index of this tile in the global SSBO buffer
		for (uint i = 0; i < n_visible_lights; i++) {
			light_indices.data[offset + i] = local_indices[i];
		}

		if (n_visible_lights != n_lights) {
			// mark the end of array for this tile with -1, so that readers of SSBO can stop earlier
			light_indices.data[offset + n_visible_lights] = -1;
		}
	}
}

#endif
