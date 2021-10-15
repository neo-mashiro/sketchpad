#version 460

// number of local invocations within each work group is specified by the layout
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
// const uvec3 gl_WorkGroupSize;

// the buffer keyword declares a SSBO
layout(std430, binding = 0) buffer Position {
    vec4 pos[];
};

// the binding point value must match the index of the SSBO on the OpenGL side
layout(std430, binding = 1) buffer Velocity {
    vec4 vel[];  // only the last item can have empty brackets, the GLSL compiler will figure out what the size is
};

// or we can also use uniform images (they can be read/write/modified via image load/store functions)
layout(binding = 0, rgba16f) uniform image2D InputImg;
layout(binding = 1, rgba16f) uniform image2D OutputImg;

// shared local data (in the local workgroup)
shared float localData[gl_WorkGroupSize.x + 2][gl_WorkGroupSize.y + 2];  // access like localData[2][3]

void main() {
    uint idx = gl_GlobalInvocationID.x;  // for 1D
    ivec2 idx = ivec2(gl_GlobalInvocationID.xy);  // for 2D

    // access the SSBO
    vec3 p = pos[idx].xyz;
    vec3 v = vel[idx].xyz;

    // update the SSBO (if invocations share data, you have to be careful)
    pos[idx] = vec4(1.0);
    vel[idx] = vec4(1.0);

    // when using `shared` local data, usually we need to call the barrier() function somewhere
    // this is to synchronize reads and writes between invocations within a work group
    // execution within the work group will not proceed until all other invocations have reached this barrier
    barrier();  // sometimes we need this with local shared data to avoid data races

    // once past the barrier(), all shared variables previously written across all invocations in the group will be visible
    // so now we can safely do some other operations that use the previous results
}
