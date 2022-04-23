#version 460 core
#pragma optimize(off)

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
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 uv2;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 binormal;
layout(location = 6) in ivec4 bone_id;
layout(location = 7) in vec4 bone_wt;

layout(location = 0) out _vtx {
    out vec3 _position;
    out vec3 _normal;
    out vec2 _uv;
    out vec2 _uv2;
    out vec3 _tangent;
    out vec3 _binormal;
};

layout(location = 100) uniform mat4 bone_transform[150];  // up to 150 bones

mat4 CalcBoneTransform() {
    mat4 T = mat4(0.0);
    for (uint i = 0; i < 4; ++i) {
        if (bone_id[i] >= 0) {
            T += (bone_transform[bone_id[i]] * bone_wt[i]);
        }
    }
    // if (length(T[0]) == 0) { return mat4(1.0); }
    return T;
}

void main() {
    bool suzune = self.material_id == 12 || (self.material_id >= 14 && self.material_id <= 18);
    mat4 BT = suzune ? CalcBoneTransform() : mat4(1.0);
    mat4 MVP = camera.projection * camera.view * self.transform;

    gl_Position = MVP * BT * vec4(position, 1.0);

    _position = vec3(self.transform * BT * vec4(position, 1.0));
    _normal   = normalize(vec3(self.transform * BT * vec4(normal, 0.0)));
    _tangent  = normalize(vec3(self.transform * BT * vec4(tangent, 0.0)));
    _binormal = normalize(vec3(self.transform * BT * vec4(binormal, 0.0)));
    _uv = uv;
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

#include "../core/pbr_uniform.glsl"
#include "../core/pbr_shading.glsl"

layout(location = 0) in _vtx {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _binormal;
};

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 bloom;

layout(std140, binding = 1) uniform PL {
    vec4  color[2];
    vec4  position[2];
    float intensity[2];
    float linear[2];
    float quadratic[2];
    float range[2];
} pl;

layout(std140, binding = 2) uniform SL {
    vec4  color;
    vec4  position;
    vec4  direction;
    float intensity;
    float inner_cos;
    float outer_cos;
    float range;
} sl;

layout(std140, binding = 3) uniform DL {
    vec4  color;
    vec4  direction[5];
    float intensity;
} dl;

layout(location = 0) uniform float ibl_exposure;
layout(location = 1) uniform bool enable_spotlight;
layout(location = 2) uniform bool enable_moonlight;
layout(location = 3) uniform bool enable_lantern;
layout(location = 4) uniform bool enable_shadow;

// light radius uniforms that control shadow softness
layout(location = 5) uniform float pl_radius;
layout(location = 6) uniform float lt_radius;

// shadow map is directly controlled by scene code as it comes from the framebuffer so this
// texture unit must be unique, otherwise it could be replaced by textures in other shaders
layout(binding = 15) uniform samplerCube shadow_map1;
layout(binding = 16) uniform samplerCube shadow_map2;

void main() {
    Pixel px;
    px._position = _position;
    px._normal   = _normal;
    px._uv       = _uv;
    px._has_tbn  = true;

    InitPixel(px, camera.position.xyz);

    vec3 Lo = vec3(0.0);
    vec3 Le = vec3(0.0);  // emission

    Lo += EvaluateIBL(px) * max(ibl_exposure, 0.5);

    if (enable_moonlight) {
        for (uint i = 0; i < 5; ++i) {
            Lo += EvaluateADL(px, dl.direction[i].xyz, 1.0) * dl.color.rgb * dl.intensity;
        }
    }

    if (true) {  // the point light is always enabled
        float visibility = 1.0;
        if (enable_shadow) {
            visibility -= EvalOcclusion(px, shadow_map1, pl.position[0].xyz, pl_radius);
        }

        vec3 pc = EvaluateAPL(px, pl.position[0].xyz, pl.range[0], pl.linear[0], pl.quadratic[0], visibility);
        Lo += pc * pl.color[0].rgb * pl.intensity[0];
    }

    if (enable_lantern) {
        float visibility = 1.0;
        if (enable_shadow) {
            visibility -= EvalOcclusion(px, shadow_map2, pl.position[1].xyz, lt_radius);
        }

        vec3 pc = EvaluateAPL(px, pl.position[1].xyz, pl.range[1], pl.linear[1], pl.quadratic[1], visibility);
        Lo += pc * pl.color[1].rgb * pl.intensity[1];
    }

    if (enable_spotlight) {
        vec3 sc = EvaluateASL(px, sl.position.xyz, sl.direction.xyz, sl.range, sl.inner_cos, sl.outer_cos);
        Lo += sc * sl.color.rgb * sl.intensity * step(-8.0, _position.z);
    }

    // if (self.material_id == 123) {
    //     Le = CircularEaseInOut(abs(sin(rdr_in.time))) * px.emission.rgb;
    // }

    color = vec4(Lo + Le, px.albedo.a);
    bloom = vec4(Le, 1.0);
}

#endif
