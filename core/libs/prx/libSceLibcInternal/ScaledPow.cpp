#include "prx/libSceLibcInternal/include/MathInternals.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestErrno.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace {

constexpr int DomainError = 33;
constexpr int RangeError = 34;

void StoreExponent(short* destination, short value) {
    if (destination == nullptr) return;
    GuestCopyOut_nid_no_patch(destination, &value, sizeof(value));
}

void SetErrno(int code) {
    *__error_nid_postfix() = code;
}

}

extern "C" {

double APS5_VABI _Pow_nid_postfix(double x, double y, short*) {
    return pow_nid_postfix(x, y);
}

float APS5_VABI _FPow_nid_postfix(float x, float y, short*) {
    return powf_nid_postfix(x, y);
}

long double APS5_VABI _LPow_nid_postfix(long double x, long double y, short* exponent) {
    if (y == 1.0L) return x;
    StoreExponent(exponent, 0);
    if (exponent == nullptr) return powl_nid_postfix(x, y);

    if (std::isnan(x) || std::isnan(y)) {
        if (x == 1.0L || y == 0.0L) return 1.0L;
        return std::numeric_limits<long double>::quiet_NaN();
    }

    if (x == 0.0L) {
        if (y == 0.0L) return 1.0L;
        const bool isOdd = std::floor(y) == y && std::fmod(std::fabs(y), 2.0L) == 1.0L;
        if (y > 0.0L) {
            return (std::signbit(x) && isOdd) ? -0.0L : 0.0L;
        }
        SetErrno(RangeError);
        return (std::signbit(x) && isOdd) ? -std::numeric_limits<long double>::infinity() : std::numeric_limits<long double>::infinity();
    }

    if (y == 0.0L) return 1.0L;

    if (std::isinf(x)) {
        if (x > 0.0L) {
            return y > 0.0L ? std::numeric_limits<long double>::infinity() : 0.0L;
        }
        const bool isOdd = std::floor(y) == y && std::fmod(std::fabs(y), 2.0L) == 1.0L;
        if (y > 0.0L) {
            return isOdd ? -std::numeric_limits<long double>::infinity() : std::numeric_limits<long double>::infinity();
        }
        return isOdd ? -0.0L : 0.0L;
    }

    if (std::isinf(y)) {
        if (x == 1.0L || x == -1.0L) return 1.0L;
        if (std::fabs(x) > 1.0L) {
            return y > 0.0L ? std::numeric_limits<long double>::infinity() : 0.0L;
        }
        return y > 0.0L ? 0.0L : std::numeric_limits<long double>::infinity();
    }

    if (x < 0.0L) {
        const bool isInteger = std::floor(y) == y;
        if (!isInteger) {
            SetErrno(DomainError);
            return std::numeric_limits<long double>::quiet_NaN();
        }
    }

    const long double absX = std::fabs(x);
    const long double totalZ = std::fabs(absX - 1.0L) < 0.25L
        ? y * (std::log1p(absX - 1.0L) / std::log(2.0L))
        : y * std::log2(absX);

    const long double floorExp = std::floor(totalZ);
    const bool isOddInteger = x < 0.0L && std::fmod(std::fabs(y), 2.0L) == 1.0L;

    if (floorExp > 32767.0L) {
        SetErrno(RangeError);
        return isOddInteger ? -std::numeric_limits<long double>::infinity() : std::numeric_limits<long double>::infinity();
    }
    if (floorExp < -32768.0L) {
        SetErrno(RangeError);
        return isOddInteger ? -0.0L : 0.0L;
    }

    const short expVal = static_cast<short>(floorExp);
    const long double rem = totalZ - floorExp;

    StoreExponent(exponent, expVal);
    long double result = std::exp2(rem);
    if (isOddInteger) {
        result = -result;
    }
    return result;
}

}
