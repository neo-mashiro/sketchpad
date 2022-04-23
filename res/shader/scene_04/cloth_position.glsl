#version 460 core

// simple cloth simulation on a 2D particle-spring lattice
// David 2018, "OpenGL 4 Shading Language Cookbook Third Edition, Chapter 11.4"

#ifdef compute_shader

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer InPosition { vec4 p_in[]; };
layout(std430, binding = 2) readonly buffer InVelocity { vec4 v_in[]; };
layout(std430, binding = 1) writeonly buffer OutPosition { vec4 p_out[]; };
layout(std430, binding = 3) writeonly buffer OutVelocity { vec4 v_out[]; };

layout(location = 0) uniform vec3 gravity = vec3(0.0, -10, 0.0);
layout(location = 1) uniform vec3 wind = vec3(0.0);
layout(location = 2) uniform float rh;  // spring length at rest (horizontal)
layout(location = 3) uniform float rv;  // spring length at rest (vertical)
layout(location = 4) uniform float rd;  // spring length at rest (diagonal)

const float dt  = 5e-6;    // time step (need to be small as simulation is sensitive to noise)
const float dt2 = 25e-12;  // time step squared
const float K   = 2000.0;  // spring stiffness
const float dp  = 0.1;     // damping factor (simulate air resistance)
const float m   = 0.1;     // vertex particle mass
const float im  = 10;      // one over mass, 1 / m

shared vec3 share_pos[1024];  // 32 x 32

void main() {
    uvec3 n_verts = gl_NumWorkGroups * gl_WorkGroupSize;
    uint n_cols = n_verts.x;
    uint n_rows = n_verts.y;

    uint col = gl_GlobalInvocationID.x;
    uint row = gl_GlobalInvocationID.y;

    // count vertices in row-major order, from lower left to upper right
    uint i = n_cols * row + col;
    vec3 p = p_in[i].xyz;
    vec3 v = v_in[i].xyz;

    share_pos[i] = p;  // cache to shared local storage so as to speed up subsequent reads
    barrier();  // make sure all local threads within the work group have completed cache

    // start with gravity and wind force, then add spring forces from neighboring vertices
    vec3 force = (gravity + wind) * m;
    vec3 r = vec3(0.0);  // vector from the vertex to its neighbor

    if (row < n_rows - 1) {  // above
        r = share_pos[i + n_cols].xyz - p;
        force += (K * (length(r) - rv)) * normalize(r);
    }
    if (row > 0) {  // below
        r = share_pos[i - n_cols].xyz - p;
        force += (K * (length(r) - rv)) * normalize(r);
    }
    if (col > 0) {  // left
        r = share_pos[i - 1].xyz - p;
        force += (K * (length(r) - rh)) * normalize(r);
    }
    if (col < n_cols - 1) {  // right
        r = share_pos[i + 1].xyz - p;
        force += (K * (length(r) - rh)) * normalize(r);
    }
    if (col > 0 && row < n_rows - 1) {  // upper-left
        r = share_pos[i + n_cols - 1].xyz - p;
        force += (K * (length(r) - rd)) * normalize(r);
    }
    if (col < n_cols - 1 && row < n_rows - 1) {  // upper-right
        r = share_pos[i + n_cols + 1].xyz - p;
        force += (K * (length(r) - rd)) * normalize(r);
    }
    if (col > 0 && row > 0) {  // lower-left
        r = share_pos[i - n_cols - 1].xyz - p;
        force += (K * (length(r) - rd)) * normalize(r);
    }
    if (col < n_cols - 1 && row > 0) {  // lower-right
        r = share_pos[i - n_cols + 1].xyz - p;
        force += (K * (length(r) - rd)) * normalize(r);
    }

    force -= dp * v;

    // apply Newton's equations of motion (integrate using Euler's method)
    vec3 a = force * im;  // f = m * a
    p_out[i] = vec4(p + v * dt + 0.5 * a * dt2, 1.0);
    v_out[i] = vec4(v + a * dt, 0.0);

    // vertices at the 4 corners are pinned (anchor vertices)
    if ((row == 0 || row == n_rows - 1) && (col == 0 || col == n_cols - 1)) {
        p_out[i] = vec4(p, 1.0);
        v_out[i] = vec4(0.0);
    }
}

#endif