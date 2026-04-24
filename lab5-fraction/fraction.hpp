//
// Created by Renatus Madrigal on 2026/04/23.
//

#pragma once

#ifndef LAB5_FRACTION_FRACTION_HPP
#define LAB5_FRACTION_FRACTION_HPP

#include <iostream>
#include <stdexcept>
#include <string>

#include "concepts.hpp"

namespace fraction {

template <typename T>
  requires Integral<T>
class Fraction {
 public:
  // Default constructors and assignment operators
  Fraction() = default;
  Fraction(const Fraction&) = default;
  Fraction& operator=(const Fraction&) = default;
  Fraction(Fraction&&) = default;
  Fraction& operator=(Fraction&&) = default;

  Fraction(T numerator, T denominator) {
    if (denominator == T{}) {
      throw std::invalid_argument("Denominator cannot be zero");
    }
    if (denominator < T{}) {
      numerator = -numerator;
      denominator = -denominator;
    }
    numerator_ = numerator;
    denominator_ = denominator;
  }

  std::string to_string() const {
    if constexpr (std::is_integral_v<T>) {
      return std::to_string(numerator_) + "/" + std::to_string(denominator_);
    } else if constexpr (ToStringable<T>) {
      return numerator_.to_string() + "/" + denominator_.to_string();
    } else {
      return "Fraction of unknown type";
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const Fraction& fraction) {
    os << fraction.to_string();
    return os;
  }

  friend auto operator==(const Fraction& lhs, const Fraction& rhs) {
    return (lhs.numerator_ * rhs.denominator_) ==
           (rhs.numerator_ * lhs.denominator_);
  }

  friend auto operator<=>(const Fraction& lhs, const Fraction& rhs) {
    return (lhs.numerator_ * rhs.denominator_) <=>
           (rhs.numerator_ * lhs.denominator_);
  }

 private:
  T numerator_{};
  T denominator_{1};
};

template <typename T>
  requires BuiltinIntegral<T>
class Fraction<T> {
 public:
  // Default constructors and assignment operators
  Fraction() = default;
  Fraction(const Fraction&) = default;
  Fraction& operator=(const Fraction&) = default;
  Fraction(Fraction&&) = default;
  Fraction& operator=(Fraction&&) = default;

  Fraction(T numerator, T denominator) {
    if (denominator == 0) {
      throw std::invalid_argument("Denominator cannot be zero");
    }
    if (denominator < 0) {
      numerator = -numerator;
      denominator = -denominator;
    }
    numerator_ = numerator;
    denominator_ = denominator;
  }

  std::string to_string() const {
    return std::to_string(numerator_) + "/" + std::to_string(denominator_);
  }

  friend std::ostream& operator<<(std::ostream& os, const Fraction& fraction) {
    os << fraction.to_string();
    return os;
  }

  friend bool operator==(const Fraction& lhs, const Fraction& rhs) {
    return lhs.numerator_ * rhs.denominator_ ==
           rhs.numerator_ * lhs.denominator_;
  }

  friend std::strong_ordering operator<=>(const Fraction& lhs,
                                          const Fraction& rhs) {
    return lhs.numerator_ * rhs.denominator_ <=>
           rhs.numerator_ * lhs.denominator_;
  }

  friend std::istream& operator>>(std::istream& is, Fraction& fraction) {
    char slash;
    is >> fraction.numerator_ >> slash >> fraction.denominator_;
    if (fraction.denominator_ == 0) is.setstate(std::ios::failbit);
    return is;
  }

  static Fraction parse(const std::string& str) {
    size_t slash_pos = str.find('/');
    size_t dot_pos = str.find('.');
    bool valid =
        (slash_pos != std::string::npos) ^ (dot_pos != std::string::npos);
    if (!valid) {
      throw std::invalid_argument("Invalid fraction format: " + str);
    }
    if (slash_pos != std::string::npos) {
      size_t end_pos = 0;
      T numerator =
          static_cast<T>(std::stoll(str.substr(0, slash_pos), &end_pos));
      if (end_pos != slash_pos) {
        throw std::invalid_argument("Invalid fraction format: " + str);
      }
      T denominator =
          static_cast<T>(std::stoll(str.substr(slash_pos + 1), &end_pos));
      if (end_pos != str.size() - slash_pos - 1) {
        throw std::invalid_argument("Invalid fraction format: " + str);
      }
      return Fraction(numerator, denominator);
    } else {
      // Remove the dot and count fractional digits to avoid floating-point
      // issues
      std::string digits = str;
      digits.erase(dot_pos, 1);
      size_t end_pos = 0;
      T numerator = static_cast<T>(std::stoll(digits, &end_pos));
      if (end_pos != digits.size()) {
        throw std::invalid_argument("Invalid fraction format: " + str);
      }
      T denominator = 1;
      for (size_t i = 0; i < str.size() - dot_pos - 1; ++i) {
        denominator *= 10;
      }
      return Fraction(numerator, denominator);
    }
  }

  void reduce() {
    T g = gcd(numerator_, denominator_);
    numerator_ /= g;
    denominator_ /= g;
  }

 private:
  T numerator_{};
  T denominator_{1};
};

}  // namespace fraction

#endif  // LAB5_FRACTION_FRACTION_HPP