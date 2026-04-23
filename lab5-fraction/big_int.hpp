//
// Created by Renatus Madrigal on 2026/04/23.
//

#pragma once
#ifndef LAB5_FRACTION_BIG_INT_HPP
#define LAB5_FRACTION_BIG_INT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fraction {
enum class MulAlgo { Auto, Naive, Karatsuba, FFT, SSA };

class BigInt {
 public:
  BigInt() : is_negative(false) {}
  BigInt(int64_t value);
  BigInt(const std::string& str);

  // Arithmetic operations
  BigInt operator+(const BigInt& other) const;
  BigInt operator-(const BigInt& other) const;
  BigInt operator*(const BigInt& other) const;
  BigInt operator/(const BigInt& other) const;
  BigInt operator%(const BigInt& other) const;

  BigInt& operator+=(const BigInt& other);
  BigInt& operator-=(const BigInt& other);
  BigInt& operator*=(const BigInt& other);
  BigInt& operator/=(const BigInt& other);
  BigInt& operator%=(const BigInt& other);

  BigInt operator-() const;  // Unary minus

  BigInt multiply(const BigInt& other, MulAlgo algo = MulAlgo::Auto)
      const;  // Multiplication with algorithm selection

  // Comparison operators
  friend bool operator==(const BigInt& lhs, const BigInt& rhs);
  friend auto operator<=>(const BigInt& lhs, const BigInt& rhs);

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
};

}  // namespace fraction

#endif  // LAB5_FRACTION_BIG_INT_HPP