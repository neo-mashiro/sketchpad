// Trowbridge-Reitz GGX normal distribution function
float TRGGX(vec3 N, vec3 H, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = pow(max(dot(N, H), 0.0), 2.0);
    return alpha2 / (pi * pow(NoH2 * (alpha2 - 1.0) + 1.0, 2.0));
}

// Schlick-GGX geometry function
float SchlickGGX(float NoV, float roughness) {
    float alpha = roughness * roughness;
    float k = pow(alpha + 1.0, 2.0) / 8.0;  // for direct lighting
    // float k = alpha * alpha / 2.0;  // for indirect lighting
    return NoV / (NoV * (1.0 - k) + k);
}

// Smith's geometry function
float Smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    return SchlickGGX(NoV, roughness) * SchlickGGX(NoL, roughness);
}

// Fresnel-Schlick approximation
vec3 FresnelSchlick(vec3 H, vec3 V, vec3 F0) {
    float HoV = clamp(dot(H, V), 0.0, 1.0);
    return F0 + (1.0 - F0) * pow(1.0 - HoV, 5.0);
}

// Cook-Torrance BRDF
vec3 CookTorranceBRDF(vec3 F0, vec3 N, vec3 V, vec3 L) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);

    vec3 F = FresnelSchlick(H, V, F0);
    float G = Smith(N, V, L, roughness);
    float D = TRGGX(N, H, roughness);

    vec3 diffuse = albedo / pi;
    vec3 specular = (D * G * F) / (4 * NoV * NoL + 0.00001);

    vec3 ks = F;
    vec3 kd = (vec3(1.0) - ks) * (1.0 - metalness);

    return kd * diffuse + specular;
}
