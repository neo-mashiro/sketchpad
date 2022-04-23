#ifndef _GAUSSIAN_H
#define _GAUSSIAN_H

#ifdef k9x9

// precomputed 9x9 Gaussian blur filter (sigma ~= 1.75)
const float weight[5] = float[] (
    0.2270270270,  // offset 0
    0.1945945946,  // offset +1 -1
    0.1216216216,  // offset +2 -2
    0.0540540541,  // offset +3 -3
    0.0162162162   // offset +4 -4
);

#endif

#ifdef k11x11

// precomputed 11x11 Gaussian blur filter (sigma = 4)
const float weight[6] = float[] (
    0.1198770,  // offset 0
    0.1161890,  // offset +1 -1
    0.1057910,  // offset +2 -2
    0.0904881,  // offset +3 -3
    0.0727092,  // offset +4 -4
    0.0548838   // offset +5 -5
);

#endif

#ifdef k13x13

// precomputed 13x13 Gaussian blur filter (sigma = 4)
const float weight[7] = float[] (
    0.1112200,  // offset 0
    0.1077980,  // offset +1 -1
    0.0981515,  // offset +2 -2
    0.0839534,  // offset +3 -3
    0.0674585,  // offset +4 -4
    0.0509203,  // offset +5 -5
    0.0361079   // offset +6 -6
);

#endif

#ifdef k15x15

// precomputed 15x15 Gaussian blur filter (sigma = 4)
const float weight[8] = float[] (
    0.1061150,  // offset 0
    0.1028510,  // offset +1 -1
    0.0936465,  // offset +2 -2
    0.0801001,  // offset +3 -3
    0.0643623,  // offset +4 -4
    0.0485832,  // offset +5 -5
    0.0344506,  // offset +6 -6
    0.0229491   // offset +7 -7
);

#endif

#ifdef k19x19

// precomputed 19x19 Gaussian blur filter (sigma = 5)
const float weight[10] = float[] (
    0.0846129,  // offset 0
    0.0829375,  // offset +1 -1
    0.0781076,  // offset +2 -2
    0.0706746,  // offset +3 -3
    0.0614416,  // offset +4 -4
    0.0513203,  // offset +5 -5
    0.0411855,  // offset +6 -6
    0.0317562,  // offset +7 -7
    0.0235255,  // offset +8 -8
    0.0167448   // offset +9 -9
);

#endif

#endif
