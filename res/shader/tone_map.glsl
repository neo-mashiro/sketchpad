// this file can be included by your shader to use a list of simple tone mapping operators.

// tone mapping maps an input color in high dynamic range to an output color in low dynamic
// range, this is necessary because most monitors are only able to display LDR colors and
// the graphics library will simply clamp the output RGB colors to [0,1] range. Speaking of
// colors, we are really referring to radiance values represented by RGB vec3.

// references:
//   https://64.github.io/tonemapping/
//   https://docs.unrealengine.com/4.26/en-US/RenderingAndGraphics/PostProcessEffects/ColorGrading/

vec3 Lerp(vec3 a, vec3 b, vec3 t) {
    return vec3(mix(a.r, b.r, t.r), mix(a.g, b.g, t.g), mix(a.b, b.b, t.b));
}

float Luminance(vec3 irradiance) {
    return dot(irradiance, vec3(0.2126, 0.7152, 0.0722));
}

vec3 Reinhard(vec3 irradiance) {
    return irradiance / (1.0 + irradiance);
}

vec3 ReinhardLuminance(vec3 irradiance, float max_luminance) {
    float li = Luminance(irradiance);
    float numerator = li * (1.0 + (li / (max_luminance * max_luminance)));
    float lo = numerator / (1.0 + li);
    return irradiance * (lo / li);
}

vec3 ReinhardJodie(vec3 irradiance) {
    vec3 t = irradiance / (1.0 + irradiance);
    return Lerp(irradiance / (1.0 + Luminance(irradiance)), t, t);
}

vec3 Uncharted2Partial(vec3 x) {
    float a = 0.15;
    float b = 0.50;
    float c = 0.10;
    float d = 0.20;
    float e = 0.02;
    float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e/f;
}

vec3 Uncharted2Filmic(vec3 irradiance) {
    float exposure_bias = 2.0;
    vec3 white_scale = vec3(1.0) / Uncharted2Partial(vec3(11.2));
    vec3 c = Uncharted2Partial(irradiance * exposure_bias);
    return c * white_scale;
}

// the default TMO used by Unreal Engine 4
vec3 ApproxACES(vec3 irradiance) {
    vec3 v = irradiance * 0.6;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}
