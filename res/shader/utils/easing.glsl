#ifndef _EASING_H
#define _EASING_H

#ifndef PI
#define PI      3.141592653589793
#define HLF_PI  1.570796326794897
#endif

#include "extension.glsl"

// some cheap easing functions (branchless) adapted from https://github.com/warrenm/AHEasing
// the effects of these easing functions can be visualized at https://easings.net/

float QuadraticEaseIn(float x) {
    return x * x;
}

float QuadraticEaseOut(float x) {
    return x * (2 - x);
}

float QuadraticEaseInOut(float x) {
    return x < 0.5 ? (2 * x * x) : (4 * x - 2 * x * x - 1);
}

float CubicEaseIn(float x) {
    return x * x * x;
}

float CubicEaseOut(float x) {
    return 1 + pow3(x - 1);
}

float CubicEaseInOut(float x) {
    return (x < 0.5) ? (4 * x * x * x) : (0.5 * pow3(2 * x - 2) + 1);
}

float QuarticEaseIn(float x) {
    return x * x * x * x;
}

float QuarticEaseOut(float x) {
    return pow3(x - 1) * (1 - x) + 1;
}

float QuarticEaseInOut(float x) {
    return (x < 0.5) ? (8 * x * x * x * x) : (1 - 8 * pow4(x - 1));
}

float QuinticEaseIn(float x) {
    return x * x * x * x * x;
}

float QuinticEaseOut(float x) {
    return pow5(x - 1) + 1;
}

float QuinticEaseInOut(float x) {
    return (x < 0.5) ? (16 * x * x * x * x * x) : (0.5 * pow5((2 * x) - 2) + 1);
}

float SineEaseIn(float x) {
    return 1 - cos(x * HLF_PI);
}

float SineEaseOut(float x) {
    return sin(x * HLF_PI);
}

float SineEaseInOut(float x) {
    return 0.5 * (1 - cos(PI * x));
}

float CircularEaseIn(float x) {
    return 1 - sqrt(1 - x * x);
}

float CircularEaseOut(float x) {
    return sqrt((2 - x) * x);
}

float CircularEaseInOut(float x) {
    return (x < 0.5) ? (0.5 * (1 - sqrt(1 - 4 * x * x))) : (0.5 * (sqrt((3 - 2 * x) * (2 * x - 1)) + 1));
}

float ExponentialEaseIn(float x) {
    return (x == 0.0) ? x : pow(2, 10 * (x - 1));
}

float ExponentialEaseOut(float x) {
    return (x == 1.0) ? x : 1 - pow(2, -10 * x);
}

float ExponentialEaseInOut(float x) {
    return (x == 0.0 || x == 1.0) ? x :
        ((x < 0.5) ? (0.5 * pow(2, (20 * x) - 10)) : (1 - 0.5 * pow(2, (-20 * x) + 10)))
    ;
}

float ElasticEaseIn(float x) {
    return sin(13 * HLF_PI * x) * pow(2, 10 * (x - 1));
}

float ElasticEaseOut(float x) {
    return 1 - sin(13 * HLF_PI * (x + 1)) * pow(2, -10 * x);
}

float ElasticEaseInOut(float x) {
    return (x < 0.5)
        ? (0.5 * sin(26 * HLF_PI * x) * pow(2, 10 * (2 * x - 1)))
        : (0.5 * (2 - sin(26 * HLF_PI * x) * pow(2, -10 * (2 * x - 1))))
    ;
}

float BackEaseIn(float x) {
    return x * x * x - x * sin(x * PI);
}

float BackEaseOut(float x) {
    float f = 1 - x;
    return 1 - (f * f * f - f * sin(f * PI));
}

float BackEaseInOut(float x) {
    if (x < 0.5) {
        float f = 2 * x;
        return 0.5 * (f * f * f - f * sin(f * PI));
    }

    float f = 2 - 2 * x;
    return 0.5 * (1 - (f * f * f - f * sin(f * PI))) + 0.5;
}

float BounceEaseOut(float x) {
    if (x < 0.36363636363) { return x * x * 7.5625; }
    if (x < 0.72727272727) { return x * x * 9.075 - x * 9.9 + 3.4; }
    if (x < 0.90000000000) { return x * x * 12.0664819945 - x * 19.6354570637 + 8.89806094183; }
    return x * x * 10.8 - x * 20.52 + 10.72;
}

float BounceEaseIn(float x) {
    return 1 - BounceEaseOut(1 - x);
}

float BounceEaseInOut(float x) {
    return (x < 0.5) ? (0.5 * BounceEaseIn(x * 2)) : (0.5 * BounceEaseOut(x * 2 - 1) + 0.5);
}

#endif