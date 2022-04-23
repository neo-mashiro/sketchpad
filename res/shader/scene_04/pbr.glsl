#version 460 core
#pragma optimize(off)

layout(std140, binding = 0) uniform Camera {
    vec4 position;
    vec4 direction;
    mat4 view;
    mat4 projection;
} camera;

#include "../core/renderer_input.glsl"
#include "../core/wireframe.glsl"

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
    _position = vec3(self.transform * vec4(position, 1.0));
    _normal = normalize(vec3(self.transform * vec4(normal, 0.0)));
    _uv = uv;
    _uv2 = uv;
    _tangent = normalize(vec3(self.transform * vec4(tangent, 0.0)));
    _binormal = normalize(vec3(self.transform * vec4(binormal, 0.0)));

    gl_Position = camera.projection * camera.view * self.transform * vec4(position, 1.0);
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
    noperspective in vec3 _distance;
};

layout(location = 0) out vec4 color;

layout(location = 0) uniform bool draw_wireframe;
layout(location = 1) uniform vec4 line_color;
layout(location = 2) uniform float line_width;
layout(location = 3) uniform float ibl_exposure;

layout(std140, binding = 1) uniform DL {
    vec4  color;
    vec4  direction;
    float intensity;
} dl;

void main() {
    Pixel px;
    px._position = _position;
    px._normal   = _normal;
    px._uv       = _uv;
    px._has_tbn  = false;

    InitPixel(px, camera.position.xyz);

    vec3 Lo = vec3(0.0);
    vec3 Le = vec3(0.0);  // emission

    Lo += EvaluateIBL(px) * max(ibl_exposure, 0.5);
    Lo += EvaluateADL(px, dl.direction.xyz, 1.0) * dl.color.rgb * dl.intensity;

    // for (uint i = 0; i < 99; i++) {
    //     vec3 pc = EvaluateAPL(px, pl[i].position.xyz, pl[i].range, pl[i].linear, pl[i].quadratic, 1.0);
    //     Lo += pc * pl.color.rgb * pl.intensity;
    // }

    // vec3 sc = EvaluateASL(px, sl.position.xyz, sl.direction.xyz, sl.range, sl.inner_cos, sl.outer_cos);
    // Lo += sc * sl.color.rgb * sl.intensity;

    // if (self.material_id == 123) {
    //     Le = CircularEaseInOut(abs(sin(rdr_in.time))) * px.emission.rgb;
    // }

    color = vec4(Lo + Le, px.albedo.a);

    if (draw_wireframe && gl_FrontFacing) {
        float t = smoothstep(line_width - 1, line_width + 1, min3(_distance));
        color = mix(line_color, color, t);
    }
}

#endif
