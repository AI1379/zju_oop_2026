#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "big_int.hpp"
#include "fraction.hpp"

using BI = fraction::BigInt;
using FractionB = fraction::Fraction<BI>;

// Helper: BigInt from literal
static BI bi(int64_t v) { return BI(v); }
static BI bi(const std::string& s) { return BI(s); }

// ============================================================
// Construction
// ============================================================

TEST(FractionBigIntConstruction, DefaultConstructor) {
  FractionB f;
  EXPECT_EQ(f.to_string(), "0/1");
}

TEST(FractionBigIntConstruction, BasicPositive) {
  EXPECT_EQ(FractionB(bi(1), bi(2)).to_string(), "1/2");
}

TEST(FractionBigIntConstruction, NegativeNumerator) {
  EXPECT_EQ(FractionB(bi(-3), bi(4)).to_string(), "-3/4");
}

TEST(FractionBigIntConstruction, NegativeDenominatorNormalized) {
  EXPECT_EQ(FractionB(bi(1), bi(-2)).to_string(), "-1/2");
}

TEST(FractionBigIntConstruction, BothNegativeNormalized) {
  EXPECT_EQ(FractionB(bi(-1), bi(-2)).to_string(), "1/2");
}

TEST(FractionBigIntConstruction, ZeroNumerator) {
  EXPECT_EQ(FractionB(bi(0), bi(5)).to_string(), "0/5");
}

TEST(FractionBigIntConstruction, DenominatorZeroThrows) {
  EXPECT_THROW(FractionB(bi(1), bi(0)), std::invalid_argument);
}

TEST(FractionBigIntConstruction, CopyConstructor) {
  FractionB f(bi(3), bi(4));
  FractionB g(f);
  EXPECT_EQ(g, f);
}

TEST(FractionBigIntConstruction, MoveConstructor) {
  FractionB f(bi(3), bi(4));
  FractionB g(std::move(f));
  EXPECT_EQ(g.to_string(), "3/4");
}

TEST(FractionBigIntConstruction, CopyAssignment) {
  FractionB f(bi(3), bi(4));
  FractionB g;
  g = f;
  EXPECT_EQ(g, f);
}

TEST(FractionBigIntConstruction, MoveAssignment) {
  FractionB f(bi(3), bi(4));
  FractionB g;
  g = std::move(f);
  EXPECT_EQ(g.to_string(), "3/4");
}

TEST(FractionBigIntConstruction, LargeBigIntNumerator) {
  BI large("123456789012345678901234567890");
  EXPECT_EQ(FractionB(large, bi(1)).to_string(),
            "123456789012345678901234567890/1");
}

TEST(FractionBigIntConstruction, LargeBigIntDenominator) {
  BI large("987654321098765432109876543210");
  EXPECT_EQ(FractionB(bi(1), large).to_string(),
            "1/987654321098765432109876543210");
}

// ============================================================
// Comparison
// ============================================================

TEST(FractionBigIntComparison, Equal) {
  EXPECT_TRUE(FractionB(bi(1), bi(2)) == FractionB(bi(1), bi(2)));
}

TEST(FractionBigIntComparison, NotEqual) {
  EXPECT_TRUE(FractionB(bi(1), bi(2)) != FractionB(bi(1), bi(3)));
}

TEST(FractionBigIntComparison, CrossMultiplication) {
  // 2/4 == 1/2 via cross-multiplication
  EXPECT_TRUE(FractionB(bi(2), bi(4)) == FractionB(bi(1), bi(2)));
}

TEST(FractionBigIntComparison, LessThan) {
  EXPECT_TRUE(FractionB(bi(1), bi(3)) < FractionB(bi(1), bi(2)));
  EXPECT_FALSE(FractionB(bi(1), bi(2)) < FractionB(bi(1), bi(3)));
}

TEST(FractionBigIntComparison, GreaterThan) {
  EXPECT_TRUE(FractionB(bi(1), bi(2)) > FractionB(bi(1), bi(3)));
  EXPECT_FALSE(FractionB(bi(1), bi(3)) > FractionB(bi(1), bi(2)));
}

TEST(FractionBigIntComparison, LessEqual) {
  EXPECT_TRUE(FractionB(bi(1), bi(2)) <= FractionB(bi(1), bi(2)));
  EXPECT_TRUE(FractionB(bi(1), bi(3)) <= FractionB(bi(1), bi(2)));
}

TEST(FractionBigIntComparison, GreaterEqual) {
  EXPECT_TRUE(FractionB(bi(1), bi(2)) >= FractionB(bi(1), bi(2)));
  EXPECT_TRUE(FractionB(bi(1), bi(2)) >= FractionB(bi(1), bi(3)));
}

TEST(FractionBigIntComparison, NegativeVsPositive) {
  EXPECT_TRUE(FractionB(bi(-1), bi(2)) < FractionB(bi(1), bi(2)));
  EXPECT_TRUE(FractionB(bi(-1), bi(2)) < FractionB(bi(1), bi(3)));
}

TEST(FractionBigIntComparison, BothNegative) {
  // -1/2 < -1/3
  EXPECT_TRUE(FractionB(bi(-1), bi(2)) < FractionB(bi(-1), bi(3)));
  EXPECT_FALSE(FractionB(bi(-1), bi(3)) < FractionB(bi(-1), bi(2)));
}

TEST(FractionBigIntComparison, ZeroComparison) {
  EXPECT_TRUE(FractionB(bi(0), bi(1)) == FractionB(bi(0), bi(5)));
  EXPECT_TRUE(FractionB(bi(0), bi(1)) < FractionB(bi(1), bi(2)));
  EXPECT_TRUE(FractionB(bi(-1), bi(2)) < FractionB(bi(0), bi(1)));
}

TEST(FractionBigIntComparison, SelfComparison) {
  FractionB f(bi(3), bi(4));
  EXPECT_TRUE(f == f);
  EXPECT_TRUE(f <= f);
  EXPECT_TRUE(f >= f);
  EXPECT_FALSE(f < f);
  EXPECT_FALSE(f > f);
}

TEST(FractionBigIntComparison, LargeValueCrossMultiplication) {
  // 2*big == big*2, so FractionB(2, big) == FractionB(2, big)
  BI big("999999999999999999999999999999");
  EXPECT_TRUE(FractionB(bi(2), big) == FractionB(bi(2), big));
  // 4/big vs 2/half_big where half_big = big/2
  // Fraction comparison uses cross-multiplication of BigInt
  EXPECT_TRUE(FractionB(bi(1), bi(2)) == FractionB(bi(2), bi(4)));
}

// ============================================================
// to_string
// ============================================================

TEST(FractionBigIntToString, Basic) {
  EXPECT_EQ(FractionB(bi(1), bi(2)).to_string(), "1/2");
}

TEST(FractionBigIntToString, NegativeNumerator) {
  EXPECT_EQ(FractionB(bi(-3), bi(4)).to_string(), "-3/4");
}

TEST(FractionBigIntToString, ZeroNumerator) {
  EXPECT_EQ(FractionB(bi(0), bi(5)).to_string(), "0/5");
}

TEST(FractionBigIntToString, LargeNumerator) {
  EXPECT_EQ(FractionB(BI("123456789012345678901234567890"), bi(1)).to_string(),
            "123456789012345678901234567890/1");
}

TEST(FractionBigIntToString, LargeDenominator) {
  EXPECT_EQ(FractionB(bi(1), BI("999999999999999999999999999999")).to_string(),
            "1/999999999999999999999999999999");
}

TEST(FractionBigIntToString, BothLarge) {
  EXPECT_EQ(FractionB(BI("111111111111111111111111111111"),
                      BI("222222222222222222222222222222"))
                .to_string(),
            "111111111111111111111111111111/222222222222222222222222222222");
}

// ============================================================
// Stream output
// ============================================================

TEST(FractionBigIntStream, OutputStream) {
  std::ostringstream oss;
  oss << FractionB(bi(3), bi(4));
  EXPECT_EQ(oss.str(), "3/4");
}

TEST(FractionBigIntStream, OutputStreamNegative) {
  std::ostringstream oss;
  oss << FractionB(bi(-1), bi(2));
  EXPECT_EQ(oss.str(), "-1/2");
}

TEST(FractionBigIntStream, OutputStreamLarge) {
  std::ostringstream oss;
  oss << FractionB(BI("123456789012345678901234567890"), bi(1));
  EXPECT_EQ(oss.str(), "123456789012345678901234567890/1");
}

// ============================================================
// Arithmetic
// ============================================================

TEST(FractionBigIntArithmetic, Addition) {
  // 1/2 + 1/3 = (3+2)/(2*3) = 5/6
  EXPECT_EQ(FractionB(bi(1), bi(2)) + FractionB(bi(1), bi(3)),
            FractionB(bi(5), bi(6)));
}

TEST(FractionBigIntArithmetic, Subtraction) {
  // 1/2 - 1/3 = (3-2)/6 = 1/6
  EXPECT_EQ(FractionB(bi(1), bi(2)) - FractionB(bi(1), bi(3)),
            FractionB(bi(1), bi(6)));
}

TEST(FractionBigIntArithmetic, Multiplication) {
  // 2/3 * 3/4 = 6/12
  EXPECT_EQ(FractionB(bi(2), bi(3)) * FractionB(bi(3), bi(4)),
            FractionB(bi(6), bi(12)));
}

TEST(FractionBigIntArithmetic, Division) {
  // (1/2) / (2/3) = 3/4
  EXPECT_EQ(FractionB(bi(1), bi(2)) / FractionB(bi(2), bi(3)),
            FractionB(bi(3), bi(4)));
}

TEST(FractionBigIntArithmetic, DivisionByZeroThrows) {
  EXPECT_THROW(FractionB(bi(1), bi(2)) / FractionB(bi(0), bi(1)),
               std::invalid_argument);
}

TEST(FractionBigIntArithmetic, AddNegative) {
  // 1/2 + (-1/3) = (3-2)/6 = 1/6
  EXPECT_EQ(FractionB(bi(1), bi(2)) + FractionB(bi(-1), bi(3)),
            FractionB(bi(1), bi(6)));
}

TEST(FractionBigIntArithmetic, SubtractNegative) {
  // 1/2 - (-1/3) = (3+2)/6 = 5/6
  EXPECT_EQ(FractionB(bi(1), bi(2)) - FractionB(bi(-1), bi(3)),
            FractionB(bi(5), bi(6)));
}

TEST(FractionBigIntArithmetic, MultiplyNegative) {
  // (-1/2) * (2/3) = -2/6
  EXPECT_EQ(FractionB(bi(-1), bi(2)) * FractionB(bi(2), bi(3)),
            FractionB(bi(-2), bi(6)));
}

TEST(FractionBigIntArithmetic, DivideNegative) {
  // (-1/2) / (2/3) = -3/4
  EXPECT_EQ(FractionB(bi(-1), bi(2)) / FractionB(bi(2), bi(3)),
            FractionB(bi(-3), bi(4)));
}

TEST(FractionBigIntArithmetic, SelfAddition) {
  FractionB f(bi(1), bi(2));
  // 1/2 + 1/2 = 4/4
  EXPECT_EQ(f + f, FractionB(bi(4), bi(4)));
}

TEST(FractionBigIntArithmetic, ZeroOperand) {
  EXPECT_EQ(FractionB(bi(0), bi(1)) + FractionB(bi(1), bi(2)),
            FractionB(bi(1), bi(2)));
  EXPECT_EQ(FractionB(bi(1), bi(2)) - FractionB(bi(1), bi(2)),
            FractionB(bi(0), bi(4)));
  EXPECT_EQ(FractionB(bi(5), bi(7)) * FractionB(bi(0), bi(1)),
            FractionB(bi(0), bi(7)));
}

// ============================================================
// Arithmetic with large BigInt values
// ============================================================

TEST(FractionBigIntArithmetic, LargeAddition) {
  // (big/1) + (1/1) = (big+1)/1
  BI big("123456789012345678901234567890");
  BI expected_num = big + bi(1);
  FractionB result = FractionB(big, bi(1)) + FractionB(bi(1), bi(1));
  EXPECT_EQ(result, FractionB(expected_num, bi(1)));
}

TEST(FractionBigIntArithmetic, LargeMultiplication) {
  // (big_a/1) * (big_b/1) = (big_a*big_b)/1
  BI a("123456789012345678901234567890");
  BI b("987654321098765432109876543210");
  BI expected = a * b;
  FractionB result = FractionB(a, bi(1)) * FractionB(b, bi(1));
  EXPECT_EQ(result, FractionB(expected, bi(1)));
}

TEST(FractionBigIntArithmetic, LargeDivision) {
  // (big/1) / (1/big) = big^2 / 1
  BI big("123456789012345678901234567890");
  BI expected = big * big;
  FractionB result = FractionB(big, bi(1)) / FractionB(bi(1), big);
  EXPECT_EQ(result, FractionB(expected, bi(1)));
}

TEST(FractionBigIntArithmetic, LargeCrossMultiplicationComparison) {
  // Test that comparisons work correctly with large BigInt cross-multiplication
  BI a("123456789012345678901234567890");
  BI b("987654321098765432109876543210");
  // a/b < b/a iff a*a < b*b (both positive)
  EXPECT_TRUE(FractionB(a, b) < FractionB(b, a));
  EXPECT_FALSE(FractionB(b, a) < FractionB(a, b));
}

TEST(FractionBigIntArithmetic, FractionalIdentity) {
  // x/x == 1/1 for any nonzero x
  BI x("999999999999999999999999999999999999");
  EXPECT_TRUE(FractionB(x, x) == FractionB(bi(1), bi(1)));
}

TEST(FractionBigIntArithmetic, NegateSignNormalization) {
  // (-3)/4 normalized to -3/4; 3/(-4) also normalized to -3/4
  FractionB a(bi(-3), bi(4));
  FractionB b(bi(3), bi(-4));
  EXPECT_EQ(a, b);
  EXPECT_EQ(a.to_string(), "-3/4");
  EXPECT_EQ(b.to_string(), "-3/4");
}

// ============================================================
// Arithmetic chain — complex expressions
// ============================================================

TEST(FractionBigIntArithmetic, ChainAddMultiply) {
  // (1/2 + 1/3) * 6/1 = (5/6) * 6/1 = 30/6 == 5/1
  FractionB result = (FractionB(bi(1), bi(2)) + FractionB(bi(1), bi(3))) *
                     FractionB(bi(6), bi(1));
  // 30/6 cross-multiplied with 5/1: 30*1 == 6*5
  EXPECT_EQ(result, FractionB(bi(5), bi(1)));
}

TEST(FractionBigIntArithmetic, ChainSubtractDivide) {
  FractionB a(bi(5), bi(6));
  FractionB b(bi(1), bi(3));
  FractionB c(bi(1), bi(2));
  FractionB result = (a - b) / c;
  EXPECT_EQ(result, FractionB(bi(1), bi(1)));
}

// ============================================================
// Reduce (DivModable feature)
// ============================================================

TEST(FractionBigIntReduce, BasicReduce) {
  FractionB f(bi(6), bi(4));
  f.reduce();
  // 6/4 reduced to 3/2
  EXPECT_TRUE(f == FractionB(bi(3), bi(2)));
}

TEST(FractionBigIntReduce, AlreadyReduced) {
  FractionB f(bi(3), bi(7));
  f.reduce();
  EXPECT_TRUE(f == FractionB(bi(3), bi(7)));
}

TEST(FractionBigIntReduce, NegativeNumerator) {
  FractionB f(bi(-6), bi(4));
  f.reduce();
  EXPECT_TRUE(f == FractionB(bi(-3), bi(2)));
}

TEST(FractionBigIntReduce, BothNegative) {
  FractionB f(bi(-6), bi(-4));
  f.reduce();
  // -6/-4 normalizes to 6/4, then reduce to 3/2
  EXPECT_TRUE(f == FractionB(bi(3), bi(2)));
}

TEST(FractionBigIntReduce, ZeroNumerator) {
  FractionB f(bi(0), bi(5));
  f.reduce();
  EXPECT_TRUE(f == FractionB(bi(0), bi(1)));
}

TEST(FractionBigIntReduce, LargeValueReduce) {
  BI big("123456789012345678901234567890");
  FractionB f(big * bi(3), big * bi(7));
  f.reduce();
  EXPECT_TRUE(f == FractionB(bi(3), bi(7)));
}

TEST(FractionBigIntReduce, CoprimeDenominator) {
  // 7 and 11 are coprime, reduce should be no-op
  FractionB f(bi(7), bi(11));
  f.reduce();
  EXPECT_TRUE(f == FractionB(bi(7), bi(11)));
}

// ============================================================
// Parse — slash format (StringConstructible feature)
// ============================================================

TEST(FractionBigIntParseSlash, Basic) {
  EXPECT_EQ(FractionB::parse("1/2"), FractionB(bi(1), bi(2)));
}

TEST(FractionBigIntParseSlash, NegativeNumerator) {
  EXPECT_EQ(FractionB::parse("-3/4"), FractionB(bi(-3), bi(4)));
}

TEST(FractionBigIntParseSlash, NegativeDenominator) {
  // 3/-4 normalizes to -3/4
  EXPECT_EQ(FractionB::parse("3/-4"), FractionB(bi(-3), bi(4)));
}

TEST(FractionBigIntParseSlash, BothNegative) {
  // -3/-4 normalizes to 3/4
  EXPECT_EQ(FractionB::parse("-3/-4"), FractionB(bi(3), bi(4)));
}

TEST(FractionBigIntParseSlash, ZeroNumerator) {
  EXPECT_EQ(FractionB::parse("0/5"), FractionB(bi(0), bi(5)));
}

TEST(FractionBigIntParseSlash, DenominatorZeroThrows) {
  EXPECT_THROW(FractionB::parse("1/0"), std::invalid_argument);
}

TEST(FractionBigIntParseSlash, LargeNumerator) {
  EXPECT_EQ(
      FractionB::parse("123456789012345678901234567890/1"),
      FractionB(BI("123456789012345678901234567890"), bi(1)));
}

TEST(FractionBigIntParseSlash, LargeBoth) {
  EXPECT_EQ(FractionB::parse("123456789012345678901234567890/"
                             "987654321098765432109876543210"),
            FractionB(BI("123456789012345678901234567890"),
                      BI("987654321098765432109876543210")));
}

// ============================================================
// Parse — decimal format (StringConstructible feature)
// ============================================================

TEST(FractionBigIntParseDecimal, ZeroPointFive) {
  EXPECT_EQ(FractionB::parse("0.5"), FractionB(bi(5), bi(10)));
}

TEST(FractionBigIntParseDecimal, NegativeDecimal) {
  EXPECT_EQ(FractionB::parse("-0.5"), FractionB(bi(-5), bi(10)));
}

TEST(FractionBigIntParseDecimal, PiApproximation) {
  EXPECT_EQ(FractionB::parse("3.14"), FractionB(bi(314), bi(100)));
}

TEST(FractionBigIntParseDecimal, LeadingDot) {
  EXPECT_EQ(FractionB::parse(".5"), FractionB(bi(5), bi(10)));
}

TEST(FractionBigIntParseDecimal, TrailingDot) {
  EXPECT_EQ(FractionB::parse("5."), FractionB(bi(5), bi(1)));
}

TEST(FractionBigIntParseDecimal, LargeDecimal) {
  // "123456789012345678901234567890.123"
  // numerator = 123456789012345678901234567890123
  // denominator = 1000
  EXPECT_EQ(
      FractionB::parse("123456789012345678901234567890.123"),
      FractionB(BI("123456789012345678901234567890123"), bi(1000)));
}

TEST(FractionBigIntParseDecimal, NegativeLeadingDot) {
  EXPECT_EQ(FractionB::parse("-.5"), FractionB(bi(-5), bi(10)));
}

TEST(FractionBigIntParseDecimal, OnePointZero) {
  EXPECT_EQ(FractionB::parse("1.0"), FractionB(bi(10), bi(10)));
}

// ============================================================
// Parse — invalid formats
// ============================================================

TEST(FractionBigIntParseInvalid, BothSlashAndDot) {
  EXPECT_THROW(FractionB::parse("1/2.3"), std::invalid_argument);
}

TEST(FractionBigIntParseInvalid, NeitherSlashNorDot) {
  EXPECT_THROW(FractionB::parse("abc"), std::invalid_argument);
}

TEST(FractionBigIntParseInvalid, EmptyString) {
  EXPECT_THROW(FractionB::parse(""), std::invalid_argument);
}

TEST(FractionBigIntParseInvalid, MultipleDots) {
  EXPECT_THROW(FractionB::parse("1.2.3"), std::invalid_argument);
}

TEST(FractionBigIntParseInvalid, SlashAndDotSameString) {
  EXPECT_THROW(FractionB::parse("1.2/3"), std::invalid_argument);
}
