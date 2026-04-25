//
// Created by Renatus Madrigal on 2026/04/23.
//

// This file contains several utility functions that are widely used.

#pragma once

#ifndef LAB5_FRACTION_UTILITY_HPP
#define LAB5_FRACTION_UTILITY_HPP

namespace fraction {

/**
 * Computes the greatest common divisor (GCD) of two integers.
 * @param a First integer
 * @param b Second integer
 * @return The GCD of a and b
 */
template <typename T>
constexpr T gcd(T a, T b) {
  while (b != 0) {
    T temp = b;
    b = a % b;
    a = temp;
  }
  return a;
}

/**
 * Computes the power of a base raised to an exponent using exponentiation by
 * squaring.
 * @param base The base value
 * @param exp The exponent value (non-negative)
 * @return The result of base raised to the power of exp
 */
template <typename T>
constexpr T power(T base, unsigned long long exp) {
  if (exp == 0) {
    return 1;
  }
  T result = 1;
  while (exp) {
    if (exp & 1) {
      result *= base;
    }
    base *= base;
    exp >>= 1;
  }
  return result;
}

/**
 * Computes base^exp mod mod using binary exponentiation.
 * All intermediate products stay within 2*mod bits, so for mod < 2^63 this
 * won't overflow uint64_t.
 */
template <typename T>
constexpr T modpow(T base, unsigned long long exp, T mod) {
  T result = 1;
  base %= mod;
  while (exp) {
    if (exp & 1)
      result = static_cast<T>(
          (static_cast<unsigned long long>(result) * base) % mod);
    base = static_cast<T>(
        (static_cast<unsigned long long>(base) * base) % mod);
    exp >>= 1;
  }
  return result;
}

/**
 * Computes the modular inverse of a number modulo m using Fermat's little
 * theorem. Note that m must be prime, otherwise we need to use the Extended
 * Euclidean Algorithm instead. Currently this is just used for NTT, where the
 * modulus is guaranteed to be prime.
 * @param a The number for which to compute the modular inverse
 * @param m The modulus (must be prime)
 * @return The modular inverse of a modulo m
 */
template <typename T>
constexpr T mod_inv(T a, unsigned long long m) {
  return modpow(a, m - 2, static_cast<T>(m));
}

}  // namespace fraction

#if FRACTION_COMPILER_CLANG || FRACTION_COMPILER_GCC

namespace fraction {
using Int128 = __int128_t;
using UInt128 = __uint128_t;
}  // namespace fraction

#elif FRACTION_COMPILER_MSVC

#include <intrin.h>

namespace fraction {

struct UInt128 {
  uint64_t lo;
  uint64_t hi;

  constexpr UInt128() : lo(0), hi(0) {}
  constexpr UInt128(int v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? ~uint64_t(0) : 0) {}
  constexpr UInt128(unsigned int v) : lo(v), hi(0) {}
  constexpr UInt128(long v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? ~uint64_t(0) : 0) {}
  constexpr UInt128(unsigned long v) : lo(v), hi(0) {}
  constexpr UInt128(long long v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? ~uint64_t(0) : 0) {}
  constexpr UInt128(unsigned long long v) : lo(v), hi(0) {}

  UInt128& operator+=(const UInt128& rhs) {
    unsigned char c = _addcarry_u64(0, lo, rhs.lo, &lo);
    _addcarry_u64(c, hi, rhs.hi, &hi);
    return *this;
  }
  UInt128& operator-=(const UInt128& rhs) {
    unsigned char b = _subborrow_u64(0, lo, rhs.lo, &lo);
    _subborrow_u64(b, hi, rhs.hi, &hi);
    return *this;
  }
  UInt128& operator*=(const UInt128& rhs) {
    uint64_t carry;
    uint64_t new_lo = _umul128(lo, rhs.lo, &carry);
    hi = lo * rhs.hi + hi * rhs.lo + carry;
    lo = new_lo;
    return *this;
  }

  UInt128 operator+() const { return *this; }
  UInt128 operator-() const {
    UInt128 r;
    r.lo = ~lo;
    r.hi = ~hi;
    r += UInt128(1u);
    return r;
  }
  UInt128 operator~() const {
    UInt128 r;
    r.lo = ~lo;
    r.hi = ~hi;
    return r;
  }
  bool operator!() const { return !lo && !hi; }

  UInt128& operator>>=(unsigned shift) {
    if (shift >= 128) {
      lo = hi = 0;
    } else if (shift >= 64) {
      lo = hi >> (shift - 64);
      hi = 0;
    } else {
      lo = (lo >> shift) | (hi << (64 - shift));
      hi >>= shift;
    }
    return *this;
  }

  explicit operator bool() const { return lo || hi; }
  explicit operator unsigned long long() const { return lo; }
  explicit operator long long() const { return static_cast<long long>(lo); }
};

inline bool operator==(const UInt128& a, const UInt128& b) { return a.lo == b.lo && a.hi == b.hi; }
inline bool operator!=(const UInt128& a, const UInt128& b) { return !(a == b); }
inline bool operator<(const UInt128& a, const UInt128& b) {
  return (a.hi == b.hi) ? (a.lo < b.lo) : (a.hi < b.hi);
}
inline bool operator>(const UInt128& a, const UInt128& b) { return b < a; }
inline bool operator<=(const UInt128& a, const UInt128& b) { return !(b < a); }
inline bool operator>=(const UInt128& a, const UInt128& b) { return !(a < b); }

inline UInt128 operator+(UInt128 a, const UInt128& b) { return a += b; }
inline UInt128 operator-(UInt128 a, const UInt128& b) { return a -= b; }
inline UInt128 operator*(UInt128 a, const UInt128& b) { return a *= b; }
inline UInt128 operator>>(UInt128 a, unsigned shift) { return a >>= shift; }

// Signed 128-bit integer. Stored as two uint64_t; arithmetic is identical to
// UInt128 at the bit level (two's complement), only comparisons differ.

struct Int128 {
  uint64_t lo;
  uint64_t hi;

  constexpr Int128() : lo(0), hi(0) {}
  constexpr Int128(int v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? ~uint64_t(0) : 0) {}
  constexpr Int128(unsigned int v) : lo(v), hi(0) {}
  constexpr Int128(long v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? ~uint64_t(0) : 0) {}
  constexpr Int128(unsigned long v) : lo(v), hi(0) {}
  constexpr Int128(long long v) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? ~uint64_t(0) : 0) {}
  constexpr Int128(unsigned long long v) : lo(v), hi(0) {}

  Int128& operator+=(const Int128& rhs) {
    unsigned char c = _addcarry_u64(0, lo, rhs.lo, &lo);
    _addcarry_u64(c, hi, rhs.hi, &hi);
    return *this;
  }
  Int128& operator-=(const Int128& rhs) {
    unsigned char b = _subborrow_u64(0, lo, rhs.lo, &lo);
    _subborrow_u64(b, hi, rhs.hi, &hi);
    return *this;
  }
  Int128& operator*=(const Int128& rhs) {
    uint64_t carry;
    uint64_t new_lo = _umul128(lo, rhs.lo, &carry);
    hi = lo * rhs.hi + hi * rhs.lo + carry;
    lo = new_lo;
    return *this;
  }

  Int128 operator+() const { return *this; }
  Int128 operator-() const {
    Int128 r;
    r.lo = ~lo;
    r.hi = ~hi;
    r += Int128(1);
    return r;
  }
  Int128 operator~() const {
    Int128 r;
    r.lo = ~lo;
    r.hi = ~hi;
    return r;
  }
  bool operator!() const { return !lo && !hi; }

  explicit operator bool() const { return lo || hi; }
  explicit operator long long() const { return static_cast<long long>(lo); }
  explicit operator unsigned long long() const { return lo; }
};

inline bool operator==(const Int128& a, const Int128& b) { return a.lo == b.lo && a.hi == b.hi; }
inline bool operator!=(const Int128& a, const Int128& b) { return !(a == b); }
inline bool operator<(const Int128& a, const Int128& b) {
  bool a_neg = static_cast<int64_t>(a.hi) < 0;
  bool b_neg = static_cast<int64_t>(b.hi) < 0;
  if (a_neg != b_neg) return a_neg;
  return (a.hi == b.hi) ? (a.lo < b.lo) : (a.hi < b.hi);
}
inline bool operator>(const Int128& a, const Int128& b) { return b < a; }
inline bool operator<=(const Int128& a, const Int128& b) { return !(b < a); }
inline bool operator>=(const Int128& a, const Int128& b) { return !(a < b); }

  Int128& operator>>=(unsigned shift) {
    if (shift >= 128) {
      lo = hi = 0;
    } else if (shift >= 64) {
      lo = hi >> (shift - 64);
      hi = 0;
    } else {
      lo = (lo >> shift) | (hi << (64 - shift));
      hi >>= shift;
    }
    return *this;
  }

  explicit operator bool() const { return lo || hi; }
  explicit operator long long() const { return static_cast<long long>(lo); }
  explicit operator unsigned long long() const { return lo; }
};

inline bool operator==(const Int128& a, const Int128& b) { return a.lo == b.lo && a.hi == b.hi; }
inline bool operator!=(const Int128& a, const Int128& b) { return !(a == b); }
inline bool operator<(const Int128& a, const Int128& b) {
  bool a_neg = static_cast<int64_t>(a.hi) < 0;
  bool b_neg = static_cast<int64_t>(b.hi) < 0;
  if (a_neg != b_neg) return a_neg;
  return (a.hi == b.hi) ? (a.lo < b.lo) : (a.hi < b.hi);
}
inline bool operator>(const Int128& a, const Int128& b) { return b < a; }
inline bool operator<=(const Int128& a, const Int128& b) { return !(b < a); }
inline bool operator>=(const Int128& a, const Int128& b) { return !(a < b); }

inline Int128 operator+(Int128 a, const Int128& b) { return a += b; }
inline Int128 operator-(Int128 a, const Int128& b) { return a -= b; }
inline Int128 operator*(Int128 a, const Int128& b) { return a *= b; }
inline Int128 operator>>(Int128 a, unsigned shift) { return a >>= shift; }

}  // namespace fraction

#endif

#endif  // LAB5_FRACTION_UTILITY_HPP