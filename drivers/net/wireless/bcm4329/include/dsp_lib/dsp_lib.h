#ifndef DSP_LIB
#define DSP_LIB
// Copyright (c) 2007 Broadcom
// \\author = Tony Kirke

// Only compile this stuff if using C++
#ifdef __cplusplus
#ifndef MAX
template <typename T> inline T MAX(T x, T y) { return((x) >= (y) ? (x) : (y)); }
#endif
#ifndef MIN
template <typename T> inline T MIN(T x, T y) { return((x) >= (y) ? (y) : (x)); }
#endif
#ifndef ABS
int16_t inline ABS(int16_t x) { return((((x) >> 15) ^ (x)) - ((x) >> 15)); }
int32_t inline ABS(int32_t x) { return((((x) >> 31) ^ (x)) - ((x) >> 31)); }
double  inline ABS(double  x) { return((x < 0) ? -x : x); }
#endif
#endif

#ifndef ROUND
#define ROUND(x, s) (((x) >> (s)) + (((x) >> ((s) - 1)) & (s!=0))) 
#endif
#ifndef ROUND_RESCALE
#define ROUND_RESCALE(x, s) ((x) + ((((x) >> ((s) - 1)) & (s!=0)) << s))
#endif

/* limit to [min, max] */
#define LIMIT_MIN_MAX(x, min, max) \
    ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/* limit to [-2^bits,2^bits-1] */
#define LIMIT_BITS(x, bits) LIMIT_MIN_MAX(x,(-(1<<bits)),((1<<bits)-1))
#define LIMIT_BITS64(x, bits) LIMIT_MIN_MAX(x,((-(int64_t)1)<<bits),((int64_t)1<<bits)-1)


// Log2 not defined in MSC
#ifdef _MSC_VER
#define log2(x) log((double)x/log(2.0))
#endif

#endif
