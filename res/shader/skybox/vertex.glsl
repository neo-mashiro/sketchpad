#version 460

layout(location = 0) in vec3 position;

out vec3 _tex_coords;

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;

layout(location = 0) uniform mat4 transform;

void main() {
    // skybox's texture coordinates have 3 dimensions u, v, w, which is roughly
    // equal to its position (because the skybox cube is centered at the origin)
    _tex_coords = position;

    // skybox is stationary, it doesn't move with the camera, so we use
    // a rectified view matrix whose translation components are removed
    mat4 rectified_view = mat4(mat3(camera.view));
    vec4 pos = camera.perspective * rectified_view * transform * vec4(position, 1.0);

    // the swizzling trick ensures that the skybox's depth value is always 1 after the /w division,
    // so it has the farthest distance in the scene, and will be rendered behind all other objects.
    gl_Position = pos.xyww;
}
