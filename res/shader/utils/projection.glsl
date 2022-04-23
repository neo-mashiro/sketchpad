#ifndef _PROJECTION_H
#define _PROJECTION_H

#include "extension.glsl"

/* convert a vector v in tangent space defined by normal N (e.g. hemisphere) to world space

   this is an easy-peezy change-of-frame problem, where texture coordinates ain't involved.
   given a normal vector N, we can find a tangent vector T and a bitangent vector B, such
   that T, B and N together define the orthonormal basis of the tangent space, then what's
   left to do is simply to project them onto the world basis unit vector X, Y and Z.

   there's an infinite number of vectors T and B that we can choose to build an orthonormal
   basis with N, so that implementations can differ, but results will be the same as long
   as the TB plane is tangent to the shading point on the hemisphere. That said, we should
   carefully choose an initial up vector U who does not overlap with N, o/w `cross(U, N)`
   might not be able to calculate the correct T due to precision errors. Also, make sure to
   normalize every vector before use.
*/
vec3 Tangent2World(vec3 N, vec3 v) {
    N = normalize(N);

    // choose the up vector U that does not overlap with N
    vec3 U = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), step(abs(N.y), 0.999));
    vec3 T = normalize(cross(U, N));
    vec3 B = normalize(cross(N, T));
    return T * v.x + B * v.y + N * v.z;  // mat3(T, B, N) * v
}

// convert a vector v in Cartesian coordinates to spherical coordinates
vec2 Cartesian2Spherical(vec3 v) {
    float phi = atan(v.z, v.x);          // ~ [-PI, PI] (assume v is normalized)
    float theta = acos(v.y);             // ~ [0, PI]
    return vec2(phi / PI2, theta / PI);  // ~ [-0.5, 0.5], [0, 1]
}

// remap a spherical vector v into equirectangle texture coordinates
vec2 Spherical2Equirect(vec2 v) {
    return vec2(v.x + 0.5, v.y);  // ~ [0, 1]
}

// convert a vector v in spherical polar angles to Cartesian coordinates
vec3 Spherical2Cartesian(vec2 v) {
    float z = sin(v.y);
    return vec3(z * cos(v.x), z * sin(v.x), cos(v.y));
}

// convert a 2D texture coordinate st on a cubemap face to its equivalent 3D
// texture lookup vector v such that `texture(cubemap, v) == texture(face, st)`
vec3 UV2Cartesian(vec2 st, uint face) {
    vec3 v = vec3(0.0);  // texture lookup vector in world space
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - 1.0;  // convert [0, 1] to [-1, 1] and invert y

    // https://en.wikipedia.org/wiki/Cube_mapping#Memory_addressing
    switch (face) {
        case 0: v = vec3( +1.0,  uv.y, -uv.x); break;  // posx
        case 1: v = vec3( -1.0,  uv.y,  uv.x); break;  // negx
        case 2: v = vec3( uv.x,  +1.0, -uv.y); break;  // posy
        case 3: v = vec3( uv.x,  -1.0,  uv.y); break;  // negy
        case 4: v = vec3( uv.x,  uv.y,  +1.0); break;  // posz
        case 5: v = vec3(-uv.x,  uv.y,  -1.0); break;  // negz
    }

    return normalize(v);
}

// convert an ILS image coordinate w to its equivalent 3D texture lookup
// vector v such that `texture(samplerCube, v) == imageLoad(imageCube, w)`
vec3 ILS2Cartesian(ivec3 w, vec2 resolution) {
    // w often comes from a compute shader in the form of `gl_GlobalInvocationID`
    vec2 st = w.xy / resolution;  // tex coordinates in [0, 1] range
    return UV2Cartesian(st, w.z);
}

// convert a cubemap texture lookup vector v to its ILS equivalent lookup
// vector w such that `texture(samplerCube, v) == imageLoad(imageCube, w)`
ivec3 Cartesian2ILS(vec3 v, vec2 resolution) {
    vec3 size = abs(v);
    v /= max3(size);

    // http://alinloghin.com/articles/compute_ibl.html
    // https://en.wikipedia.org/wiki/Cube_mapping#Memory_addressing

    int face;  // posx = 0, negx = 1, posy = 2, negy = 3, posz = 4, negz = 5
    vec2 uv;   // ~ [-1, 1]

    // x major
    if (size.x > size.y && size.x > size.z) {
        float negx = step(v.x, 0.0);
        face = int(negx);
        uv = mix(vec2(-v.z, v.y), v.zy, negx);  // ~ [-1, 1]
    }
    // y major
    else if (size.y > size.z) {
        float negy = step(v.y, 0.0);
        face = int(negy) + 2;
        uv = mix(vec2(v.x, -v.z), v.xz, negy);  // ~ [-1, 1]
    }
    // z major
    else {
        float negz = step(v.z, 0.0);
        face = int(negz) + 4;
        uv = mix(v.xy, vec2(-v.x, v.y), negz);  // ~ [-1, 1]
    }

    // convert [-1, 1] to [0, 1] and invert y
    vec2 st = (uv + 1.0) * 0.5;
    st = vec2(st.x, 1.0 - st.y);

    // scale by the cubemap resolution and clamp to [0, resolution - 1]
    st = st * resolution;
    st = clamp(st, vec2(0.0), resolution - vec2(1.0));

    return ivec3(ivec2(st), face);
}

#endif
