//
// Created by Renatus Madrigal on 2026/04/23.
//

#include "big_int.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "utility.hpp"

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
    std::fill(result.begin(), result.end(), 0);
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

  int64_t z1_high = 0;
  if (middle_neg) {
    for (auto& d : z1) d = ~d;
    z1_high = -1 + add_to(z1, ONE_SPAN);
  }

  z1_high += add_to(z1, z0);
  z1_high += add_to(z1, z2);

  auto middle = result.subspan(half);
  add_to(middle, z1);
  if (z1_high > 0) {
    auto high_limb = static_cast<uint32_t>(z1_high);
    add_to(middle.subspan(z1.size()), std::span<const uint32_t>(&high_limb, 1));
  }
}

static constexpr uint32_t NTT_PRIMES[] = {
    998244353,   // 2^23 * 7 * 17 + 1, primitive root = 3
    1004535809,  // 2^21 * 479 + 1, primitive root = 3
    469762049,   // 2^26 * 7 + 1, primitive root = 3
};

static constexpr uint32_t NTT_ROOTS[] = {
    3,  // Primitive root for 998244353
    3,  // Primitive root for 1004535809
    3,  // Primitive root for 469762049
};

void ntt(std::span<uint32_t> a, uint32_t mod, uint32_t primitive_root,
         bool inverse = false) {
  auto n = a.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    auto bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }

  for (size_t len = 2; len <= n; len <<= 1) {
    // Per-level twiddle step: g^((p-1)/len) for forward,
    // g^(-(p-1)/len) = g^((p-1)-(p-1)/len) for inverse.
    uint64_t exp = static_cast<uint64_t>(mod - 1) / len;
    if (inverse) exp = static_cast<uint64_t>(mod - 1) - exp;
    uint64_t w = modpow(static_cast<uint64_t>(primitive_root), exp,
                        static_cast<uint64_t>(mod));

    for (size_t i = 0; i < n; i += len) {
      uint64_t wn = 1;
      for (size_t j = 0; j < len / 2; ++j) {
        uint64_t u = a[i + j];
        uint64_t v = static_cast<uint64_t>(a[i + j + len / 2]) * wn % mod;
        a[i + j] = (u + v) % mod;
        a[i + j + len / 2] = (u - v + mod) % mod;
        wn = wn * w % mod;
      }
    }
  }

  if (inverse) {
    uint64_t inv_n =
        modpow(static_cast<uint64_t>(n), static_cast<uint64_t>(mod - 2),
               static_cast<uint64_t>(mod));
    for (auto& x : a) {
      x = static_cast<uint64_t>(x) * inv_n % mod;
    }
  }
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

BigInt& BigInt::mul_fft_ntt(const BigInt& other) {
  bool result_neg = (is_negative != other.is_negative);

  size_t n = digits.size();
  size_t m = other.digits.size();

  if ((n == 1 && digits[0] == 0) || (m == 1 && other.digits[0] == 0)) {
    *this = BigInt(0);
    return *this;
  }

  if (this == &other) {
    BigInt tmp = other;
    return mul_fft_ntt(tmp);
  }

  // Linear convolution of two vectors of sizes n, m needs n+m coefficients.
  size_t padded_size = 1;
  while (padded_size < n + m) padded_size *= 2;

  std::vector<uint32_t> a(padded_size, 0);
  std::vector<uint32_t> b(padded_size, 0);
  std::copy(digits.begin(), digits.end(), a.begin());
  std::copy(other.digits.begin(), other.digits.end(), b.begin());

  // For each of the 3 NTT primes: NTT(a), NTT(b), pointwise multiply, INTT.
  std::vector<uint32_t> conv[3];
  for (size_t k = 0; k < 3; ++k) {
    auto p = detail::NTT_PRIMES[k];
    auto root = detail::NTT_ROOTS[k];

    auto ak = a;
    auto bk = b;

    detail::ntt(ak, p, root, false);
    detail::ntt(bk, p, root, false);

    for (size_t i = 0; i < padded_size; ++i) {
      ak[i] = static_cast<uint64_t>(ak[i]) * bk[i] % p;
    }

    detail::ntt(ak, p, root, true);
    conv[k] = std::move(ak);
  }

  // CRT reconstruction constants (constexpr, evaluated at compile time).
  static constexpr uint64_t P1 = detail::NTT_PRIMES[0];
  static constexpr uint64_t P2 = detail::NTT_PRIMES[1];
  static constexpr uint64_t P3 = detail::NTT_PRIMES[2];
  static constexpr uint64_t INV_P1_MOD_P2 =
      mod_inv(static_cast<uint64_t>(P1), static_cast<uint64_t>(P2));
  static constexpr uint64_t INV_P1P2_MOD_P3 =
      mod_inv(static_cast<uint64_t>(P1) * static_cast<uint64_t>(P2),
              static_cast<uint64_t>(P3));

  std::vector<UInt128> result(padded_size);
  for (size_t i = 0; i < padded_size; ++i) {
    uint64_t r1 = conv[0][i];
    uint64_t r2 = conv[1][i];
    uint64_t r3 = conv[2][i];

    // Two-step CRT: recover exact coefficient from 3 residues.
    uint64_t t1 = r1;
    uint64_t t2 = (r2 + P2 - t1 % P2) % P2 * INV_P1_MOD_P2 % P2;
    uint64_t r12 = t1 + t2 * P1;  // value mod P1*P2, fits in uint64_t
    uint64_t t3 = (r3 + P3 - r12 % P3) % P3 * INV_P1P2_MOD_P3 % P3;

    // t3 * P1 * P2 will overflow, we need to use uint128.
    result[i] = UInt128(t1) + UInt128(t2) * UInt128(P1) +
                UInt128(t3) * UInt128(P1) * UInt128(P2);
  }

  // Carry propagation: base-2^32 digits from 128-bit coefficients.
  digits.resize(padded_size);
  UInt128 carry = 0;
  for (size_t i = 0; i < padded_size; ++i) {
    UInt128 sum = result[i] + carry;
    digits[i] = static_cast<uint32_t>(static_cast<unsigned long long>(sum));
    carry = sum >> 32;
  }
  while (carry) {
    digits.push_back(
        static_cast<uint32_t>(static_cast<unsigned long long>(carry)));
    carry >>= 32;
  }

  is_negative = result_neg;
  trim();
  return *this;
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