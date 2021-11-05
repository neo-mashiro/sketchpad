#version 460

// equirectangle to cubemap convertion uses a technique called spherical uv
// projection or uv mapping, which maps the xyz-coordinates on a sphere into
// uv texture coordinates on a plane.
//
// references:
//   https://en.wikipedia.org/wiki/UV_mapping
//   https://learnopengl.com/PBR/IBL/Diffuse-irradiance

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 _spherical_position;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform mat4 projection;

void main() {
    _spherical_position = position;
    gl_Position = projection * view * vec4(position, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec3 _spherical_position;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D equirectangle;

void main() {
    vec3 normal = normalize(_spherical_position);
    vec2 uv = vec2(atan(normal.z, normal.x), asin(normal.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;

    color = vec4(texture(equirectangle, uv).rgb, 1.0);
}

#endif
