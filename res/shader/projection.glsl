#ifndef _PROJECTION_H
#define _PROJECTION_H

#ifndef PI
#define PI 3.141592653589793
#endif

// convert a vector v in tangent space defined by normal N (e.g. hemisphere) to world space
vec3 Tangent2World(vec3 N, vec3 v) {
    vec3 binormal = vec3(0.0, 1.0, 0.0);
    vec3 normal = normalize(N);
    vec3 tangent = normalize(cross(binormal, normal));
    binormal = normalize(cross(normal, tangent));
    return v.x * tangent + v.y * binormal + v.z * normal;
}

// convert a vector v in Cartesian coordinates to spherical coordinates
vec2 Cartesian2Spherical(vec3 v) {
    float phi = atan(v.z, v.x);               // ~ [-PI, PI] (assume v is normalized)
    float theta = acos(v.y);                  // ~ [0, PI]
    return vec2(phi / (2 * PI), theta / PI);  // ~ [-0.5, 0.5], [0, 1]
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

#endif
