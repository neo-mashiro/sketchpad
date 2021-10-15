#version 460

// this shader is used by: `FBO::DebugDraw()`
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

layout(binding = 0) uniform sampler2D color_depth_texture;
layout(binding = 1) uniform usampler2D stencil_texture;

const float near = 0.1;
const float far = 100.0;

subroutine vec4 draw_buffer(void);  // typedef the subroutine (like a function pointer)
layout(location = 0) subroutine uniform draw_buffer buffer_switch;

layout(index = 0)
subroutine(draw_buffer)
vec4 DrawColorBuffer() {
    return texture(color_depth_texture, _uv);
}

layout(index = 1)
subroutine(draw_buffer)
vec4 DrawDepthBuffer() {
    // depth in screen space is non-linear, the precision is high for small z-values
    // and low for large z-values, we need to linearize depth values before drawing.
    // while `z` is the real linearized depth in [near, far], the visualized value is
    // further divided by `far` in order to fit in the [0, 1] range.
    float depth = texture(color_depth_texture, _uv).r;
    float ndc_depth = depth * 2.0 - 1.0;
    float z = (2.0 * near * far) / (far + near - ndc_depth * (far - near));
    float linear_depth = z / far;

    return vec4(vec3(linear_depth), 1.0);
}

layout(index = 2)
subroutine(draw_buffer)
vec4 DrawStencilBuffer() {
    uint stencil = texture(stencil_texture, _uv).r;
    return vec4(vec3(stencil), 1.0);
}

void main() {
    color = buffer_switch();
}

#endif
