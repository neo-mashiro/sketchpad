#if !defined(_PBR_UNIFORM_H) && defined(fragment_shader)
#define _PBR_UNIFORM_H

// sampler binding points (texture units) 17-19 are reserved for PBR IBL
layout(binding = 17) uniform samplerCube irradiance_map;
layout(binding = 18) uniform samplerCube prefilter_map;
layout(binding = 19) uniform sampler2D BRDF_LUT;

// sampler binding points (texture units) >= 20 are reserved for PBR use
layout(binding = 20) uniform sampler2D albedo_map;
layout(binding = 21) uniform sampler2D normal_map;
layout(binding = 22) uniform sampler2D metallic_map;
layout(binding = 23) uniform sampler2D roughness_map;
layout(binding = 24) uniform sampler2D ao_map;
layout(binding = 25) uniform sampler2D emission_map;
layout(binding = 26) uniform sampler2D displace_map;
layout(binding = 27) uniform sampler2D opacity_map;
layout(binding = 28) uniform sampler2D light_map;
layout(binding = 29) uniform sampler2D anisotan_map;  // anisotropic tangent map (RGB)
layout(binding = 30) uniform sampler2D ext_unit_30;
layout(binding = 31) uniform sampler2D ext_unit_31;

// default-block (loose) uniform locations >= 900 are reserved for PBR use
layout(location = 900) uniform bool sample_albedo;
layout(location = 901) uniform bool sample_normal;
layout(location = 902) uniform bool sample_metallic;
layout(location = 903) uniform bool sample_roughness;
layout(location = 904) uniform bool sample_ao;
layout(location = 905) uniform bool sample_emission;
layout(location = 906) uniform bool sample_displace;
layout(location = 907) uniform bool sample_opacity;
layout(location = 908) uniform bool sample_lightmap;
layout(location = 909) uniform bool sample_anisotan;
layout(location = 910) uniform bool sample_ext_910;
layout(location = 911) uniform bool sample_ext_911;

/***** physically-based material input properties *****/

// shared properties
layout(location = 912) uniform vec4  albedo;         // alpha is not pre-multiplied
layout(location = 913) uniform float roughness;      // clamped at 0.045 so that specular highlight is visible
layout(location = 914) uniform float ao;             // 0.0 = occluded, 1.0 = not occluded
layout(location = 915) uniform vec4  emission;       // optional emissive color
layout(location = 916) uniform vec2  uv_scale;       // texture coordinates tiling factor
layout(location = 928) uniform float alpha_mask;     // alpha threshold below which fragments are discarded

// standard model, mostly opaque but can have simple alpha blending
layout(location = 917) uniform float metalness;      // should be a binary value 0 or 1
layout(location = 918) uniform float specular;       // 4% F0 = 0.5, 2% F0 = 0.35 (water), clamped to [0.35, 1]
layout(location = 919) uniform float anisotropy;     // ~ [-1, 1]
layout(location = 920) uniform vec3  aniso_dir;      // anisotropy defaults to the tangent direction

// refraction model considers only isotropic dielectrics
layout(location = 921) uniform float transmission;   // ratio of diffuse light transmitted through the material
layout(location = 922) uniform float thickness;      // max volume thickness in the direction of the normal
layout(location = 923) uniform float ior;            // air 1.0, plastic/glass 1.5, water 1.33, gemstone 1.6-2.33
layout(location = 924) uniform vec3  transmittance;  // transmittance color as linear RGB, may differ from albedo
layout(location = 925) uniform float tr_distance;    // transmission distance, smaller for denser IOR
layout(location = 931) uniform uint  volume_type;    // refraction varies by the volume's interior geometry

// cloth model considers only single-layer isotropic dielectrics w/o refraction
layout(location = 926) uniform vec3  sheen_color;
layout(location = 927) uniform vec3  subsurface_color;

// (optional) an additive clear coat layer, not applicable to cloth
layout(location = 929) uniform float clearcoat;
layout(location = 930) uniform float clearcoat_roughness;

// the subsurface scattering model is very different and hard so we'll skip it for now, the way
// that Disney BSDF (2015) and Filament handles real-time SSS is very hacky, which is not worth
// learning at the moment. A decent SSS model usually involves an approximation of path tracing
// volumetrics, which I'll come back to in a future project when taking the course CMU 15-468.

// a two-digit number indicative of how the pixel should be shaded
// component x encodes the shading model: standard = 1, refraction = 2, cloth = 3
// component y encodes an additive layer: none = 0, clearcoat = 1, sheen = 2
layout(location = 999) uniform uvec2 model;

// pixel data definition
struct Pixel {
    vec3 _position;
    vec3 _normal;
    vec2 _uv;
    vec2 _uv2;
    vec3 _tangent;
    vec3 _binormal;
    bool _has_tbn;
    bool _has_uv2;
    vec3  position;
    vec2  uv;
    mat3  TBN;
    vec3  V;
    vec3  N;
    vec3  R;
    vec3  GN;
    vec3  GR;
    float NoV;
    vec4  albedo;
    float roughness;
    float alpha;
    vec3  ao;
    vec4  emission;
    vec3  diffuse_color;
    vec3  F0;
    vec3  DFG;
    vec3  Ec;
    float metalness;
    float specular;
    float anisotropy;
    vec3  aniso_T;
    vec3  aniso_B;
    float clearcoat;
    float clearcoat_roughness;
    float clearcoat_alpha;
    float eta;
    float transmission;
    vec3  absorption;
    float thickness;
    uint  volume;
    vec3  subsurface_color;
};

#endif