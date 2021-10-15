#version 460

// this shader is used by: `FBO::PostProcessDraw()`
////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 0) out vec2 _uv;

void main() {
    _uv = uv;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec2 _uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D color_texture;
layout(binding = 1) uniform sampler2D depth_texture;
layout(binding = 2) uniform usampler2D stencil_texture;

const float near = 0.1;
const float far = 100.0;

void main() {
    color = texture(color_texture, _uv);
}

#endif
