#version 460

layout(location = 0) in __ {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
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

layout(location = 1) uniform float shininess;

layout(binding = 0) uniform sampler2D diffuse;
layout(binding = 1) uniform sampler2D specular;

layout(location = 0) out vec4 color;

const int uv_repeat = 12;

vec3 DLColor(uint i, vec3 frag_pos) {
    return DL[i].intensity * DL[i].color;
}

vec3 PLColor(uint i, vec3 frag_pos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(PL[i].position, frag_pos);
    float attenuation = d >= PL[i].range ? 0.0 : 1.0 / (1.0 + PL[i].linear * d + PL[i].quadratic * d * d);
    return attenuation * PL[i].intensity * PL[i].color;
}

void main() {
    vec3 V = normalize(camera.position - _position);  // view direction vector

    // directional light
    vec3 L1 = normalize(-DL[0].direction);  // light direction vector
    vec3 R1 = reflect(-L1, _normal);         // reflection vector

    // point light
    vec3 L2 = normalize(PL[0].position - _position);  // light direction vector
    vec3 R2 = reflect(-L2, _normal);                  // reflection vector

    vec3 diff = max(dot(_normal, L1), 0.0) * DLColor(0, _position) +
                max(dot(_normal, L2), 0.0) * PLColor(0, _position);

    vec3 spec = pow(max(dot(V, R1), 0.0), shininess) * DLColor(0, _position) +
                pow(max(dot(V, R2), 0.0), shininess) * PLColor(0, _position);

    color = vec4(0.43 * texture(diffuse, _uv * uv_repeat).rgb +
                 diff * texture(diffuse, _uv * uv_repeat).rgb +
                 spec * texture(specular, _uv * uv_repeat).rgb, 1.0);
}
