#version 460 core

layout(std140, binding = 0) uniform Camera {
    vec4 position;
    vec4 direction;
    mat4 view;
    mat4 projection;
} camera;

#include "../core/renderer_input.glsl"

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 _tex_coords;

void main() {
    // skybox's texture coordinates have 3 dimensions u, v, w, which is roughly
    // equal to its position (because the skybox cube is centered at the origin)
    _tex_coords = position;

    // skybox is stationary, it doesn't move with the camera, so we need to use a
    // rectified view matrix whose translation components have been stripped out
    mat4 rectified_view = mat4(mat3(camera.view));
    vec4 pos = camera.projection * rectified_view * self.transform * vec4(position, 1.0);

    // the swizzling trick ensures that the skybox's depth value is always 1 after the /w division
    // so it has the farthest distance in the scene, and will be rendered behind all other objects
    gl_Position = pos.xyww;
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec3 _tex_coords;
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bloom;

layout(binding = 0) uniform samplerCube skybox;

// note that the same HDRI image often looks different across applications or even scenes
// this is because some tone-mapping operators depend on the maximum or average luminance
// of a scene, which can vary slightly based on the brightness of pixels in the viewport.
// if the skybox appears too dark after getting toned down, you can adjust its brightness
// using the uniform `exposure`, before tone mapping is applied.

layout(location = 0) uniform float exposure = 1.0;
layout(location = 1) uniform float lod = 0.0;

void main() {
    if (rdr_in.depth_prepass) {
        return;  // in the depth prepass, we don't draw anything in the fragment shader
    }

    const float max_level = textureQueryLevels(skybox) - 1.0;
    vec3 irradiance = textureLod(skybox, _tex_coords, clamp(lod, 0.0, max_level)).rgb;
    color = vec4(irradiance * exposure, 1.0);

    // if the 2nd MRT isn't enabled, bloom will write to GL_NONE and be discarded
    bloom = vec4(0.0, 0.0, 0.0, 1.0);  // make sure the skybox is not bloomed
}

#endif
