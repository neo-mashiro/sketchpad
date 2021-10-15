#version 460

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 0) uniform mat4 transform;

void main() {
    gl_Position = camera.perspective * camera.view * transform * vec4(position, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

void main() {
	// in the depth prepass, we don't draw anything in the fragment shader
}

# endif
