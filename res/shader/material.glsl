#ifndef _BRDF_H
#define _BRDF_H

// reference:
// https://sites.cs.ucsb.edu/~lingqi/teaching/resources/GAMES101_Lecture_17.pdf
// https://sites.cs.ucsb.edu/~lingqi/teaching/resources/GAMES202_Lecture_10.pdf
// https://learnopengl.com/PBR/Theory
// https://google.github.io/filament/Filament.html#materialsystem

#ifndef PI
#define PI 3.14159265359
#endif

#ifndef EPS
#define EPS 0.000001
#endif

// Trowbridge-Reitz GGX normal distribution function (long tail distribution)
// Trowbridge-Reitz GGX NDF is favored over Beckmann NDF because it's much smoother
float D_TRGGX(float NoH, float alpha) {
    float a2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    return a2 / (PI * pow(NoH2 * (a2 - 1.0) + 1.0, 2.0));
}

// Trowbridge-Reitz GGX implementation in Filament
float D_TRGGX2(float NoH, float alpha) {
    float a = NoH * alpha;
    float k = alpha / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

// Generalized Trowbridge-Reitz (GTR) NDF (tail length can be tuned by parameter `gamma`)
// this is an extended GGX NDF proposed by Brent Burley from Walt Disney Animation Studios
float D_GTR(float NoH, float alpha, float gamma) {
    return 0.0;  // TODO
}

// Fresnel-Schlick approximation of specular reflectance
vec3 F_Schlick(float HoV, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - HoV, 5.0);
}

// no halfway vector V.....
vec3 F_SchlickRoughness(float NoV, vec3 F0, float alpha) {
    return F0 + (max(vec3(1.0 - alpha), F0) - F0) * pow(1.0 - NoV, 5.0);
}

// Smith's geometry function (the shadowing-masking term)
float G_SmithGGX(float NoV, float NoL, float alpha) {
    float k = pow(alpha + 1.0, 2.0) / 8.0;  // for direct lighting
    // float k = alpha * alpha / 2.0;  // for indirect lighting
    float GGXV = NoV / (NoV * (1.0 - k) + k);  // Schlick-GGX
    float GGXL = NoL / (NoL * (1.0 - k) + k);  // Schlick-GGX
    return GGXV * GGXL;
}

// height-correlated Smith's visibility function (V = G / normalization term)
// when using V instead of G in BRDF, we don't need to divide by (4 * NoV * NoL)
float V_SmithGGX(float NoV, float NoL, float alpha) {
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - alpha) + alpha);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - alpha) + alpha);
    return 0.5 / (GGXV + GGXL);
}

vec3 StandardBRDF(vec3 F0, vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float HoV = max(dot(H, V), 0.0);

    // roughness remapping and clamping
    float alpha = clamp(roughness * roughness, 0.045, 1.0);

    vec3 F = F_Schlick(HoV, F0);  // you can also use `HoL` for the Fresnel effect

    // float D = D_TRGGX(NoH, alpha);
    float D = D_TRGGX2(NoH, alpha);
    // float D = D_GTR(NoH, alpha, 10.0);  // bell shape curve close to Beckmann
    // float D = D_GTR(NoH, alpha, 2.0);   // equivalent to Trowbridge-Reitz GGX
    // float D = D_GTR(NoH, alpha, 1.0);   // very smooth with even longer tails

    // float G = G_SmithGGX(NoV, NoL, alpha);  // geometric shadowing masking term
    float V = V_SmithGGX(NoV, NoL, alpha);  // geometric visibility term

    // vec3 specular = (D * G * F) / (4 * NoV * NoL + EPS);
    vec3 specular = (D * V) * F;

    // Lambertian diffuse BRDF
    float diffuse = 1.0 / PI;

    // Lambertian diffuse BRDF + Cook-Torrance specular BRDF

    vec3 diffuse_color = (1.0 - metallic) * albedo;

    vec3 specular_color = albedo;

    return diffuse * diffuse_color + specular * specular_color;
}

{

    return (1.0 - mix(vec3(0.04), albedo, metallic)) * diffuse_color * diffuse + specular;

    metallic = 0: (1 - vec3(0.04)) * albedo * diffuse + specular;
    metallic = 1: specular;

    return diffuse * diffuse_color + specular * albedo;

    metallic = 0: diffuse * albedo + specular * albedo;
    metallic = 1: specular * albedo;
}

//

#endif
