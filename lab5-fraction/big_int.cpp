//
// Created by Renatus Madrigal on 2026/04/23.
//

#include "big_int.hpp"

#include <algorithm>
#include <cstdint>
#include <span>

namespace fraction {

// Span-level primitives for Karatsuba (and future NTT/SSA).
// Defined in the .cpp so the compiler inlines them into karatsuba_core
// within the same translation unit. No hpp exposure needed.
namespace detail {

// result = a + b, returns carry (0 or 1).
// result.size() must be >= max(a.size(), b.size()).
uint32_t add_into(std::span<uint32_t> result, std::span<const uint32_t> a,
                  std::span<const uint32_t> b) {
  uint64_t carry = 0;
  size_t i = 0;
  size_t na = a.size(), nb = b.size(), nr = result.size();
  size_t limit = std::min({na, nb, nr});
  for (; i < limit; ++i) {
    uint64_t s = uint64_t(a[i]) + b[i] + carry;
    result[i] = uint32_t(s);
    carry = s >> 32;
  }
  for (; i < na && i < nr; ++i) {
    uint64_t s = uint64_t(a[i]) + carry;
    result[i] = uint32_t(s);
    carry = s >> 32;
  }
  for (; i < nb && i < nr; ++i) {
    uint64_t s = uint64_t(b[i]) + carry;
    result[i] = uint32_t(s);
    carry = s >> 32;
  }
  return uint32_t(carry);
}

// result = a - b, assuming a >= b. Returns borrow (0 or 1).
// result.size() must be >= a.size().
uint32_t sub_into(std::span<uint32_t> result, std::span<const uint32_t> a,
                  std::span<const uint32_t> b) {
  uint64_t borrow = 0;
  size_t i = 0;
  size_t na = a.size(), nb = b.size(), nr = result.size();
  size_t limit = std::min({na, nb, nr});
  for (; i < limit; ++i) {
    uint64_t A = a[i], B = b[i] + borrow;
    if (A < B) {
      result[i] = uint32_t(A + (1ULL << 32) - B);
      borrow = 1;
    } else {
      result[i] = uint32_t(A - B);
      borrow = 0;
    }
  }
  for (; i < na && i < nr; ++i) {
    uint64_t A = a[i];
    if (A < borrow) {
      result[i] = uint32_t(A + (1ULL << 32) - borrow);
      borrow = 1;
    } else {
      result[i] = uint32_t(A - borrow);
      borrow = 0;
    }
  }
  return uint32_t(borrow);
}

// result += a, propagates carry within result. Returns carry out.
uint32_t add_to(std::span<uint32_t> result, std::span<const uint32_t> a) {
  uint64_t carry = 0;
  size_t limit = std::min(result.size(), a.size());
  for (size_t i = 0; i < limit; ++i) {
    uint64_t s = uint64_t(result[i]) + a[i] + carry;
    result[i] = uint32_t(s);
    carry = s >> 32;
  }
  for (size_t i = limit; carry && i < result.size(); ++i) {
    uint64_t s = uint64_t(result[i]) + carry;
    result[i] = uint32_t(s);
    carry = s >> 32;
  }
  return uint32_t(carry);
}

// result -= a, propagates borrow within result. Returns borrow out.
// Assumes result >= a.
uint32_t sub_from(std::span<uint32_t> result, std::span<const uint32_t> a) {
  uint64_t borrow = 0;
  size_t limit = std::min(result.size(), a.size());
  for (size_t i = 0; i < limit; ++i) {
    uint64_t r = result[i];
    uint64_t s = uint64_t(a[i]) + borrow;
    if (r < s) {
      result[i] = uint32_t(r + (1ULL << 32) - s);
      borrow = 1;
    } else {
      result[i] = uint32_t(r - s);
      borrow = 0;
    }
  }
  for (size_t i = limit; borrow && i < result.size(); ++i) {
    if (result[i] < borrow) {
      result[i] = uint32_t(uint64_t(result[i]) + (1ULL << 32) - borrow);
      borrow = 1;
    } else {
      result[i] -= uint32_t(borrow);
      borrow = 0;
    }
  }
  return uint32_t(borrow);
}

// Naive O(n*m) multiplication on raw spans.
// result must be zero-initialized, size >= a.size() + b.size().
void mul_naive_raw(std::span<const uint32_t> a, std::span<const uint32_t> b,
                   std::span<uint32_t> result) {
  for (size_t i = a.size(); i-- > 0;) {
    uint64_t carry = 0;
    for (size_t j = 0; j < b.size(); ++j) {
      uint64_t prod = uint64_t(a[i]) * b[j] + result[i + j] + carry;
      result[i + j] = uint32_t(prod);
      carry = prod >> 32;
    }
    for (size_t k = i + b.size(); carry; ++k) {
      uint64_t s = uint64_t(result[k]) + carry;
      result[k] = uint32_t(s);
      carry = s >> 32;
    }
  }
}

void karatsuba_core(std::span<const uint32_t> x, std::span<const uint32_t> y,
                    std::span<uint32_t> result, std::span<uint32_t> temp) {
  static constexpr uint32_t ONE_SPAN_BUFFER[1] = {1};
  static constexpr std::span<const uint32_t> ONE_SPAN(ONE_SPAN_BUFFER);
  // We assume that the size of x and y are the same and a power of 2 for
  // simplicity. The padding will be handled by `mul_karatsuba`.
  auto n = x.size();

  if (n <= 32) {
    mul_naive_raw(x, y, result);
    return;
  }

  size_t half = n / 2;
  auto x_low = x.subspan(0, std::min(half, x.size()));
  auto x_high = x.subspan(half);
  auto y_low = y.subspan(0, std::min(half, y.size()));
  auto y_high = y.subspan(half);

  // Here we use the substract version of Karatsuba to save temporary space.

  size_t temp_size = half;

  auto diff_x = temp.subspan(0, temp_size);
  auto diff_y = temp.subspan(temp_size, temp_size);

  // diff_x = x_high - x_low
  // diff_y = y_high - y_low
  auto b1 = sub_into(diff_x, x_high, x_low);
  if (b1) {
    // If x_high < x_low, we negate the result and remember to negate z1 later
    for (auto& d : diff_x) d = ~d;
    add_to(diff_x, ONE_SPAN);  // Two's complement negation
  }
  auto b2 = sub_into(diff_y, y_high, y_low);
  if (b2) {
    // If y_high < y_low, we negate the result and remember to negate z1 later
    for (auto& d : diff_y) d = ~d;
    add_to(diff_y, ONE_SPAN);  // Two's complement negation
  }

  // z0 = x_low * y_low
  // z1 = (x_high - x_low) * (y_high - y_low)
  // z2 = x_high * y_high
  auto z0 = result.subspan(0, n);
  auto z1 = temp.subspan(2 * temp_size, 2 * temp_size);
  auto z2 = result.subspan(n, n);

  auto new_temp = temp.subspan(4 * temp_size);  // For recursive calls

  karatsuba_core(x_low, y_low, z0, new_temp);
  karatsuba_core(diff_x, diff_y, z1, new_temp);
  karatsuba_core(x_high, y_high, z2, new_temp);

  // x0y1 + x1y0 = z0 + z2 - (x1-x0)(y1-y0)
  // if b1 == b2 then (x1-x0)(y1-y0) >= 0 => the middle product should be neg.
  bool middle_neg = (b1 == b2);  // z1 should be negative

  // Here we use the Two's complement negation trick to make the code simpler
  // and more efficient, at the cost of modifying z1. This can be easily
  // optimized with SIMD.
  if (middle_neg) {
    // z1 = -z1
    for (auto& d : z1) d = ~d;
    add_to(z1, ONE_SPAN);  // Two's complement negation
  }

  add_to(z1, z0);
  add_to(z1, z2);

  auto middle = result.subspan(half);
  add_to(middle, z1);
}

}  // namespace detail

void BigInt::add_abs(const BigInt& other) {
  size_t n = std::max(digits.size(), other.digits.size());
  digits.resize(n, 0);
  auto carry = detail::add_to(std::span<uint32_t>(digits),
                              std::span<const uint32_t>(other.digits));
  if (carry) digits.push_back(carry);
  trim();
}

void BigInt::sub_abs(const BigInt& other) {
  detail::sub_from(std::span<uint32_t>(digits),
                   std::span<const uint32_t>(other.digits));
  trim();
}

BigInt& BigInt::mul_naive(const BigInt& other) {
  bool result_neg = (is_negative != other.is_negative);

  size_t n = digits.size();
  size_t m = other.digits.size();

  if ((n == 1 && digits[0] == 0) || (m == 1 && other.digits[0] == 0)) {
    *this = BigInt(0);
    return *this;
  }

  if (this == &other) {
    BigInt tmp = other;
    return mul_naive(tmp);
  }

  std::vector<uint32_t> buf(n + m, 0);
  detail::mul_naive_raw(std::span<const uint32_t>(digits),
                        std::span<const uint32_t>(other.digits),
                        std::span<uint32_t>(buf));

  digits = std::move(buf);
  is_negative = result_neg;
  trim();
  return *this;
}

BigInt& BigInt::mul_karatsuba(const BigInt& other) {
  bool result_neg = (is_negative != other.is_negative);

  size_t n = digits.size();
  size_t m = other.digits.size();

  if ((n == 1 && digits[0] == 0) || (m == 1 && other.digits[0] == 0)) {
    *this = BigInt(0);
    return *this;
  }

  if (this == &other) {
    BigInt tmp = other;
    return mul_karatsuba(tmp);
  }

  // Fall back to naive for small operands
  if (n < 32 || m < 32) {
    return mul_naive(other);
  }

  // Pad both operands to the same power-of-2 length
  size_t padded = 1;
  while (padded < n || padded < m) padded *= 2;

  std::vector<uint32_t> x(padded, 0);
  std::vector<uint32_t> y(padded, 0);
  std::copy(digits.begin(), digits.end(), x.begin());
  std::copy(other.digits.begin(), other.digits.end(), y.begin());

  std::vector<uint32_t> result(2 * padded, 0);

  // Temp space: karatsuba_core needs ~4n per level, total ~4n for the
  // top-level call (reused across recursive calls).
  std::vector<uint32_t> temp(4 * padded, 0);

  detail::karatsuba_core(x, y, result, temp);

  digits.assign(result.begin(), result.end());
  is_negative = result_neg;
  trim();
  return *this;
}

BigInt& BigInt::mul_fft_ntt(const BigInt&) {
  throw std::runtime_error("FFT/NTT multiplication not implemented");
}

BigInt& BigInt::mul_ssa(const BigInt&) {
  throw std::runtime_error("SSA multiplication not implemented");
}

BigInt& BigInt::div_naive(const BigInt&) {
  throw std::runtime_error("Naive division not implemented");
}

BigInt& BigInt::div_newton(const BigInt&) {
  throw std::runtime_error("Newton division not implemented");
}

BigInt& BigInt::div_burnikel_ziegler(const BigInt&) {
  throw std::runtime_error("Burnikel-Ziegler division not implemented");
}

}  // namespace fraction