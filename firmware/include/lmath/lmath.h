#ifndef __LMATH_H__
#define __LMATH_H__

#include <stdbool.h>

/*
 * Lmath is a library of math functions which are defined
 * as lookup tables. The bounds for each function are defined
 * in the documentation. Each function takes in an _Accum which
 * is the input value and saves the result in the pointer 'out'
 * which is also an _Accum type.
 *
 * The return value is true if the input value was sucessfully
 * mapped with the table or false if not. The error state happens
 * if the input value is out of bounds.
 *
 * The lookup table sizes are defined below, but also have +1 entry
 * for the upper bound and +1 for protection from lerp going out of
 * bounds. The protection entry is 0.0k at the end of the array.
 *
 * The functions use linear interpolation to improve accuracy
 *
 * All trig functions use radians
 *
 */

/*
        Look up table defined for sqrt
        Lower bound of 0.000000
        Upper bound of 8.000000
        256 entries
*/
bool lsqrt(_Accum in, _Accum *out);

/*
        Look up table defined for sin
        Lower bound of -4.000000
        Upper bound of 4.000000
        256 entries
*/
bool lsin(_Accum in, _Accum *out);

/*
        Look up table defined for cos
        Lower bound of -4.000000
        Upper bound of 4.000000
        256 entries
*/
bool lcos(_Accum in, _Accum *out);

/*
        Look up table defined for tan
        Lower bound of -4.000000
        Upper bound of 4.000000
        256 entries
*/
bool ltan(_Accum in, _Accum *out);

/*
        Look up table defined for asin
        Lower bound of -1.000000
        Upper bound of 1.000000
        256 entries
*/
bool lasin(_Accum in, _Accum *out);

/*
        Look up table defined for acos
        Lower bound of -1.000000
        Upper bound of 1.000000
        256 entries
*/
bool lacos(_Accum in, _Accum *out);

/*
        Look up table defined for atan
        Lower bound of -1.000000
        Upper bound of 1.000000
        256 entries
*/
bool latan(_Accum in, _Accum *out);

#endif /* __LMATH_H__ */
