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

layout(location = 0) out vec4 color;

in vec3 _position;  // in world space
in vec3 _normal;    // in world space

uniform Material u_material;
uniform Light u_light;
uniform vec3 u_camera_pos;
uniform vec3 u_light_src;  // light source position in world space

void main() {
    // for simplicity, all calculations are done in world space
    vec3 L = normalize(u_light_src - _position);   // light direction vector
    vec3 V = normalize(u_camera_pos - _position);  // view direction vector
    vec3 R = reflect(-L, _normal);                 // reflection vector

    // ambient
    vec3 ambient = u_light.ambient * u_material.ambient;

    // diffuse
    float diff = max(dot(_normal, L), 0.0);
    vec3 diffuse = diff * (u_light.diffuse * u_material.diffuse);

    // specular
    float spec = pow(max(dot(V, R), 0.0), u_material.shininess);
    vec3 specular = spec * (u_light.specular * u_material.specular);

    color = vec4(ambient + diffuse + specular, 1.0);
}
