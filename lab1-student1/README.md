# Lab1 — 学生成绩汇总与排序

## 项目描述

这是一个用于读取包含学生姓名和三门成绩的文本文件，计算每个学生平均分、按平均分降序排序，并以表格形式输出结果的简单 C++ 程序。

主要功能

- 从输入文件读取每行数据，格式为：`Name score1 score2 score3`（姓名中不含空格，示例：`S.Jones 56 27 9`）。
- 计算每名学生的平均分 (保留小数)。
- 按平均分降序输出编号、姓名、三门成绩与平均分。
- 在输出末尾显示每门课的平均值、最小值与最大值。

文件说明

- 主程序： [main.cpp](lab1-student1/main.cpp#L1)
- 测试数据生成脚本： [generate_text.py](lab1-student1/generate_text.py#L1)
- 示例输入： [test_text.txt](lab1-student1/test_text.txt#L1)

## 编译与运行

推荐使用已安装的 C++ 编译器（g++ / clang++）。仓库包含两种实现：

- 兼容模式（默认，使用 C++14）：

```bash
clang++ main.cpp -std=c++14 -o main.exe
./main.exe test_text.txt
```

- 现代 C++ 实现（可选，使用 C++23 并定义宏 `USE_MODERN_CPP`，需要支持 <format>/<print> 等特性）：

```bash
clang++ main.cpp -std=c++23 -DUSE_MODERN_CPP -o main.exe
./main.exe test_text.txt
```

## 输入格式

每行包含 4 个字段，由空格分隔：姓名（无空格）与三门课的整数成绩。示例行：

```
S.Jones 56 27 9
C.Taylor 6 33 51
```

## 示例输出

程序会输出表格，包括表头、每个学生的编号/姓名/3 门成绩/平均分，以及统计行（average/min/max）。示例：

```
no    name        score1    score2    score3    average
1     A.Lopez     87        74        82        81
2     J.Lopez     89        54        29        57.3333
3     M.Anderson  29        56        85        56.6667
4     J.Wilson    73        10        80        54.3333
5     M.Rodriguez 39        19        87        48.3333
6     R.Jackson   95        16        31        47.3333
7     R.Wilson    61        72        0         44.3333
8     W.Williams  96        32        2         43.3333
9     S.Jones     56        27        9         30.6667
10    C.Taylor    6         33        51        30
      average     63.1      39.3      45.6
      min         6         10        0
      max         96        74        87
```

## 备注

- 若输入文件为空或格式不正确，程序会尽量报告错误并退出。
- 若要生成更多测试数据，可使用 [generate_text.py](lab1-student1/generate_text.py#L1)：

```bash
python generate_text.py -n 50 -o sample.txt
```

- 生成数据的 Python 代码是 Copilot 辅助写的，但是 C++ 部分是手写的。
