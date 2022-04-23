#version 460 core

// compute the normal vectors for the vertices on the cloth lattice
// David 2018, "OpenGL 4 Shading Language Cookbook Third Edition, Chapter 11.4"

#ifdef compute_shader

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer InPosition { vec4 position[]; };
layout(std430, binding = 4) writeonly buffer OutNormal { vec4 normal[]; };

void main() {
    uvec3 n_verts = gl_NumWorkGroups * gl_WorkGroupSize;
    uint n_cols = n_verts.x;
    uint n_rows = n_verts.y;

    uint col = gl_GlobalInvocationID.x;
    uint row = gl_GlobalInvocationID.y;

    // count vertices in row-major order, from lower left to upper right
    uint i = n_cols * row + col;
    vec3 p = vec3(position[i]);
    vec3 n = vec3(0.0);
    vec3 a, b, c;

    // each neighbor defines a vector that connects it to the vertex, for every 3 such
    // adjacent vectors a, b and c, the cross products of (a, b) and (b, c) give us 2
    // local normals for the given gradient directions. To calculate the global vertex
    // normal vector, we simply sum up (average over) all of them and normalize.

    if (row < n_rows - 1) {
        c = position[i + n_cols].xyz - p;

        if (col < n_cols - 1) {
            a = position[i + 1].xyz - p;
            b = position[i + n_cols + 1].xyz - p;
            n += cross(a, b);
            n += cross(b, c);
        }

        if (col > 0) {
            a = c;
            b = position[i + n_cols - 1].xyz - p;
            c = position[i - 1].xyz - p;
            n += cross(a, b);
            n += cross(b, c);
        }
    }

    if (row > 0) {
        c = position[i - n_cols].xyz - p;

        if (col > 0) {
            a = position[i - 1].xyz - p;
            b = position[i - n_cols - 1].xyz - p;
            n += cross(a, b);
            n += cross(b, c);
        }

        if (col < n_cols - 1) {
            a = c;
            b = position[i - n_cols + 1].xyz - p;
            c = position[i + 1].xyz - p;
            n += cross(a, b);
            n += cross(b, c);
        }
    }

    normal[i] = vec4(normalize(n), 0.0);
}

#endif