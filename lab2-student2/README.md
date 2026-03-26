# Lab2 - Student 2

## 任务说明

本程序读取学生选课成绩记录，支持：

- 学生数量不固定
- 课程数量不固定
- 每个学生选课集合可不同（允许缺省列）

输出一个成绩汇总表：

- 按学生平均分降序排序
- 列出所有出现过的课程列（按课程名升序）
- 每位学生在未选课程对应列留空
- 最后输出各课程 `average / min / max` 行

## 输入格式

每行表示一个学生记录：

`Name Course1 Score1 Course2 Score2 ...`

例如：

```text
W.Xu Math 5 OOP 5 DS 5
T.Dixon DS 4 OOP 3
V.Chu ADS 3 OS 4 DB 4 OOP 4
```

## 编译与运行

### C++17 兼容版本（默认）

```bash
clang++ main.cpp -std=c++17 -O2 -Wall -Wextra -o lab2_17
./lab2_17 test_input.txt
```

### C++23+ 现代版本（ranges）

```bash
clang++ main.cpp -std=c++23 -O2 -Wall -Wextra -DUSE_MODERN_CPP -o lab2_23
./lab2_23 test_input.txt
```

## 随机测试数据生成

```bash
python generate_text.py -n 50 -o sample.txt
```
