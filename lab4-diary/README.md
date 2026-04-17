# Lab 4: Personal Diary

一个由四个独立程序组成的命令行个人日记软件，共享公共基础设施。

## 程序说明

| 程序 | 说明 |
|------|------|
| `pdadd <date>` | 添加或替换指定日期的日记条目。从标准输入读取内容，遇到单独一个 `.` 的行或 EOF 时结束。 |
| `pdlist [start end]` | 按日期顺序列出所有条目。可指定起止日期进行范围筛选。 |
| `pdshow <date>` | 显示指定日期条目的内容。 |
| `pdremove <date>` | 删除指定日期的条目。成功返回 0，失败返回 -1。 |

### 日期格式

所有程序支持以下日期格式：`YYYY-MM-DD`、`YYYY/MM/DD`、`YYYY.MM.DD`、`YYYYMMDD`。

## 构建方式

需要 C++23 编译器及 CMake ≥ 3.10。

```bash
cmake -B build -G Ninja
cmake --build build
```

### 持久化后端

通过 CMake 选项 `DIARY_USE_BINARY_PERSISTENCE` 可选择两种存储后端：

| 选项 | 后端 | 文件 |
|------|------|------|
| `ON`（默认） | 二进制格式，含 FNV-1a 完整性校验 | `diary.dat` |
| `OFF` | JSON 格式（nlohmann/json） | `diary.json` |

```bash
# 使用 JSON 模式
cmake -B build -G Ninja -DDIARY_USE_BINARY_PERSISTENCE=OFF
cmake --build build
```

可通过环境变量 `DIARY_FILE` 自定义日记文件路径。

#### 二进制后端设计

默认的二进制后端采用自定义格式，文件布局如下：

```
┌─────────────────────────────────────────────────────┐
│  File Header: "Yoimiya!" (8 bytes magic)            │
├─────────────────────────────────────────────────────┤
│  Entry Block × N:                                   │
│  ┌───────────────────────────────────────────────┐  │
│  │  "Klee" (4 bytes magic)                       │  │
│  │  FNV-1a hash (8 bytes)                        │  │
│  │  DiaryBlock { timestamp: u32, length: u32 }   │  │
│  │  content (variable length)                    │  │
│  │  end_tag = MAGIC_ENTRY_END ⊕ hash (8 bytes)   │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

- **文件头**：8 字节 magic `Yoimiya!`，用于快速识别文件格式。
- **条目块**：每个条目以 4 字节 magic `Klee` 起始，后接 FNV-1a 64 位哈希用于完整性校验。
- **日期编码**：`DiaryBlock.timestamp` 将年、月、日打包进单个 `uint32_t`（高 16 位年，中 8 位月，低 8 位日）。
- **变长内容**：利用 GNU C flexible array member 存储任意长度的日记正文（上限 1 MiB）。
- **防篡改**：末尾的 `end_tag` 由常量 `0x4B6C65654B6C6565`（"KleeKlee"）与哈希异或得到，加载时验证哈希与 end_tag 的一致性。

## 文件结构

```
diary_base.hpp    — DiaryEntry、DiaryStore 类定义，parse_date()
diary_base.cpp    — 加载/保存实现（二进制 & JSON 双后端）
pdadd.cpp         — 添加/替换条目
pdlist.cpp        — 列出条目
pdshow.cpp        — 显示单条内容
pdremove.cpp      — 删除条目
json.hpp          — nlohmann/json 单头文件（仅 JSON 后端使用）
demo.sh           — Bash 演示脚本
demo.bat          — Windows Batch 演示脚本
demo.ps1          — PowerShell 演示脚本
```

## 演示脚本

```bash
# Bash（Linux / macOS / MSYS2）
bash demo.sh ./build

# Windows Batch
demo.bat .\build

# PowerShell
.\demo.ps1 -BuildDir .\build
```

每个脚本覆盖所有四个程序的典型用例：添加条目、列出全部/按范围列出、查看指定条目、覆盖已有条目、删除条目、以及删除不存在日期时的错误处理。
