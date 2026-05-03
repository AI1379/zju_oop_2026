#include "big_int.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using BigInt = fraction::BigInt;

static BigInt H(const std::string& s) { return BigInt::from_hex_string(s); }

static std::vector<uint32_t> F32(uint64_t value) {
  return {static_cast<uint32_t>(value),
          static_cast<uint32_t>(value >> 32)};
}

static uint64_t F32Value(const std::vector<uint32_t>& limbs) {
  return uint64_t(limbs[0]) | (uint64_t(limbs[1]) << 32);
}

static fraction::detail::FermatElem FE(std::vector<uint32_t>& limbs) {
  return fraction::detail::FermatElem{std::span<uint32_t>(limbs)};
}

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

// ============================================================
// Fermat residue helpers for SSA
// ============================================================

TEST(FermatElem, AddReachesTwoPowerM) {
  auto a = F32(0xffffffffULL);
  auto b = F32(1);
  auto out = F32(0);

  fraction::detail::fermat_add(FE(out), FE(a), FE(b));

  EXPECT_EQ(F32Value(out), 0x100000000ULL);
}

TEST(FermatElem, AddReducesModulusToZero) {
  auto a = F32(0x100000000ULL);
  auto b = F32(1);
  auto out = F32(0);

  fraction::detail::fermat_add(FE(out), FE(a), FE(b));

  EXPECT_EQ(F32Value(out), 0ULL);
}

TEST(FermatElem, SubBorrowWrapsToModulusMinusOne) {
  auto a = F32(0);
  auto b = F32(1);
  auto out = F32(0);

  fraction::detail::fermat_sub(FE(out), FE(a), FE(b));

  EXPECT_EQ(F32Value(out), 0x100000000ULL);
}

TEST(FermatElem, SubFromTwoPowerM) {
  auto a = F32(0x100000000ULL);
  auto b = F32(1);
  auto out = F32(0);

  fraction::detail::fermat_sub(FE(out), FE(a), FE(b));

  EXPECT_EQ(F32Value(out), 0xffffffffULL);
}

TEST(FermatElem, MulPow2CrossesMBitBoundary) {
  auto x = F32(1);
  auto out = F32(0);

  fraction::detail::fermat_mul_pow2(FE(out), FE(x), 32);
  EXPECT_EQ(F32Value(out), 0x100000000ULL);

  fraction::detail::fermat_mul_pow2(FE(out), FE(x), 33);
  EXPECT_EQ(F32Value(out), 0xffffffffULL);
}

TEST(FermatElem, MulPow2UsesPeriodTwoM) {
  auto x = F32(0x12345678ULL);
  auto out = F32(0);

  fraction::detail::fermat_mul_pow2(FE(out), FE(x), 64);

  EXPECT_EQ(F32Value(out), F32Value(x));
}

TEST(FermatElem, ButterflyUsesPow2Twiddle) {
  auto a = F32(3);
  auto b = F32(5);
  auto scratch = F32(0);

  fraction::detail::fermat_butterfly(FE(a), FE(b), 1, FE(scratch));

  EXPECT_EQ(F32Value(a), 13ULL);
  EXPECT_EQ(F32Value(b), 0xfffffffaULL);
}

// ============================================================
// multiply_inplace (naive)
// ============================================================

TEST(BigIntMulInplace, SmallPositive) {
  BigInt a(6);
  a.multiply_inplace(BigInt(7), fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(42));
}

TEST(BigIntMulInplace, ZeroTimesValue) {
  BigInt a(0);
  a.multiply_inplace(BigInt(12345), fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntMulInplace, ValueTimesZero) {
  BigInt a(12345);
  a.multiply_inplace(BigInt(0), fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntMulInplace, Identity) {
  BigInt a(42);
  a.multiply_inplace(BigInt(1), fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(42));
}

TEST(BigIntMulInplace, NegativeTimesPositive) {
  BigInt a(-3);
  a.multiply_inplace(BigInt(5), fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(-15));
}

TEST(BigIntMulInplace, PositiveTimesNegative) {
  BigInt a(3);
  BigInt b(-5);
  a.multiply_inplace(b, fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(-15));
}

TEST(BigIntMulInplace, NegativeTimesNegative) {
  BigInt a(-3);
  a.multiply_inplace(BigInt(-5), fraction::MulAlgo::Naive);
  EXPECT_EQ(a, BigInt(15));
}

TEST(BigIntMulInplace, SingleLimbCarry) {
  // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001
  BigInt a = H("ffffffff");
  a.multiply_inplace(H("ffffffff"), fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "fffffffe00000001");
}

TEST(BigIntMulInplace, MultiLimbBySingle) {
  // 0xFFFFFFFF * 2 = 0x1FFFFFFFE
  BigInt a = H("ffffffff");
  a.multiply_inplace(BigInt(2), fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "00000001fffffffe");
}

TEST(BigIntMulInplace, MultiLimbByMultiLimb) {
  // (2^64-1)^2 = 2^128 - 2^65 + 1
  BigInt a = H("ffffffffffffffff");
  a.multiply_inplace(H("ffffffffffffffff"), fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "fffffffffffffffe0000000000000001");
}

TEST(BigIntMulInplace, SelfMultiplication) {
  // a *= a (squaring), tests this == &other path
  BigInt a = H("0000ffff");
  a.multiply_inplace(a, fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "fffe0001");
}

TEST(BigIntMulInplace, SelfMultiplicationLarge) {
  BigInt a = H("ffffffffffffffff");
  a.multiply_inplace(a, fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "fffffffffffffffe0000000000000001");
}

TEST(BigIntMulInplace, DifferentSizes) {
  // 3-limb * 1-limb
  BigInt a = H("00000001ffffffffffffffff");  // 0x1FFFFFFFFFFFFFFFF
  BigInt b(2);
  a.multiply_inplace(b, fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "00000003fffffffffffffffe");
}

TEST(BigIntMulInplace, AllMaxDigits) {
  // (2^32-1)^3
  BigInt a = H("ffffffff");
  a.multiply_inplace(H("ffffffff"), fraction::MulAlgo::Naive);
  a.multiply_inplace(H("ffffffff"), fraction::MulAlgo::Naive);
  EXPECT_EQ(a.to_hex_string(), "fffffffd00000002ffffffff");
}

// ============================================================
// Karatsuba multiplication
// ============================================================

TEST(BigIntKaratsuba, SmallFallbackToNaive) {
  BigInt a(42);
  a.multiply_inplace(BigInt(7), fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(a, BigInt(294));
}

TEST(BigIntKaratsuba, MultiLimbSameAsNaive) {
  // Compare Karatsuba result against naive on a multi-limb input
  BigInt a = H("ffffffffffffffff");
  BigInt expected = a;
  expected.multiply_inplace(H("ffffffffffffffff"), fraction::MulAlgo::Naive);

  BigInt b = H("ffffffffffffffff");
  b.multiply_inplace(H("ffffffffffffffff"), fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(b.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntKaratsuba, LargeProductSameAsNaive) {
  // 64-limb * 64-limb: exercises one Karatsuba recursion level
  std::string sa, sb;
  for (int i = 0; i < 64; ++i) {
    sa += "ffffffff";
    sb += "12345678";
  }
  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(result.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntKaratsuba, AsymmetricSizes) {
  // 33 limbs * 40 limbs: tests padding logic
  std::string sa, sb;
  for (int i = 0; i < 33; ++i) sa += "ffffffff";
  for (int i = 0; i < 40; ++i) sb += "00000001";

  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(result, expected);
}

TEST(BigIntKaratsuba, NegativeResult) {
  BigInt a(12345);
  BigInt b(-67890);
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(result, expected);
}

TEST(BigIntKaratsuba, SelfMultiplication) {
  BigInt a = H("ffffffffffffffff");
  BigInt expected = a;
  expected.multiply_inplace(expected, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(result, fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(result, expected);
}

// ============================================================
// NTT multiplication
// ============================================================

TEST(BigIntNTT, SmallSameAsNaive) {
  BigInt a(42);
  a.multiply_inplace(BigInt(7), fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(a, BigInt(294));
}

TEST(BigIntNTT, ZeroTimesValue) {
  BigInt a(0);
  a.multiply_inplace(BigInt(12345), fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntNTT, ValueTimesZero) {
  BigInt a(12345);
  a.multiply_inplace(BigInt(0), fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntNTT, NegativeResult) {
  BigInt a(-3);
  BigInt b(5);
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result, expected);
}

TEST(BigIntNTT, NegativeTimesNegative) {
  BigInt a(-3);
  BigInt b(-5);
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result, expected);
}

TEST(BigIntNTT, SelfMultiplication) {
  BigInt a = H("ffffffffffffffff");
  BigInt expected = a;
  expected.multiply_inplace(expected, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(result, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result, expected);
}

TEST(BigIntNTT, MultiLimbSameAsNaive) {
  BigInt a = H("ffffffffffffffff");
  BigInt b = H("123456789abcdef0");
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result, expected);
}

TEST(BigIntNTT, AllMaxDigitsSmall) {
  // (2^32-1)^2 — single-limb case, tests NTT with tiny inputs
  BigInt a = H("ffffffff");
  BigInt expected = a;
  expected.multiply_inplace(H("ffffffff"), fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(H("ffffffff"), fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result, expected);
}

TEST(BigIntNTT, LargeProductSameAsNaive) {
  // 64-limb * 64-limb: a decent NTT workout
  std::string sa, sb;
  for (int i = 0; i < 64; ++i) {
    sa += "ffffffff";
    sb += "12345678";
  }
  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntNTT, AsymmetricSizes) {
  // 100-limb * 30-limb: tests asymmetric padding
  std::string sa, sb;
  for (int i = 0; i < 100; ++i) sa += "deadbeef";
  for (int i = 0; i < 30; ++i) sb += "cafef00d";

  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Karatsuba);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result, expected);
}

TEST(BigIntNTT, LargeAllMaxVsNaive) {
  // 128-limb * 128-limb, all digits 0xFFFFFFFF — compare NTT vs Naive
  std::string sa;
  for (int i = 0; i < 128; ++i) sa += "ffffffff";

  BigInt a = H(sa);
  BigInt b = H(sa);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntNTT, Large256LimbsVsNaive) {
  // 256-limb * 256-limb, all digits 0xFFFFFFFF — stresses CRT range
  std::string sa;
  for (int i = 0; i < 256; ++i) sa += "ffffffff";

  BigInt a = H(sa);
  BigInt b = H(sa);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::FFT_NTT);
  EXPECT_EQ(result.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntKaratsuba, LargeAllMaxVsNaive) {
  // Sanity-check Karatsuba for 512 limbs vs Naive
  std::string sa;
  for (int i = 0; i < 512; ++i) sa += "ffffffff";

  BigInt a = H(sa);
  BigInt b = H(sa);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::Karatsuba);

  if (result != expected) {
    auto es = expected.to_hex_string();
    auto rs = result.to_hex_string();
    size_t pos = 0;
    while (pos < es.size() && pos < rs.size() && es[pos] == rs[pos]) pos++;
    ADD_FAILURE() << "Karatsuba mismatch for 512 limbs, first diff at hex char "
                  << pos << " (limb " << pos / 8 << ")\n"
                  << "  expected: ..." << es.substr(pos > 16 ? pos - 16 : 0, 40) << "...\n"
                  << "  result:   ..." << rs.substr(pos > 16 ? pos - 16 : 0, 40) << "...";
  }
}

// Bisect to find where Karatsuba breaks with all-max digits
class KaratsubaSizeTest : public ::testing::TestWithParam<int> {};
TEST_P(KaratsubaSizeTest, AllMaxVsNaive) {
  int limbs = GetParam();
  std::string sa;
  for (int i = 0; i < limbs; ++i) sa += "ffffffff";

  BigInt a = H(sa);
  BigInt b = H(sa);
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::Karatsuba);
  EXPECT_EQ(result, expected) << "Failed at " << limbs << " limbs";
}
INSTANTIATE_TEST_SUITE_P(Sizes, KaratsubaSizeTest,
                          ::testing::Values(64, 96, 128, 160, 192, 256, 320, 384, 448, 512));

// ============================================================
// SSA multiplication
// ============================================================

TEST(BigIntSSA, SmallSameAsNaive) {
  BigInt a(42);
  a.multiply_inplace(BigInt(7), fraction::MulAlgo::SSA);
  EXPECT_EQ(a, BigInt(294));
}

TEST(BigIntSSA, ZeroTimesValue) {
  BigInt a(0);
  a.multiply_inplace(BigInt(12345), fraction::MulAlgo::SSA);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntSSA, ValueTimesZero) {
  BigInt a(12345);
  a.multiply_inplace(BigInt(0), fraction::MulAlgo::SSA);
  EXPECT_EQ(a, BigInt(0));
}

TEST(BigIntSSA, NegativeResult) {
  BigInt a(-3);
  BigInt b(5);
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::SSA);
  EXPECT_EQ(result, expected);
}

TEST(BigIntSSA, NegativeTimesNegative) {
  BigInt a(-3);
  BigInt b(-5);
  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::SSA);
  EXPECT_EQ(result, expected);
}

TEST(BigIntSSA, SelfMultiplication) {
  BigInt a = H("ffffffffffffffff");
  BigInt expected = a;
  expected.multiply_inplace(expected, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(result, fraction::MulAlgo::SSA);
  EXPECT_EQ(result, expected);
}

TEST(BigIntSSA, MediumProductSameAsNaive) {
  // 64-limb × 64-limb
  std::string sa, sb;
  for (int i = 0; i < 64; ++i) {
    sa += "ffffffff";
    sb += "12345678";
  }
  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::SSA);
  EXPECT_EQ(result.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntSSA, LargeProduct1024Limbs) {
  // 1024-limb × 1024-limb — first size tier that actually triggers SSA
  std::string sa, sb;
  for (int i = 0; i < 1024; ++i) {
    sa += "ffffffff";
    sb += "deadbeef";
  }
  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Karatsuba);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::SSA);
  EXPECT_EQ(result, expected);
}

TEST(BigIntSSA, ModerateSizedProduct) {
  // 200-limb × 200-limb: large enough to exercise the transform.
  std::string sa, sb;
  for (int i = 0; i < 200; ++i) {
    sa += "ffffffff";
    sb += "12345678";
  }
  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Naive);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::SSA);
  EXPECT_EQ(result.to_hex_string(), expected.to_hex_string());
}

TEST(BigIntSSA, AsymmetricSizes) {
  // 1500-limb × 800-limb
  std::string sa, sb;
  for (int i = 0; i < 1500; ++i) sa += "cafef00d";
  for (int i = 0; i < 800; ++i) sb += "baddcafe";

  BigInt a = H(sa);
  BigInt b = H(sb);

  BigInt expected = a;
  expected.multiply_inplace(b, fraction::MulAlgo::Karatsuba);

  BigInt result = a;
  result.multiply_inplace(b, fraction::MulAlgo::SSA);
  EXPECT_EQ(result, expected);
}

TEST(FermatElem, FNTRoundTrip) {
  // 4-element FNT with M=64, elem_limbs=3 (2 value limbs + 1 carry).
  size_t L = 4, elem_limbs = 3;
  std::vector<uint32_t> buf(L * elem_limbs, 0);

  // Initialize: [3, 0, 0], [5, 0, 0], [7, 0, 0], [2, 0, 0]
  buf[0] = 3; buf[3] = 5; buf[6] = 7; buf[9] = 2;

  auto orig = buf;  // save copy

  // Forward FNT, then inverse — should get back the original.
  fraction::detail::fnt(std::span<uint32_t>(buf), elem_limbs, L, false);
  fraction::detail::fnt(std::span<uint32_t>(buf), elem_limbs, L, true);

  for (size_t i = 0; i < buf.size(); ++i) {
    EXPECT_EQ(buf[i], orig[i]) << "mismatch at limb " << i;
  }
}

TEST(FermatElem, FNTConvolution) {
  // 4-element FNT with M=64, elem_limbs=3.
  // Convolve a=[1,2,0,0] and b=[3,4,0,0].
  // Expected convolution: [3, 10, 8, 0] (indices 0..3).
  size_t L = 4, elem_limbs = 3;
  std::vector<uint32_t> buf_a(L * elem_limbs, 0);
  std::vector<uint32_t> buf_b(L * elem_limbs, 0);

  buf_a[0] = 1; buf_a[3] = 2;  // a = [1, 2, 0, 0]
  buf_b[0] = 3; buf_b[3] = 4;  // b = [3, 4, 0, 0]

  fraction::detail::fnt(std::span<uint32_t>(buf_a), elem_limbs, L, false);
  fraction::detail::fnt(std::span<uint32_t>(buf_b), elem_limbs, L, false);

  // Pointwise multiply
  std::vector<uint32_t> scratch(4, 0);
  for (size_t i = 0; i < L; ++i) {
    auto a = fraction::detail::FermatElem{
        std::span<uint32_t>(buf_a.data() + i * elem_limbs, elem_limbs)};
    auto b = fraction::detail::FermatElem{
        std::span<uint32_t>(buf_b.data() + i * elem_limbs, elem_limbs)};
    fraction::detail::fermat_mul(a, a, b, std::span<uint32_t>(scratch));
  }

  // Inverse FNT
  fraction::detail::fnt(std::span<uint32_t>(buf_a), elem_limbs, L, true);

  EXPECT_EQ(buf_a[0], 3u);   // c[0] = 3
  EXPECT_EQ(buf_a[3], 10u);  // c[1] = 10
  EXPECT_EQ(buf_a[6], 8u);   // c[2] = 8
  EXPECT_EQ(buf_a[9], 0u);   // c[3] = 0
}

TEST(FermatElem, FNT8Convolution) {
  // 8-element FNT with M=96, elem_limbs=4 (3 value limbs + 1 carry).
  // a = [1, 2, 3, 4, 0, 0, 0, 0], b = [5, 6, 7, 8, 0, 0, 0, 0]
  // Expected: c[0]=5, c[1]=16, c[2]=34, c[3]=60, c[4]=61, c[5]=52, c[6]=32, c[7]=0
  size_t L = 8, elem_limbs = 4;
  std::vector<uint32_t> buf_a(L * elem_limbs, 0);
  std::vector<uint32_t> buf_b(L * elem_limbs, 0);

  buf_a[0] = 1; buf_a[4] = 2; buf_a[8] = 3; buf_a[12] = 4;
  buf_b[0] = 5; buf_b[4] = 6; buf_b[8] = 7; buf_b[12] = 8;

  fraction::detail::fnt(std::span<uint32_t>(buf_a), elem_limbs, L, false);
  fraction::detail::fnt(std::span<uint32_t>(buf_b), elem_limbs, L, false);

  std::vector<uint32_t> scratch(6, 0);  // 2 * (M/32) = 2 * 3
  for (size_t i = 0; i < L; ++i) {
    auto a = fraction::detail::FermatElem{
        std::span<uint32_t>(buf_a.data() + i * elem_limbs, elem_limbs)};
    auto b = fraction::detail::FermatElem{
        std::span<uint32_t>(buf_b.data() + i * elem_limbs, elem_limbs)};
    fraction::detail::fermat_mul(a, a, b, std::span<uint32_t>(scratch));
  }

  fraction::detail::fnt(std::span<uint32_t>(buf_a), elem_limbs, L, true);

  EXPECT_EQ(buf_a[0], 5u);    // c[0]
  EXPECT_EQ(buf_a[4], 16u);   // c[1]
  EXPECT_EQ(buf_a[8], 34u);   // c[2]
  EXPECT_EQ(buf_a[12], 60u);  // c[3]
  EXPECT_EQ(buf_a[16], 61u);  // c[4]
  EXPECT_EQ(buf_a[20], 52u);  // c[5]
  EXPECT_EQ(buf_a[24], 32u);  // c[6]
  EXPECT_EQ(buf_a[28], 0u);   // c[7]
}
