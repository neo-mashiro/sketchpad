#version 460 core

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 _spherical_position;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform mat4 projection;

void main() {
    _spherical_position = position;
    gl_Position = projection * view * vec4(position, 1.0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec3 _spherical_position;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform samplerCube environment_map;

// for 2K resolution we need ~ 16000 samples to avoid undersampling artifacts
const uint n_samples = 16384;
const float ratio = 1.0 / float(n_samples);

#include sample.glsl
#include projection.glsl

vec2 Hammersley2DX(uint i) {
    return vec2(float(i) * ratio, RadicalInverse_VdC(i));
}

// this way of sampling over the hemisphere is simple but biased and slow.
// formula: http://www.codinglabs.net/article_physically_based_rendering.aspx
vec3 NaiveConvolution(vec3 N, float h_step, float v_step) {
    // note: make sure to tweak the steps based on your environment map resolution, if the
    // steps are too large, there will be noticeable artifacts due to undersampling. 0.025
    // should be fine for 1K resolution, but 2K/4K resolution may need 0.005 or even lower.

    // `h_step`: increment on the azimuth angle (longitude)
    // `v_step`: increment on the polar angle (latitude)

    vec3 irradiance = vec3(0.0);
    float n_samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += h_step) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += v_step) {
            vec3 L = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            L = Tangent2World(N, L);
            irradiance += texture(environment_map, L).rgb * cos(theta) * sin(theta);
            n_samples++;
        }
    }

    return PI * irradiance * (1.0 / n_samples);
}

// this way of sampling over the hemisphere is uniform, unbiased and faster.
vec3 UniformConvolution(vec3 N) {
    vec3 irradiance = vec3(0.0);

    for (uint i = 0; i < n_samples; i++) {
        vec2 x = Hammersley2DX(i);
        vec3 L = UniformSampleHemisphere(x.x, x.y);
        L = Tangent2World(N, L);

        float NoL = max(dot(N, L), 0.0);
        irradiance += texture(environment_map, L).rgb * NoL;
    }

    return 2.0 * ratio * irradiance;
}

// this way of sampling over the hemisphere is cosine-weighted, preciser and faster.
vec3 CosineConvolution(vec3 N) {
    vec3 irradiance = vec3(0.0);

    for (uint i = 0; i < n_samples; i++) {
        vec2 x = Hammersley2DX(i);
        vec3 L = CosineSampleHemisphere(x.x, x.y);
        L = Tangent2World(N, L);

        float NoL = max(dot(N, L), 0.0);
        irradiance += texture(environment_map, L).rgb * NoL;
    }

    return 2.0 * ratio * irradiance;
}

void main() {
    vec3 N = normalize(_spherical_position);

    vec3 irradiance = NaiveConvolution(N, 0.005, 0.005);
    // vec3 irradiance = UniformConvolution(N);
    // vec3 irradiance = CosineConvolution(N);

    color = vec4(irradiance, 1.0);
}

#endif
