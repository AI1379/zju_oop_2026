# Lab 5 - Fraction

## 题目要求

实现一个分数类 `Fraction`，支持构造、算术运算、关系运算、类型转换、字符串转换、流输入输出、有限小数字符串解析等功能。

## 实现概览

本项目使用 **C++23** 标准，实现了基于模板的分数类与配套的任意精度整数类，核心代码约 2000 行。

### Fraction 模板类 (`fraction.hpp`)

`Fraction<T>` 是一个模板类，通过 C++20 concepts 约束模板参数，可搭配 `int`、`BigInt`、`Int128` 等类型使用：

- **构造函数**：默认构造、双参数构造 `(numerator, denominator)`、拷贝/移动构造，构造时自动规范化负分母
- **算术运算**：`+` `-` `*` `/`
- **关系运算**：通过 C++20 `<=>` 三路比较运算符实现 `== != < <= > >=`
- **类型转换**：`explicit operator double()`（仅 `BuiltinIntegral`）
- **字符串**：`to_string()`、静态方法 `parse(string)` 支持分数格式 `"a/b"` 与小数格式 `"x.y"`
- **流操作**：`operator<<`（始终可用）和 `operator>>`（仅 `BuiltinIntegral`）
- **约分**：`reduce()` 方法（仅 `DivModable`）

#### 各类型功能可用性

`Fraction<T>` 的部分功能通过 C++20 `requires` 子句约束，仅当模板参数 `T` 满足特定 concept 时才启用。

> **注**：`Int128` 在 GCC/Clang 下为 `__int128_t`（内建类型，满足 `BuiltinIntegral`），在 MSVC 下为自定义结构体（无 `/`、`%` 运算符，不满足 `Integral`，无法实例化 `Fraction<Int128>`）。下表以 GCC/Clang 下的 `__int128_t` 为准。

| 功能 | 约束条件 | `int` | `BigInt` | `Int128` |
|------|----------|:-----:|:--------:|:--------:|
| 默认构造 / 拷贝构造 / 移动构造 | `Integral` | ✓ | ✓ | ✓ |
| `Fraction(n, d)` 双参数构造 | `Integral` | ✓ | ✓ | ✓ |
| 算术 `+` `-` `*` `/` | `Integral` | ✓ | ✓ | ✓ |
| 关系 `< <= == != >= >`（via `<=>`） | `Integral` | ✓ | ✓ | ✓ |
| `to_string()` | `Integral` | ✓ | ✓ | ✓ |
| `operator<<`（输出流） | `Integral` | ✓ | ✓ | ✓ |
| `operator double` | `BuiltinIntegral` | ✓ | | ✓ |
| `operator>>`（输入流） | `BuiltinIntegral` | ✓ | | ✓ |
| `parse(string)` | `BuiltinIntegral \|\| StringConstructible` | ✓ | ✓ | ✓ |
| `reduce()` | `DivModable` | ✓ | ✓ | ✓ |

### BigInt 任意精度整数类 (`big_int.hpp` / `big_int.cpp`)

> 为什么写这个：因为看到这个题目的时候想起了以前写了一半没写完的大整数类，于是拿来并且完善了一下，同时也是对 `Fraction<T>` 模板的一个测试，确保对于其他大整数类模板本身也能用。

为实现大数分数运算，完整实现了一个大整数类：

- 底层采用 base-2^32 存储，`std::vector<uint32_t>` + 符号位
- 完整的算术运算符重载与三路比较
- 十进制/十六进制字符串互转

#### 乘法算法（自动分派）

| 规模 | 算法 | 复杂度 |
|------|------|--------|
| < 32 limbs | Naive（朴素乘法） | $O(n^2)$ |
| 32–256 limbs | Karatsuba | $O(n^{\log_2 3})$ |
| 256–1024 limbs | NTT + CRT 重建 | $O(n\log n)$ |
| ≥ 1024 limbs | SSA（Schönhage-Strassen） | $O(n \log n \log \log n)$ |

- NTT 使用三个 NTT-friendly 素数 + CRT（中国剩余定理）精确重建
- SSA 在 Fermat 环 $\mathbb{Z}/(2^M+1)\mathbb{Z}$ 中实现，原根为 2，所有旋转因子为移位操作

#### 除法算法

| 规模 | 算法 |
|------|------|
| ≤ 64 limbs | Knuth Algorithm D（经典长除法） |
| > 64 limbs | Newton-Raphson（牛顿迭代求倒数） |
| *Not implemented* | Burnikel-Ziegler 算法 |

### 辅助设施

- **`Int128` / `UInt128`**（`utility.hpp`）：跨平台 128 位整数，MSVC 下使用内联汇编 intrinsic 手写实现
- **C++20 Concepts 体系**（`concepts.hpp`）：`Integral`、`BuiltinIntegral`、`DivModable`、`StringConstructible` 等概念，控制 `Fraction<T>` 的条件功能启用

### 测试与基准

基于 GoogleTest 编写了完整的单元测试，覆盖 `Fraction<int>`、`BigInt`、`Int128`、`Fraction<BigInt>`、`Fraction<Int128>`。使用 Google Benchmark 对 BigInt 乘法与除法算法进行了性能评测。

## 构建

### 依赖

- **编译器**：支持 C++23 的编译器（Clang ≥ 17 / GCC ≥ 13 / MSVC ≥ 19.35）
- **CMake** ≥ 3.16
- **GoogleTest**（测试启用时必须）
- **Google Benchmark**（benchmark 启用时必须）

### CMake Options

| Option | 默认值 | 说明 |
|--------|--------|------|
| `FRACTION_ENABLE_TESTING` | `ON` | 构建 GoogleTest 测试目标 |
| `FRACTION_ENABLE_BENCHMARK` | `ON` | 构建 Google Benchmark 目标 |

### 配置与构建

```bash
cmake -B build \
  -DGTest_DIR=<GTest 配置路径> \
  [-Dbenchmark_DIR=<Benchmark 配置路径>] \
  [-DFRACTION_ENABLE_TESTING=OFF] \
  [-DFRACTION_ENABLE_BENCHMARK=OFF]
cmake --build build
```

Windows 示例：

```powershell
cmake -B build -DGTest_DIR=C:\path\to\gtest\lib\cmake\GTest
cmake --build build
```

### 运行测试

```bash
ctest --test-dir build --output-on-failure
```

或直接运行单个测试可执行文件：

```bash
./build/fraction_test
./build/big_int_test
./build/fraction_bigint_test
```

## AI 使用

核心的 `Fraction<T>`、大整数的 `Karatsuba` 和 `NTT` 为完全手工实现，`SSA` 和牛顿迭代法在 AI 辅助下完成，文档、测试和 Benchmark 以 AI 生成为主。
