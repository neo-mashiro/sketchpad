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
layout(location = 2) uniform int material_id;

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

// our light culling only applies to the 28 static point lights
// it doesn't include the dynamic spotlight and the orbit light
const uint n_lights = 28;

// should we sample from a texture (0 or 1)? is this texture available?
// we use type `int` rather than `bool` to avoid `if-else` branching for parallelism.
layout(location = 3) uniform int sample_albedo;     // if not, defaults to magenta color
layout(location = 4) uniform int sample_normal;     // if not, defaults to world-space normal
layout(location = 5) uniform int sample_metallic;   // if not, defaults to dielectric 0.0
layout(location = 6) uniform int sample_roughness;  // if not, defaults to smooth 0.0
layout(location = 7) uniform int sample_ao;         // if not, defaults to no AO 1.0
layout(location = 8) uniform int sample_emission;   // no emission by default
layout(location = 9) uniform int sample_displace;   // no displacement by default

// standard PBR attributes (metallic workflow)
layout(location = 10) uniform vec3 albedo = vec3(1.0, 0.0, 1.0);
layout(location = 11) uniform float metalness = 0.0;
layout(location = 12) uniform float roughness = 0.0;
layout(location = 13) uniform float ao = 1.0;
layout(location = 14) uniform vec2 uv_scale;  // how many times do we repeat uv?

// standard PBR textures (metallic workflow)
layout(binding = 0) uniform sampler2D albedo_map;     // a.k.a diffuse map
layout(binding = 1) uniform sampler2D normal_map;
layout(binding = 2) uniform sampler2D metallic_map;
layout(binding = 3) uniform sampler2D roughness_map;  // a.k.a glossiness map
layout(binding = 4) uniform sampler2D ao_map;
layout(binding = 5) uniform sampler2D emission_map;      // optional
layout(binding = 6) uniform sampler2D displacement_map;  // optional

// a magic function for extracting tangent-space normals and bringing them to world space,
// without the need of precomputed tangent vectors from the vertex shader (to be detailed)
vec3 GetNormal(in vec2 uv) {
    // look up the (tangent-space) normal from the texture, convert it to range [-1,1]
    vec3 ts_normal = texture(normal_map, uv).rgb * 2.0 - 1.0;

    // dFdx(p) = p(x + 1, y) - p(x, y)
    // dFdy(p) = p(x, y + 1) - p(x, y)
    // this function essentially gives us the two edges along the x and y direction

    // compute the partial derivatives w.r.t. _position.x, _position.y, uv.u and uv.v
    vec3 dpx = dFdxFine(_position);
    vec3 dpy = dFdyFine(_position);
    vec2 duu = dFdxFine(uv);
    vec2 dvv = dFdyFine(uv);

    vec3 N = normalize(_normal);
    vec3 T = normalize(dpx * dvv.t - dpy * duu.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * ts_normal);
}

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

    vec2 uv = _uv * uv_scale;

    // albedo maps are often encoded in sRGB space so that they appear visually correct
    // to the artists, but we must convert it from sRGB to linear space before lighting
    // calculations, we must work in linear space to achieve physically correct results.

    vec3 _albedo = sample_albedo * pow(texture(albedo_map, uv).rgb, vec3(2.2)) + (1 - sample_albedo) * albedo;
    float _metalness = sample_metallic * texture(metallic_map, uv).r + (1 - sample_metallic) * metalness;
    float _roughness = sample_roughness * texture(roughness_map, uv).r + (1 - sample_roughness) * roughness;
    float _ao = sample_ao * texture(ao_map, uv).r + (1 - sample_ao) * ao;

    vec3 V = normalize(camera.position - _position);
    vec3 N = sample_normal * GetNormal(uv) + (1 - sample_normal) * normalize(_normal);
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

    // find out which tile this fragment belongs to and the starting index (offset) of this
    // tile in the global SSBO index buffer, then, we can index into the SSBO array to find
    // all lights that contribute to this fragment without having to iterate over every point light

    ivec2 tile_id = ivec2(gl_FragCoord.xy) / ivec2(16, 16);
    uint n_tiles_x = window_size.x / 16;
    uint tile_index = tile_id.y * n_tiles_x + tile_id.x;
    uint offset = tile_index * n_lights;

    // contribution of point lights x 28 (if not culled and visible)
    for (uint i = 0; i < n_lights && light_indices.data[offset + i] != -1; i++) {
        uint light_index = light_indices.data[offset + i];
        L = normalize(light_positions.data[light_index].xyz - _position);
        Li = PointLightRadience(light_index, _position);
        Lo += CookTorranceBRDF(F0, N, V, L, _albedo, _metalness, _roughness) * Li * max(dot(N, L), 0.0);
    }

    // similar to albedo maps, emission maps are also often encoded in sRGB space
    // for the same reason, so we must first convert it to linear space, keep in
    // mind that emission values do not participate in lighting calculations

    // add emission color value to the calculated irradiance
    if (material_id == 5) {
        Lo += pow(texture(emission_map, uv).rgb, vec3(2.2));
    }

    // physically-based lighting computations are done in linear space and the radiance
    // varies wildly over a high dynamic range (HDR), so we must exposure map the final
    // out radiance `Lo` to the LDR range, and then gamma correct it.

    Lo = Lo / (Lo + vec3(1.0));   // tone map HDR
    Lo = pow(Lo, vec3(1.0/2.2));  // gamma correction
    color = vec4(Lo, 1.0);
}

# endif
