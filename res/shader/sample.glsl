#ifndef _SAMPLING_H
#define _SAMPLING_H

#ifndef PI
#define PI 3.141592653589793
#endif

// reference:
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations

// the Van der Corput radical inverse sequence
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;  // (1 / 0x100000000)
}

// the Hammersley point set (a random low-discrepancy sequence)
vec2 Hammersley2D(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

// uniformly sample a point (also a direction vector) on the unit sphere
// the probability of a point being sampled is 1 / (4 * PI), i.e. unbiased
vec3 UniformSampleSphere(float u, float v) {
    float phi = v * 2.0 * PI;
    float cos_theta = 1.0 - 2.0 * u;  // ~ [-1, 1]
    float sin_theta = sqrt(max(0, 1 - cos_theta * cos_theta));
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// uniformly sample a point (also a direction vector) on the unit hemisphere
// the probability of a point being sampled is 1 / (2 * PI), i.e. unbiased
vec3 UniformSampleHemisphere(float u, float v) {
    float phi = v * 2.0 * PI;
    float cos_theta = 1.0 - u;  // ~ [0, 1]
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// cosine-weighted point sampling on the unit hemisphere
// the probability of a point being sampled is (cosine / PI), i.e. biased by cosine
// this method is favored over uniform sampling for our cosine-weighted rendering equation
vec3 CosineSampleHemisphere(float u, float v) {
    float phi = v * 2.0 * PI;
    float cos_theta = sqrt(1.0 - u);  // bias toward cosine using the `sqrt` function
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

#endif
