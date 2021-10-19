#version 460
#pragma optimize(off)

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 projection;
} camera;

layout(location = 0) uniform bool depth_prepass;
layout(location = 1) uniform mat4 transform;
layout(location = 2) uniform vec3 material_id;

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

    gl_Position = camera.projection * camera.view * transform * vec4(position, 1.0);
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

layout(std430, binding = 0) readonly buffer Color {
	vec4 data[];
} light_colors;

layout(std430, binding = 1) readonly buffer Position {
	vec4 data[];
} light_positions;

layout(std430, binding = 2) readonly buffer Range {
	float data[];
} light_ranges;

layout(std430, binding = 3) readonly buffer Index {
	int data[];
} light_indices;

// our light culling only applies to the 16 static point lights
// it doesn't include the dynamic spotlight and the orbit light
const uint n_lights = 28;

// should we sample from a texture (0 or 1)? is this texture available?
// we use type `int` rather than `bool` to avoid `if-else` branching for parallelism.
layout(location = 3) uniform int sample_albedo;     // if not, defaults to magenta color
layout(location = 4) uniform int sample_normal;     // if not, defaults to world-space normal
layout(location = 5) uniform int sample_metallic;   // if not, defaults to dielectric 0.0
layout(location = 6) uniform int sample_roughness;  // if not, defaults to smooth 0.0
layout(location = 7) uniform int sample_ao;         // if not, defaults to no AO 1.0

// standard PBR attributes
layout(location = 8) uniform vec3 albedo = vec3(1.0, 0.0, 1.0);
layout(location = 9) uniform float metalness = 0.0;
layout(location = 10) uniform float roughness = 0.0;
layout(location = 11) uniform float ao = 1.0;
layout(location = 12) uniform float uv_scale = 1.0;

// standard PBR textures
layout(binding = 0) uniform sampler2D albedo_map;
layout(binding = 1) uniform sampler2D normal_map;
layout(binding = 2) uniform sampler2D metallic_map;
layout(binding = 3) uniform sampler2D roughness_map;
layout(binding = 4) uniform sampler2D ao_map;

// a magic function for extracting tangent-space normals and bringing them to world space,
// without the need of precomputed tangent vectors from the vertex shader (to be detailed)
vec3 ExtractNormal() {
    // look up the (tangent-space) normal from the texture, convert it to range [-1,1]
    vec3 ts_normal = texture(normal_map, _uv * uv_scale).rgb * 2.0 - 1.0;

    vec3 dpx = dFdx(_position);
    vec3 dpy = dFdy(_position);
    vec2 duu = dFdx(_uv * uv_scale);
    vec2 dvv = dFdy(_uv * uv_scale);

    vec3 N = normalize(_normal);
    vec3 T = normalize(dpx * dvv.t - dpy * duu.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * ts_normal);
}

// how to also include the heightmap for parallax mapping?

// include light uniform buffers and attenuation functions
// all include files follow this naming convention: scene number + "~" (tilde) + include name
#include 01~light.glsl

// include Cook-Torrance BRDF
#include 01~brdf.glsl

void main() {
    // in the depth prepass, we don't draw anything in the fragment shader
    if (depth_prepass) {
        return;
    }

    // find out which tile this fragment belongs to
	ivec2 tile_id = ivec2(gl_FragCoord.xy) / ivec2(16, 16);
    uint n_tiles_x = window_size.x / 16;
	uint tile_index = tile_id.y * n_tiles_x + tile_id.x;

    // starting index of this tile in the global SSBO buffer
    uint offset = tile_index * n_lights;

    // albedo maps are often encoded in sRGB space so that they appear visually correct
    // to the artists, but we must convert it from sRGB to linear space before lighting
    // calculations, we must work in linear space to achieve physically correct results.
    // vec3 _albedo = sample_albedo * pow(texture(albedo_map, _uv * uv_scale).rgb, vec3(2.2))
    //     + (1 - sample_albedo) * albedo;
    //
    // float _metalness = sample_metallic * texture(metallic_map, _uv * uv_scale).r
    //     + (1 - sample_metallic) * metalness;
    //
    // float _roughness = sample_roughness * texture(roughness_map, _uv * uv_scale).r
    //     + (1 - sample_roughness) * roughness;
    //
    // float _ao = sample_ao * texture(ao_map, _uv * uv_scale).r
    //     + (1 - sample_ao) * ao;
    //
    // vec3 N = sample_normal * ExtractNormal() + (1 - sample_normal) * normalize(_normal);
    vec3 _albedo;
    if (sample_albedo > 0) {
        _albedo = pow(texture(albedo_map, _uv * uv_scale).rgb, vec3(2.2));
    }
    else {
        _albedo = albedo;
    }

    float _metalness;
    if (sample_metallic > 0) {
        _metalness = texture(metallic_map, _uv * uv_scale).r;
    }
    else {
        _metalness = metalness;
    }

    float _roughness;
    if (sample_roughness > 0) {
        _roughness = texture(roughness_map, _uv * uv_scale).r;
    }
    else {
        _roughness = roughness;
    }

    float _ao;
    if (sample_ao > 0) {
        _ao = texture(ao_map, _uv * uv_scale).r;
    }
    else {
        _ao = ao;
    }
    vec3 N;
    if (sample_normal > 0) {
        N = ExtractNormal();
    }
    else {
        N = normalize(_normal);
    }
    vec3 V = normalize(camera.position - _position);
    vec3 F0 = mix(vec3(0.04), _albedo, _metalness);  // 0.04: heuristic for dielectrics

    // contribution of directional light x 1 (plus ambient occlussion)
    // this scene is dark with the space skybox so we will ignore environment lighting
    vec3 L = normalize(DL.direction);
    vec3 Li = DL.intensity * DL.color;
    vec3 Lo = CookTorranceBRDF(F0, N, V, L, _albedo, _metalness, _roughness) * Li * max(dot(N, L), 0.0) * _ao;

    // contribution of spotlight x 1
    L = normalize(SL.position - _position);
    Li = SpotlightRadience(_position);
    Lo += CookTorranceBRDF(F0, N, V, L, _albedo, _metalness, _roughness) * Li * max(dot(N, L), 0.0);

    // contribution of orbit light x 1
    L = normalize(OL.position - _position);
    Li = OrbitLightRadience(_position);
    Lo += CookTorranceBRDF(F0, N, V, L, _albedo, _metalness, _roughness) * Li * max(dot(N, L), 0.0);

    // contribution of point lights x 28 (if not culled and visible)
    for (uint i = 0; i < n_lights && light_indices.data[offset + i] != -1; i++) {
		uint light_index = light_indices.data[offset + i];
        L = normalize(light_positions.data[light_index].xyz - _position);
        Li = PointLightRadience(light_index, _position);
        Lo += CookTorranceBRDF(F0, N, V, L, _albedo, _metalness, _roughness) * Li * max(dot(N, L), 0.0);
	}

    // physically-based lighting computations are done in linear space and the radiance
    // varies wildly over a high dynamic range (HDR), so we must exposure map the final
    // out radiance `Lo` to the LDR range, and then gamma correct it.
    Lo = Lo / (Lo + vec3(1.0));   // tone map HDR
    Lo = pow(Lo, vec3(1.0/2.2));  // gamma correction
    color = vec4(Lo, 1.0);
}

# endif
