#include "utility.hpp"

#include <gtest/gtest.h>

using namespace fraction;

// ============================================================
// UInt128 — Comparisons (portable)
// ============================================================

TEST(UInt128Cmp, Equal) {
  UInt128 a(42u), b(42u);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(UInt128Cmp, LessLow) {
  UInt128 a(10u), b(20u);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST(UInt128Cmp, GreaterEq) {
  UInt128 a(20u), b(10u);
  EXPECT_TRUE(a > b);
  EXPECT_TRUE(a >= b);
  EXPECT_TRUE(b <= a);
}

TEST(UInt128Cmp, SelfEqual) {
  UInt128 v(12345u);
  EXPECT_TRUE(v == v);
  EXPECT_FALSE(v != v);
  EXPECT_FALSE(v < v);
  EXPECT_TRUE(v <= v);
  EXPECT_TRUE(v >= v);
}

// ============================================================
// UInt128 — Addition (portable)
// ============================================================

TEST(UInt128Add, Simple) {
  EXPECT_TRUE(UInt128(10u) + UInt128(20u) == UInt128(30u));
}

TEST(UInt128Add, Commutative) {
  UInt128 a(17u), b(25u);
  EXPECT_TRUE(a + b == b + a);
}

TEST(UInt128Add, ZeroIdentity) {
  UInt128 v(42u), z(0);
  EXPECT_TRUE(v + z == v);
  EXPECT_TRUE(z + v == v);
}

// ============================================================
// UInt128 — Subtraction (portable)
// ============================================================

TEST(UInt128Sub, Simple) {
  EXPECT_TRUE(UInt128(30u) - UInt128(10u) == UInt128(20u));
}

TEST(UInt128Sub, SelfIsZero) {
  UInt128 v(999u), z(0);
  EXPECT_TRUE(v - v == z);
}

// ============================================================
// UInt128 — Multiplication (portable)
// ============================================================

TEST(UInt128Mul, ByZero) {
  UInt128 v(42u), z(0);
  EXPECT_TRUE(v * z == z);
}

TEST(UInt128Mul, ByOne) {
  UInt128 v(42u);
  EXPECT_TRUE(v * UInt128(1u) == v);
}

TEST(UInt128Mul, Simple) {
  EXPECT_TRUE(UInt128(6u) * UInt128(7u) == UInt128(42u));
}

TEST(UInt128Mul, Commutative) {
  UInt128 a(123u), b(456u);
  EXPECT_TRUE(a * b == b * a);
}

TEST(UInt128Mul, Distributive) {
  UInt128 a(3u), b(5u), c(7u);
  EXPECT_TRUE(a * (b + c) == a * b + a * c);
}

// ============================================================
// UInt128 — Unary (portable)
// ============================================================

TEST(UInt128Unary, NotOperator) {
  UInt128 z(0);
  EXPECT_TRUE(!z);
  UInt128 w(1u);
  EXPECT_FALSE(!w);
}

TEST(UInt128Unary, BoolConversion) {
  UInt128 z(0);
  EXPECT_FALSE(static_cast<bool>(z));
  UInt128 w(1u);
  EXPECT_TRUE(static_cast<bool>(w));
}

TEST(UInt128Unary, NegateZero) {
  UInt128 z(0);
  EXPECT_TRUE(-z == z);
}

TEST(UInt128Unary, DoubleNegate) {
  UInt128 v(42u);
  EXPECT_TRUE(-(-v) == v);
}

// ============================================================
// UInt128 — Compound assignment (portable)
// ============================================================

TEST(UInt128Compound, AddAssign) {
  UInt128 a(10u);
  a += UInt128(20u);
  EXPECT_TRUE(a == UInt128(30u));
}

TEST(UInt128Compound, SubAssign) {
  UInt128 a(30u);
  a -= UInt128(10u);
  EXPECT_TRUE(a == UInt128(20u));
}

TEST(UInt128Compound, MulAssign) {
  UInt128 a(6u);
  a *= UInt128(7u);
  EXPECT_TRUE(a == UInt128(42u));
}

// ============================================================
// Int128 — Comparisons (portable)
// ============================================================

TEST(Int128Cmp, PositiveEqual) {
  Int128 a(42), b(42);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(Int128Cmp, PositiveLess) {
  Int128 a(10), b(20);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST(Int128Cmp, NegativeLessThanPositive) {
  Int128 a(-1), b(1);
  EXPECT_TRUE(a < b);
}

TEST(Int128Cmp, NegativeOrdering) {
  Int128 a(-5), b(-3);
  EXPECT_TRUE(a < b);   // -5 < -3
  EXPECT_FALSE(b < a);
}

TEST(Int128Cmp, BothNegativeEqual) {
  Int128 a(-1), b(-1);
  EXPECT_TRUE(a == b);
}

// ============================================================
// Int128 — Addition / Subtraction (portable)
// ============================================================

TEST(Int128Add, Positive) {
  EXPECT_TRUE(Int128(10) + Int128(20) == Int128(30));
}

TEST(Int128Add, Negative) {
  EXPECT_TRUE(Int128(-10) + Int128(5) < Int128(0));
  EXPECT_TRUE(Int128(-10) + Int128(5) == -Int128(5));
}

TEST(Int128Add, ZeroIdentity) {
  Int128 v(42), z(0);
  EXPECT_TRUE(v + z == v);
}

TEST(Int128Sub, Positive) {
  EXPECT_TRUE(Int128(30) - Int128(10) == Int128(20));
}

TEST(Int128Sub, NegativeResult) {
  EXPECT_TRUE(Int128(10) - Int128(30) == -Int128(20));
}

TEST(Int128Sub, NegNeg) {
  EXPECT_TRUE(Int128(-5) - Int128(-3) == Int128(-2));
}

// ============================================================
// Int128 — Multiplication (portable)
// ============================================================

TEST(Int128Mul, Positive) {
  EXPECT_TRUE(Int128(6) * Int128(7) == Int128(42));
}

TEST(Int128Mul, NegPos) {
  EXPECT_TRUE(Int128(-6) * Int128(7) == -Int128(42));
}

TEST(Int128Mul, NegNeg) {
  EXPECT_TRUE(Int128(-2) * Int128(-3) == Int128(6));
}

TEST(Int128Mul, ByZero) {
  Int128 z(0);
  EXPECT_TRUE(Int128(42) * z == z);
}

TEST(Int128Mul, ByOne) {
  Int128 v(99);
  EXPECT_TRUE(v * Int128(1) == v);
}

// ============================================================
// Int128 — Unary (portable)
// ============================================================

TEST(Int128Unary, NegatePositive) {
  EXPECT_TRUE(-Int128(1) < Int128(0));
}

TEST(Int128Unary, NegateNegative) {
  EXPECT_TRUE(-Int128(-1) == Int128(1));
}

TEST(Int128Unary, NegateZero) {
  Int128 z(0);
  EXPECT_TRUE(-z == z);
}

TEST(Int128Unary, DoubleNegate) {
  Int128 v(42);
  EXPECT_TRUE(-(-v) == v);
}

// ============================================================
// Int128 — Compound assignment (portable)
// ============================================================

TEST(Int128Compound, AddAssign) {
  Int128 a(10);
  a += Int128(20);
  EXPECT_TRUE(a == Int128(30));
}

TEST(Int128Compound, SubAssign) {
  Int128 a(10);
  a -= Int128(20);
  EXPECT_TRUE(a == Int128(-10));
}

TEST(Int128Compound, MulAssign) {
  Int128 a(-3);
  a *= Int128(4);
  EXPECT_TRUE(a == Int128(-12));
}

// ============================================================
// UInt128 — Explicit conversion (portable)
// ============================================================

TEST(UInt128Conv, ToUnsignedLongLong) {
  UInt128 v(42u);
  EXPECT_EQ(static_cast<unsigned long long>(v), 42ull);
}

TEST(UInt128Conv, ToLongLong) {
  UInt128 v(42u);
  EXPECT_EQ(static_cast<long long>(v), 42ll);
}

// ============================================================
// Int128 — Explicit conversion (portable)
// ============================================================

TEST(Int128Conv, ToLongLongPositive) {
  Int128 v(42);
  EXPECT_EQ(static_cast<long long>(v), 42ll);
}

TEST(Int128Conv, ToUnsignedLongLong) {
  Int128 v(42);
  EXPECT_EQ(static_cast<unsigned long long>(v), 42ull);
}

// ============================================================
// MSVC-specific: verify internal lo/hi representation
// ============================================================
#if FRACTION_COMPILER_MSVC

TEST(UInt128Msvc, CtorDefault) {
  UInt128 v;
  EXPECT_EQ(v.lo, 0u);
  EXPECT_EQ(v.hi, 0u);
}

TEST(UInt128Msvc, CtorFromUnsigned) {
  UInt128 v(0xDEADBEEFCAFEULL);
  EXPECT_EQ(v.lo, 0xDEADBEEFCAFEULL);
  EXPECT_EQ(v.hi, 0u);
}

TEST(UInt128Msvc, CtorFromNegativeSignExtends) {
  UInt128 v(-1);
  EXPECT_EQ(v.lo, ~uint64_t(0));
  EXPECT_EQ(v.hi, ~uint64_t(0));
}

TEST(UInt128Msvc, AddCarry) {
  UInt128 a(~uint64_t(0));  // lo = MAX, hi = 0
  UInt128 b(1u);
  auto r = a + b;
  EXPECT_EQ(r.lo, 0u);
  EXPECT_EQ(r.hi, 1u);
}

TEST(UInt128Msvc, SubBorrow) {
  UInt128 a(0u, 1);  // 2^64
  UInt128 b(1u);
  auto r = a - b;
  EXPECT_EQ(r.lo, ~uint64_t(0));
  EXPECT_EQ(r.hi, 0u);
}

TEST(UInt128Msvc, MulOverflowToHigh) {
  UInt128 a(0x100000000ULL);  // 2^32
  auto r = a * a;             // 2^64
  EXPECT_EQ(r.lo, 0u);
  EXPECT_EQ(r.hi, 1u);
}

TEST(UInt128Msvc, MulLargeValues) {
  UInt128 a(0xFFFFFFFFFFFFFFFFULL);  // 2^64 - 1
  auto r = a * UInt128(2u);          // 2^65 - 2
  EXPECT_EQ(r.lo, ~uint64_t(0) - 1);
  EXPECT_EQ(r.hi, 1u);
}

TEST(UInt128Msvc, MulSquaredMax64) {
  UInt128 a(0xFFFFFFFFFFFFFFFFULL);
  auto r = a * a;  // (2^64-1)^2 = 2^128 - 2^65 + 1
  EXPECT_EQ(r.lo, 1u);
  EXPECT_EQ(r.hi, ~uint64_t(0) - 1);
}

TEST(UInt128Msvc, NegateOne) {
  UInt128 one(1u);
  auto r = -one;
  EXPECT_EQ(r.lo, ~uint64_t(0));
  EXPECT_EQ(r.hi, ~uint64_t(0));
}

TEST(UInt128Msvc, HighPartAdd) {
  UInt128 a(0u, 100);
  UInt128 b(0u, 200);
  auto r = a + b;
  EXPECT_EQ(r.lo, 0u);
  EXPECT_EQ(r.hi, 300u);
}

TEST(UInt128Msvc, CrossHalfCarry) {
  UInt128 a(~uint64_t(0), 5);
  UInt128 b(1u, 10);
  auto r = a + b;
  EXPECT_EQ(r.lo, 0u);
  EXPECT_EQ(r.hi, 16u);
}

TEST(UInt128Msvc, LessHigh) {
  UInt128 a(100u, 1);
  UInt128 b(0u, 2);
  EXPECT_TRUE(a < b);
}

TEST(Int128Msvc, CtorFromNegative) {
  Int128 v(-1);
  EXPECT_EQ(v.lo, ~uint64_t(0));
  EXPECT_EQ(v.hi, ~uint64_t(0));
}

TEST(Int128Msvc, AddNegativeResult) {
  Int128 a(-10), b(5);
  auto r = a + b;
  EXPECT_EQ(r.lo, ~uint64_t(0) - 4);
  EXPECT_EQ(r.hi, ~uint64_t(0));
}

TEST(Int128Msvc, MulNegPos) {
  Int128 a(-6), b(7);
  auto r = a * b;  // -42
  EXPECT_EQ(r.lo, ~uint64_t(0) - 41);
  EXPECT_EQ(r.hi, ~uint64_t(0));
}

TEST(Int128Msvc, SubNegative) {
  Int128 a(10), b(30);
  auto r = a - b;  // -20
  EXPECT_EQ(r.lo, ~uint64_t(0) - 19);
  EXPECT_EQ(r.hi, ~uint64_t(0));
}

TEST(Int128Msvc, MulAssignNegative) {
  Int128 a(-3);
  a *= Int128(4);
  EXPECT_EQ(a.lo, ~uint64_t(0) - 11);  // -12
  EXPECT_EQ(a.hi, ~uint64_t(0));
}

#endif  // FRACTION_COMPILER_MSVC
