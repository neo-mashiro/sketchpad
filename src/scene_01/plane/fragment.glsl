#version 460

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

out FS_IN {
    layout(location = 0) in vec3 _position;  // in world space
    layout(location = 1) in vec3 _normal;  // in world space
    layout(location = 2) in vec2 _uv;
    layout(location = 3) in vec3 _tangent;
    layout(location = 4) in vec3 _bitangent;
} fs_in;

layout(location = 0) out vec4 color;

uniform sampler2D checkboard;
uniform Material u_material;
uniform Light u_light;
uniform vec3 u_camera_pos;
uniform vec3 u_light_src;  // light source position in world space

void main() {
    // for simplicity, all calculations are done in world space
    vec3 L = normalize(u_light_src - _position);   // light direction vector
    vec3 V = normalize(u_camera_pos - _position);  // view direction vector
    vec3 R = reflect(-L, _normal);                 // reflection vector

    // our plane has textures, so we only need to add the specular component
    float spec = pow(max(dot(V, R), 0.0), u_material.shininess);
    vec3 specular = spec * (u_light.specular * u_material.specular);

    color = texture(checkboard, _uv) + vec4(specular, 1.0);
}
