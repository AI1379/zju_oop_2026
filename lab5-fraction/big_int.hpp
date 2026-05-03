//
// Created by Renatus Madrigal on 2026/04/23.
//

#pragma once
#ifndef LAB5_FRACTION_BIG_INT_HPP
#define LAB5_FRACTION_BIG_INT_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace fraction {
enum class MulAlgo { Auto, Naive, Karatsuba, FFT_NTT, SSA };
enum class DivAlgo { Auto, Naive, Newton, Burnikel_Ziegler };

class BigInt {
 public:
  BigInt() : is_negative(false) {}
  BigInt(int64_t value) {
    if (value < 0) {
      is_negative = true;
      value = -value;
    } else {
      is_negative = false;
    }
    while (value > 0) {
      digits.push_back(static_cast<uint32_t>(value & 0xFFFFFFFF));
      value >>= 32;
    }
  }

  BigInt(const std::string& str) {
    if (str.ends_with("h") || str.ends_with("H")) {
      *this = from_hex_string(str.substr(0, str.size() - 1));
    } else {
      *this = from_dec_string(str);
    }
  }

  static BigInt from_dec_string(const std::string& dec_str);

  static BigInt from_hex_string(const std::string& hex_str) {
    BigInt result;
    size_t start = 0;
    if (!hex_str.empty() && hex_str[0] == '-') {
      result.is_negative = true;
      start = 1;
    }
    for (size_t i = hex_str.size(); i > start;) {
      uint32_t digit = 0;
      size_t chunk_start = (i >= start + 8) ? i - 8 : start;
      for (size_t j = chunk_start; j < i; ++j) {
        uint8_t value = result.hex_char_to_digit(hex_str[j]);
        if (value == static_cast<uint8_t>(-1)) {
          throw std::invalid_argument("Invalid hex character");
        }
        digit = (digit << 4) | value;
      }
      result.digits.push_back(digit);
      i = chunk_start;
    }
    result.trim();
    return result;
  }

  // Arithmetic operations
  BigInt operator+(const BigInt& other) const {
    auto result = *this;
    result += other;
    return result;
  }

  BigInt operator-(const BigInt& other) const {
    auto result = *this;
    result -= other;
    return result;
  }

  BigInt operator*(const BigInt& other) const { return multiply(other); }
  BigInt operator/(const BigInt& other) const { return divide(other); }
  BigInt operator%(const BigInt& other) const { return mod(other); }

  BigInt& operator+=(const BigInt& other) {
    if (is_negative == other.is_negative) {
      add_abs(other);
    } else {
      if (abs_compare(*this, other) >= 0) {
        sub_abs(other);
      } else {
        BigInt temp = other;
        temp.sub_abs(*this);
        *this = std::move(temp);
      }
    }
    return *this;
  }

  BigInt& operator-=(const BigInt& other) {
    if (is_negative != other.is_negative) {
      add_abs(other);
    } else {
      if (abs_compare(*this, other) >= 0) {
        sub_abs(other);
      } else {
        BigInt temp = other;
        temp.sub_abs(*this);
        temp.is_negative = !temp.is_negative;  // Flip the sign
        *this = std::move(temp);
      }
    }
    return *this;
  }

  BigInt& operator*=(const BigInt& other) { return multiply_inplace(other); }
  BigInt& operator/=(const BigInt& other) { return divide_inplace(other); }
  BigInt& operator%=(const BigInt& other) {
    *this = this->mod(other);
    return *this;
  }

  BigInt operator-() const {
    BigInt result = *this;
    if (!result.digits.empty()) {
      result.is_negative = !result.is_negative;
    }
    return result;
  }

  // In-place operations for multiplication and division, which can be optimized
  // The algorithm can be specified by the caller, so these functions are public
  BigInt& multiply_inplace(const BigInt& other, MulAlgo algo = MulAlgo::Auto) {
    // This function is just a dispatcher.
    switch (algo) {
      case MulAlgo::Auto: {
        // Dispatch to the appropriate algorithm based on operand sizes
        if (digits.size() < 32 || other.digits.size() < 32) {
          return mul_naive(other);
        } else if (digits.size() < 256 || other.digits.size() < 256) {
          return mul_karatsuba(other);
        } else if (digits.size() < 1024 || other.digits.size() < 1024) {
          // For very large numbers, we can use FFT-based multiplication
          return mul_fft_ntt(other);
        } else {
          // For extremely large numbers, we can use the SSA algorithm
          return mul_ssa(other);
        }
      }
      case MulAlgo::Naive:
        return mul_naive(other);
      case MulAlgo::Karatsuba:
        return mul_karatsuba(other);
      case MulAlgo::FFT_NTT:
        return mul_fft_ntt(other);
      case MulAlgo::SSA:
        return mul_ssa(other);
    }
    return *this;
  }
  BigInt& divide_inplace(const BigInt& other, DivAlgo algo = DivAlgo::Auto);

  BigInt multiply(const BigInt& other, MulAlgo algo = MulAlgo::Auto) const {
    BigInt result = *this;
    result.multiply_inplace(other, algo);
    return result;
  }
  BigInt divide(const BigInt& other, DivAlgo algo = DivAlgo::Auto) const {
    BigInt result = *this;
    result.divide_inplace(other, algo);
    return result;
  }
  BigInt mod(const BigInt& other, DivAlgo algo = DivAlgo::Auto) const {
    return *this - (this->divide(other, algo) * other);
  }

  // Comparison operators
  friend bool operator==(const BigInt& lhs, const BigInt& rhs) {
    if (lhs.is_negative != rhs.is_negative) {
      return false;
    }
    // We cannot compare the size of digits if the numbers have leading zeros
    // if (lhs.digits.size() != rhs.digits.size()) {
    //   return false;
    // }

    auto n = std::min(lhs.digits.size(), rhs.digits.size());

    // Not sure if compiler will optimize this loop. Maybe SIMD is needed for
    // better performance.
    for (size_t i = 0; i < n; ++i) {
      if (lhs.digits[i] != rhs.digits[i]) {
        return false;
      }
    }
    for (size_t i = n; i < lhs.digits.size(); ++i) {
      if (lhs.digits[i] != 0) {
        return false;
      }
    }
    for (size_t i = n; i < rhs.digits.size(); ++i) {
      if (rhs.digits[i] != 0) {
        return false;
      }
    }
    return true;
  }

  friend auto operator<=>(const BigInt& lhs, const BigInt& rhs) {
    // A helper macro to reduce code duplication when we find a non-equal digit
#define EQUAL_OR_RETURN(expr) \
  return (expr) ? std::strong_ordering::greater : std::strong_ordering::less
    if (lhs.is_negative != rhs.is_negative) {
      EQUAL_OR_RETURN(rhs.is_negative);
    }

    for (size_t i = rhs.digits.size(); i < lhs.digits.size(); ++i) {
      if (lhs.digits[i] != 0) {
        EQUAL_OR_RETURN(!lhs.is_negative);
      }
    }
    for (size_t i = lhs.digits.size(); i < rhs.digits.size(); ++i) {
      if (rhs.digits[i] != 0) {
        EQUAL_OR_RETURN(rhs.is_negative);
      }
    }

    auto n = std::min(lhs.digits.size(), rhs.digits.size());

    for (size_t i = n; i-- > 0;) {
      if (lhs.digits[i] != rhs.digits[i]) {
        if (lhs.is_negative) {
          EQUAL_OR_RETURN(lhs.digits[i] < rhs.digits[i]);
        } else {
          EQUAL_OR_RETURN(lhs.digits[i] > rhs.digits[i]);
        }
      }
    }
    return std::strong_ordering::equal;
#undef EQUAL_OR_RETURN
  }

  static int abs_compare(const BigInt& a, const BigInt& b) {
    for (size_t i = b.digits.size(); i < a.digits.size(); ++i) {
      if (a.digits[i] != 0) return 1;
    }
    for (size_t i = a.digits.size(); i < b.digits.size(); ++i) {
      if (b.digits[i] != 0) return -1;
    }
    auto n = std::min(a.digits.size(), b.digits.size());
    for (size_t i = n; i-- > 0;) {
      if (a.digits[i] != b.digits[i]) {
        return (a.digits[i] > b.digits[i]) ? 1 : -1;
      }
    }
    return 0;
  }

  // Utility functions

  // Decimal string representation is difficult if use base 2^32, so
  // we keep it unimplemented for now
  std::string to_string() const;

  // Hexadecimal string representation is easier to implement with base 2^32
  std::string to_hex_string() const {
    if (digits.empty()) {
      return "0";
    }

    std::string hex_str = is_negative ? "-" : "";
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
      // Each digit is a 32-bit number, so we convert it to 8 hex digits
      // The compiler will optimize this buffer usage so we declare it here
      char buffer[9];  // 8 hex digits + null terminator
      std::snprintf(buffer, sizeof(buffer), "%08x", *it);
      hex_str += buffer;
    }

    // We do not remove leading zeros here to better debug the internal
    // representation, but we can trim them if needed

    return hex_str;
  }

  // Get the number of digits (in base 2^32)
  size_t size() const { return digits.size(); }

  // Trim leading zeros
  void trim() {
    while (!digits.empty() && digits.back() == 0) {
      digits.pop_back();
    }
    if (digits.empty()) {
      is_negative = false;  // Zero is not negative
    }
  }

 private:
  // Use 2^32 as the base and auto overflow
  std::vector<uint32_t> digits;  // Each element stores a part of the number
  bool is_negative;              // Sign of the number

  uint8_t hex_char_to_digit(char c) const {
    if (c >= '0' && c <= '9') {
      return c - '0';
    } else if (c >= 'a' && c <= 'f') {
      return 10 + (c - 'a');
    } else if (c >= 'A' && c <= 'F') {
      return 10 + (c - 'A');
    }
    throw std::invalid_argument("Invalid hex character");
  }

  // Internal helper functions for arithmetic operations.
  // Implementations in .cpp — they delegate to span-level primitives
  // in the detail namespace, sharing logic with karatsuba_core.
  void add_abs(const BigInt& other);
  void sub_abs(const BigInt& other);
  BigInt& mul_naive(const BigInt& other);

  BigInt& mul_karatsuba(const BigInt& other);
  BigInt& mul_fft_ntt(const BigInt& other);
  BigInt& mul_ssa(const BigInt& other);

  BigInt& div_naive(const BigInt& other);
  BigInt& div_newton(const BigInt& other);
  BigInt& div_burnikel_ziegler(const BigInt& other);
};

// Expose detail types for testing.
namespace detail {
struct FermatElem {
  std::span<uint32_t> limbs;
  size_t value_limbs() const { return limbs.size() - 1; }
  size_t modulus_bits() const { return value_limbs() * 32; }
};

void fermat_zero(FermatElem x);
void fermat_copy(FermatElem dst, FermatElem src);
void fermat_normalize(FermatElem x);
void fermat_add(FermatElem out, FermatElem a, FermatElem b);
void fermat_sub(FermatElem out, FermatElem a, FermatElem b);
void fermat_mul_pow2(FermatElem out, FermatElem in, size_t shift_bits);
void fermat_butterfly(FermatElem a, FermatElem b, size_t twiddle_shift,
                      FermatElem scratch);
void fermat_mul(FermatElem out, FermatElem a, FermatElem b,
                std::span<uint32_t> scratch);
void fnt(std::span<uint32_t> buffer, size_t elem_limbs, size_t transform_len,
         bool inverse);
}  // namespace detail

}  // namespace fraction

#endif  // LAB5_FRACTION_BIG_INT_HPP