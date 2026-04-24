#include "big_int.hpp"

#include <gtest/gtest.h>

using BigInt = fraction::BigInt;

static BigInt H(const std::string& s) { return BigInt::from_hex_string(s); }

// ============================================================
// Construction
// ============================================================

TEST(BigIntConstruction, DefaultIsZero) {
  BigInt b;
  EXPECT_EQ(b.to_hex_string(), "0");
  EXPECT_EQ(b.size(), 0u);
}

TEST(BigIntConstruction, Int64Positive) {
  EXPECT_EQ(BigInt(42).to_hex_string(), "0000002a");
}

TEST(BigIntConstruction, Int64Zero) {
  EXPECT_EQ(BigInt(0).to_hex_string(), "0");
}

TEST(BigIntConstruction, Int64Negative) {
  EXPECT_EQ(BigInt(-42).to_hex_string(), "-0000002a");
}

TEST(BigIntConstruction, Int64MaxUint32) {
  EXPECT_EQ(BigInt(0xFFFFFFFF).to_hex_string(), "ffffffff");
}

TEST(BigIntConstruction, Int64MultiLimb) {
  EXPECT_EQ(BigInt(0x100000000LL).to_hex_string(), "0000000100000000");
}

TEST(BigIntConstruction, FromHexPositive) {
  EXPECT_EQ(H("000000ff").to_hex_string(), "000000ff");
}

TEST(BigIntConstruction, FromHexNegative) {
  EXPECT_EQ(H("-000000ff").to_hex_string(), "-000000ff");
}

TEST(BigIntConstruction, FromHexZero) {
  EXPECT_EQ(H("00000000").to_hex_string(), "0");
}

TEST(BigIntConstruction, FromHexLong) {
  EXPECT_EQ(H("123456789abcdef0").to_hex_string(), "123456789abcdef0");
}

TEST(BigIntConstruction, FromHexInvalidThrows) {
  EXPECT_THROW(BigInt::from_hex_string("zzzzzzzz"), std::invalid_argument);
}

TEST(BigIntConstruction, FromHexShortString) {
  EXPECT_EQ(H("ff").to_hex_string(), "000000ff");
}

TEST(BigIntConstruction, FromHexNegativeShort) {
  EXPECT_EQ(H("-ff").to_hex_string(), "-000000ff");
}

TEST(BigIntConstruction, FromHexSingleZero) {
  EXPECT_EQ(H("0").to_hex_string(), "0");
}

TEST(BigIntConstruction, CopyConstructor) {
  BigInt a(42);
  BigInt b(a);
  EXPECT_EQ(a, b);
}

TEST(BigIntConstruction, MoveConstructor) {
  BigInt a(42);
  BigInt b(std::move(a));
  EXPECT_EQ(b.to_hex_string(), "0000002a");
}

// ============================================================
// Comparison
// ============================================================

TEST(BigIntComparison, EqualBothZero) { EXPECT_EQ(BigInt(0), BigInt(0)); }

TEST(BigIntComparison, EqualSameValue) { EXPECT_EQ(BigInt(42), BigInt(42)); }

TEST(BigIntComparison, NotEqualDifferent) { EXPECT_NE(BigInt(1), BigInt(2)); }

TEST(BigIntComparison, NotEqualSign) { EXPECT_NE(BigInt(1), BigInt(-1)); }

TEST(BigIntComparison, Less) {
  EXPECT_TRUE(BigInt(1) < BigInt(2));
  EXPECT_FALSE(BigInt(2) < BigInt(1));
}

TEST(BigIntComparison, Greater) {
  EXPECT_TRUE(BigInt(2) > BigInt(1));
  EXPECT_FALSE(BigInt(1) > BigInt(2));
}

TEST(BigIntComparison, NegativeLess) {
  EXPECT_TRUE(BigInt(-2) < BigInt(-1));
  EXPECT_FALSE(BigInt(-1) < BigInt(-2));
}

TEST(BigIntComparison, NegativeLessThanPositive) {
  EXPECT_TRUE(BigInt(-1) < BigInt(1));
  EXPECT_TRUE(BigInt(-1) < BigInt(0));
}

TEST(BigIntComparison, LessEqual) {
  EXPECT_TRUE(BigInt(1) <= BigInt(1));
  EXPECT_TRUE(BigInt(1) <= BigInt(2));
  EXPECT_FALSE(BigInt(2) <= BigInt(1));
}

TEST(BigIntComparison, GreaterEqual) {
  EXPECT_TRUE(BigInt(1) >= BigInt(1));
  EXPECT_TRUE(BigInt(2) >= BigInt(1));
  EXPECT_FALSE(BigInt(1) >= BigInt(2));
}

TEST(BigIntComparison, SelfComparison) {
  BigInt a(42);
  EXPECT_TRUE(a == a);
  EXPECT_TRUE(a <= a);
  EXPECT_TRUE(a >= a);
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(a > a);
}

TEST(BigIntComparison, HexEquality) {
  EXPECT_EQ(H("00001234"), H("00001234"));
  EXPECT_NE(H("00001234"), H("00005678"));
}

TEST(BigIntComparison, MultiLimbGreaterThanSingle) {
  EXPECT_TRUE(BigInt(0x100000000LL) > BigInt(1));
}

// ============================================================
// abs_compare
// ============================================================

TEST(BigIntAbsCompare, Equal) {
  EXPECT_EQ(BigInt::abs_compare(BigInt(5), BigInt(5)), 0);
  EXPECT_EQ(BigInt::abs_compare(BigInt(-5), BigInt(5)), 0);
}

TEST(BigIntAbsCompare, Less) {
  EXPECT_EQ(BigInt::abs_compare(BigInt(3), BigInt(5)), -1);
}

TEST(BigIntAbsCompare, Greater) {
  EXPECT_EQ(BigInt::abs_compare(BigInt(5), BigInt(3)), 1);
}

TEST(BigIntAbsCompare, MultiLimbVsSingle) {
  EXPECT_EQ(BigInt::abs_compare(BigInt(0x100000000LL), BigInt(1)), 1);
}

// ============================================================
// operator+= (addition)
// ============================================================

TEST(BigIntAddAssign, PositivePlusPositive) {
  BigInt a(1);
  a += BigInt(2);
  EXPECT_EQ(a, BigInt(3));
}

TEST(BigIntAddAssign, NegativePlusNegative) {
  BigInt a(-1);
  a += BigInt(-2);
  EXPECT_EQ(a, BigInt(-3));
}

TEST(BigIntAddAssign, PositivePlusNegative) {
  BigInt a(5);
  a += BigInt(-3);
  EXPECT_EQ(a, BigInt(2));
}

TEST(BigIntAddAssign, NegativePlusPositive) {
  BigInt a(-3);
  a += BigInt(5);
  EXPECT_EQ(a, BigInt(2));
}

TEST(BigIntAddAssign, ResultZero) {
  BigInt a(3);
  a += BigInt(-3);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntAddAssign, ZeroPlusZero) {
  BigInt a(0);
  a += BigInt(0);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntAddAssign, Carry) {
  BigInt a(0xFFFFFFFF);
  a += BigInt(1);
  EXPECT_EQ(a.to_hex_string(), "0000000100000000");
}

TEST(BigIntAddAssign, HexLarge) {
  BigInt a = H("12345678");
  a += H("9abcdef0");
  EXPECT_EQ(a.to_hex_string(), "acf13568");
}

// ============================================================
// operator-= (subtraction)
// ============================================================

TEST(BigIntSubAssign, PositiveMinusPositive) {
  BigInt a(5);
  a -= BigInt(3);
  EXPECT_EQ(a, BigInt(2));
}

TEST(BigIntSubAssign, ResultNegative) {
  BigInt a(3);
  a -= BigInt(5);
  EXPECT_EQ(a, BigInt(-2));
}

TEST(BigIntSubAssign, NegativeMinusNegative) {
  BigInt a(-3);
  a -= BigInt(-5);
  EXPECT_EQ(a, BigInt(2));
}

TEST(BigIntSubAssign, ResultZero) {
  BigInt a(3);
  a -= BigInt(3);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntSubAssign, Borrow) {
  BigInt a(0x100000000LL);
  a -= BigInt(1);
  EXPECT_EQ(a.to_hex_string(), "ffffffff");
}

// ============================================================
// Unary negation
// ============================================================

TEST(BigIntUnaryNeg, NegatePositive) { EXPECT_EQ(-BigInt(5), BigInt(-5)); }

TEST(BigIntUnaryNeg, NegateNegative) { EXPECT_EQ(-BigInt(-5), BigInt(5)); }

TEST(BigIntUnaryNeg, DoubleNegate) {
  BigInt a(42);
  EXPECT_EQ(-(-a), a);
}

// ============================================================
// trim
// ============================================================

TEST(BigIntTrim, TrimsLeadingZeros) {
  auto b = H("00000000");
  b.trim();
  EXPECT_EQ(b.size(), 0u);
}

// ============================================================
// Hex roundtrip
// ============================================================

TEST(BigIntHexRoundtrip, Positive) {
  auto original = H("deadbeef");
  EXPECT_EQ(BigInt::from_hex_string(original.to_hex_string()), original);
}

TEST(BigIntHexRoundtrip, Negative) {
  auto original = BigInt::from_hex_string("-cafebabe");
  auto roundtripped = BigInt::from_hex_string(original.to_hex_string());
  EXPECT_EQ(roundtripped, original);
}

// ============================================================
// Edge cases
// ============================================================

TEST(BigIntEdge, LargeAddition) {
  BigInt a = H("ffffffffffffffff");
  a += BigInt(1);
  EXPECT_EQ(a.to_hex_string(), "000000010000000000000000");
}
