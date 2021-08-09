#version 460

layout(location = 0) in vec3 position;

out vec3 _uvw;

uniform mat4 u_MVP;

// if the skybox is centered at the origin, tex coordinates ~ = positions.
// the swizzling trick ensures that the depth value is always 1 after the
// /w division, so that our skybox has the farthest distance in the scene,
// which will be rendered behind all other objects.

void main() {
    _uvw = position;  // skybox texture coordinates have 3 dimensions u, v, w
    vec4 pos = u_MVP * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
