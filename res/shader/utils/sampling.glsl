#ifndef _SAMPLING_H
#define _SAMPLING_H

// reference:
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

#include "extension.glsl"

// the Van der Corput radical inverse sequence
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;  // (1 / 0x100000000)
}

// the Hammersley point set (a low-discrepancy random sequence)
vec2 Hammersley2D(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

// uniformly sample a point (also a direction vector) on the unit sphere
// the probability of a point being sampled is 1 / (4 * PI), i.e. unbiased
vec3 UniformSampleSphere(float u, float v) {
    float phi = v * PI2;
    float cos_theta = 1.0 - 2.0 * u;  // ~ [-1, 1]
    float sin_theta = sqrt(max(0, 1 - cos_theta * cos_theta));
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// uniformly sample a point (also a direction vector) on the unit hemisphere
// the probability of a point being sampled is 1 / (2 * PI), i.e. unbiased
vec3 UniformSampleHemisphere(float u, float v) {
    float phi = v * PI2;
    float cos_theta = 1.0 - u;  // ~ [0, 1]
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// cosine-weighted point sampling on the unit hemisphere
// the probability of a point being sampled is (cosine / PI), i.e. biased by cosine
// this method is favored over uniform sampling for cosine-weighted rendering equations
vec3 CosineSampleHemisphere(float u, float v) {
    float phi = v * PI2;
    float cos_theta = sqrt(1.0 - u);  // bias toward cosine using the `sqrt` function
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// importance sampling with GGX NDF for a given alpha (roughness squared)
// this also operates on the unit hemisphere, and the PDF is D_TRGGX() * cosine
// this function returns the halfway vector H (because NDF is evaluated at H)
vec3 ImportanceSampleGGX(float u, float v, float alpha) {
    float a2 = alpha * alpha;
    float phi = u * PI2;
    float cos_theta = sqrt((1.0 - v) / (1.0 + (a2 - 1.0) * v));  // bias toward cosine and TRGGX NDF
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// uniformly sample a 2D point on a planar circular disk
vec2 UniformSampleDisk(float u, float v) {
    float radius = sqrt(u);
    float theta = v * PI2;
    return vec2(radius * cos(theta), radius * sin(theta));
}

// compute an array of Poisson samples on the unit disk, used for PCSS shadows
void PoissonSampleDisk(float seed, inout vec2 samples[16]) {
    const int n_samples = 16;
    float radius_step = 1.0 / float(n_samples);
    float angle_step = 3.883222077450933;  // PI2 * float(n_rings) / float(n_samples)

    float radius = radius_step;
    float angle = random1D(seed) * PI2;

    for (int i = 0; i < n_samples; ++i) {
        samples[i] = vec2(cos(angle), sin(angle)) * pow(radius, 0.75);  // 0.75 is key
        radius += radius_step;
        angle += angle_step;
    }
}

#endif
