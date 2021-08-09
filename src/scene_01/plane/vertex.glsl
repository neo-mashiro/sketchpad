#version 460

// vertex attributes in local model space
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out VS_OUT {
    layout(location = 0) out vec3 _position;  // in world space
    layout(location = 1) out vec3 _normal;  // in world space
    layout(location = 2) out vec2 _uv;
    layout(location = 3) out vec3 _tangent;
    layout(location = 4) out vec3 _bitangent;
} vs_out;


uniform mat4 u_MVP;
uniform mat4 M;
layout (std140) uniform VP {
    mat4 P;
    mat4 V;
};

void main() {
    // model space -> world space
    _position = vec3(u_M * vec4(position, 1.0));
    _normal = normalize(vec3(u_M * vec4(normal, 0.0)));  // orthogonal transform
    // _normal = normalize(mat3(transpose(inverse(u_M))) * normal);  // non-orthogonal transform
    _uv = uv;

    gl_Position = u_MVP * vec4(position, 1.0);
}
