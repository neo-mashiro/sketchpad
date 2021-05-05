#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out vec2 _uv;
out vec3 _normal;

uniform mat4 mvp;

void main() {
    _uv = uv;
    _normal = normal;
    
    gl_Position = mvp * vec4(position, 1.0);
}