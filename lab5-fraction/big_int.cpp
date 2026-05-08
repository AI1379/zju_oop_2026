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

// A fixed-width residue modulo 2^M + 1 (a Fermat pseudoprime), used by the
// Schönhage-Strassen Algorithm (SSA) / Fermat Number Transform (FNT) layer.
//
// SSA multiplies two large integers by embedding them as polynomials and
// performing pointwise multiplication via an FFT-like transform in the ring
// Z/(2^M+1)Z.  Unlike the NTT approach (which works modulo small primes and
// requires CRT reconstruction), SSA works in a single large Fermat ring where
// the "twiddle factors" are powers of 2 — so all multiplications in the
// transform reduce to shifts and adds, which are cheap on big-int limb arrays.
//
// Storage layout: the span holds (M/32 + 1) uint32_t limbs.
//   - limbs[0 .. M/32-1] are the low M bits of the value.
//   - limbs[M/32] is an extra "carry" limb that can hold 1 when the value
//     reaches or exceeds the modulus 2^M + 1, enabling easy normalization.
// M must be a multiple of 32.
// (FermatElem struct declared in big_int.hpp)

// Set a FermatElem to zero (all limbs = 0).
void fermat_zero(FermatElem x) { std::fill(x.limbs.begin(), x.limbs.end(), 0); }

// Copy src into dst (both must have the same size).
void fermat_copy(FermatElem dst, FermatElem src) {
  std::copy(src.limbs.begin(), src.limbs.end(), dst.limbs.begin());
}

// Check whether x >= 2^M + 1 (the modulus).
// The modulus in limb representation is: low limb = 1, all middle limbs = 0,
// high limb (index n = value_limbs()) = 1.
// So we check if x's high limb exceeds 1, or equals 1 with any middle limb
// nonzero, or equals 1 with all middle limbs zero but low limb >= 1.
bool fermat_ge_modulus(FermatElem x) {
  auto n = x.value_limbs();
  if (x.limbs[n] != 1) return x.limbs[n] > 1;

  for (size_t i = n; i-- > 1;) {
    if (x.limbs[i] != 0) return true;
  }
  return x.limbs[0] >= 1;
}

// Subtract the modulus (2^M + 1) from x in-place.
// The modulus has value 1 in the low limb and 1 in the high limb (index n),
// with all middle limbs = 0. We subtract the low-1 and high-1 separately.
void fermat_sub_modulus(FermatElem x) {
  auto n = x.value_limbs();

  // Subtract 1 from the low limb, propagating borrow through middle limbs.
  uint64_t borrow = 1;
  for (size_t i = 0; i < x.limbs.size() && borrow; ++i) {
    uint64_t cur = x.limbs[i];
    if (cur < borrow) {
      x.limbs[i] = uint32_t(cur + (1ULL << 32) - borrow);
      borrow = 1;
    } else {
      x.limbs[i] = uint32_t(cur - borrow);
      borrow = 0;
    }
  }

  // Subtract 1 from the high limb (index n). No further propagation needed.
  borrow = 1;
  uint64_t cur = x.limbs[n];
  if (cur < borrow) {
    x.limbs[n] = uint32_t(cur + (1ULL << 32) - borrow);
  } else {
    x.limbs[n] = uint32_t(cur - borrow);
  }
}

// Reduce x into the range [0, 2^M] by subtracting the modulus until x < 2^M+1.
// In practice the value rarely exceeds the modulus by more than a small
// multiple, so this loop executes at most 2-3 times.
void fermat_normalize(FermatElem x) {
  while (fermat_ge_modulus(x)) {
    fermat_sub_modulus(x);
  }
}

// Compare two FermatElem values lexicographically from the high limb down.
// Returns -1 if a < b, 0 if equal, 1 if a > b.
int fermat_compare(FermatElem a, FermatElem b) {
  for (size_t i = a.limbs.size(); i-- > 0;) {
    if (a.limbs[i] != b.limbs[i]) return a.limbs[i] < b.limbs[i] ? -1 : 1;
  }
  return 0;
}

// Raw addition/subtraction on FermatElem — delegates to the span-level
// add_into / sub_into, which are identical when all spans are the same size.
uint32_t fermat_add_raw(FermatElem out, FermatElem a, FermatElem b) {
  return add_into(out.limbs, a.limbs, b.limbs);
}

uint32_t fermat_sub_raw(FermatElem out, FermatElem a, FermatElem b) {
  return sub_into(out.limbs, a.limbs, b.limbs);
}

// Modular addition: out = (a + b) mod (2^M + 1).
// Adds first, then normalizes to bring the result below the modulus.
void fermat_add(FermatElem out, FermatElem a, FermatElem b) {
  fermat_add_raw(out, a, b);
  fermat_normalize(out);
}

// Modular subtraction: out = (a - b) mod (2^M + 1).
// If a >= b, a simple subtraction suffices.
// If a < b, we compute (2^M + 1 + a - b) instead — adding the modulus first
// limb-by-limb to avoid underflow, then normalizing.
void fermat_sub(FermatElem out, FermatElem a, FermatElem b) {
  if (fermat_compare(a, b) >= 0) {
    fermat_sub_raw(out, a, b);
    return;
  }

  // a < b: compute (2^M + 1) + a - b per limb.
  // Conceptually this is add_into(a, modulus) + sub_into(result, b), but the
  // two are fused into a single loop to handle aliasing: fermat_butterfly calls
  // fermat_sub(b, scratch, b) where out == b_param. A two-step decomposition
  // would overwrite b before reading it. The fused loop reads a[i] and b[i]
  // before writing out[i], so all aliasing patterns are safe.
  auto n = out.value_limbs();
  uint64_t carry = 0;
  uint64_t borrow = 0;
  for (size_t i = 0; i < out.limbs.size(); ++i) {
    uint64_t modulus_limb = (i == 0 || i == n) ? 1 : 0;
    uint64_t sum = uint64_t(a.limbs[i]) + modulus_limb + carry;
    carry = sum >> 32;

    uint64_t low = uint32_t(sum);
    uint64_t subtrahend = uint64_t(b.limbs[i]) + borrow;
    if (low < subtrahend) {
      out.limbs[i] = uint32_t(low + (1ULL << 32) - subtrahend);
      borrow = 1;
    } else {
      out.limbs[i] = uint32_t(low - subtrahend);
      borrow = 0;
    }
  }
  fermat_normalize(out);
}

// Multiply by 2^shift_bits in the Fermat ring: out = in * 2^s mod (2^M + 1).
//
// Key property: 2^M ≡ -1 (mod 2^M+1), so 2^(2M) ≡ 1, period = 2M.
//
// Algorithm: word-level shift into a double-width buffer, then reduce
// via the identity in*2^s = low + high*2^M ≡ low - high (mod 2^M+1).
// If the original shift was >= M, we additionally negate the result
// (since 2^(M+s) = -2^s).
void fermat_mul_pow2(FermatElem out, FermatElem in, size_t shift_bits) {
  auto M = in.modulus_bits();
  auto n = in.value_limbs();
  auto period = M * 2;
  if (period == 0) return;

  shift_bits %= period;
  if (shift_bits == 0) {
    if (out.limbs.data() != in.limbs.data()) fermat_copy(out, in);
    return;
  }

  bool neg = (shift_bits >= M);
  if (neg) shift_bits -= M;
  // shift_bits < M now.

  // Copy in's value limbs into a 2n-limb temp, then word-shift left.
  thread_local std::vector<uint32_t> tmp;
  tmp.assign(2 * n, 0);
  std::copy(in.limbs.begin(), in.limbs.begin() + n, tmp.begin());

  size_t limb_shift = shift_bits / 32;
  size_t bit_shift = shift_bits % 32;

  // Limb-level shift: move limbs up by limb_shift positions.
  for (size_t i = 2 * n; i-- > 0;) {
    tmp[i] = (i >= limb_shift) ? tmp[i - limb_shift] : 0;
  }
  // Sub-word bit shift.
  if (bit_shift > 0) {
    uint32_t carry = 0;
    for (size_t i = limb_shift; i < 2 * n; ++i) {
      uint32_t new_carry = tmp[i] >> (32 - bit_shift);
      tmp[i] = (tmp[i] << bit_shift) | carry;
      carry = new_carry;
    }
  }

  // Split at bit M (limb n): low = tmp[0..n-1], high = tmp[n..2n-1].
  // result = low - high mod (2^M+1).
  std::copy(tmp.begin(), tmp.begin() + n, out.limbs.begin());
  out.limbs[n] = 0;

  auto high = std::span<const uint32_t>(tmp.data() + n, n);
  auto borrow = sub_from(out.limbs.first(n), high);
  if (borrow) {
    // Underflow: stored value is (low + 2^M - high), need (low - high + 2^M + 1).
    // Difference is 1, so just add 1 and propagate carry.
    uint64_t carry = 1;
    for (size_t i = 0; i < n; ++i) {
      uint64_t s = uint64_t(out.limbs[i]) + carry;
      out.limbs[i] = uint32_t(s);
      carry = s >> 32;
    }
    out.limbs[n] = uint32_t(carry);
  }

  // If shift was >= M, negate: out = (2^M+1) - out.
  if (neg) {
    // Save current value into tmp (which is no longer needed).
    std::copy(out.limbs.begin(), out.limbs.end(), tmp.begin());
    // Set out to modulus: limbs[0]=1, limbs[1..n-1]=0, limbs[n]=1.
    out.limbs[0] = 1;
    for (size_t i = 1; i < n; ++i) out.limbs[i] = 0;
    out.limbs[n] = 1;
    // out = modulus - saved_value.
    sub_from(out.limbs, std::span<const uint32_t>(tmp.data(), n + 1));
  }

  fermat_normalize(out);
}

// Butterfly operation for the Fermat Number Transform (FNT).
// Given two FermatElem values and a "twiddle" shift s, computes:
//   a' = a + b * 2^s    (mod 2^M + 1)
//   b' = a - b * 2^s    (mod 2^M + 1)
//
// This is the analogue of an FFT butterfly, but the twiddle factor is 2^s
// instead of a complex root of unity. The FNT is a DFT over Z/(2^M+1)Z where
// 2 is a 2M-th root of unity (since 2^M ≡ -1, 2^(2M) ≡ 1).
//
// scratch is temporary storage used to save a copy of the original a.
void fermat_butterfly(FermatElem a, FermatElem b, size_t twiddle_shift,
                      FermatElem scratch) {
  fermat_copy(scratch, a);
  fermat_mul_pow2(b, b, twiddle_shift);
  fermat_add(a, scratch, b);
  fermat_sub(b, scratch, b);
}

// General modular multiplication: out = a * b mod (2^M + 1).
//
// A FermatElem stores value = a_low + a_high * 2^M where a_high is the carry
// limb (0 or 1).  Using 2^M ≡ -1 and 2^(2M) ≡ 1:
//   a * b = (a_low + a_h*2^M)(b_low + b_h*2^M)
//         ≡ a_low*b_low − a_h*b_low − b_h*a_low + a_h*b_h  (mod 2^M+1)
//
// Step 1: compute a_low * b_low via schoolbook + low-high reduction.
// Step 2: apply cross-term corrections (subtract b_low if a_h, etc.)
//         using fermat_sub/fermat_add which handle aliasing and normalization.
// scratch must hold at least 2 * a.value_limbs() uint32_t values.
void fermat_mul(FermatElem out, FermatElem a, FermatElem b,
                std::span<uint32_t> scratch) {
  auto n = a.value_limbs();

  // Save a_low and b_low before out is modified (out may alias a and/or b).
  thread_local std::vector<uint32_t> saved_a, saved_b;
  saved_a.assign(a.limbs.begin(), a.limbs.begin() + n);
  saved_a.push_back(0);
  saved_b.assign(b.limbs.begin(), b.limbs.begin() + n);
  saved_b.push_back(0);

  bool a_high = a.limbs[n] != 0;
  bool b_high = b.limbs[n] != 0;

  // Step 1: a_low * b_low, reduced mod (2^M+1).
  auto prod = scratch.subspan(0, 2 * n);
  std::fill(prod.begin(), prod.end(), 0);
  mul_naive_raw(std::span<const uint32_t>(saved_a).first(n),
                std::span<const uint32_t>(saved_b).first(n), prod);

  auto low = prod.subspan(0, n);
  auto high = prod.subspan(n, n);
  std::copy(low.begin(), low.end(), out.limbs.begin());
  out.limbs[n] = 0;

  auto borrow = sub_from(out.limbs.first(n), high);
  if (borrow) {
    uint64_t carry = 1;
    for (size_t i = 0; i < n; ++i) {
      uint64_t s = uint64_t(out.limbs[i]) + carry;
      out.limbs[i] = uint32_t(s);
      carry = s >> 32;
    }
    out.limbs[n] = uint32_t(carry);
  }

  // Step 2: corrections for carry limbs.
  FermatElem a_low_fe{std::span<uint32_t>(saved_a)};
  FermatElem b_low_fe{std::span<uint32_t>(saved_b)};

  if (a_high) fermat_sub(out, out, b_low_fe);
  if (b_high) fermat_sub(out, out, a_low_fe);
  if (a_high && b_high) {
    // +1 from a_h * b_h = 1
    uint64_t carry = 1;
    for (size_t i = 0; i <= n; ++i) {
      uint64_t s = uint64_t(out.limbs[i]) + carry;
      out.limbs[i] = uint32_t(s);
      carry = s >> 32;
    }
  }

  fermat_normalize(out);
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

// Fermat Number Transform (FNT) — iterative Cooley-Tukey over Z/(2^M+1)Z.
// Operates on a flat buffer of L FermatElem values, each elem_limbs uint32_t.
// The primitive root is 2 (a 2M-th root of unity). Twiddle factors are powers
// of 2, applied via fermat_butterfly.
//
// Forward:  X[k] = sum_{j=0}^{L-1} x[j] * 2^(j*k*2M/L)
// Inverse:  x[j] = (1/L) * sum_{k=0}^{L-1} X[k] * 2^(-j*k*2M/L)
// Division by L = 2^k is a shift by (2M - k) in the Fermat ring.
void fnt(std::span<uint32_t> buffer, size_t elem_limbs, size_t transform_len,
         bool inverse) {
  auto L = transform_len;
  auto M = (elem_limbs - 1) * 32;

  // Bit-reversal permutation (swap whole FermatElem blocks).
  for (size_t i = 1, j = 0; i < L; ++i) {
    auto bit = L >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      for (size_t k = 0; k < elem_limbs; ++k)
        std::swap(buffer[i * elem_limbs + k], buffer[j * elem_limbs + k]);
    }
  }

  // Scratch FermatElem for butterfly.
  thread_local std::vector<uint32_t> scratch_buf;
  scratch_buf.assign(elem_limbs, 0);
  FermatElem scratch{std::span<uint32_t>(scratch_buf)};

  // Butterfly stages.
  for (size_t len = 2; len <= L; len <<= 1) {
    size_t half = len / 2;
    size_t twiddle_step = 2 * M / len;
    if (inverse) twiddle_step = 2 * M - twiddle_step;

    for (size_t i = 0; i < L; i += len) {
      size_t twiddle_shift = 0;
      for (size_t j = 0; j < half; ++j) {
        FermatElem a{buffer.subspan((i + j) * elem_limbs, elem_limbs)};
        FermatElem b{buffer.subspan((i + j + half) * elem_limbs, elem_limbs)};

        fermat_butterfly(a, b, twiddle_shift, scratch);

        twiddle_shift += twiddle_step;
        if (twiddle_shift >= 2 * M) twiddle_shift -= 2 * M;
      }
    }
  }

  // Inverse: divide each element by L = 2^k, i.e. multiply by 2^(2M - k).
  if (inverse) {
    size_t log_L = 0;
    for (auto tmp = L; tmp > 1; tmp >>= 1) ++log_L;
    size_t inv_shift = 2 * M - log_L;
    for (size_t i = 0; i < L; ++i) {
      FermatElem elem{buffer.subspan(i * elem_limbs, elem_limbs)};
      fermat_mul_pow2(elem, elem, inv_shift);
    }
  }
}

// Count leading zeros of a 32-bit integer.
int clz32(uint32_t x) {
  if (x == 0) return 32;
#if FRACTION_COMPILER_MSVC
  unsigned long index;
  _BitScanReverse(&index, x);
  return 31 - static_cast<int>(index);
#elif FRACTION_COMPILER_CLANG || FRACTION_COMPILER_GCC
  return __builtin_clz(x);
#else
  int n = 0;
  if (x <= 0x0000FFFF) { n += 16; x <<= 16; }
  if (x <= 0x00FFFFFF) { n += 8; x <<= 8; }
  if (x <= 0x0FFFFFFF) { n += 4; x <<= 4; }
  if (x <= 0x3FFFFFFF) { n += 2; x <<= 2; }
  if (x <= 0x7FFFFFFF) { n += 1; }
  return n;
#endif
}

// Left-shift a digit vector by 'bits' bits (0 <= bits < 32).
// Analogous to add_to / sub_from — operates on raw limb storage.
void shift_left_small(std::vector<uint32_t>& digits, int bits) {
  if (bits == 0 || digits.empty()) return;
  uint32_t carry = 0;
  for (auto& d : digits) {
    uint64_t val = (static_cast<uint64_t>(d) << bits) | carry;
    d = static_cast<uint32_t>(val);
    carry = static_cast<uint32_t>(val >> 32);
  }
  if (carry) digits.push_back(carry);
}

// Span-level multiply dispatcher. No internal allocation.
// result: zero-initialized, size >= 2 * next_pow2(max(na, nb))
// temp:   size >= 6 * next_pow2(max(na, nb))
//   layout: [a_copy: padded][b_copy: padded][karatemp: 4*padded]
void mul_dispatch(std::span<const uint32_t> a, std::span<const uint32_t> b,
                  std::span<uint32_t> result, std::span<uint32_t> temp) {
  size_t na = a.size(), nb = b.size();
  if (na == 0 || nb == 0) {
    std::fill(result.begin(), result.end(), 0);
    return;
  }
  if (na < 32 || nb < 32) {
    std::fill(result.begin(), result.begin() + std::min(result.size(), na + nb), 0);
    mul_naive_raw(a, b, result);
    return;
  }
  size_t padded = 1;
  while (padded < na || padded < nb) padded *= 2;
  auto a_copy = temp.first(padded);
  auto b_copy = temp.subspan(padded, padded);
  auto ktemp = temp.subspan(2 * padded);
  std::fill(a_copy.begin(), a_copy.end(), 0);
  std::fill(b_copy.begin(), b_copy.end(), 0);
  std::copy(a.begin(), a.end(), a_copy.begin());
  std::copy(b.begin(), b.end(), b_copy.begin());
  auto r = result.first(2 * padded);
  std::fill(r.begin(), r.end(), 0);
  karatsuba_core(a_copy, b_copy, r, ktemp);
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

// Schönhage-Strassen multiplication.
// Strategy: split inputs into pieces of M bits, embed as polynomials,
// perform FNT (Fermat Number Transform), multiply pointwise mod (2^M+1),
// then inverse FNT and carry propagation.
//
// The FermatElem + fermat_butterfly infrastructure above provides the
// modular arithmetic primitives. Still needs: FNT implementation, the
// top-level mul_ssa driver that chooses M, splits operands, and assembles
// the final product.
BigInt& BigInt::mul_ssa(const BigInt& other) {
  bool result_neg = (is_negative != other.is_negative);

  size_t n = digits.size();
  size_t m = other.digits.size();

  if ((n == 1 && digits[0] == 0) || (m == 1 && other.digits[0] == 0)) {
    *this = BigInt(0);
    return *this;
  }

  if (this == &other) {
    BigInt tmp = other;
    return mul_ssa(tmp);
  }

  // Fallback for moderate sizes (SSA has high overhead).
  if (n < 64 || m < 64) return mul_naive(other);
  if (n < 1024 || m < 1024) return mul_fft_ntt(other);

  // === Parameter selection ===
  // Choose transform length L = 2^k and modulus M (multiple of 32) such that:
  //   1) L <= 2M  (root of unity: 2 has order 2M)
  //   2) 2 * piece_bits + k <= M  (convolution coefficients don't wrap)
  //   3) L * piece_limbs >= n + m  (enough room for the result)
  // Pick the (k, M) pair that minimizes M * L (total work ~ L*log(L)*M/32).
  size_t total_limbs = n + m;
  size_t best_k = 0, best_M = 0, best_piece_limbs = 0;
  size_t best_cost = SIZE_MAX;

  for (size_t k = 4; k <= 24; ++k) {
    size_t L = size_t(1) << k;
    size_t piece_limbs = (total_limbs + L - 1) / L;
    if (piece_limbs == 0) piece_limbs = 1;
    size_t piece_bits = piece_limbs * 32;
    size_t M_needed = 2 * piece_bits + k + 1;
    // Round up to multiple of 32.
    M_needed = ((M_needed + 31) / 32) * 32;
    if (M_needed < 64) M_needed = 64;

    // Root of unity constraint: L <= 2M and L must divide 2M exactly
    // (so that twiddle_step = 2M/L is an integer).
    if (L > 2 * M_needed) {
      M_needed = ((L + 31) / 32) * 32;
    }
    while ((2 * M_needed) % L != 0) {
      M_needed += 32;
    }

    // Verify coefficient constraint still holds after adjusting M.
    if (2 * piece_bits + k <= M_needed && L <= 2 * M_needed) {
      // Cost ~ L pointwise muls of (M/32)-limb values ~ L * (M/32)^2.
      size_t m_limbs = M_needed / 32;
      size_t cost = L * m_limbs * m_limbs;
      if (cost < best_cost) {
        best_cost = cost;
        best_k = k;
        best_M = M_needed;
        best_piece_limbs = piece_limbs;
      }
    }
  }

  size_t k = best_k;
  size_t L = size_t(1) << k;
  size_t M = best_M;
  size_t piece_limbs = best_piece_limbs;
  size_t elem_limbs = M / 32 + 1;

  // === Allocate buffers ===
  std::vector<uint32_t> buf_a(L * elem_limbs, 0);
  std::vector<uint32_t> buf_b(L * elem_limbs, 0);

  // === Split inputs into L pieces of piece_limbs limbs each ===
  for (size_t i = 0; i < L; ++i) {
    auto* dst = buf_a.data() + i * elem_limbs;
    size_t src_start = i * piece_limbs;
    size_t src_count = (src_start < n) ? std::min(piece_limbs, n - src_start) : 0;
    for (size_t j = 0; j < src_count; ++j) dst[j] = digits[src_start + j];
  }
  for (size_t i = 0; i < L; ++i) {
    auto* dst = buf_b.data() + i * elem_limbs;
    size_t src_start = i * piece_limbs;
    size_t src_count = (src_start < m) ? std::min(piece_limbs, m - src_start) : 0;
    for (size_t j = 0; j < src_count; ++j) dst[j] = other.digits[src_start + j];
  }

  // === Forward FNT on both arrays ===
  detail::fnt(std::span<uint32_t>(buf_a), elem_limbs, L, false);
  detail::fnt(std::span<uint32_t>(buf_b), elem_limbs, L, false);

  // === Pointwise multiply ===
  std::vector<uint32_t> mul_scratch(2 * (M / 32), 0);
  for (size_t i = 0; i < L; ++i) {
    detail::FermatElem a{std::span<uint32_t>(buf_a.data() + i * elem_limbs, elem_limbs)};
    detail::FermatElem b{std::span<uint32_t>(buf_b.data() + i * elem_limbs, elem_limbs)};
    detail::fermat_mul(a, a, b, std::span<uint32_t>(mul_scratch));
  }

  // === Inverse FNT ===
  detail::fnt(std::span<uint32_t>(buf_a), elem_limbs, L, true);

  // === Carry propagation ===
  // Each inverse-FNT coefficient c_i (up to M bits = elem_limbs limbs) is placed
  // at limb offset i * piece_limbs.  Coefficients overlap, so add_to propagates
  // carry through the full output.
  digits.assign(total_limbs, 0);
  for (size_t i = 0; i < L; ++i) {
    size_t dst_start = i * piece_limbs;
    if (dst_start >= total_limbs) break;

    auto piece = std::span<const uint32_t>(buf_a.data() + i * elem_limbs, elem_limbs);
    auto dst = std::span<uint32_t>(digits).subspan(dst_start);
    detail::add_to(dst, piece);
  }

  is_negative = result_neg;
  trim();
  return *this;
}

BigInt& BigInt::divide_inplace(const BigInt& other, DivAlgo algo) {
  switch (algo) {
    case DivAlgo::Auto:
    case DivAlgo::Naive:
      return div_naive(other);
    case DivAlgo::Newton:
      return div_newton(other);
    case DivAlgo::Burnikel_Ziegler:
      return div_burnikel_ziegler(other);
  }
  return *this;
}

BigInt& BigInt::div_naive(const BigInt& other) {
  if (other.digits.empty() || (other.digits.size() == 1 && other.digits[0] == 0)) {
    throw std::invalid_argument("Division by zero");
  }

  bool result_neg = (is_negative != other.is_negative);

  BigInt divisor = other;
  divisor.is_negative = false;
  BigInt dividend = std::move(*this);
  dividend.is_negative = false;

  if (abs_compare(dividend, divisor) < 0) {
    *this = BigInt(0);
    is_negative = false;
    return *this;
  }

  // Knuth's Algorithm D: normalize so leading digit of divisor >= BASE/2
  uint32_t d_norm = static_cast<uint32_t>(
      (UINT64_C(1) << 32) / (static_cast<uint64_t>(divisor.digits.back()) + 1));

  auto mul_limb = [](std::vector<uint32_t>& v, uint32_t limb) {
    uint64_t carry = 0;
    for (auto& x : v) {
      uint64_t p = static_cast<uint64_t>(x) * limb + carry;
      x = static_cast<uint32_t>(p);
      carry = p >> 32;
    }
    if (carry) v.push_back(static_cast<uint32_t>(carry));
  };

  mul_limb(dividend.digits, d_norm);
  mul_limb(divisor.digits, d_norm);

  size_t n = divisor.digits.size();
  size_t q_size = dividend.digits.size() - n + 1;

  // Pad with one zero at the top so dividend[j+n] is always valid
  dividend.digits.push_back(0);

  std::vector<uint32_t> q_digits(q_size, 0);

  for (size_t j = q_size; j-- > 0;) {
    uint64_t hi2 = (static_cast<uint64_t>(dividend.digits[j + n]) << 32) |
                   dividend.digits[j + n - 1];
    uint64_t q_hat = hi2 / divisor.digits[n - 1];
    uint64_t r_hat = hi2 % divisor.digits[n - 1];

    // Refine estimate (only when divisor has >= 2 limbs)
    if (n >= 2) {
      while (q_hat >= (UINT64_C(1) << 32) ||
             q_hat * divisor.digits[n - 2] >
                 ((r_hat << 32) | dividend.digits[j + n - 2])) {
        --q_hat;
        r_hat += divisor.digits[n - 1];
        if (r_hat >= (UINT64_C(1) << 32)) break;
      }
    } else if (q_hat >= (UINT64_C(1) << 32)) {
      q_hat = 0xFFFFFFFF;
    }

    // Multiply and subtract: dividend[j..j+n] -= q_hat * divisor
    int64_t borrow = 0;
    for (size_t i = 0; i < n; ++i) {
      uint64_t prod = q_hat * divisor.digits[i];
      int64_t diff = static_cast<int64_t>(dividend.digits[j + i]) -
                     static_cast<int64_t>(static_cast<uint32_t>(prod)) - borrow;
      dividend.digits[j + i] = static_cast<uint32_t>(diff);
      borrow = static_cast<int64_t>(prod >> 32) - (diff >> 32);
    }
    int64_t diff = static_cast<int64_t>(dividend.digits[j + n]) - borrow;
    dividend.digits[j + n] = static_cast<uint32_t>(diff);

    q_digits[j] = static_cast<uint32_t>(q_hat);

    if (diff < 0) {
      --q_digits[j];
      uint64_t carry = 0;
      for (size_t i = 0; i < n; ++i) {
        uint64_t s = static_cast<uint64_t>(dividend.digits[j + i]) +
                     divisor.digits[i] + carry;
        dividend.digits[j + i] = static_cast<uint32_t>(s);
        carry = s >> 32;
      }
      dividend.digits[j + n] += static_cast<uint32_t>(carry);
    }
  }

  digits = std::move(q_digits);
  is_negative = result_neg;
  trim();
  return *this;
}

// ============================================================
// Newton-Raphson division
// ============================================================

// Zero-allocation Newton reciprocal core.
// Computes floor(B^target_prec / d) using Newton iteration.
// All data is passed as spans — no BigInt temporaries.
//
// Newton step: R_{k+1} = R_k * (2 * B^{p_k} - d * R_k), precision doubles.
//
// scratch layout at each level:
//   [r_buf: half_prec+1][mul_buf: 2*target_prec][karatemp: rest]
// Total scratch needed: ~10 * target_prec limbs.
//
// Returns the number of significant limbs written to out.
size_t BigInt::newton_reciprocal_core(std::span<const uint32_t> d,
                              size_t target_prec,
                              std::span<uint32_t> out,
                              std::span<uint32_t> scratch) {
  static constexpr size_t kBaseCaseThreshold = 64;
  size_t d_size = d.size();

  // Trim d to significant limbs
  while (d_size > 0 && d[d_size - 1] == 0) --d_size;
  d = d.first(d_size);

  // Base case: fall back to BigInt div_mod (only at recursion leaves).
  if (target_prec <= kBaseCaseThreshold || target_prec <= d_size + 1) {
    // Build numerator B^target_prec in out
    std::fill(out.begin(), out.begin() + target_prec, 0);
    out[target_prec] = 1;

    BigInt numerator;
    numerator.digits.assign(out.data(), out.data() + target_prec + 1);
    numerator.is_negative = false;

    BigInt divisor_bi;
    divisor_bi.digits.assign(d.begin(), d.end());
    divisor_bi.is_negative = false;

    auto [q, r] = numerator.div_mod(divisor_bi, DivAlgo::Naive);

    std::fill(out.begin(), out.end(), 0);
    std::copy(q.digits.begin(), q.digits.end(), out.begin());
    return q.digits.size();
  }

  size_t half_prec = (target_prec + 1) / 2;

  // --- Recursive call: r = B^{half_prec} / d ---
  // Writes r into out, uses scratch for its own temp.
  size_t r_size = newton_reciprocal_core(d, half_prec, out, scratch);

  // Copy r from out into scratch r_buf (first half_prec+1 limbs).
  auto r_buf = scratch.first(half_prec + 1);
  std::fill(r_buf.begin(), r_buf.end(), 0);
  std::copy(out.begin(), out.begin() + r_size, r_buf.begin());
  auto r_span = r_buf.first(r_size);

  // Subdivide remaining scratch:
  //   mul_buf:  2 * target_prec limbs (shared by dr and R_new, never live together)
  //   karatemp: rest (for mul_dispatch)
  auto mul_buf = scratch.subspan(half_prec + 1, 2 * target_prec);
  auto karatemp = scratch.subspan(half_prec + 1 + 2 * target_prec);

  // --- dr = d * r ---
  std::fill(mul_buf.begin(), mul_buf.end(), 0);
  detail::mul_dispatch(d, r_span, mul_buf, karatemp);
  size_t dr_size = d_size + r_size;
  while (dr_size > 0 && mul_buf[dr_size - 1] == 0) --dr_size;
  auto dr = mul_buf.first(dr_size);

  // --- corr = 2 * B^{half_prec} - dr ---
  // Construct 2*B^{half_prec} inline in out, then subtract dr.
  size_t corr_limbs = half_prec + 1;
  std::fill(out.begin(), out.begin() + corr_limbs, 0);
  out[half_prec] = 2;
  detail::sub_from(out.first(corr_limbs), dr);
  // Trim corr
  while (corr_limbs > 0 && out[corr_limbs - 1] == 0) --corr_limbs;
  auto corr = out.first(corr_limbs);

  // --- R_new = r * corr ---
  // Overwrites mul_buf (dr is consumed).
  std::fill(mul_buf.begin(), mul_buf.end(), 0);
  detail::mul_dispatch(r_span, corr, mul_buf, karatemp);
  size_t rn_size = r_size + corr_limbs;
  while (rn_size > 0 && mul_buf[rn_size - 1] == 0) --rn_size;

  // Copy result from mul_buf to out
  std::fill(out.begin(), out.end(), 0);
  std::copy(mul_buf.begin(), mul_buf.begin() + rn_size, out.begin());

  return rn_size;
}

// Thin wrapper: allocate buffers, call span-level core.
BigInt BigInt::compute_reciprocal(const BigInt& d, size_t target_prec) {
  std::vector<uint32_t> out_buf(2 * target_prec + 1, 0);
  std::vector<uint32_t> scratch(10 * target_prec, 0);

  size_t rsize = newton_reciprocal_core(
      std::span<const uint32_t>(d.digits), target_prec,
      std::span<uint32_t>(out_buf), std::span<uint32_t>(scratch));

  BigInt result;
  result.digits.assign(out_buf.begin(), out_buf.begin() + rsize);
  result.is_negative = false;
  result.trim();
  return result;
}

BigInt& BigInt::div_newton(const BigInt& other) {
  if (other.digits.empty() ||
      (other.digits.size() == 1 && other.digits[0] == 0)) {
    throw std::invalid_argument("Division by zero");
  }

  bool result_neg = (is_negative != other.is_negative);

  BigInt divisor = other;
  divisor.is_negative = false;
  BigInt dividend = *this;
  dividend.is_negative = false;

  if (abs_compare(dividend, divisor) < 0) {
    *this = BigInt(0);
    return *this;
  }

  size_t n = divisor.size();

  // For small divisors, Knuth's Algorithm D is faster (less overhead).
  if (n <= 64) {
    return div_naive(other);
  }

  // Normalize: shift divisor so its leading limb has bit 31 set.
  int shift = detail::clz32(divisor.digits.back());
  if (shift > 0) {
    detail::shift_left_small(divisor.digits, shift);
    detail::shift_left_small(dividend.digits, shift);
  }

  size_t m = dividend.size();
  size_t prec = m + 1;

  // Compute reciprocal using span-based core (2 vector allocations).
  std::vector<uint32_t> R_buf(2 * prec + 1, 0);
  std::vector<uint32_t> recip_scratch(10 * prec, 0);

  size_t R_size = newton_reciprocal_core(
      std::span<const uint32_t>(divisor.digits), prec,
      std::span<uint32_t>(R_buf), std::span<uint32_t>(recip_scratch));

  // q = floor(dividend * R / B^prec)
  // Span-level multiply into pre-allocated product buffer.
  size_t prod_size = m + R_size;
  std::vector<uint32_t> product(prod_size, 0);

  if (prod_size > 0) {
    // Allocate mul_dispatch temp: 6 * next_pow2(max(m, R_size))
    size_t padded = 1;
    while (padded < m || padded < R_size) padded *= 2;
    std::vector<uint32_t> mul_temp(6 * padded, 0);

    detail::mul_dispatch(
        std::span<const uint32_t>(dividend.digits),
        std::span<const uint32_t>(R_buf).first(R_size),
        std::span<uint32_t>(product),
        std::span<uint32_t>(mul_temp));
  }

  // Extract upper limbs (right shift by prec limbs).
  if (prod_size > prec) {
    digits.assign(product.begin() + prec, product.end());
  } else {
    digits.clear();
  }
  is_negative = result_neg;
  trim();

  if (digits.empty()) {
    *this = BigInt(0);
    return *this;
  }

  // Newton's method may be off by at most 1. Adjust.
  BigInt one(1);
  BigInt test = (*this + one) * divisor;
  if (test <= dividend) {
    *this += one;
  }
  test = *this * divisor;
  while (test > dividend) {
    *this -= one;
    test = *this * divisor;
  }

  return *this;
}

BigInt& BigInt::div_burnikel_ziegler(const BigInt&) {
  throw std::runtime_error("Burnikel-Ziegler division not implemented");
}

// ============================================================
// Divide-and-conquer decimal conversion
// ============================================================

namespace {

// Convert a non-negative BigInt that fits in uint64_t to a decimal string,
// zero-padded to exactly `width` digits.
std::string small_to_dec_padded(uint64_t val, int width) {
  std::string s(width, '0');
  for (int i = width - 1; i >= 0 && val > 0; --i) {
    s[i] = '0' + static_cast<char>(val % 10);
    val /= 10;
  }
  return s;
}

// Precompute pow10_table[k] = BigInt(10^(2^k)) for k = 0..max_level.
std::vector<BigInt> precompute_pow10_table(int max_level) {
  std::vector<BigInt> table(max_level + 1);
  table[0] = BigInt(10);
  for (int i = 1; i <= max_level; ++i) {
    table[i] = table[i - 1];
    table[i] *= table[i - 1];  // square: 10^(2^i) = (10^(2^(i-1)))^2
  }
  return table;
}

// Estimate the number of decimal digits from the number of base-2^32 limbs.
// Each limb is ~9.63 decimal digits; we round up.
int estimate_dec_digits(const BigInt& x) {
  if (x.size() == 0) return 1;
  // Top limb contribution
  uint32_t top = x.size() > 0 ? 0 : 0;
  // Count bits
  int bits = static_cast<int>(x.size()) * 32;
  return static_cast<int>(static_cast<double>(bits) * 0.30103) + 2;
}

// Recursive helper: convert X to exactly 2^level decimal digits (zero-padded).
// X must be non-negative and X < pow10[level+1].
std::string to_dec_padded(BigInt X, int level,
                          const std::vector<BigInt>& pow10) {
  if (level <= 3) {
    // Base case: at most 8 decimal digits, fits in uint64_t
    return small_to_dec_padded(X.to_uint64(), 1 << level);
  }

  auto [q, r] = X.div_mod(pow10[level - 1]);

  std::string high = to_dec_padded(std::move(q), level - 1, pow10);
  std::string low = to_dec_padded(std::move(r), level - 1, pow10);

  return high + low;
}

}  // anonymous namespace

std::string BigInt::to_string() const {
  // Handle zero
  if (digits.empty()) return "0";

  // Handle sign
  std::string sign;
  BigInt abs_val = *this;
  if (is_negative) {
    sign = "-";
    abs_val.is_negative = false;
  }

  // Estimate decimal digits
  int d = estimate_dec_digits(abs_val);

  // Find the top level: largest k such that 2^k >= d
  int level = 0;
  while ((1 << (level + 1)) < d) ++level;

  // Precompute powers of 10
  auto pow10 = precompute_pow10_table(level);

  // Split: abs_val = Q * 10^(2^level) + R
  auto [q, r] = abs_val.div_mod(pow10[level]);

  std::string high = to_dec_padded(std::move(q), level, pow10);
  std::string low = to_dec_padded(std::move(r), level, pow10);

  // Remove leading zeros from high part (but not from low)
  size_t first_nonzero = high.find_first_not_of('0');
  if (first_nonzero == std::string::npos) {
    // Entire number is just the low part
    size_t lz = low.find_first_not_of('0');
    if (lz == std::string::npos) return "0";
    return sign + low.substr(lz);
  }

  return sign + high.substr(first_nonzero) + low;
}

// ============================================================
// from_dec_string (Horner's method — only uses multiply and add)
// ============================================================

BigInt BigInt::from_dec_string(const std::string& dec_str) {
  BigInt result;
  result.is_negative = false;

  size_t start = 0;
  if (!dec_str.empty() && dec_str[0] == '-') {
    result.is_negative = true;
    start = 1;
  } else if (!dec_str.empty() && dec_str[0] == '+') {
    start = 1;
  }

  if (start >= dec_str.size()) {
    throw std::invalid_argument("Empty number string");
  }

  // Process 9 decimal digits at a time for efficiency
  // (10^9 < 2^32, so each chunk fits in one limb)
  static constexpr uint32_t CHUNK = 1000000000;  // 10^9
  static constexpr int CHUNK_DIGITS = 9;

  size_t len = dec_str.size() - start;

  // Handle leading digits (less than CHUNK_DIGITS)
  size_t leading = len % CHUNK_DIGITS;
  if (leading > 0) {
    uint32_t val = 0;
    for (size_t i = 0; i < leading; ++i) {
      if (dec_str[start + i] < '0' || dec_str[start + i] > '9') {
        throw std::invalid_argument("Invalid decimal character");
      }
      val = val * 10 + (dec_str[start + i] - '0');
    }
    result.digits.push_back(val);
  }

  // Process remaining chunks
  for (size_t i = leading; i < len; i += CHUNK_DIGITS) {
    uint32_t val = 0;
    for (int j = 0; j < CHUNK_DIGITS; ++j) {
      char c = dec_str[start + i + j];
      if (c < '0' || c > '9') {
        throw std::invalid_argument("Invalid decimal character");
      }
      val = val * 10 + (c - '0');
    }
    // result = result * 10^9 + val
    // Multiply by CHUNK (single-limb multiply)
    uint64_t carry = 0;
    for (auto& d : result.digits) {
      uint64_t p = static_cast<uint64_t>(d) * CHUNK + carry;
      d = static_cast<uint32_t>(p);
      carry = p >> 32;
    }
    if (carry) result.digits.push_back(static_cast<uint32_t>(carry));

    // Add val
    carry = val;
    for (auto& d : result.digits) {
      if (carry == 0) break;
      uint64_t s = static_cast<uint64_t>(d) + carry;
      d = static_cast<uint32_t>(s);
      carry = s >> 32;
    }
    if (carry) result.digits.push_back(static_cast<uint32_t>(carry));
  }

  result.trim();
  return result;
}

}  // namespace fraction