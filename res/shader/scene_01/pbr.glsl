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

layout(std430, binding = 0) readonly buffer Color    { vec4  pl_color[];    };
layout(std430, binding = 1) readonly buffer Position { vec4  pl_position[]; };
layout(std430, binding = 2) readonly buffer Range    { float pl_range[];    };
layout(std430, binding = 3) readonly buffer Index    { int   pl_index[];    };

layout(std140, binding = 1) uniform DL {
    vec4  color;
    vec4  direction;
    float intensity;
} dl;

layout(std140, binding = 2) uniform SL {
    vec4  color;
    vec4  position;
    vec4  direction;
    float intensity;
    float inner_cos;
    float outer_cos;
    float range;
} sl;

layout(std140, binding = 3) uniform OL {
    vec4  color;
    vec4  position;
    float intensity;
    float linear;
    float quadratic;
    float range;
} ol;

layout(std140, binding = 4) uniform PL {
    float intensity;
    float linear;
    float quadratic;
} pl;

const uint n_pls = 28;  // 28 static point lights in light culling
const uint tile_size = 16;

// find out which tile this pixel belongs to and its starting offset in the "Index" SSBO
uint GetTileOffset() {
    ivec2 tile_id = ivec2(gl_FragCoord.xy) / ivec2(tile_size);
    uint n_cols = rdr_in.resolution.x / tile_size;
    uint tile_index = tile_id.y * n_cols + tile_id.x;
    return tile_index * n_pls;
}

void main() {
    // in the depth prepass, we don't draw anything in the fragment shader
    if (rdr_in.depth_prepass) {
        return;
    }

    Pixel px;
    px._position = _position;
    px._normal   = _normal;
    px._uv       = _uv;
    px._has_tbn  = true;

    InitPixel(px, camera.position.xyz);

    vec3 Lo = vec3(0.0);
    vec3 Le = vec3(0.0);  // emission

    // contribution of directional light
    Lo += EvaluateADL(px, dl.direction.xyz, 1.0) * dl.color.rgb * dl.intensity;

    // contribution of camera flashlight
    vec3 sc = EvaluateASL(px, sl.position.xyz, sl.direction.xyz, sl.range, sl.inner_cos, sl.outer_cos);
    Lo += sc * sl.color.rgb * sl.intensity;

    // contribution of orbit light
    vec3 oc = EvaluateAPL(px, ol.position.xyz, ol.range, ol.linear, ol.quadratic, 1.0);
    Lo += oc * ol.color.rgb * ol.intensity;

    // contribution of point lights x 28 (if not culled and visible)
    uint offset = GetTileOffset();
    for (uint i = 0; i < n_pls && pl_index[offset + i] != -1; ++i) {
        int index = pl_index[offset + i];
        vec3 pc = EvaluateAPL(px, pl_position[index].xyz, pl_range[index], pl.linear, pl.quadratic, 1.0);
        Lo += pc * pl_color[index].rgb * pl.intensity;
    }

    if (self.material_id == 6) {  // runestone platform emission
        Le = CircularEaseInOut(abs(sin(rdr_in.time * 2.0))) * px.emission.rgb;
    }

    color = vec4(Lo + Le, px.albedo.a);
    bloom = vec4(Le, 1.0);
}

#endif
