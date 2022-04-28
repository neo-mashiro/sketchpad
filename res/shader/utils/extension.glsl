#ifndef _EXT_H
#define _EXT_H

////////////////////////////////////////////////////////////////////////////////

#define EPS      1e-5
#define PI       3.141592653589793
#define PI2      6.283185307179586
#define INV_PI   0.318309886183791  // 1 over PI
#define HLF_PI   1.570796326794897  // half PI
#define SQRT2    1.414213562373095
#define SQRT3    1.732050807568877
#define SQRT5    2.236067977499789
#define CBRT2    1.259921049894873  // cube root 2
#define CBRT3    1.442249570307408  // cube root 3
#define G_PHI    1.618033988749894  // golden ratio
#define EULER_E  2.718281828459045  // natural exponent e
#define LN2      0.693147180559945
#define LN10     2.302585092994046
#define INV_LN2  1.442695040888963  // 1 over ln2
#define INV_LN10 0.434294481903252  // 1 over ln10

#define clamp01(x) clamp(x, 0.0, 1.0)

////////////////////////////////////////////////////////////////////////////////

// computes the min/max component of a vec2/vec3/vec4
float min2(const vec2 v) { return min(v.x, v.y); }
float max2(const vec2 v) { return max(v.x, v.y); }
float min3(const vec3 v) { return min(min(v.x, v.y), v.z); }
float max3(const vec3 v) { return max(max(v.x, v.y), v.z); }
float min4(const vec4 v) { return min(min3(v.xyz), v.w); }
float max4(const vec4 v) { return max(max3(v.xyz), v.w); }

// logarithm base 10 and logarithm base 2
float log10(float x) { return log(x) * INV_LN10; }
float log2(float x) { return log(x) * INV_LN2; }

// checks if value x is in range (a, b), returns value type to avoid branching
float step3(float a, float x, float b) { return step(a, x) * step(x, b); }
vec2 step3(const vec2 a, const vec2 x, const vec2 b) { return step(a, x) - step(b, x); }
vec3 step3(const vec3 a, const vec3 x, const vec3 b) { return step(a, x) - step(b, x); }
vec4 step3(const vec4 a, const vec4 x, const vec4 b) { return step(a, x) - step(b, x); }

// optimizes lower power functions, removes the implicit `exp/log` call
float pow2(float x) { return x * x; }
float pow3(float x) { return x * x * x; }
float pow4(float x) { return x * x * x * x; }
float pow5(float x) { return x * x * x * x * x; }

// computes the luminance of a linear RGB color, sRGB must be converted to linear first
float luminance(const vec3 linear_rgb) {
    return dot(linear_rgb, vec3(0.2126, 0.7152, 0.0722));
}

vec3 hsl2rgb(float h, float s, float l) {
    vec3 u = mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0);
    vec3 v = abs(u - 3.0) - 1.0;
    return l + s * (clamp01(v) - 0.5) * (1.0 - abs(2.0 * l - 1.0));
}

vec3 hsv2rgb(float h, float s, float v) {
    if (s <= 1e-4) return vec3(v);  // zero saturation = grayscale color

    float x = fract(h) * 6.0;
    float f = fract(x);
    uint  i = uint(x);

    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i) {
        case 0u: return vec3(v, t, p);
        case 1u: return vec3(q, v, p);
        case 2u: return vec3(p, v, t);
        case 3u: return vec3(p, q, v);
        case 4u: return vec3(t, p, v);
        default: return vec3(v, p, q);
    }
}

vec3 hsl2rgb(const vec3 hsl) {
    return hsl2rgb(hsl.x, hsl.y, hsl.z);
}

vec3 hsv2rgb(const vec3 hsv) {
    return hsv2rgb(hsv.x, hsv.y, hsv.z);
}

// returns a rgb color from the continuous rainbow bar, based on the hue param (0 ~ 1)
// to create a smooth color transition, hue must be unidirectional, so once hue hits 1
// it must jump back to 0, such a value can be created from a modulo or fract function
vec3 rainbow(float hue) {
    return hsv2rgb(hue, 1.0, 1.0);
}

// returns a float that bounces between 0.0 and k as the value of x changes monotonically
float bounce(float x, float k) {
    return k - abs(k - mod(x, k * 2));
}

// packs a 32-bit float into a vec4 (RGBA format), each component of RGBA = 8 bits and 1/256 precision
// this is often used on mobiles that do not support a high precision format like GL_DEPTH_COMPONENT32
// for example, implementations of shadow mapping in WebGL have been using this code extensively
vec4 pack(float x) {
    const vec4 bit_shift = vec4(1.0, 255.0, 255.0 * 255.0, 255.0 * 255.0 * 255.0);
    const vec4 bit_mask = vec4(vec3(1.0 / 255.0), 0.0);
    vec4 v = fract(x * bit_shift);
    return v - v.gbaa * bit_mask;  // cut off values that do not fit in 8 bits
}

// unpacks a vec4 RGBA into a 32-bit precision floating point scalar, for the explanation see:
// https://stackoverflow.com/questions/9882716/packing-float-into-vec4-how-does-this-code-work
float unpack(vec4 v) {
    const vec4 bit_shift = vec4(1.0, 1.0 / 255.0, 1.0 / (255.0 * 255.0), 1.0 / (255.0 * 255.0 * 255.0));
    return dot(v, bit_shift);
}

// returns a naive pseudo-random number between 0 and 1, seed x can be any number within (-inf, +inf)
// note that this randomness is more concentrated around 0.5 and sparser at the two ends 0.0 and 1.0
// it's extremely flawed at the sinusoid's peaks, so seed x should not be close to multiples of PI/2
// source: https://thebookofshaders.com/10/
float random1D(float x) {
    return fract(sin(x) * 100000.0);
}

// returns a naive pseudo-random number between 0 and 1, based on a 2D seed vector
// the random pattern can be changed using different values of a, b and c, but the
// seed value (for each component) must be a floating point number between 0 and 1
float random2D(const vec2 uv) {
    const float a = 12.9898, b = 78.233, c = 43758.5453123;
    float dt = dot(uv.xy, vec2(a, b));
    float sn = mod(dt, PI);
    return fract(sin(sn) * c);
}

#endif
