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

layout(std140, binding = 3) uniform PointLight {
    float intensity;
    vec3 color;
    vec3 position;
    float linear;
    float quadratic;
    float range;
} point_light[4];

layout(location = 1) uniform vec3 specular;
layout(location = 2) uniform float shininess;

layout(binding = 0) uniform sampler2D checkerboard;

layout(location = 0) out vec4 color;

float PointLightAttenuation(uint i, vec3 frag_pos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(point_light[i].position, frag_pos);
    return d >= point_light[i].range ? 0.0 :
        1.0 / (1.0 + point_light[i].linear * d + point_light[i].quadratic * pow(d, 2.0));
}

void main() {
    vec3 L = normalize(point_light[0].position - _position);  // light direction vector
    vec3 V = normalize(camera.position - _position);          // view direction vector
    vec3 R = reflect(-L, _normal);                            // reflection vector

    vec3 C = point_light[0].color;
    float I = point_light[0].intensity * PointLightAttenuation(0, _position);

    vec3 specular = pow(max(dot(V, R), 0.0), shininess) * texture(checkerboard, _uv).rgb;
    color = vec4(I * C * specular, 1.0);
}
