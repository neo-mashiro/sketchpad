layout(std140, binding = 1) uniform DirectionalLight {
    vec3 color;
    vec3 direction;
    float intensity;
} DL;

layout(std140, binding = 2) uniform Spotlight {
    vec3 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float inner_cosine;
    float outer_cosine;
    float range;
} SL;

layout(std140, binding = 3) uniform PointLight {
    vec3 color;
    vec3 position;
    float intensity;
    float linear;
    float quadratic;
    float range;
} PL[17];  // 16 point lights on the border + 1 orbit light

vec3 PointLightRadience(uint i, vec3 fpos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(PL[i].position, fpos);
    float attenuation = d >= PL[i].range ? 0.0 : 1.0 / (1.0 + PL[i].linear * d + PL[i].quadratic * d * d);
    return attenuation * PL[i].intensity * PL[i].color;
}

vec3 SpotlightRadience(vec3 fpos) {
    // the spotlight distance attenuation uses a linear falloff
    vec3 ray = fpos - SL.position;  // inward ray from the light to the fragment
    float projected_distance = dot(SL.direction, ray);
    float linear_attenuation = 1.0 - clamp(projected_distance / SL.range, 0.0, 1.0);

    // the spotlight angular attenuation fades out from the inner to the outer cone
    float cosine = dot(SL.direction, normalize(ray));
    float angular_diff = SL.inner_cosine - SL.outer_cosine;
    float angular_attenuation = clamp((cosine - SL.outer_cosine) / angular_diff, 0.0, 1.0);

    return linear_attenuation * angular_attenuation * SL.intensity * SL.color;
}
