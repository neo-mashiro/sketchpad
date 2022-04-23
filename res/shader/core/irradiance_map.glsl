#version 460 core

// compute diffuse irradiance map from an HDR environment cubemap texture

#ifdef compute_shader

#include "../utils/projection.glsl"
#include "../utils/sampling.glsl"

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
layout(binding = 0) uniform samplerCube environment_map;
layout(binding = 0, rgba16f) restrict writeonly uniform imageCube irradiance_map;

// this way of sampling over the hemisphere is simple but biased and slow.
vec3 NaiveConvolution(vec3 N, float h_step, float v_step) {
    vec3 irradiance = vec3(0.0);
    uint n_samples = 0;

    // tweak the step size based on the environment map resolution, 1K needs
    // a step size of ~ 0.025, 2K/4K requires 0.01 or even lower, very large
    // step size can lead to noticeable artifacts due to undersampling.
    // h_step: increment on the azimuth angle (longitude)
    // v_step: increment on the polar angle (latitude)

    for (float phi = 0.0; phi < PI2; phi += h_step) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += v_step) {
            vec3 L = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            L = Tangent2World(N, L);
            irradiance += texture(environment_map, L).rgb * cos(theta) * sin(theta);
            n_samples++;
        }
    }

    /* this expression already takes into account the Lambertian diffuse factor `INV_PI`
       o/w we should have multiplied by `PI squared` instead of `PI`, check the formula:
       http://www.codinglabs.net/article_physically_based_rendering.aspx

       if you still wonder why we multiply by `PI`, here's another perspective:
       every texture lookup is weighted by `sin(theta) * cos(theta) = sin(2 * theta) / 2`
       where the polar angle `theta` on the hemisphere ranges from 0 to `PI / 2`. Hence,
       the expected value of all weights is `1 / PI` if you solve the integral, that is,
       the PDF is 1 over PI, therefore we multiply `PI` back to compensate for that.
    */

    return PI * irradiance / float(n_samples);
}

// this way of sampling over the hemisphere is uniform, unbiased and faster.
// for 2K resolution we need ~ 16000 samples to avoid undersampling artifacts
vec3 UniformConvolution(vec3 N, uint n_samples) {
    vec3 irradiance = vec3(0.0);

    for (uint i = 0; i < n_samples; i++) {
        vec2 u = Hammersley2D(i, n_samples);
        vec3 L = UniformSampleHemisphere(u.x, u.y);
        L = Tangent2World(N, L);

        float NoL = max(dot(N, L), 0.0);
        irradiance += texture(environment_map, L).rgb * NoL;
    }

    // every texture lookup is weighted by `NoL` which ranges from 0 to 1,
    // and sampling is uniform over the hemisphere, so the averaged weight
    // is 0.5. To compensate for this, we need to double the result.

    return 2.0 * irradiance / float(n_samples);
}

// this way of sampling over the hemisphere is cosine-weighted, preciser and faster.
// for 2K resolution we only need ~ 8000 samples to achieve nice results.
vec3 CosineConvolution(vec3 N, uint n_samples) {
    vec3 irradiance = vec3(0.0);

    for (uint i = 0; i < n_samples; i++) {
        vec2 u = Hammersley2D(i, n_samples);
        vec3 L = CosineSampleHemisphere(u.x, u.y);
        L = Tangent2World(N, L);
        irradiance += texture(environment_map, L).rgb;
    }

    /* since the sampling is already cosine-weighted, we can directly sum up the retrieved texel
       values and divide by the total number of samples, there's no need to include a weight and
       then balance the result with a multiplier. If we multiply each texel by `NoL` and then
       double the result as we did in uniform sampling, we are essentially weighing the radiance
       twice, in which case the result irradiance map would be less blurred where bright pixels
       appear brighter and dark areas are darker, in fact many people were doing this wrong.
    */

    return irradiance / float(n_samples);
}

void main() {
    ivec3 ils_coordinate = ivec3(gl_GlobalInvocationID);
    vec2 resolution = vec2(imageSize(irradiance_map));
    vec3 N = ILS2Cartesian(ils_coordinate, resolution);

    // here we present 3 different ways of computing diffuse irradiance map from an HDR
    // environment map, all 3 have considered the cosine term in the integral, and will
    // yield results that are hardly distinguishable. The last one uses cosine-weighted
    // sampling, it's a lot more performant and requires much fewer samples to converge.

    // vec3 irradiance = NaiveConvolution(N, 0.01, 0.01);
    // vec3 irradiance = UniformConvolution(N, 16384);
    vec3 irradiance = CosineConvolution(N, 16384);

    imageStore(irradiance_map, ils_coordinate, vec4(irradiance, 1.0));
}

#endif
