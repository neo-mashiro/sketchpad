#version 460

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 projection;
} camera;

// location 0, 1, 2 are always reserved even if we don't use PBR shaders
// location 0 ~ 12 are all reserved for PBR shaders
layout(location = 1) uniform mat4 transform;
layout(location = 3) uniform vec3 light_color;

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 _position;
layout(location = 1) out vec4 _color;

void main() {
    _position = vec3(transform * vec4(position, 1.0));
    _color = vec4(light_color, 1.0);
    gl_Position = camera.projection * camera.view * transform * vec4(position, 1.0);
}

# endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec3 _position;
layout(location = 1) in vec4 _color;

layout(location = 0) out vec4 color;

void main() {
    color = _color;
}

# endif
