#version 460

struct Light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(location = 0) out vec4 color;

in vec3 _position;  // in world space
in vec3 _normal;    // in world space
in vec2 _uv;

uniform Light u_light;
uniform vec3 u_camera_pos;
uniform vec3 u_light_src;  // light source position in world space

layout(binding = 0) uniform sampler2D diffuse_map;
layout(binding = 1) uniform sampler2D specular_map;
layout(binding = 2) uniform sampler2D emission_map;
uniform float u_shininess;

const int uv_repeat = 10;

void main() {
    // for simplicity, all calculations are done in world space
    vec3 L = normalize(u_light_src - _position);   // light direction vector
    vec3 V = normalize(u_camera_pos - _position);  // view direction vector
    vec3 R = reflect(-L, _normal);                 // reflection vector

    // ambient
    vec3 ambient = u_light.ambient * texture(diffuse_map, _uv * uv_repeat).rgb;

    // diffuse
    float diff = max(dot(_normal, L), 0.0);
    vec3 diffuse = diff * (u_light.diffuse * texture(diffuse_map, _uv * uv_repeat).rgb);

    // specular
    float spec = pow(max(dot(V, R), 0.0), u_shininess);
    vec3 specular = spec * (u_light.specular * texture(specular_map, _uv * uv_repeat).rgb);

    // emission
    vec3 emission = texture(emission_map, _uv * 0).rgb;

    color = vec4(ambient + diffuse + specular + emission, 1.0);
}
