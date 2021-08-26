#version 460
#pragma optimize(off)

// vertex attributes in global world space
layout(location = 0) in __ {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec3 _tangent;
    in vec3 _bitangent;
};

// uniform blocks (globally shared data)
layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;

layout(std140, binding = 1) uniform DirectLight {
    vec3 color;
    vec3 direction;
    float intensity;
} DL[2];

layout(std140, binding = 3) uniform PointLight {
    vec3 color;
    vec3 position;
    float intensity;
    float linear;
    float quadratic;
    float range;
} PL[4];

layout(std140, binding = 7) uniform SpotLight {
    vec3 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float inner_cosine;
    float outer_cosine;
    float range;
} SL[4];

layout(std140, binding = 11) uniform Config {
    vec3 v1; int i1;
    vec3 v2; int i2;
    vec3 v3; int i3;
    int i4; int i5; int i6;
    float f1; float f2; float f3;
};

// loose uniforms (locally scoped data)
layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform vec3 albedo;
layout(location = 2) uniform vec3 metallic;
layout(location = 3) uniform vec3 smoothness;

// uniform samplers
layout(binding = 0) uniform sampler2D albedo_map;
layout(binding = 1) uniform sampler2D normal_map;
layout(binding = 2) uniform sampler2D height_map;
layout(binding = 3) uniform sampler2D emission_map;

// fragment shader outputs
layout(location = 0) out vec4 color;

vec3 DLColor(uint i, vec3 frag_pos) {
    return DL[i].intensity * DL[i].color;
}

vec3 PLColor(uint i, vec3 frag_pos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(PL[i].position, frag_pos);
    float attenuation = d >= PL[i].range ? 0.0 : 1.0 / (1.0 + PL[i].linear * d + PL[i].quadratic * d * d);
    return attenuation * PL[i].intensity * PL[i].color;
}

vec3 SLColor(uint i, vec3 frag_pos) {
    // the spotlight distance attenuation uses a linear falloff
    vec3 ray = frag_pos - SL[i].position;  // inward ray from the light to the fragment
    float projected_distance = dot(SL[i].direction, ray);
    float linear_attenuation = 1.0 - clamp(projected_distance / SL[i].range, 0.0, 1.0);

    // the spotlight angular attenuation fades out from the inner to the outer cone
    float cosine = dot(SL[i].direction, normalize(ray));
    float angular_diff = SL[i].inner_cosine - SL[i].outer_cosine;
    float angular_attenuation = clamp((cosine - SL[i].outer_cosine) / angular_diff, 0.0, 1.0);

    return linear_attenuation * angular_attenuation * SL[i].intensity * SL[i].color;
}

void main() {
    // use all the uniform blocks to prevent them from being optimized out
    vec3 dummy0 = camera.position;
    vec3 dummy1 = PL[0].color;
    vec3 dummy2 = DL[0].color;
    vec3 dummy3 = SL[0].color;
    vec3 dummy4 = v1;

    color = vec4(1.0);
}
