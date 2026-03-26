#!/usr/bin/env python3
"""Generate random student-course-score records for Lab2.

Each line format:
  <StudentName> <Course1> <Score1> <Course2> <Score2> ...

Example:
  A.Smith OOP 5 Math 4 DS 3
"""

import argparse
import random

FIRST = [
    "Alex", "Ben", "Chris", "Dylan", "Ethan", "Finn", "Grace", "Helen",
    "Ivy", "Jack", "Kevin", "Luna", "Mia", "Nora", "Olivia", "Peter",
]
LAST = [
    "Wang", "Li", "Zhang", "Liu", "Chen", "Yang", "Huang", "Zhao",
    "Xu", "Wu", "Zhou", "Sun", "Ma", "Zhu", "Hu", "Guo",
]
COURSES = ["Math", "OOP", "DS", "ADS", "OS", "DB", "AI", "SE"]


def random_name() -> str:
    return f"{random.choice(FIRST)[0]}.{random.choice(LAST)}"


def one_line(min_courses: int, max_courses: int) -> str:
    name = random_name()
    k = random.randint(min_courses, max_courses)
    chosen = random.sample(COURSES, k=k)
    tokens = [name]
    for c in chosen:
        score = random.randint(0, 5)
        tokens.extend([c, str(score)])
    return " ".join(tokens)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--number", type=int, default=20)
    parser.add_argument("-o", "--output", default="")
    parser.add_argument("--min-courses", type=int, default=2)
    parser.add_argument("--max-courses", type=int, default=5)
    parser.add_argument("--seed", type=int, default=2026)
    args = parser.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    lines = [one_line(args.min_courses, args.max_courses) for _ in range(args.number)]

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            for line in lines:
                f.write(line + "\n")
    else:
        for line in lines:
            print(line)


if __name__ == "__main__":
    main()
