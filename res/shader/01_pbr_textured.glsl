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

// metallic workflow (with textures)
layout(binding = 0) uniform sampler2D albedo_map;
layout(binding = 1) uniform sampler2D normal_map;
layout(binding = 2) uniform sampler2D metallic_map;
layout(binding = 3) uniform sampler2D roughness_map;
layout(binding = 4) uniform sampler2D ao_map;

// a magic function for extracting tangent-space normals and bringing them to world space,
// without the need of precomputed tangent vectors from the vertex shader (to be detailed)
vec3 ExtractNormal() {
    // look up the (tangent-space) normal from the texture, convert it to range [-1,1]
    vec3 ts_normal = texture(normal_map, _uv).rgb * 2.0 - 1.0;

    vec3 dpx = dFdx(_position);
    vec3 dpy = dFdy(_position);
    vec2 duu = dFdx(_uv);
    vec2 dvv = dFdy(_uv);

    vec3 N = normalize(_normal);
    vec3 T = normalize(dpx * dvv.t - dpy * duu.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * ts_normal);
}

// how to also include the heightmap for parallax mapping?

// include light uniform buffers and attenuation functions
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

    // albedo maps are often encoded in sRGB space so that they appear visually correct
    // to the artists, but we must convert it from sRGB to linear space before lighting
    // calculations, we must work in linear space to achieve physically correct results.
    vec3 albedo = texture(albedo_map, _uv).rgb;
    albedo = pow(albedo, vec3(2.2));
    float metalness = texture(metallic_map, _uv).r;
    float roughness = texture(roughness_map, _uv).r;
    float ao        = texture(ao_map, _uv).r;

    vec3 N = ExtractNormal();
    vec3 V = normalize(camera.position - _position);
    vec3 F0 = mix(vec3(0.04), albedo, metalness);  // 0.04: heuristic for dielectrics

    // contribution of directional light x 1 (ambient)
    vec3 L = normalize(DL.direction);
    vec3 Li = DL.intensity * DL.color;
    vec3 Lo = CookTorranceBRDF(F0, N, V, L) * Li * max(dot(N, L), 0.0) * ao;

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
