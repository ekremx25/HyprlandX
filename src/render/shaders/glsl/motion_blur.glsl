#ifndef MOTION_BLUR_GLSL
#define MOTION_BLUR_GLSL

const int MOTION_BLUR_MAX_SAMPLES = 64;

#define MOTION_BLUR_SAMPLER sampler2D
#include "motion_blur_sample.glsl"
#undef MOTION_BLUR_SAMPLER

#if USE_EXTERNAL
#define MOTION_BLUR_SAMPLER samplerExternalOES
#include "motion_blur_sample.glsl"
#undef MOTION_BLUR_SAMPLER
#endif

#endif
