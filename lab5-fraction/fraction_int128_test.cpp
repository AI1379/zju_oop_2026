// Tests for Fraction<Int128> compatibility.
// On this platform (Clang + MSVC STL), __int128 does NOT satisfy std::integral,
// so BuiltinIntegral is false. Fraction<Int128> only gets the base features:
// construction, comparison, arithmetic, to_string (returns "unknown type"),
// and stream output. parse(), reduce(), operator double(), operator>> are
// NOT available since they require BuiltinIntegral.

#include <gtest/gtest.h>

#include <sstream>

#include "fraction.hpp"
#include "utility.hpp"

using Int128 = fraction::Int128;
using FractionI128 = fraction::Fraction<Int128>;

static Int128 i128(int64_t v) { return static_cast<Int128>(v); }

// ============================================================
// Construction
// ============================================================

TEST(FractionI128Construction, DefaultConstructor) {
  FractionI128 f;
  EXPECT_TRUE(f == FractionI128(i128(0), i128(1)));
}

TEST(FractionI128Construction, BasicPositive) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) == FractionI128(i128(1), i128(2)));
}

TEST(FractionI128Construction, NegativeNumerator) {
  FractionI128 f(i128(-3), i128(4));
  EXPECT_TRUE(f == FractionI128(i128(-3), i128(4)));
}

TEST(FractionI128Construction, NegativeDenominatorNormalized) {
  FractionI128 f(i128(1), i128(-2));
  EXPECT_TRUE(f == FractionI128(i128(-1), i128(2)));
}

TEST(FractionI128Construction, BothNegativeNormalized) {
  FractionI128 f(i128(-1), i128(-2));
  EXPECT_TRUE(f == FractionI128(i128(1), i128(2)));
}

TEST(FractionI128Construction, ZeroNumerator) {
  FractionI128 f(i128(0), i128(5));
  EXPECT_TRUE(f == FractionI128(i128(0), i128(5)));
}

TEST(FractionI128Construction, DenominatorZeroThrows) {
  EXPECT_THROW(FractionI128(i128(1), i128(0)), std::invalid_argument);
}

TEST(FractionI128Construction, CopyConstructor) {
  FractionI128 f(i128(3), i128(4));
  FractionI128 g(f);
  EXPECT_TRUE(g == f);
}

TEST(FractionI128Construction, MoveConstructor) {
  FractionI128 f(i128(3), i128(4));
  FractionI128 g(std::move(f));
  EXPECT_TRUE(g == FractionI128(i128(3), i128(4)));
}

TEST(FractionI128Construction, CopyAssignment) {
  FractionI128 f(i128(3), i128(4));
  FractionI128 g;
  g = f;
  EXPECT_TRUE(g == f);
}

TEST(FractionI128Construction, MoveAssignment) {
  FractionI128 f(i128(3), i128(4));
  FractionI128 g;
  g = std::move(f);
  EXPECT_TRUE(g == FractionI128(i128(3), i128(4)));
}

TEST(FractionI128Construction, Large128BitValue) {
  Int128 big = i128(1);
  for (int i = 0; i < 80; ++i) big *= i128(2);
  FractionI128 f(big, i128(1));
  EXPECT_TRUE(f == FractionI128(big, i128(1)));
}

// ============================================================
// Comparison
// ============================================================

TEST(FractionI128Comparison, Equal) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) == FractionI128(i128(1), i128(2)));
}

TEST(FractionI128Comparison, NotEqual) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) != FractionI128(i128(1), i128(3)));
}

TEST(FractionI128Comparison, CrossMultiplication) {
  EXPECT_TRUE(FractionI128(i128(2), i128(4)) == FractionI128(i128(1), i128(2)));
}

TEST(FractionI128Comparison, LessThan) {
  EXPECT_TRUE(FractionI128(i128(1), i128(3)) < FractionI128(i128(1), i128(2)));
  EXPECT_FALSE(FractionI128(i128(1), i128(2)) < FractionI128(i128(1), i128(3)));
}

TEST(FractionI128Comparison, GreaterThan) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) > FractionI128(i128(1), i128(3)));
}

TEST(FractionI128Comparison, LessEqual) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) <= FractionI128(i128(1), i128(2)));
  EXPECT_TRUE(FractionI128(i128(1), i128(3)) <= FractionI128(i128(1), i128(2)));
}

TEST(FractionI128Comparison, GreaterEqual) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) >= FractionI128(i128(1), i128(2)));
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) >= FractionI128(i128(1), i128(3)));
}

TEST(FractionI128Comparison, NegativeVsPositive) {
  EXPECT_TRUE(FractionI128(i128(-1), i128(2)) < FractionI128(i128(1), i128(2)));
  EXPECT_TRUE(FractionI128(i128(-1), i128(2)) < FractionI128(i128(1), i128(3)));
}

TEST(FractionI128Comparison, BothNegative) {
  EXPECT_TRUE(FractionI128(i128(-1), i128(2)) < FractionI128(i128(-1), i128(3)));
  EXPECT_FALSE(FractionI128(i128(-1), i128(3)) < FractionI128(i128(-1), i128(2)));
}

TEST(FractionI128Comparison, ZeroComparison) {
  EXPECT_TRUE(FractionI128(i128(0), i128(1)) == FractionI128(i128(0), i128(5)));
  EXPECT_TRUE(FractionI128(i128(0), i128(1)) < FractionI128(i128(1), i128(2)));
  EXPECT_TRUE(FractionI128(i128(-1), i128(2)) < FractionI128(i128(0), i128(1)));
}

TEST(FractionI128Comparison, SelfComparison) {
  FractionI128 f(i128(3), i128(4));
  EXPECT_TRUE(f == f);
  EXPECT_TRUE(f <= f);
  EXPECT_TRUE(f >= f);
  EXPECT_FALSE(f < f);
  EXPECT_FALSE(f > f);
}

TEST(FractionI128Comparison, LargeValueComparison) {
  Int128 big_a = i128(1);
  Int128 big_b = i128(1);
  for (int i = 0; i < 70; ++i) big_a *= i128(2);  // 2^70
  for (int i = 0; i < 71; ++i) big_b *= i128(2);  // 2^71
  EXPECT_TRUE(FractionI128(i128(1), big_a) < FractionI128(i128(1), i128(1)));
  EXPECT_TRUE(FractionI128(big_a, i128(1)) > FractionI128(i128(1), i128(1)));
  EXPECT_TRUE(FractionI128(big_a, big_b) == FractionI128(i128(1), i128(2)));
}

// ============================================================
// to_string & stream output
// ============================================================

TEST(FractionI128ToString, ReturnsUnknownType) {
  // __int128 is not std::is_integral on this STL and has no .to_string(),
  // so Fraction falls through to the else branch.
  EXPECT_EQ(FractionI128(i128(1), i128(2)).to_string(),
            "Fraction of unknown type");
}

TEST(FractionI128Stream, OutputStream) {
  std::ostringstream oss;
  oss << FractionI128(i128(1), i128(2));
  EXPECT_EQ(oss.str(), "Fraction of unknown type");
}

// ============================================================
// Arithmetic
// ============================================================

TEST(FractionI128Arithmetic, Addition) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) + FractionI128(i128(1), i128(3)) ==
              FractionI128(i128(5), i128(6)));
}

TEST(FractionI128Arithmetic, Subtraction) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) - FractionI128(i128(1), i128(3)) ==
              FractionI128(i128(1), i128(6)));
}

TEST(FractionI128Arithmetic, Multiplication) {
  EXPECT_TRUE(FractionI128(i128(2), i128(3)) * FractionI128(i128(3), i128(4)) ==
              FractionI128(i128(6), i128(12)));
}

TEST(FractionI128Arithmetic, Division) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) / FractionI128(i128(2), i128(3)) ==
              FractionI128(i128(3), i128(4)));
}

TEST(FractionI128Arithmetic, DivisionByZeroThrows) {
  EXPECT_THROW(FractionI128(i128(1), i128(2)) / FractionI128(i128(0), i128(1)),
               std::invalid_argument);
}

TEST(FractionI128Arithmetic, AddNegative) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) + FractionI128(i128(-1), i128(3)) ==
              FractionI128(i128(1), i128(6)));
}

TEST(FractionI128Arithmetic, SubtractNegative) {
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) - FractionI128(i128(-1), i128(3)) ==
              FractionI128(i128(5), i128(6)));
}

TEST(FractionI128Arithmetic, MultiplyNegative) {
  EXPECT_TRUE(FractionI128(i128(-1), i128(2)) * FractionI128(i128(2), i128(3)) ==
              FractionI128(i128(-2), i128(6)));
}

TEST(FractionI128Arithmetic, DivideNegative) {
  EXPECT_TRUE(FractionI128(i128(-1), i128(2)) / FractionI128(i128(2), i128(3)) ==
              FractionI128(i128(-3), i128(4)));
}

TEST(FractionI128Arithmetic, SelfAddition) {
  FractionI128 f(i128(1), i128(2));
  EXPECT_TRUE(f + f == FractionI128(i128(4), i128(4)));
}

TEST(FractionI128Arithmetic, ZeroOperand) {
  EXPECT_TRUE(FractionI128(i128(0), i128(1)) + FractionI128(i128(1), i128(2)) ==
              FractionI128(i128(1), i128(2)));
  EXPECT_TRUE(FractionI128(i128(1), i128(2)) - FractionI128(i128(1), i128(2)) ==
              FractionI128(i128(0), i128(4)));
  EXPECT_TRUE(FractionI128(i128(5), i128(7)) * FractionI128(i128(0), i128(1)) ==
              FractionI128(i128(0), i128(7)));
}

TEST(FractionI128Arithmetic, LargeValueAddition) {
  Int128 big = i128(1);
  for (int i = 0; i < 80; ++i) big *= i128(2);  // 2^80
  FractionI128 a(big, i128(1));
  FractionI128 b(i128(1), i128(1));
  FractionI128 result = a + b;
  EXPECT_TRUE(result == FractionI128(big + i128(1), i128(1)));
}

TEST(FractionI128Arithmetic, LargeValueMultiplication) {
  Int128 a = i128(1);
  Int128 b = i128(1);
  for (int i = 0; i < 64; ++i) a *= i128(2);  // 2^64
  for (int i = 0; i < 63; ++i) b *= i128(2);  // 2^63
  FractionI128 fa(a, i128(1));
  FractionI128 fb(b, i128(1));
  // 2^64 * 2^63 = 2^127 — still fits in Int128
  EXPECT_TRUE(fa * fb == FractionI128(a * b, i128(1)));
}

TEST(FractionI128Arithmetic, ChainExpression) {
  // (1/2 + 1/3) * 6/1 = 5/6 * 6/1 = 30/6 == 5/1
  FractionI128 result =
      (FractionI128(i128(1), i128(2)) + FractionI128(i128(1), i128(3))) *
      FractionI128(i128(6), i128(1));
  EXPECT_TRUE(result == FractionI128(i128(5), i128(1)));
}
