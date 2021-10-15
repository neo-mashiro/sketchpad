#version 460
#pragma optimize(off)

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;

layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform vec3 material_id;

const float pi = 3.14159265359;
const ivec2 window_size = ivec2(1600, 900);

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 uv2;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

layout(location = 0) out __ {
    out vec3 _position;
    out vec3 _normal;
    out vec2 _uv;
    out vec2 _uv2;
    out vec3 _tangent;
    out vec3 _bitangent;
};

void main() {
    _position = vec3(transform * vec4(position, 1.0));
    _normal = normalize(vec3(transform * vec4(normal, 0.0)));
    _uv = uv;
    _uv2 = uv2;
    _tangent = tangent;
    _bitangent = bitangent;

    gl_Position = camera.perspective * camera.view * transform * vec4(position, 1.0);
}

# endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in __ {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _bitangent;
};

layout(location = 0) out vec4 color;

layout(std430, binding = 2) readonly buffer Index {
	int data[];
} light_indices;

const uint n_lights = 16;  // culling does not include the spotlight and orbit light

// metallic workflow (without textures)
layout(location = 2) uniform vec3 albedo;
layout(location = 3) uniform float metalness;
layout(location = 4) uniform float roughness;

// include light uniform buffers and attenuation functions
// all include files follow this naming convention: scene number + "~" (tilde) + include name
#include 01~light.glsl

// include Cook-Torrance BRDF
#include 01~brdf.glsl

void main() {
    // find out which tile this fragment belongs to
	ivec2 tile_id = ivec2(gl_FragCoord.xy) / ivec2(16, 16);
    uint n_tiles_x = window_size.x / 16;
	uint tile_index = tile_id.y * n_tiles_x + tile_id.x;

    // starting index of this tile in the global SSBO buffer
    uint offset = tile_index * n_lights;

    vec3 N = normalize(_normal);
    vec3 V = normalize(camera.position - _position);
    vec3 F0 = mix(vec3(0.04), albedo, metalness);  // 0.04: heuristic for dielectrics

    // contribution of directional light x 1 (ambient)
    vec3 L = normalize(DL.direction);
    vec3 Li = DL.intensity * DL.color;
    vec3 Lo = CookTorranceBRDF(F0, N, V, L) * Li * max(dot(N, L), 0.0);

    // contribution of spotlight x 1
    L = normalize(SL.position - _position);
    Li = SpotlightRadience(_position);
    Lo += CookTorranceBRDF(F0, N, V, L) * Li * max(dot(N, L), 0.0);

    // contribution of orbit light x 1
    L = normalize(PL[16].position - _position);
    Li = PointLightRadience(16, _position);
    Lo += CookTorranceBRDF(F0, N, V, L) * Li * max(dot(N, L), 0.0);

    // contribution of point lights x 16 (if not culled and visible)
    for (uint i = 0; i < n_lights && light_indices.data[offset + i] != -1; i++) {
		uint light_index = light_indices.data[offset + i];
        L = normalize(PL[light_index].position - _position);
        Li = PointLightRadience(light_index, _position);
        Lo += CookTorranceBRDF(F0, N, V, L) * Li * max(dot(N, L), 0.0);
	}

    // physically-based lighting computations are done in linear space and the radiance
    // varies wildly over a high dynamic range (HDR), so we must exposure map the final
    // out radiance `Lo` to the LDR range, and then gamma correct it.
    color = Lo / (Lo + vec3(1.0));      // tone map HDR
    color = pow(color, vec3(1.0/2.2));  // gamma correction
    color = vec4(color, 1.0);
}

# endif
