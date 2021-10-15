#version 460
#pragma optimize(off)

// -----------------------------------------------------------------------------
// uniform blocks (globally shared)
// -----------------------------------------------------------------------------
layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;  // main camera

layout(std140, binding = 1) uniform DLT {
    vec3 color;
    vec3 direction;
    float intensity;
} DL[2];  // directional light x 2

layout(std140, binding = 3) uniform PLT {
    vec3 color;
    vec3 position;
    float intensity;
    float linear;
    float quadratic;
    float range;
} PL[4];  // point light x 4

layout(std140, binding = 7) uniform SLT {
    vec3 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float inner_cosine;
    float outer_cosine;
    float range;
} SL[4];  // spotlight x 4

layout(std140, binding = 11) uniform Config {
    vec3 v1; int i1;
    vec3 v2; int i2;
    vec3 v3; int i3;
    int i4; int i5; int i6;
    float f1; float f2; float f3;
};  // configuration block

// -----------------------------------------------------------------------------
// loose uniforms (locally scoped)
// -----------------------------------------------------------------------------
layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform vec3 albedo;
layout(location = 2) uniform vec3 metallic;
layout(location = 3) uniform vec3 smoothness;

// -----------------------------------------------------------------------------
// uniform samplers (textures data)
// -----------------------------------------------------------------------------
layout(binding = 0) uniform sampler2D albedo_map;
layout(binding = 1) uniform sampler2D normal_map;
layout(binding = 2) uniform sampler2D height_map;
layout(binding = 3) uniform sampler2D emission_map;

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

// vertex attributes in local model space
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 uv2;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

// vertex attributes in global world space or local tangent space
layout(location = 0) out __ {
    out vec3 _position;
    out vec3 _normal;
    out vec2 _uv;
    out vec2 _uv2;
    out vec3 _tangent;
    out vec3 _bitangent;
};

void main() {
    // model space -> world space
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

// vertex attributes in global world space
layout(location = 0) in __ {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _bitangent;
};

// fragment shader outputs
layout(location = 0) out vec4 color;

// light factors (directional light)
vec3 DLColor(uint i) {
    return DL[i].intensity * DL[i].color;
}

// light factors (point light)
vec3 PLColor(uint i, vec3 frag_pos) {
    // the point light attenuation follows the inverse-square law
    float d = distance(PL[i].position, frag_pos);
    float attenuation = d >= PL[i].range ? 0.0 : 1.0 / (1.0 + PL[i].linear * d + PL[i].quadratic * d * d);
    return attenuation * PL[i].intensity * PL[i].color;
}

// light factors (spotlight)
vec3 SLColor(uint i, vec3 frag_pos) {
    // the spotlight distance attenuation uses a linear falloff
    vec3 ray = frag_pos - SL[i].position;  // inward ray from the light to the fragment
    float projected_distance = dot(SL[i].direction, ray);
    float linear_attenuation = 1.0 - clamp(projected_distance / SL[i].range, 0.0, 1.0);

    // the spotlight angular attenuation fades out from the inner to the outer cone
    float cosine = dot(SL[i].direction, normalize(ray));
    float angular_diff = SL[i].inner_cosine - SL[i].outer_cosine;
    float angular_attenuation = clamp((cosine - SL[i].outer_cosine) / angular_diff, 0.0, 1.0);

    return linear_attenuation * angular_attenuation * SL[i].intensity * SL[i].color;
}

void main() {
    // use all the uniform blocks to prevent them from being optimized out
    vec3 dummy0 = camera.position;
    vec3 dummy1 = PL[0].color;
    vec3 dummy2 = DL[0].color;
    vec3 dummy3 = SL[0].color;
    vec3 dummy4 = v1;

    color = vec4(1.0);
}

# endif
