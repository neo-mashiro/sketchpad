#if !defined(_PBR_SHADING_H) && defined(_PBR_UNIFORM_H)
#define _PBR_SHADING_H

#include "../utils/extension.glsl"
#include "../utils/easing.glsl"
#include "../utils/sampling.glsl"
#include "../utils/material.glsl"
#include "../utils/postprocess.glsl"

// this shader implements our physically based rendering API, reference:
// https://google.github.io/filament/Filament.html
// https://blog.selfshadow.com/publications/
// https://pbr-book.org/3ed-2018/contents

/* computes the LD term (from prefiltered envmap) in the split-sum equation

   we're using a non-linear mapping between roughness and mipmap level here to
   emphasize specular reflection, this quartic ease in function will map most
   surfaces with a roughness < 0.5 to the base level of the prefiltered envmap.
   depending on the context, people might prefer rough appearance over strongly
   glossy smooth surfaces, this often looks better and much more realistic. In
   this case, you can fit roughness using a cubic ease out function, so that
   most surfaces will sample from higher mip levels, and specular IBL will be
   limited to highly smooth surfaces only.
*/
vec3 ComputeLD(const vec3 R, float roughness) {
    const float max_level = textureQueryLevels(prefilter_map) - 1.0;
    float miplevel = max_level * QuarticEaseIn(roughness);
    return textureLod(prefilter_map, R, miplevel).rgb;
}

/* computes the tangent-binormal-normal (TBN) matrix at the current pixel

   the TBN matrix is used to convert a vector from tangent space to world space
   if tangents and binormals are not provided by VBO as vertex attributes input
   this function will use local partial derivatives to approximate the matrix.
   if tangents and binormals are already provided, we don't need this function
   at all unless the interpolated T, B and N vectors could lose orthogonality.
*/
mat3 ComputeTBN(const vec3 position, const vec3 normal, const vec2 uv) {
    vec3 dpx = dFdxFine(position);
    vec3 dpy = dFdyFine(position);
    vec2 duu = dFdxFine(uv);
    vec2 dvv = dFdyFine(uv);

    vec3 N = normalize(normal);
    vec3 T = normalize(dpx * dvv.t - dpy * duu.t);
    vec3 B = -normalize(cross(N, T));

    return mat3(T, B, N);
}

/* computes the roughness-anisotropy-corrected reflection vector RAR

   for a perfect specular surface, we use the reflection vector R to fetch the IBL.
   for 100% diffuse, normal vector N is used instead as diffuse is view-independent.
   given a roughness ~ [0, 1], we now have a mixture of diffuse and specular, so we
   should interpolate between the two based on how rough the surface is, that gives
   the vector RAR. RAR also takes account of anisotropy, thus the letter A.

   the RAR vector is only meant to compute the specular LD term. As for the diffuse
   part, we should still use the isotropic normal vector N to fetch the precomputed
   irradiance map or evaluate SH9 regardless of anisotropy. Anisotropy only applies
   to brushed metals so it's mainly concerned with specular component.
*/
vec3 ComputeRAR(const Pixel px) {
    vec3 R = px.R;

    if (abs(px.anisotropy) > 1e-5) {
        vec3 aniso_B = px.anisotropy >= 0.0 ? px.aniso_B : px.aniso_T;
        vec3 aniso_T = cross(aniso_B, px.V);
        vec3 aniso_N = cross(aniso_T, aniso_B);  // anisotropic normal

        // the anisotropic pattern fully reveals at a roughness of 0.25
        vec3 N = mix(px.N, aniso_N, abs(px.anisotropy) * clamp(px.roughness * 4, 0.0, 1.0));
        R = reflect(-px.V, normalize(N));
    }

    return mix(R, px.N, px.alpha * px.alpha);
}

/* computes diffuse color from the base color (albedo) and metalness
   for dielectrics, diffuse color is the persistent base color, F0 is achromatic
   for conductors, there's no diffuse color, diffuse = vec3(0), F0 is chromatic
*/
vec3 ComputeAlbedo(const vec3 albedo, float metalness) {
    return albedo * (1.0 - metalness);
}

/* computes F0 from the base color (albedo), metalness and specular
   this is chromatic for conductors and achromatic for dielectrics
*/
vec3 ComputeF0(const vec3 albedo, float metalness, float specular) {
    vec3 dielectric_F0 = vec3(0.16 * specular * specular);
    return mix(dielectric_F0, albedo, metalness);
}

/* computes F0 from the incident IOR and transmitted IOR (index of refraction)
   this assumes an air-dielectric interface where the incident IOR of air = 1

   by convention, nt = transmitted IOR, ni = incident IOR, and the ratio of IOR
   is denoted by eta = nt / ni, but in our case the order of ni and nt does not
   matter as we are taking the expression squared where ni and nt are symmetric
*/
vec3 ComputeF0(float ni, float nt) {
    float x = (nt - ni) / (nt + ni);  // x = (eta - 1) / (eta + 1)
    return vec3(x * x);
}

/* computes IOR of the refraction medium from F0, assumes an air-dielectric interface
   the value returned is really eta = nt / ni, but since ni = 1, it's essentially IOR
*/
float ComputeIOR(vec3 F0) {
    float x = sqrt(F0.x);
    return (1.0 + x) / (1.0 - x);
}

/* computes the bias term that is used to offset the shadow map and remove shadow acne
   we need more bias when NoL is small, less when NoL is large (perpendicular surfaces)
   if the resolution of the shadow map is high, the min/max bias values can be reduced
   N and L must be normalized, and N must be geometric normal, not from the normal map
*/
float ComputeDepthBias(const vec3 L, const vec3 N) {
    const float max_bias = 0.001;
    const float min_bias = 0.0001;
    return max(max_bias * (1.0 - dot(N, L)), min_bias);
}

/* evaluates the path of refraction at the pixel, assumes an air-dielectric interface
   the exact behavior of refraction depends on the medium's interior geometric structures
   for simplicity we only consider uniform solid volumes in the form of spheres or cubes

   if light enters a medium whose volume is a uniform sphere, cylinder or capsule, it is
   spherically distorted, and each point on the surface has a different local thickness.
   local thickness drops from diameter d to 0 as we go from the sphere center to the rim

   if light enters a uniform flat volume such as a cube, plastic bar or glass plate, it's
   not distorted but shifted due to the thickness of the volume. The entry/exit interface
   are symmetric to each other, which implies that the exit direction of light equals the
   entry direction, that's the view vector V. Therefore, when sampling the infinitely far
   environment map, we won't be able to observe the varying shifts as would o/w appear in
   real life. For this, we adopt a cheap solution by adding a hardcoded offset.

   if local light probes were to be used instead of distant IBL, we would need another
   function because sampling local IBL depends on the position as well
*/
void EvalRefraction(const Pixel px, out vec3 transmittance, out vec3 direction) {
    // spherical refraction
    if (px.volume == 0) {
        vec3 r_in = refract(-px.V, px.N, px.eta);
        float NoR = dot(-px.N, r_in);

        float m_thickness = px.thickness * px.NoV;  // local thickness varies
        float r_distance = m_thickness * NoR;

        vec3 T = clamp(exp(-px.absorption * r_distance), 0.0, 1.0);  // Beer–Lambert's law
        vec3 n_out = -normalize(NoR * r_in + px.N * 0.5);  // vector from the exit to sphere center
        vec3 r_out = refract(r_in, n_out, 1.0 / px.eta);

        transmittance = T;
        direction = r_out;
    }

    // cubic or flat refraction
    else if (px.volume == 1) {
        vec3 r_in = refract(-px.V, px.N, px.eta);
        float NoR = dot(-px.N, r_in);

        float m_thickness = px.thickness;  // thickness is constant across the flat surface
        float r_distance = m_thickness / max(NoR, 0.001);  // refracted distance is longer

        vec3 T = clamp(exp(-px.absorption * r_distance), 0.0, 1.0);  // Beer–Lambert's law
        vec3 r_out = normalize(r_in * r_distance - px.V * 10.0);  // a fixed offset of 10.0

        transmittance = T;
        direction = r_out;
    }
}

// evaluates base material's diffuse BRDF lobe
vec3 EvalDiffuseLobe(const Pixel px, float NoV, float NoL, float HoL) {
    return px.diffuse_color * Fd_Burley(px.alpha, NoV, NoL, HoL);
}

// evaluates base material's specular BRDF lobe
vec3 EvalSpecularLobe(const Pixel px, const vec3 L, const vec3 H, float NoV, float NoL, float NoH, float HoL) {
    float D = 0.0;
    float V = 0.0;
    vec3  F = vec3(0.0);

    if (model.x == 3) {  // cloth specular BRDF
        D = D_Charlie(px.alpha, NoH);
        V = V_Neubelt(NoV, NoL);
        F = px.F0;  // replace Fresnel with sheen color to simulate the soft luster
    }
    else if (abs(px.anisotropy) <= 1e-5) {  // non-cloth, isotropic specular BRDF
        D = D_TRGGX(px.alpha, NoH);
        V = V_SmithGGX(px.alpha, NoV, NoL);
        F = F_Schlick(px.F0, HoL);
    }
    else {  // non-cloth, anisotropic specular BRDF
        float HoT = dot(px.aniso_T, H);
        float HoB = dot(px.aniso_B, H);
        float LoT = dot(px.aniso_T, L);
        float LoB = dot(px.aniso_B, L);
        float VoT = dot(px.aniso_T, px.V);
        float VoB = dot(px.aniso_B, px.V);

        // Brent Burley 2012, "Physically Based Shading at Disney"
        // float aspect = sqrt(1.0 - 0.9 * px.anisotropy);
        // float at = max(px.alpha / aspect, 0.002025);  // alpha along the tangent direction
        // float ab = max(px.alpha * aspect, 0.002025);  // alpha along the binormal direction

        // Kulla 2017, "Revisiting Physically Based Shading at Imageworks"
        float at = max(px.alpha * (1.0 + px.anisotropy), 0.002025);  // clamp to 0.045 ^ 2 = 0.002025
        float ab = max(px.alpha * (1.0 - px.anisotropy), 0.002025);

        D = D_AnisoGTR2(at, ab, HoT, HoB, NoH);
        V = V_AnisoSmithGGX(at, ab, VoT, VoB, LoT, LoB, NoV, NoL);
        F = F_Schlick(px.F0, HoL);
    }

    return (D * V) * F;
}

// evaluates the specular BRDF lobe of the additive clearcoat layer
vec3 EvalClearcoatLobe(const Pixel px, float NoH, float HoL, out float Fcc) {
    float D = D_TRGGX(px.clearcoat_alpha, NoH);
    float V = V_Kelemen(HoL);
    vec3  F = F_Schlick(vec3(0.04), HoL) * px.clearcoat;  // assume a fixed IOR of 1.5 (4% F0)
    Fcc = F.x;
    return (D * V) * F;
}

// evaluates the contribution of a white analytical light source of unit intensity
vec3 EvaluateAL(const Pixel px, const vec3 L) {
    float NoL = dot(px.N, L);
    if (NoL <= 0.0) return vec3(0.0);

    vec3 H = normalize(px.V + L);
    vec3 Fr = vec3(0.0);
    vec3 Fd = vec3(0.0);
    vec3 Lo = vec3(0.0);

    float NoV = px.NoV;
    float NoH = max(dot(px.N, H), 0.0);
    float HoL = max(dot(H, L), 0.0);

    if (model.x == 1) {  // standard model
        Fr = EvalSpecularLobe(px, L, H, NoV, NoL, NoH, HoL) * px.Ec;  // compensate energy
        Fd = EvalDiffuseLobe(px, NoV, NoL, HoL);
        Lo = (Fd + Fr) * NoL;
    }
    else if (model.x == 2) {  // refraction model
        Fr = EvalSpecularLobe(px, L, H, NoV, NoL, NoH, HoL) * px.Ec;  // compensate energy
        Fd = EvalDiffuseLobe(px, NoV, NoL, HoL) * (1.0 - px.transmission);
        Lo = (Fd + Fr) * NoL;
    }
    else if (model.x == 3) {  // cloth model
        Fr = EvalSpecularLobe(px, L, H, NoV, NoL, NoH, HoL);  // cloth specular needs no compensation
        Fd = EvalDiffuseLobe(px, NoV, NoL, HoL) * clamp01(px.subsurface_color + NoL);  // hack subsurface color
        float cloth_NoL = Fd_Wrap(NoL, 0.5);  // simulate subsurface scattering
        Lo = Fd * cloth_NoL + Fr * NoL;
    }

    if (model.y == 1) {  // additive clearcoat layer
        float NoLcc = max(dot(px.GN, L), 0.0);  // use geometric normal
        float NoHcc = max(dot(px.GN, H), 0.0);  // use geometric normal
        float Fcc = 0.0;
        // clearcoat only has a specular lobe, diffuse is hacked by overwriting the base roughness
        vec3 Fr_cc = EvalClearcoatLobe(px, NoHcc, HoL, Fcc);
        Lo *= (1.0 - Fcc);
        Lo += (Fr_cc * NoLcc);
    }

    return Lo;
}

/*********************************** MAIN API ***********************************/

// initializes the current pixel (fragment), values are computed from the material inputs
void InitPixel(inout Pixel px, const vec3 camera_pos) {
    px.position = px._position;
    px.uv = px._uv * uv_scale;
    px.TBN = px._has_tbn ? mat3(px._tangent, px._binormal, px._normal) :
        ComputeTBN(px._position, px._normal, px.uv);  // approximate using partial derivatives

    px.V = normalize(camera_pos - px.position);
    px.N = sample_normal ? normalize(px.TBN * (texture(normal_map, px.uv).rgb * 2.0 - 1.0)) : px._normal;
    px.R = reflect(-px.V, px.N);
    px.NoV = max(dot(px.N, px.V), 1e-4);

    px.GN = px._normal;  // geometric normal vector, unaffected by normal map
    px.GR = reflect(-px.V, px._normal);  // geometric reflection vector

    px.albedo = sample_albedo ? vec4(Gamma2Linear(texture(albedo_map, px.uv).rgb), 1.0) : albedo;
    px.albedo.a = sample_opacity ? texture(opacity_map, px.uv).r : px.albedo.a;
    // px.albedo.rgb *= px.albedo.a;  // pre-multiply alpha channel

    if (px.albedo.a < alpha_mask) {
        discard;
    }

    px.roughness = sample_roughness ? texture(roughness_map, px.uv).r : roughness;
    px.roughness = clamp(px.roughness, 0.045, 1.0);
    px.alpha = pow2(px.roughness);

    px.ao = sample_ao ? texture(ao_map, px.uv).rrr : vec3(ao);
    px.emission = sample_emission ? vec4(Gamma2Linear(texture(emission_map, px.uv).rgb), 1.0) : emission;
    px.DFG = texture(BRDF_LUT, vec2(px.NoV, px.roughness)).rgb;

    // standard model, insulators or metals, with optional anisotropy
    if (model.x == 1) {
        px.metalness = sample_metallic ? texture(metallic_map, px.uv).r : metalness;
        px.specular = clamp(specular, 0.35, 1.0);
        px.diffuse_color = ComputeAlbedo(px.albedo.rgb, px.metalness);
        px.F0 = ComputeF0(px.albedo.rgb, px.metalness, px.specular);
        px.anisotropy = anisotropy;
        px.aniso_T = sample_anisotan ? texture(anisotan_map, px.uv).rgb : aniso_dir;
        px.aniso_T = normalize(px.TBN * px.aniso_T);
        px.aniso_B = normalize(cross(px._normal, px.aniso_T));  // use geometric normal instead of normal map
        px.Ec = 1.0 + px.F0 * (1.0 / px.DFG.y - 1.0);  // energy compensation factor >= 1.0
    }

    // refraction model, for isotropic dielectrics only
    else if (model.x == 2) {
        px.anisotropy = 0.0;
        px.metalness = 0.0;
        px.diffuse_color = px.albedo.rgb;
        px.F0 = ComputeF0(1.0, clamp(ior, 1.0, 2.33));  // no real-world ior is > 2.33 (diamonds)
        px.Ec = 1.0 + px.F0 * (1.0 / px.DFG.y - 1.0);

        px.eta = 1.0 / ior;  // air -> dielectric
        px.transmission = clamp01(transmission);

        // note that transmission distance defines how far the light can travel through the medium
        // for dense medium with high IOR, light is bent more and attenuates notably as it travels
        // so `tr_distance` should be set small, otherwise it should be set large, do not clamp it
        // to a maximum of thickness as it could literally goes to infinity (e.g. in the vacuum)

        px.absorption = -log(clamp(transmittance, 1e-5, 1.0)) / max(1e-5, tr_distance);
        px.thickness = max(thickness, 1e-5);  // max thickness of the volume, not per pixel
        px.volume = clamp(volume_type, uint(0), uint(1));
    }

    // cloth model, single-layer isotropic dielectrics w/o refraction
    else if (model.x == 3) {
        px.anisotropy = 0.0;
        px.metalness = 0.0;

        if (!sample_roughness) {
            px.roughness = px.roughness * 0.2 + 0.8;  // cloth roughness under 0.8 is unrealistic
            px.alpha = pow2(px.roughness);
        }

        px.diffuse_color = px.albedo.rgb;  // use base color as diffuse color
        px.F0 = sheen_color;  // use sheen color as specular F0
        px.subsurface_color = subsurface_color;
        px.Ec = vec3(1.0);  // subsurface scattering loses energy so needs no compensation
    }

    // additive clear coat layer
    if (model.y == 1) {
        px.clearcoat = clearcoat;
        px.clearcoat_roughness = clamp(clearcoat_roughness, 0.045, 1.0);
        px.clearcoat_alpha = pow2(px.clearcoat_roughness);

        // if the coat layer is rougher, it should overwrite the base roughness
        float max_roughness = max(px.roughness, px.clearcoat_roughness);
        float mix_roughness = mix(px.roughness, max_roughness, px.clearcoat);
        px.roughness = clamp(mix_roughness, 0.045, 1.0);
        px.alpha = pow2(px.roughness);
    }
}

// evaluates the contribution of environment IBL at the pixel
vec3 EvaluateIBL(const Pixel px) {
    vec3 Fr = vec3(0.0);  // specular reflection (the Fresnel effect), weighted by E
    vec3 Fd = vec3(0.0);  // diffuse reflection, weighted by (1 - E) * (1 - transmission)
    vec3 Ft = vec3(0.0);  // diffuse refraction, weighted by (1 - E) * transmission

    vec3 E = vec3(0.0);  // specular BRDF's total energy contribution (integral after the LD term)
    vec3 AO = px.ao;     // diffuse ambient occlusion

    if (model.x == 3) {  // cloth model
        E = px.F0 * px.DFG.z;
        AO *= MultiBounceGTAO(AO.r, px.diffuse_color);
        AO *= Fd_Wrap(px.NoV, 0.5);  // simulate subsurface scattering with a wrap diffuse term
        AO *= clamp01(px.subsurface_color + px.NoV);  // simulate subsurface color (cheap hack)
    }
    else {
        E = mix(px.DFG.xxx, px.DFG.yyy, px.F0);
        AO *= MultiBounceGTAO(AO.r, px.diffuse_color);
    }

    Fr = ComputeLD(ComputeRAR(px), px.roughness) * E;
    Fr *= px.Ec;  // apply multi-scattering energy compensation (Kulla-Conty 17 and Lagarde 18)

    // the irradiance map already includes the Lambertian BRDF so we multiply the texel by
    // diffuse color directly. Do not divide by PI here cause that will be double-counting
    // for spherical harmonics, INV_PI should be rolled into SH9 during C++ precomputation

    Fd = texture(irradiance_map, px.N).rgb * px.diffuse_color * (1.0 - E);
    Fd *= AO;  // apply ambient occlussion and multi-scattering colored GTAO

    if (model.x == 2) {  // refraction model
        vec3 transmittance;
        vec3 r_out;
        EvalRefraction(px, transmittance, r_out);

        Ft = ComputeLD(r_out, px.roughness) * px.diffuse_color;
        Ft *= transmittance;  // apply absorption (transmittance color may differ from the base albedo)

        // note that reflection and refraction are mutually exclusive, photons that bounce off
        // the surface do not enter the object, so the presence of refraction will only eat up
        // some of diffuse's contribution but will not affect the specular part

        Fd *= (1.0 - px.transmission);  // already multiplied by (1.0 - E)
        Ft *= (1.0 - E) * px.transmission;
    }

    if (model.y == 1) {  // additive clear coat layer
        float Fcc = F_Schlick(vec3(0.04), px.NoV).x * px.clearcoat;  // polyurethane F0 = 4%
        Fd *= (1.0 - Fcc);
        Fr *= (1.0 - Fcc);
        Fr += ComputeLD(px.GR, px.clearcoat_roughness) * Fcc;
    }

    return Fr + Fd + Ft;
}

// evaluates the contribution of a white directional light of unit intensity
vec3 EvaluateADL(const Pixel px, const vec3 L, float visibility) {
    return visibility <= 0.0 ? vec3(0.0) : (EvaluateAL(px, L) * visibility);
}

// evaluates the contribution of a white point light of unit intensity
vec3 EvaluateAPL(const Pixel px, const vec3 position, float range, float linear, float quadratic, float visibility) {
    vec3 L = normalize(position - px.position);

    // distance attenuation: inverse square falloff
    float d = distance(position, px.position);
    float attenuation = (d >= range) ? 0.0 : (1.0 / (1.0 + linear * d + quadratic * d * d));

    return (attenuation <= 0.0 || visibility <= 0.0) ? vec3(0.0) : (attenuation * visibility * EvaluateAL(px, L));
}

// evaluates the contribution of a white spotlight of unit intensity
vec3 EvaluateASL(const Pixel px, const vec3 pos, const vec3 dir, float range, float inner_cos, float outer_cos) {
    vec3 l = pos - px.position;
    vec3 L = normalize(l);

    // distance attenuation uses a cheap linear falloff (does not follow the inverse square law)
    float ds = dot(dir, l);  // projected distance along the spotlight beam direction
    float da = 1.0 - clamp01(ds / range);

    // angular attenuation fades out from the inner to the outer cone
    float cosine = dot(dir, L);
    float aa = clamp01((cosine - outer_cos) / (inner_cos - outer_cos));
    float attenuation = da * aa;

    return attenuation <= 0.0 ? vec3(0.0) : (EvaluateAL(px, L) * attenuation);
}

/* evaluates the amount of occlusion for a single light source using the PCSS algorithm

   this function works with omni-directional SM in linear space, the shadow map must be
   a cubemap depth texture that stores linear values, note that this is just a hack for
   casting soft shadows from a point light, spotlight or directional light, but in real
   life they really should be hard shadows since only area lights can cast soft shadows.

   for PCF, texels are picked using Poisson disk sampling which favors samples that are
   more nearby, it can preserve the shadow shape very well even when `n_samples` or the
   search radius is large, which isn't true for uniform disk sampling where shadows are
   often overly blurred. To increase shadow softness, you can use a larger light radius.

   https://sites.cs.ucsb.edu/~lingqi/teaching/resources/GAMES202_Lecture_03.pdf
   https://developer.download.nvidia.cn/shaderlibrary/docs/shadow_PCSS.pdf
   https://developer.download.nvidia.cn/whitepapers/2008/PCSS_Integration.pdf
   https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
*/
float EvalOcclusion(const Pixel px, in samplerCube shadow_map, const vec3 light_pos, float light_radius) {
    const float near_clip = 0.1;
    const float far_clip = 100.0;
    vec3 l = light_pos - px.position;
    vec3 L = normalize(l);
    float depth = length(l) / far_clip;  // linear depth of the current pixel

    // find a tangent T and binormal B such that T, B and L are orthogonal to each other
    // there's an infinite set of possible Ts and Bs, but we can choose them arbitrarily
    // because sampling on unit disk is symmetrical (T and B define the axes of the disk)
    // just pick a vector U that's not collinear with L, then cross(U, L) will give us T

    vec3 U = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), step(abs(L.y), 0.999));
    vec3 T = normalize(cross(U, -L));
    vec3 B = normalize(cross(-L, T));

    // option 1: generate 16 random Poisson samples for each pixel (very slow at runtime)
    // const int n_samples = 16;
    // vec2 samples[16];
    // vec3 scale = abs(l);
    // float seed = min3(scale) / max3(scale);
    // PoissonSampleDisk(seed, samples);

    // option 2: use a pre-computed Poisson disk of 16 samples (much faster)
    const int n_samples = 16;
    const vec2 samples[16] = vec2[] (  // source: Nvidia 2008, "PCSS Integration whitepaper"
        vec2(-0.94201624, -0.39906216), vec2( 0.94558609, -0.76890725),
        vec2(-0.09418410, -0.92938870), vec2( 0.34495938,  0.29387760),
        vec2(-0.91588581,  0.45771432), vec2(-0.81544232, -0.87912464),
        vec2(-0.38277543,  0.27676845), vec2( 0.97484398,  0.75648379),
        vec2( 0.44323325, -0.97511554), vec2( 0.53742981, -0.47373420),
        vec2(-0.26496911, -0.41893023), vec2( 0.79197514,  0.19090188),
        vec2(-0.24188840,  0.99706507), vec2(-0.81409955,  0.91437590),
        vec2( 0.19984126,  0.78641367), vec2( 0.14383161, -0.14100790)
    );

    // sample the unit disk, count blockers and total depth
    float search_radius = light_radius * depth;  // if pixel is far, search in a larger area
    float total_depth = 0.0;
    int n_blocker = 0;

    for (int i = 0; i < n_samples; ++i) {
        vec2 offset = samples[i];
        vec3 v = -L + (offset.x * T + offset.y * B) * search_radius;
        float sm_depth = texture(shadow_map, v).r;

        if (depth > sm_depth) {  // in this step we don't need a bias
            total_depth += sm_depth;
            n_blocker++;
        }
    }

    // early out if no blockers are found (100% visible)
    if (n_blocker == 0) {
        return 0.0;
    }

    // compute the average blocker depth and penumbra size
    float z_blocker = total_depth / float(n_blocker);
    float penumbra = (depth - z_blocker) / z_blocker;  // ~ [0.0, 1.0]

    // compute occlusion with PCF, this time use penumbra to determine the kernel size
    float PCF_radius = light_radius * penumbra;
    float bias = ComputeDepthBias(L, px.GN);
    float occlusion = 0.0;

    for (int i = 0; i < n_samples; ++i) {
        vec2 offset = samples[i];
        vec3 v = -L + (offset.x * T + offset.y * B) * PCF_radius;
        float sm_depth = texture(shadow_map, v).r;

        if (depth - bias > sm_depth) {
            occlusion += 1.0;
        }
    }

    return occlusion / float(n_samples);
}

#endif