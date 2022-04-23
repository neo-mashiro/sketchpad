#ifndef _SPHERICAL_HARMONICS_H
#define _SPHERICAL_HARMONICS_H

#ifndef SH_UNIFORM_LOCATION
#define SH_UNIFORM_LOCATION 90
#endif

// SH coefficients are precomputed in C++ and fed to GLSL (INV_PI factor is baked in)
// if sh9's uniform location is not specified, locations 90-98 will be used by default

layout(location = SH_UNIFORM_LOCATION) uniform vec3 sh9[9];

// evaluate 2 bands (4 coefficients)
vec3 EvaluateSH2(const vec3 n) {
    vec3 irradiance
        = sh9[0]
        + sh9[1] * (n.y)
        + sh9[2] * (n.z)
        + sh9[3] * (n.x)
    ;
    return irradiance;
}

// evaluate 3 bands (9 coefficients)
vec3 EvaluateSH3(const vec3 n) {
    vec3 irradiance
        = sh9[0]
        + sh9[1] * (n.y)
        + sh9[2] * (n.z)
        + sh9[3] * (n.x)
        + sh9[4] * (n.y * n.x)
        + sh9[5] * (n.y * n.z)
        + sh9[6] * (3.0 * n.z * n.z - 1.0)
        + sh9[7] * (n.z * n.x)
        + sh9[8] * (n.x * n.x - n.y * n.y)
    ;
    return irradiance;
}

#endif