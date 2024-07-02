#include "lmath.h"

#include <stdint.h>

#include "lmath_luts.h"

#define BOUNDS_CHECK(var, lower_bound, upper_bound)               \
    do {                                                          \
        if (var < lower_bound || var > upper_bound) return false; \
    } while (0)

// Convert the raw value from the user into an index
#define GET_INDEX(var, lower_bound, shift) ((var - lower_bound) << shift)

/*
 * lerp interpolates between the x0 and x0+1 y values
 * of a lookup table and returns the result.
 *
 * 'table' is a pointer to the lookup table to reference
 * 'x' is the input converted into an index for lookup
 *
 * Note this function only works for these lookup tables
 * as the width is 1 apart
 *
 *
 */
static inline _Accum lerp(_Accum *table, _Accum x) {
    int x0 = (int)x;
    _Accum y1 = table[x0];
    _Accum y2 = table[x0 + 1];

    return y1 + (y2 - y1) * (x - (_Accum)x0);
}

bool lsqrt(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, sqrt_LOWER_BOUND, sqrt_UPPER_BOUND);
    _Accum x = GET_INDEX(in, sqrt_LOWER_BOUND, sqrt_SHIFT_VALUE);
    *out = lerp(sqrt_lut, x);
    return true;
}

bool lsin(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, sin_LOWER_BOUND, sin_UPPER_BOUND);
    _Accum x = GET_INDEX(in, sin_LOWER_BOUND, sin_SHIFT_VALUE);
    *out = lerp(sin_lut, x);
    return true;
}

bool lcos(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, cos_LOWER_BOUND, cos_UPPER_BOUND);
    _Accum x = GET_INDEX(in, cos_LOWER_BOUND, cos_SHIFT_VALUE);
    *out = lerp(cos_lut, x);
    return true;
}

bool ltan(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, tan_LOWER_BOUND, tan_UPPER_BOUND);
    _Accum x = GET_INDEX(in, tan_LOWER_BOUND, tan_SHIFT_VALUE);
    *out = lerp(tan_lut, x);
    return true;
}

bool lasin(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, asin_LOWER_BOUND, asin_UPPER_BOUND);
    _Accum x = GET_INDEX(in, asin_LOWER_BOUND, asin_SHIFT_VALUE);
    *out = lerp(asin_lut, x);
    return true;
}

bool lacos(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, acos_LOWER_BOUND, acos_UPPER_BOUND);
    _Accum x = GET_INDEX(in, acos_LOWER_BOUND, acos_SHIFT_VALUE);
    *out = lerp(acos_lut, x);
    return true;
}

bool latan(_Accum in, _Accum *out) {
    BOUNDS_CHECK(in, atan_LOWER_BOUND, atan_UPPER_BOUND);
    _Accum x = GET_INDEX(in, atan_LOWER_BOUND, atan_SHIFT_VALUE);
    *out = lerp(atan_lut, x);
    return true;
}
