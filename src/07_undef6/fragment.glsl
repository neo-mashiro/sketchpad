#version 440

layout(location = 0) out vec4 o_color;

// input in view space
in vec3 s_position;
in vec3 s_normal;
in vec3 s_light_position;

uniform vec3 u_color;
uniform vec3 u_light_color;

const int c_shininess = 256;  // shininess value of the specular highlight

void main() {
    vec3 normal_direction = normalize(s_normal);
    vec3 light_direction = normalize(s_light_position - s_position);
    vec3 view_direction = normalize(vec3(0.0) - s_position);  // viewer is at (0, 0, 0) in view space
    vec3 reflect_direction = reflect(-light_direction, normal_direction);  

    // ambient lighting (simplistic model of global illumination)
    float ambient_intensity = 0.1;
    vec3 ambient = ambient_intensity * u_light_color;    
    
    // diffuse lighting
    float diffuse_factor = max(dot(normal_direction, light_direction), 0.0);
    vec3 diffuse = diffuse_factor * u_light_color;
    
    // specular lighting
    float specular_intensity = 0.8;
    float specular_factor = pow(max(dot(view_direction, reflect_direction), 0.0), c_shininess);
    vec3 specular = specular_intensity * specular_factor * u_light_color; 
    
    o_color = vec4((ambient + diffuse + specular) * u_color, 1.0);
}