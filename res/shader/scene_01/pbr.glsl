#version 460 core
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

layout(location = 0) out _vtx {
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

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in _vtx {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _bitangent;
};

layout(location = 0) out vec4 irradiance;
layout(location = 1) out vec4 bloom;

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

layout(std140, binding = 1) uniform DirectionalLight {
    vec3 color;
    vec3 direction;
    float intensity;
} DL;

layout(std140, binding = 2) uniform Spotlight {
    vec3 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float inner_cosine;
    float outer_cosine;
    float range;
} SL;

layout(std140, binding = 3) uniform OrbitLight {
    vec3 color;
    vec3 position;
    float intensity;
    float linear;
    float quadratic;
    float range;
} OL;

layout(std140, binding = 4) uniform LightCluster {
    // the whole 28 point lights cluster shares the same light intensity and attenuation
    // coefficients, the intensity value can be controlled dynamically by ImGui sliders.
    // light cluster colors, positions and ranges are globally fixed (stored in SSBOs)
    float intensity;
    float linear;
    float quadratic;
};

float Luminance(vec3 irradiance) {
    return dot(irradiance, vec3(0.2126, 0.7152, 0.0722));
}

vec3 PointLightRadience(uint i, vec3 fpos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(light_positions.data[i].xyz, fpos);
    float attenuation = d >= light_ranges.data[i] ? 0.0 : 1.0 / (1.0 + linear * d + quadratic * d * d);
    return attenuation * intensity * light_colors.data[i].rgb;
}

vec3 OrbitLightRadience(vec3 fpos) {
    float d = distance(OL.position, fpos);
    float attenuation = d >= OL.range ? 0.0 : 1.0 / (1.0 + OL.linear * d + OL.quadratic * d * d);
    return attenuation * OL.intensity * OL.color;
}

vec3 SpotlightRadience(vec3 fpos) {
    // the spotlight distance attenuation uses a linear falloff
    vec3 ray = fpos - SL.position;  // inward ray from the light to the fragment
    float projected_distance = dot(SL.direction, ray);
    float linear_attenuation = 1.0 - clamp(projected_distance / SL.range, 0.0, 1.0);

    // the spotlight angular attenuation fades out from the inner to the outer cone
    float cosine = dot(SL.direction, normalize(ray));
    float angular_diff = SL.inner_cosine - SL.outer_cosine;
    float angular_attenuation = clamp((cosine - SL.outer_cosine) / angular_diff, 0.0, 1.0);

    return linear_attenuation * angular_attenuation * SL.intensity * SL.color;
}

// Trowbridge-Reitz GGX normal distribution function (long tail distribution)
// Trowbridge-Reitz GGX NDF is generally favored over Beckmann NDF because it's much smoother
float TRGGX(vec3 N, vec3 H, float _roughness) {
    float alpha = _roughness * _roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = pow(max(dot(N, H), 0.0), 2.0);
    return alpha2 / (pi * pow(NoH2 * (alpha2 - 1.0) + 1.0, 2.0));
}

// Generalized Trowbridge-Reitz (GTR) NDF (tail length can be tuned by parameter `gamma`)
// this is an extended GGX NDF proposed by Brent Burley from Walt Disney Animation Studios
float GTR(vec3 N, vec3 H, float _roughness, float gamma) {
    return 0.0;  // TODO
}

// Schlick-GGX geometry function
float SchlickGGX(float NoV, float _roughness) {
    float alpha = _roughness * _roughness;
    float k = pow(alpha + 1.0, 2.0) / 8.0;  // for direct lighting
    // float k = alpha * alpha / 2.0;  // for indirect lighting
    return NoV / (NoV * (1.0 - k) + k);
}

// Smith's geometry function (the shadowing-masking visibility term)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float _roughness) {
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    return SchlickGGX(NoV, _roughness) * SchlickGGX(NoL, _roughness);
}

// Fresnel-Schlick approximation
vec3 FresnelSchlick(vec3 H, vec3 V, vec3 F0) {
    // H should always be the halfway vector, but V can be either the view direction or light direction
    float HoV = clamp(dot(H, V), 0.0, 1.0);
    return F0 + (1.0 - F0) * pow(1.0 - HoV, 5.0);
}

// Cook-Torrance BRDF
vec3 CookTorranceBRDF(vec3 F0, vec3 N, vec3 V, vec3 L, vec3 _albedo, float _metalness, float _roughness) {
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);

    vec3 F = FresnelSchlick(H, V, F0);
    float G = GeometrySmith(N, V, L, _roughness);
    float D = TRGGX(N, H, _roughness);

    vec3 diffuse = _albedo / pi;
    vec3 specular = (D * G * F) / (4 * NoV * NoL + 0.00001);

    vec3 ks = F;
    vec3 kd = (vec3(1.0) - ks) * (1.0 - _metalness);

    return kd * diffuse + specular;
}

void main() {
    // in the depth prepass, we don't draw anything in the fragment shader
    if (depth_prepass) {
        return;
    }

    vec4 emissive_irradiance = vec4(0.0, 0.0, 0.0, 1.0);
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

    // this term represents the Fresnel specular reflectance at normal incidence.
    // a heuristic of 0.04 works for most air-dielectric/conductor interface.
    // for light incoming from other media such as water or glass, `F0` must be computed differently.
    // https://google.github.io/filament/Filament.html#table_commonmatreflectance

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

    // add emission color value to the calculated irradiance (runestone platform emission part)
    if (material_id == 8) {
        vec3 emission_value = texture(emission_map, uv).rgb;
        Lo += pow(emission_value, vec3(2.2));
        float luminance = Luminance(Lo);

        if (luminance > 0.5) {
            emissive_irradiance += 1.5 * vec4(emission_value, 1.0);
        }
    }

    // physically-based lighting computations are done in linear space and the radiance
    // varies wildly over a high dynamic range (HDR), so we must apply tone mapping and
    // gamma correction on the output irradiance. This will be done in the final pass.

    irradiance = vec4(Lo, 1.0);
    bloom = emissive_irradiance;
}

#endif
