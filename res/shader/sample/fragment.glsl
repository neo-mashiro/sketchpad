#version 460

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
    float intensity;
    vec3 color;
    vec3 direction;
} direction_light[4];

layout(std140, binding = 5) uniform PointLight {
    float intensity;
    vec3 color;
    vec3 position;
    float linear;
    float quadratic;
    float range;
} point_light[4];

layout(std140, binding = 9) uniform SpotLight {
    float intensity;
    vec3 color;
    vec3 position;
    vec3 direction;
    float inner_cosine;
    float outer_cosine;
    float range;
} spot_light[4];

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

// returns the attenuation of a point light on the given fragment
float PointLightAttenuation(uint i, vec3 frag_pos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(point_light[i].position, frag_pos);
    return d >= point_light[i].range ? 0.0 :
        1.0 / (1.0 + point_light[i].linear * d + point_light[i].quadratic * pow(d, 2.0));
}

// returns the attenuation of a spotlight on the given fragment
float SpotlightAttenuation(uint i, vec3 frag_pos) {
    // the spotlight linear attenuation uses a linear falloff
    vec3 ray = frag_pos - spot_light[i].position;  // inward ray from the light to the fragment
    float projected_distance = dot(spot_light[i].direction, ray);
    float linear_attenuation = 1.0 - clamp(projected_distance / spot_light[i].range, 0.0, 1.0);

    // the spotlight angular attenuation fades out from the inner to the outer cone
    float cosine = dot(spot_light[i].direction, normalize(ray));
    float angular_diff = spot_light[i].inner_cosine - spot_light[i].outer_cosine;
    float angular_attenuation = clamp((cosine - spot_light[i].outer_cosine) / angular_diff, 0.0, 1.0);

    return linear_attenuation * angular_attenuation;
}

void main() {
    vec3 L = normalize(point_light[0].position - _position);  // light direction vector
    vec3 V = normalize(camera.position - _position);          // view direction vector
    vec3 R = reflect(-L, _normal);                            // reflection vector

    color = vec4(point_light[0].color * (albedo + texture(normal_map, _uv).rgb), 1.0);
}
