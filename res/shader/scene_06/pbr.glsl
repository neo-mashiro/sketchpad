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

layout(location = 0) out _vtx {
    out vec3 _position;
    out vec3 _normal;
    out vec2 _uv;
    out vec2 _uv2;
    out vec3 _tangent;
    out vec3 _binormal;
};

void main() {
    gl_Position = camera.projection * camera.view * self.transform * vec4(position, 1.0);

    _position = vec3(self.transform * vec4(position, 1.0));
    _normal   = normalize(vec3(self.transform * vec4(normal, 0.0)));
    _tangent  = normalize(vec3(self.transform * vec4(tangent, 0.0)));
    _binormal = normalize(vec3(self.transform * vec4(binormal, 0.0)));
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
    vec4  color[4];
    vec4  position[4];
    float intensity[4];
    float linear[4];
    float quadratic[4];
    float range[4];
} pl;

// layout(std140, binding = 2) uniform SL {
//     vec4  color;
//     vec4  position;
//     vec4  direction;
//     float intensity;
//     float inner_cos;
//     float outer_cos;
//     float range;
// } sl;

// layout(std140, binding = 3) uniform DL {
//     vec4  color;
//     vec4  direction[5];
//     float intensity;
// } dl;

layout(location = 0) uniform float ibl_exposure;
layout(location = 1) uniform bool enable_pl;

void main() {
    Pixel px;
    px._position = _position;
    px._normal   = _normal;
    px._uv       = _uv;
    px._has_tbn  = true;

    InitPixel(px, camera.position.xyz);

    vec3 Lo = vec3(0.0);
    vec3 Le = vec3(0.0);  // emission

    Lo += EvaluateIBL(px) * ibl_exposure;

    // if (enable_moonlight) {
    //     for (uint i = 0; i < 5; ++i) {
    //         Lo += EvaluateADL(px, dl.direction[i].xyz, 1.0) * dl.color.rgb * dl.intensity;
    //     }
    // }

    if (enable_pl) {
        for (uint i = 0; i < 4; ++i) {
            if (dot(_normal, pl.position[i].xyz - _position) < 0.0) {
                continue;  // skip pixels on the exterior of the cathedral
            }
            vec3 pc = EvaluateAPL(px, pl.position[i].xyz, pl.range[i], pl.linear[i], pl.quadratic[i], 1.0);
            Lo += pc * pl.color[i].rgb * pl.intensity[i];
        }
    }

    // if (enable_spotlight) {
    //     vec3 sc = EvaluateASL(px, sl.position.xyz, sl.direction.xyz, sl.range, sl.inner_cos, sl.outer_cos);
    //     Lo += sc * sl.color.rgb * sl.intensity * step(-8.0, _position.z);
    // }

    // if (self.material_id == 123) {
    //     Le = CircularEaseInOut(abs(sin(rdr_in.time))) * px.emission.rgb;
    // }

    color = vec4(Lo + Le, px.albedo.a);
    bloom = vec4(Le, 1.0);
}

#endif
