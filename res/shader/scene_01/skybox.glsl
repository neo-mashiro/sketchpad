#version 460

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
layout(location = 1) out vec4 bloom;

layout(binding = 0) uniform samplerCube skybox;

layout(location = 3) uniform float brightness;

void main() {
    // in the depth prepass, we don't draw anything in the fragment shader
    if (depth_prepass) {
        return;
    }

    vec3 hdr_color = texture(skybox, _tex_coords).rgb;

    // skybox is usually loaded from HDR images, so it often looks different across apps
    // or even scenes. Since tone mapping operators may depend on the maximum or average
    // luminance of a scene, skybox's color can be affected by other objects in the scene.
    // if the skybox appears too dark after getting toned down, you can manually adjust
    // its brightness uniform before tone mapping is applied, or do some other fun stuff.

    hdr_color *= brightness;

    hdr_color = hdr_color / (hdr_color + vec3(1.0));
    hdr_color = pow(hdr_color, vec3(1.0/2.2));

    color = vec4(hdr_color, 1.0);
    bloom = vec4(0.0, 0.0, 0.0, 1.0);  // make sure our skybox doesn't get blurred
}

#endif
