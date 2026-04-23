//
// Created by Renatus Madrigal on 2026/04/23.
//

#pragma once

#ifndef LAB5_FRACTION_CONCEPTS_HPP
#define LAB5_FRACTION_CONCEPTS_HPP

#include <concepts>
#include <string>
#include <type_traits>


namespace fraction {
template <typename T>
concept BuiltinIntegral = std::integral<T>;

template <typename T>
concept Integral = BuiltinIntegral<T> || requires(T a, T b) {
  { a + b } -> std::same_as<T>;
  { a - b } -> std::same_as<T>;
  { a * b } -> std::same_as<T>;
  { a / b } -> std::same_as<T>;
  { a % b } -> std::same_as<T>;
};

template <typename T>
concept Comparable = requires(T a, T b) {
  { a == b } -> std::same_as<bool>;
  { a != b } -> std::same_as<bool>;
};

template <typename T>
concept ToStringable = requires(T a) {
  { a.to_string() } -> std::same_as<std::string>;
};

}  // namespace fraction

#endif  // LAB5_FRACTION_CONCEPTS_HPP