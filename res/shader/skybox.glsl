#version 460

// this shader is used by the skybox
////////////////////////////////////////////////////////////////////////////////

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 projection;
} camera;

layout(location = 0) uniform bool depth_prepass;
layout(location = 1) uniform mat4 transform;

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 _tex_coords;

void main() {
    // skybox's texture coordinates have 3 dimensions u, v, w, which is roughly
    // equal to its position (because the skybox cube is centered at the origin)
    _tex_coords = position;

    // skybox is stationary, it doesn't move with the camera, so we use
    // a rectified view matrix whose translation components are removed
    mat4 rectified_view = mat4(mat3(camera.view));
    vec4 pos = camera.projection * rectified_view * transform * vec4(position, 1.0);

    // the swizzling trick ensures that the skybox's depth value is always 1 after the /w division,
    // so it has the farthest distance in the scene, and will be rendered behind all other objects.
    gl_Position = pos.xyww;
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec3 _tex_coords;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform samplerCube skybox;

void main() {
    // in the depth prepass, we don't draw anything in the fragment shader
    if (depth_prepass) {
        return;
    }

    color = texture(skybox, _tex_coords);
}

#endif
