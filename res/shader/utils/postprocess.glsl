#ifndef _POSTPROCESS_H
#define _POSTPROCESS_H

#include "extension.glsl"

/* commonly used tone mapping operators (TMO) */
// https://64.github.io/tonemapping/
// https://docs.unrealengine.com/4.26/en-US/RenderingAndGraphics/PostProcessEffects/ColorGrading/

vec3 Reinhard(vec3 radiance) {
    return radiance / (1.0 + radiance);
}

vec3 ReinhardLuminance(vec3 radiance, float max_luminance) {
    float li = luminance(radiance);
    float numerator = li * (1.0 + (li / (max_luminance * max_luminance)));
    float lo = numerator / (1.0 + li);
    return radiance * (lo / li);
}

vec3 ReinhardJodie(vec3 radiance) {
    vec3 t = radiance / (1.0 + radiance);
    vec3 x = radiance / (1.0 + luminance(radiance));
    return vec3(mix(x.r, t.r, t.r), mix(x.g, t.g, t.g), mix(x.b, t.b, t.b));
}

vec3 Uncharted2Partial(vec3 x) {
    float a = 0.15;
    float b = 0.50;
    float c = 0.10;
    float d = 0.20;
    float e = 0.02;
    float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e/f;
}

vec3 Uncharted2Filmic(vec3 radiance) {
    float exposure_bias = 2.0;
    vec3 white_scale = vec3(1.0) / Uncharted2Partial(vec3(11.2));
    vec3 c = Uncharted2Partial(radiance * exposure_bias);
    return c * white_scale;
}

vec3 ApproxACES(vec3 radiance) {
    vec3 v = radiance * 0.6;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

/* simple gamma correction, use an approximated power of 2.2 */

vec3 Gamma2Linear(vec3 color) {
    return pow(color, vec3(2.2));  // component-wise
}

vec3 Linear2Gamma(vec3 color) {
    return pow(color, vec3(1.0 / 2.2));  // component-wise
}

float Gamma2Linear(float grayscale) {
    return pow(grayscale, 2.2);
}

float Linear2Gamma(float grayscale) {
    return pow(grayscale, 1.0 / 2.2);
}

#endif