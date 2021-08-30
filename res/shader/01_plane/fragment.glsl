#version 460

layout(location = 0) in __ {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _bitangent;
};

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

layout(location = 1) uniform float shininess;

layout(binding = 0) uniform sampler2D checkerboard;

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
    vec3 V = normalize(camera.position - _position);  // view direction vector

    // directional light
    vec3 L1 = normalize(-DL[0].direction);  // light direction vector
    vec3 R1 = reflect(-L1, _normal);        // reflection vector

    // point light
    vec3 L2 = normalize(PL[0].position - _position);  // light direction vector
    vec3 R2 = reflect(-L2, _normal);                  // reflection vector

    vec3 diffuse = max(dot(_normal, L1), 0.0) * DLColor(0, _position) +
                   max(dot(_normal, L2), 0.0) * PLColor(0, _position);

    vec3 specular = pow(max(dot(V, R1), 0.0), shininess) * DLColor(0, _position) +
                    pow(max(dot(V, R2), 0.0), shininess) * PLColor(0, _position);

    color = vec4((0.4 + diffuse + specular + SLColor(0, _position)) * (texture(checkerboard, _uv).rgb + 0.01), 1.0);
}
