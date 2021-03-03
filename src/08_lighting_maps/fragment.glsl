#version 440

struct Material {
    vec3 ambient;     // ambient color
    vec3 diffuse;     // diffuse color
    vec3 specular;    // specular color
    float shininess;
}; 

struct Light {
    vec3 source;      // light source position in world space
    vec3 ambient;     // ambient intensity
    vec3 diffuse;     // diffuse intensity
    vec3 specular;    // specular intensity
};

layout(location = 0) out vec4 o_color;

in vec3 s_position;  // in world space
in vec3 s_normal;    // in world space

uniform Material material;
uniform Light light;
uniform vec3 camera_position;

void main() {
    // all calculations are done in world space
    vec3 light_direction = normalize(light.source - s_position);
    vec3 camera_direction = normalize(camera_position - s_position);
    vec3 reflect_direction = reflect(-light_direction, s_normal);  

    // ambient
    vec3 ambient = light.ambient * material.ambient;
    
    // diffuse
    float diff = max(dot(s_normal, light_direction), 0.0);
    vec3 diffuse = diff * (light.diffuse * material.diffuse);

    // specular
    float spec = pow(max(dot(camera_direction, reflect_direction), 0.0), material.shininess);
    vec3 specular = spec * (light.specular * material.specular);
    
    o_color = vec4(ambient + diffuse + specular, 1.0);
}