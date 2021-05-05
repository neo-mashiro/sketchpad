#version 460

struct Material {
    sampler2D diffuse;   // lighting map
    sampler2D specular;  // lighting map
    sampler2D emission;  // lighting map
    float shininess;
}; 

struct Light {
    vec3 source;      // light source position in world space
    vec3 ambient;     // ambient intensity
    vec3 diffuse;     // diffuse intensity
    vec3 specular;    // specular intensity
};

layout(location = 0) out vec4 o_color;

in vec3 s_position;
in vec3 s_normal;
in vec2 s_uv;

uniform Material material;
uniform Light light;
uniform vec3 camera_position;

const int uv_repeat = 10;

void main() {
    // all calculations are done in world space
    vec3 light_direction = normalize(light.source - s_position);
    vec3 camera_direction = normalize(camera_position - s_position);
    vec3 reflect_direction = reflect(-light_direction, s_normal);  

    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, s_uv * uv_repeat).rgb;
    
    // diffuse
    float diff = max(dot(s_normal, light_direction), 0.0);
    vec3 diffuse = diff * (light.diffuse * texture(material.diffuse, s_uv * uv_repeat).rgb);

    // specular
    float spec = pow(max(dot(camera_direction, reflect_direction), 0.0), material.shininess);
    vec3 specular = spec * (light.specular * texture(material.specular, s_uv * uv_repeat).rgb);

    // emission
    vec3 emission = texture(material.emission, s_uv * 0).rgb;
    
    o_color = vec4(ambient + diffuse + specular + emission, 1.0);
}