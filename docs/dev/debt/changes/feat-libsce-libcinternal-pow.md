# libSceLibcInternal

## add Functional

- [_Pow, _FPow and _LPow](../../../core/libs/prx/libSceLibcInternal/ScaledPow.cpp) (libSceLibcInternal) - `_Pow` and `_FPow` ignore the `short*` exponent pointer and return `pow`/`powf`'s result, leaving the pointer unread and unmodified as measured from firmware 13.42 disassembly on #22452. `_LPow` with a non-null exponent scales its result: returns `x` unmodified without writing the exponent when `y == 1`, initializes the exponent to 0 on entry for any other `y`, computes a scaled fraction in [1.0, 2.0) and stores the binary exponent in `*exponent`, avoiding near-one cancellation via log1p, and returns `inf`, `0` or `NaN` with errno `ERANGE` (34) or `EDOM` (33) on overflow, domain error, or zero base with negative exponent. Results come from libc's msun functions, not Dinkumware's, so last bits can differ.
