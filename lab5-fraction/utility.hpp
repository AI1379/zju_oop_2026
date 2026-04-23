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
T gcd(T a, T b) {
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
T power(T base, unsigned long long exp) {
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

}  // namespace fraction

#endif  // LAB5_FRACTION_UTILITY_HPP