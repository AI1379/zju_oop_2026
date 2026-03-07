#!/usr/bin/env python3
"""Generate a random list of English names (no spaces) with three random integers per name.

Each output line is: <Name><space><int1><space><int2><space><int3>

Usage examples:
  python lab1-student1/generate_text.py -n 50
  python lab1-student1/generate_text.py -n 20 -o names.txt
"""

import argparse
import random
import sys

EN_FIRST = [
	"James", "John", "Robert", "Michael", "William", "David", "Richard",
	"Joseph", "Thomas", "Charles", "Christopher", "Daniel", "Matthew",
	"Anthony", "Mark", "Donald", "Steven", "Paul", "Andrew", "Joshua",
]

EN_LAST = [
	"Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller",
	"Davis", "Rodriguez", "Martinez", "Hernandez", "Lopez", "Gonzalez",
	"Wilson", "Anderson", "Thomas", "Taylor", "Moore", "Jackson", "Martin",
]


def generate_english_name_no_space():
	# concatenate first and last name without spaces
	return f"{random.choice(EN_FIRST)[0]}.{random.choice(EN_LAST)}"


def generate_line():
	name = generate_english_name_no_space()
	nums = [random.randint(0, 100) for _ in range(3)]
	return "{} {} {} {}".format(name, nums[0], nums[1], nums[2])


def main():
	parser = argparse.ArgumentParser(description="Generate random English names (no spaces) with 3 integers")
	parser.add_argument("-n", "--number", type=int, default=100, help="number of lines to generate")
	parser.add_argument("-o", "--output", help="output file (if omitted, prints to stdout)")

	args = parser.parse_args()

	lines = [generate_line() for _ in range(args.number)]

	if args.output:
		with open(args.output, "w", encoding="utf-8") as f:
			for line in lines:
				f.write(line + "\n")
	else:
		for line in lines:
			print(line)


if __name__ == "__main__":
	main()

