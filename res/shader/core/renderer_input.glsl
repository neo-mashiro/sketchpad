#ifndef _RENDERER_H
#define _RENDERER_H

// uniform blocks >= 10 are reserved for internal use only
layout(std140, binding = 10) uniform RendererInput {
    ivec2 resolution;     // viewport size in pixels
    ivec2 cursor_pos;     // cursor position relative to viewport's upper-left corner
    float near_clip;      // frustum near clip distance
    float far_clip;       // frustum far clip distance
    float time;           // number of seconds since window created
    float delta_time;     // number of seconds since the last frame
    bool  depth_prepass;  // early z test
    uint  shadow_index;   // index of the shadow map render pass (one index per light source)
} rdr_in;

// default-block (loose) uniform locations >= 1000 are reserved for internal use only
struct self_t {
    mat4 transform;    // 1000, model matrix of the current entity
    uint material_id;  // 1001, current mesh's material id
    uint ext_1002;
    uint ext_1003;
    uint ext_1004;
    uint ext_1005;
    uint ext_1006;
    uint ext_1007;
};

layout(location = 1000) uniform self_t self;

#endif