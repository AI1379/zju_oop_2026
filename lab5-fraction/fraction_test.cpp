#include "fraction.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

using FractionI = fraction::Fraction<int>;

// ============================================================
// Construction
// ============================================================

TEST(FractionConstruction, DefaultConstructor) {
  FractionI f;
  EXPECT_EQ(f.to_string(), "0/1");
}

TEST(FractionConstruction, BasicPositive) {
  EXPECT_EQ(FractionI(1, 2).to_string(), "1/2");
}

TEST(FractionConstruction, NegativeNumerator) {
  EXPECT_EQ(FractionI(-3, 4).to_string(), "-3/4");
}

TEST(FractionConstruction, NegativeDenominatorNormalized) {
  EXPECT_EQ(FractionI(1, -2).to_string(), "-1/2");
}

TEST(FractionConstruction, BothNegativeNormalized) {
  EXPECT_EQ(FractionI(-1, -2).to_string(), "1/2");
}

TEST(FractionConstruction, ZeroNumerator) {
  EXPECT_EQ(FractionI(0, 5).to_string(), "0/5");
}

TEST(FractionConstruction, DenominatorZeroThrows) {
  EXPECT_THROW(FractionI(1, 0), std::invalid_argument);
}

TEST(FractionConstruction, CopyConstructor) {
  FractionI f(3, 4);
  FractionI g(f);
  EXPECT_EQ(g, f);
}

TEST(FractionConstruction, MoveConstructor) {
  FractionI f(3, 4);
  FractionI g(std::move(f));
  EXPECT_EQ(g.to_string(), "3/4");
}

TEST(FractionConstruction, CopyAssignment) {
  FractionI f(3, 4);
  FractionI g;
  g = f;
  EXPECT_EQ(g, f);
}

TEST(FractionConstruction, MoveAssignment) {
  FractionI f(3, 4);
  FractionI g;
  g = std::move(f);
  EXPECT_EQ(g.to_string(), "3/4");
}

// ============================================================
// Comparison
// ============================================================

TEST(FractionComparison, Equal) {
  EXPECT_TRUE(FractionI(1, 2) == FractionI(1, 2));
}

TEST(FractionComparison, NotEqual) {
  EXPECT_TRUE(FractionI(1, 2) != FractionI(1, 3));
}

TEST(FractionComparison, CrossMultiplication) {
  EXPECT_TRUE(FractionI(2, 4) == FractionI(1, 2));
}

TEST(FractionComparison, NegativeCrossMultiplication) {
  // -2/-4 normalizes to 2/4
  EXPECT_TRUE(FractionI(-2, -4) == FractionI(1, 2));
}

TEST(FractionComparison, LessThan) {
  EXPECT_TRUE(FractionI(1, 3) < FractionI(1, 2));
  EXPECT_FALSE(FractionI(1, 2) < FractionI(1, 3));
}

TEST(FractionComparison, GreaterThan) {
  EXPECT_TRUE(FractionI(1, 2) > FractionI(1, 3));
  EXPECT_FALSE(FractionI(1, 3) > FractionI(1, 2));
}

TEST(FractionComparison, LessEqual) {
  EXPECT_TRUE(FractionI(1, 2) <= FractionI(1, 2));
  EXPECT_TRUE(FractionI(1, 3) <= FractionI(1, 2));
  EXPECT_FALSE(FractionI(1, 2) <= FractionI(1, 3));
}

TEST(FractionComparison, GreaterEqual) {
  EXPECT_TRUE(FractionI(1, 2) >= FractionI(1, 2));
  EXPECT_TRUE(FractionI(1, 2) >= FractionI(1, 3));
  EXPECT_FALSE(FractionI(1, 3) >= FractionI(1, 2));
}

TEST(FractionComparison, NegativeVsPositive) {
  EXPECT_TRUE(FractionI(-1, 2) < FractionI(1, 2));
  EXPECT_TRUE(FractionI(-1, 2) < FractionI(1, 3));
}

TEST(FractionComparison, BothNegative) {
  // -1/2 = -0.5 < -1/3 ≈ -0.333
  EXPECT_TRUE(FractionI(-1, 2) < FractionI(-1, 3));
  EXPECT_FALSE(FractionI(-1, 3) < FractionI(-1, 2));
}

TEST(FractionComparison, ZeroComparison) {
  EXPECT_TRUE(FractionI(0, 1) == FractionI(0, 5));
  EXPECT_TRUE(FractionI(0, 1) < FractionI(1, 2));
  EXPECT_TRUE(FractionI(-1, 2) < FractionI(0, 1));
}

TEST(FractionComparison, SelfComparison) {
  FractionI f(3, 4);
  EXPECT_TRUE(f == f);
  EXPECT_TRUE(f <= f);
  EXPECT_TRUE(f >= f);
  EXPECT_FALSE(f < f);
  EXPECT_FALSE(f > f);
}

// ============================================================
// to_string
// ============================================================

TEST(FractionToString, Basic) { EXPECT_EQ(FractionI(1, 2).to_string(), "1/2"); }

TEST(FractionToString, NegativeNumerator) {
  EXPECT_EQ(FractionI(-3, 4).to_string(), "-3/4");
}

TEST(FractionToString, ZeroNumerator) {
  EXPECT_EQ(FractionI(0, 5).to_string(), "0/5");
}

TEST(FractionToString, Default) { EXPECT_EQ(FractionI().to_string(), "0/1"); }

TEST(FractionToString, NegativeDenominatorNormalized) {
  EXPECT_EQ(FractionI(3, -4).to_string(), "-3/4");
}

// ============================================================
// Stream I/O
// ============================================================

TEST(FractionStream, OutputStream) {
  std::ostringstream oss;
  oss << FractionI(3, 4);
  EXPECT_EQ(oss.str(), "3/4");
}

TEST(FractionStream, OutputStreamNegative) {
  std::ostringstream oss;
  oss << FractionI(-1, 2);
  EXPECT_EQ(oss.str(), "-1/2");
}

TEST(FractionStream, InputStream) {
  FractionI f;
  std::istringstream iss("5/7");
  iss >> f;
  EXPECT_EQ(f.to_string(), "5/7");
}

TEST(FractionStream, InputStreamNegative) {
  FractionI f;
  std::istringstream iss("-3/4");
  iss >> f;
  EXPECT_EQ(f.to_string(), "-3/4");
}

TEST(FractionStream, InputStreamZeroDenominatorFails) {
  FractionI f;
  std::istringstream iss("1/0");
  iss >> f;
  EXPECT_TRUE(iss.fail());
}

// ============================================================
// Parse — slash format
// ============================================================

TEST(FractionParseSlash, Basic) {
  EXPECT_EQ(FractionI::parse("1/2"), FractionI(1, 2));
}

TEST(FractionParseSlash, NegativeNumerator) {
  EXPECT_EQ(FractionI::parse("-3/4"), FractionI(-3, 4));
}

TEST(FractionParseSlash, NegativeDenominator) {
  // 3/-4 normalizes to -3/4
  EXPECT_EQ(FractionI::parse("3/-4"), FractionI(-3, 4));
}

TEST(FractionParseSlash, BothNegative) {
  // -3/-4 normalizes to 3/4
  EXPECT_EQ(FractionI::parse("-3/-4"), FractionI(3, 4));
}

TEST(FractionParseSlash, ZeroNumerator) {
  EXPECT_EQ(FractionI::parse("0/5"), FractionI(0, 5));
}

TEST(FractionParseSlash, DenominatorZeroThrows) {
  EXPECT_THROW(FractionI::parse("1/0"), std::invalid_argument);
}

TEST(FractionParseSlash, MultipleSlashesThrows) {
  EXPECT_THROW(FractionI::parse("1/2/3"), std::invalid_argument);
}

TEST(FractionParseSlash, InvalidNumeratorThrows) {
  EXPECT_THROW(FractionI::parse("abc/2"), std::invalid_argument);
}

TEST(FractionParseSlash, InvalidDenominatorThrows) {
  EXPECT_THROW(FractionI::parse("1/abc"), std::invalid_argument);
}

// ============================================================
// Parse — decimal format
// ============================================================

TEST(FractionParseDecimal, ZeroPointFive) {
  EXPECT_EQ(FractionI::parse("0.5"), FractionI(5, 10));
}

TEST(FractionParseDecimal, NegativeDecimal) {
  EXPECT_EQ(FractionI::parse("-0.5"), FractionI(-5, 10));
}

TEST(FractionParseDecimal, PiApproximation) {
  EXPECT_EQ(FractionI::parse("3.14"), FractionI(314, 100));
}

TEST(FractionParseDecimal, LeadingDot) {
  EXPECT_EQ(FractionI::parse(".5"), FractionI(5, 10));
}

TEST(FractionParseDecimal, TrailingDot) {
  EXPECT_EQ(FractionI::parse("5."), FractionI(5, 1));
}

TEST(FractionParseDecimal, NegativeLeadingDot) {
  EXPECT_EQ(FractionI::parse("-.5"), FractionI(-5, 10));
}

TEST(FractionParseDecimal, OnePointZero) {
  EXPECT_EQ(FractionI::parse("1.0"), FractionI(10, 10));
}

// ============================================================
// Parse — invalid formats
// ============================================================

TEST(FractionParseInvalid, BothSlashAndDot) {
  EXPECT_THROW(FractionI::parse("1/2.3"), std::invalid_argument);
}

TEST(FractionParseInvalid, NeitherSlashNorDot) {
  EXPECT_THROW(FractionI::parse("abc"), std::invalid_argument);
}

TEST(FractionParseInvalid, EmptyString) {
  EXPECT_THROW(FractionI::parse(""), std::invalid_argument);
}

TEST(FractionParseInvalid, MultipleDots) {
  EXPECT_THROW(FractionI::parse("1.2.3"), std::invalid_argument);
}

TEST(FractionParseInvalid, SlashAndDotSameString) {
  EXPECT_THROW(FractionI::parse("1.2/3"), std::invalid_argument);
}

// ============================================================
// Arithmetic
// ============================================================

TEST(FractionArithmetic, Addition) {
  EXPECT_EQ(FractionI(1, 2) + FractionI(1, 3), FractionI(5, 6));
}

TEST(FractionArithmetic, Subtraction) {
  EXPECT_EQ(FractionI(1, 2) - FractionI(1, 3), FractionI(1, 6));
}

TEST(FractionArithmetic, Multiplication) {
  EXPECT_EQ(FractionI(2, 3) * FractionI(3, 4), FractionI(6, 12));
}

TEST(FractionArithmetic, Division) {
  EXPECT_EQ(FractionI(1, 2) / FractionI(2, 3), FractionI(3, 4));
}

TEST(FractionArithmetic, DivisionByZeroThrows) {
  EXPECT_THROW(FractionI(1, 2) / FractionI(0, 1), std::invalid_argument);
}

TEST(FractionArithmetic, AddNegative) {
  EXPECT_EQ(FractionI(1, 2) + FractionI(-1, 3), FractionI(1, 6));
}

TEST(FractionArithmetic, SubtractNegative) {
  EXPECT_EQ(FractionI(1, 2) - FractionI(-1, 3), FractionI(5, 6));
}

TEST(FractionArithmetic, MultiplyNegative) {
  EXPECT_EQ(FractionI(-1, 2) * FractionI(2, 3), FractionI(-2, 6));
}

TEST(FractionArithmetic, DivideNegative) {
  EXPECT_EQ(FractionI(-1, 2) / FractionI(2, 3), FractionI(-3, 4));
}

TEST(FractionArithmetic, SelfAddition) {
  FractionI f(1, 2);
  // 1/2 + 1/2 = (1*2 + 1*2)/(2*2) = 4/4 == 1/1
  EXPECT_EQ(f + f, FractionI(1, 1));
}

TEST(FractionArithmetic, ZeroOperand) {
  EXPECT_EQ(FractionI(0, 1) + FractionI(1, 2), FractionI(1, 2));
  EXPECT_EQ(FractionI(1, 2) - FractionI(1, 2), FractionI(0, 4));
  EXPECT_EQ(FractionI(5, 7) * FractionI(0, 1), FractionI(0, 7));
}

// ============================================================
// Double conversion
// ============================================================

TEST(FractionDouble, OneHalf) {
  EXPECT_DOUBLE_EQ(static_cast<double>(FractionI(1, 2)), 0.5);
}

TEST(FractionDouble, Zero) {
  EXPECT_DOUBLE_EQ(static_cast<double>(FractionI(0, 1)), 0.0);
}

TEST(FractionDouble, Negative) {
  EXPECT_DOUBLE_EQ(static_cast<double>(FractionI(-1, 2)), -0.5);
}

TEST(FractionDouble, OneThird) {
  EXPECT_NEAR(static_cast<double>(FractionI(1, 3)), 1.0 / 3.0, 1e-15);
}

TEST(FractionDouble, WholeNumber) {
  EXPECT_DOUBLE_EQ(static_cast<double>(FractionI(3, 1)), 3.0);
}
