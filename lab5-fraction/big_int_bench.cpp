#include <benchmark/benchmark.h>

#include <random>
#include <string>

#include "big_int.hpp"

using BigInt = fraction::BigInt;
using MulAlgo = fraction::MulAlgo;
using DivAlgo = fraction::DivAlgo;

static BigInt H(const std::string& s) { return BigInt::from_hex_string(s); }

// Generate a random BigInt with `limbs` limbs, each filled with random uint32
// values.
static BigInt make_random(size_t limbs, std::mt19937& rng) {
  std::string hex;
  for (size_t i = 0; i < limbs; ++i) {
    uint32_t v = rng();
    char buf[9];
    std::snprintf(buf, sizeof(buf), "%08x", v);
    hex += buf;
  }
  return H(hex);
}

// ============================================================
// Addition benchmarks
// ============================================================

static void BM_Add_Small(benchmark::State& state) {
  BigInt a(0x123456789ABCDEF0LL);
  BigInt b(0xFEDCBA9876543210LL);
  for (auto _ : state) {
    BigInt c = a + b;
    benchmark::DoNotOptimize(c);
  }
}
BENCHMARK(BM_Add_Small);

static void BM_Add_Large(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a + b;
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Add_Large)->Arg(64)->Arg(256)->Arg(1024)->Complexity();

// ============================================================
// Subtraction benchmarks
// ============================================================

static void BM_Sub_Small(benchmark::State& state) {
  BigInt a(0xFEDCBA9876543210LL);
  BigInt b(0x123456789ABCDEF0LL);
  for (auto _ : state) {
    BigInt c = a - b;
    benchmark::DoNotOptimize(c);
  }
}
BENCHMARK(BM_Sub_Small);

static void BM_Sub_Large(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  // Ensure a > b for subtraction
  if (BigInt::abs_compare(a, b) < 0) std::swap(a, b);
  for (auto _ : state) {
    BigInt c = a - b;
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Sub_Large)->Arg(64)->Arg(256)->Arg(1024)->Complexity();

// ============================================================
// Multiplication benchmarks — per algorithm
// ============================================================

static void BM_Mul_Naive(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(b, MulAlgo::Naive);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Mul_Naive)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128)
    ->Complexity();

static void BM_Mul_Karatsuba(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(b, MulAlgo::Karatsuba);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Mul_Karatsuba)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Complexity();

static void BM_Mul_NTT(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(b, MulAlgo::FFT_NTT);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Mul_NTT)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Complexity();

static void BM_Mul_SSA(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(b, MulAlgo::SSA);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Mul_SSA)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->Complexity();

static void BM_Mul_Auto(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a * b;
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Mul_Auto)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(2048)
    ->Complexity();

// ============================================================
// Squaring benchmarks
// ============================================================

static void BM_Square_Naive(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(a, MulAlgo::Naive);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Square_Naive)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128)
    ->Complexity();

static void BM_Square_Karatsuba(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(a, MulAlgo::Karatsuba);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Square_Karatsuba)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Complexity();

static void BM_Square_NTT(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  for (auto _ : state) {
    BigInt c = a.multiply(a, MulAlgo::FFT_NTT);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Square_NTT)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Complexity();

// ============================================================
// Division benchmarks
// ============================================================

static void BM_Div_Small(benchmark::State& state) {
  BigInt a = H("deadbeef12345678");
  BigInt b = H("0000ffff");
  for (auto _ : state) {
    BigInt c = a / b;
    benchmark::DoNotOptimize(c);
  }
}
BENCHMARK(BM_Div_Small);

static void BM_Div_Naive(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs / 2 + 1, rng);
  for (auto _ : state) {
    BigInt c = a.divide(b, DivAlgo::Naive);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Div_Naive)->Arg(32)->Arg(64)->Arg(128)->Complexity();

static void BM_Div_Newton(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs / 2 + 1, rng);
  for (auto _ : state) {
    BigInt c = a.divide(b, DivAlgo::Newton);
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Div_Newton)
    ->Arg(128)
    ->Arg(192)
    ->Arg(256)
    ->Arg(384)
    ->Arg(512)
    ->Arg(768)
    ->Arg(1024)
    ->Arg(1536)
    ->Arg(2048)
    ->Complexity();

static void BM_Div_Auto(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs / 2 + 1, rng);
  for (auto _ : state) {
    BigInt c = a / b;
    benchmark::DoNotOptimize(c);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Div_Auto)
    ->Arg(128)
    ->Arg(192)
    ->Arg(256)
    ->Arg(384)
    ->Arg(512)
    ->Arg(768)
    ->Arg(1024)
    ->Arg(1536)
    ->Arg(2048)
    ->Complexity();

// ============================================================
// String conversion benchmarks
// ============================================================

static void BM_FromHexString(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::string hex;
  for (size_t i = 0; i < limbs; ++i) hex += "deadbeef";
  for (auto _ : state) {
    BigInt v = BigInt::from_hex_string(hex);
    benchmark::DoNotOptimize(v);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_FromHexString)->Arg(8)->Arg(64)->Arg(256)->Arg(1024)->Complexity();

static void BM_ToHexString(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  for (auto _ : state) {
    std::string s = a.to_hex_string();
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_ToHexString)->Arg(8)->Arg(64)->Arg(256)->Arg(1024)->Complexity();

static void BM_FromDecString(benchmark::State& state) {
  auto digits = static_cast<size_t>(state.range(0));
  std::string dec;
  std::mt19937 rng(42);
  dec += static_cast<char>('1' + rng() % 9);
  for (size_t i = 1; i < digits; ++i)
    dec += static_cast<char>('0' + rng() % 10);
  for (auto _ : state) {
    BigInt v = BigInt::from_dec_string(dec);
    benchmark::DoNotOptimize(v);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(digits);
}
BENCHMARK(BM_FromDecString)
    ->Arg(20)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Complexity();

static void BM_ToString(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  for (auto _ : state) {
    std::string s = a.to_string();
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_ToString)->Arg(8)->Arg(64)->Arg(256)->Arg(1024)->Complexity();

// ============================================================
// Comparison benchmark
// ============================================================

static void BM_Compare(benchmark::State& state) {
  auto limbs = static_cast<size_t>(state.range(0));
  std::mt19937 rng(42);
  BigInt a = make_random(limbs, rng);
  BigInt b = make_random(limbs, rng);
  for (auto _ : state) {
    bool eq = (a == b);
    benchmark::DoNotOptimize(eq);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetComplexityN(limbs);
}
BENCHMARK(BM_Compare)->Arg(8)->Arg(64)->Arg(256)->Arg(1024)->Complexity();

BENCHMARK_MAIN();
