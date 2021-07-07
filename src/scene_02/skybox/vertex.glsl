#version 460

layout(location = 0) in vec3 position;

out vec3 _uvw;

uniform mat4 u_MVP;

void main() {
    // when the skybox is centered at the origin, tex coordinates ~= positions
    _uvw = position;  // skybox texture coordinates have 3 dimensions u, v, w

    // the swizzling trick ensures that depth value is always 1 after the /w division
    // a depth value of 1 means the farthest distance in the scene, so the skybox is
    // always rendered behind all other objects regardless of its vertices positions.
    vec4 pos = u_MVP * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
