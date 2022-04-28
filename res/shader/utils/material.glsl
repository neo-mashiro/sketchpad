#ifndef _DISNEY_BSDF_H
#define _DISNEY_BSDF_H

/* this is a simplified Disney BSDF material model adapted from Google Filament, for theory
   and details behind physically based rendering, check out the Filament core documentation
   and the SIGGRAPH physically based shading course series since 2012.

   reference:
   - https://google.github.io/filament/Filament.html
   - https://google.github.io/filament/Materials.html
   - https://blog.selfshadow.com/publications/
   - http://www.advances.realtimerendering.com/
   - https://pbr-book.org/3ed-2018/contents
*/

#include "extension.glsl"

/* ------------------------------ Diffuse BRDF model ------------------------------ */

// Disney's diffuse BRDF that takes account of roughness (not energy conserving though)
// Brent Burley 2012, "Physically Based Shading at Disney"
float Fd_Burley(float alpha, float NoV, float NoL, float HoL) {
    float F90 = 0.5 + 2.0 * HoL * HoL * alpha;
    float a = 1.0 + (F90 - 1.0) * pow5(1.0 - NoL);
    float b = 1.0 + (F90 - 1.0) * pow5(1.0 - NoV);
    return a * b * INV_PI;
}

// Lambertian diffuse BRDF that assumes a uniform response over the hemisphere H2, note
// that 1/PI comes from the energy conservation constraint (integrate BRDF over H2 = 1)
float Fd_Lambert() {
    return INV_PI;
}

// Cloth diffuse BRDF approximated using a wrap diffuse term (energy conserving)
// source: Physically Based Rendering in Filament documentation, section 4.12.2
float Fd_Wrap(float NoL, float w) {
    float x = pow2(1.0 + w);
    return clamp((NoL + w) / x, 0.0, 1.0);
}

/* ------------------------- Specular D - Normal Distribution Function ------------------------- */

// Trowbridge-Reitz GGX normal distribution function (long tail distribution)
// Bruce Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
float D_TRGGX(float alpha, float NoH) {
    float a = NoH * alpha;
    float k = alpha / (1.0 - NoH * NoH + a * a);
    return k * k * INV_PI;
}

// Generalized Trowbridge-Reitz NDF when gamma = 1 (tail is even longer)
// Brent Burley 2012, "Physically Based Shading at Disney"
float D_GTR1(float alpha, float NoH) {
    if (alpha >= 1.0) return INV_PI;  // singularity case when gamma = alpha = 1
    float a2 = alpha * alpha;
    float t = 1.0 + (a2 - 1.0) * NoH * NoH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

// GTR2 (equivalent to GGX) anisotropic normal distribution function
// Brent Burley 2012, "Physically Based Shading at Disney"
float D_AnisoGTR2(float at, float ab, float ToH, float BoH, float NoH) {
    float a2 = at * ab;
    vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
    float d2 = dot(d, d);
    float b2 = a2 / d2;
    return a2 * b2 * b2 * INV_PI;
}

// Ashikhmin's inverted Gaussian based velvet distribution, normalized by Neubelt
// Ashikhmin and Premoze 2007, "Distribution-based BRDFs"
// Neubelt 2013, "Crafting a Next-Gen Material Pipeline for The Order: 1886"
float D_Ashikhmin(float alpha, float NoH) {
    float a2 = alpha * alpha;
    float cos2 = NoH * NoH;
    float sin2 = 1.0 - cos2;
    float sin4 = sin2 * sin2;
    float cot2 = -cos2 / (a2 * sin2);
    return 1.0 / (PI * (4.0 * a2 + 1.0) * sin4) * (4.0 * exp(cot2) + sin4);
}

// Charlie distribution function based on an exponentiated sinusoidal
// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
float D_Charlie(float alpha, float NoH) {
    float inv_alpha = 1.0 / alpha;
    float cos2 = NoH * NoH;
    float sin2 = 1.0 - cos2;
    return (2.0 + inv_alpha) * pow(sin2, inv_alpha * 0.5) / PI2;
}

/* ------------------------- Specular G - Geometry Function (shadowing-masking) ------------------------- */

// Smith's geometry function (used for indirect image-based lighting)
// Schlick 1994, "An Inexpensive BRDF Model for Physically-based Rendering"
float G_SmithGGX_IBL(float NoV, float NoL, float alpha) {
    float k = alpha / 2.0;
    float GGXV = NoV / (NoV * (1.0 - k) + k);  // Schlick-GGX from view direction V
    float GGXL = NoL / (NoL * (1.0 - k) + k);  // Schlick-GGX from light direction L
    return GGXV * GGXL;
}

// Smith's geometry function modified for analytic light sources
// Burley 2012, "Physically Based Shading at Disney", Karis 2013, "Real Shading in Unreal Engine 4"
float G_SmithGGX(float NoV, float NoL, float roughness) {
    float r = (roughness + 1.0) * 0.5;  // remap roughness before squaring to reduce hotness
    return G_SmithGGX_IBL(NoV, NoL, r * r);
}

// Smith's height-correlated visibility function (V = G / normalization term)
// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
float V_SmithGGX(float alpha, float NoV, float NoL) {
    float a2 = alpha * alpha;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

// height-correlated GGX distribution anisotropic visibility function
// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
float V_AnisoSmithGGX(float at, float ab, float ToV, float BoV, float ToL, float BoL, float NoV, float NoL) {
    float GGXV = NoL * length(vec3(at * ToV, ab * BoV, NoV));
    float GGXL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
    return 0.5 / (GGXV + GGXL);
}

// Kelemen's visibility function for clear coat specular BRDF
// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
float V_Kelemen(float HoL) {
    return 0.25 / (HoL * HoL);
}

// Neubelt's smooth visibility function for use with cloth and velvet distribution
// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float V_Neubelt(float NoV, float NoL) {
    return 1.0 / (4.0 * (NoL + NoV - NoL * NoV));
}

/* ------------------------- Specular F - Fresnel Reflectance Function ------------------------- */

// Schlick's approximation of specular reflectance (the Fresnel factor)
// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
vec3 F_Schlick(const vec3 F0, float F90, float HoV) {
    return F0 + (F90 - F0) * pow5(1.0 - HoV);  // HoV = HoL
}

// Schlick's approximation when grazing angle reflectance F90 is assumed to be 1
vec3 F_Schlick(const vec3 F0, float HoV) {
    return F0 + (1.0 - F0) * pow5(1.0 - HoV);  // HoV = HoL
}

/* ------------------------- Ambient Occlusion, Misc ------------------------- */

// Ground truth based colored ambient occlussion (colored GTAO)
// Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion"
vec3 MultiBounceGTAO(float visibility, const vec3 albedo) {
    vec3 a =  2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c =  2.7552 * albedo + 0.6903;
    float v = visibility;
    return max(vec3(v), ((v * a + b) * v + c) * v);
}

#endif