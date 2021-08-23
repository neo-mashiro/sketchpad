#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(location = 0) out __ {
    out vec3 _position;
    out vec3 _normal;
    out vec2 _uv;
    out vec3 _tangent;
    out vec3 _bitangent;
};

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;

layout(location = 0) uniform mat4 transform;

void main() {
    // model space -> world space
    _position = vec3(transform * vec4(position, 1.0));
    _normal = normalize(vec3(transform * vec4(normal, 0.0)));
    _uv = uv;
    _tangent = tangent;
    _bitangent = bitangent;

    gl_Position = camera.perspective * camera.view * transform * vec4(position, 1.0);
}
