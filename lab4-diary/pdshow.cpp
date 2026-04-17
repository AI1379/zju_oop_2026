//
// Created by Renatus Madrigal on 2026/04/17.
//

#include <iostream>
#include <print>

#include "diary_base.hpp"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::println("Usage: {} <date>", argv[0]);
    return -1;
  }

  Diary::DiaryStore store(Diary::diary_filename());
  try {
    store.load();
  } catch (const std::exception& e) {
    std::println(std::cerr, "Error loading diary: {}", e.what());
    return -1;
  }

  Diary::DiaryEntry::Date date;
  try {
    date = Diary::parse_date(argv[1]);
  } catch (const std::exception& e) {
    std::println(std::cerr, "Error parsing date: {}", e.what());
    return -1;
  }

  const auto& entries = store.get_entries_map();
  auto it = entries.find(date);
  if (it == entries.end()) {
    std::println(std::cerr, "No entry found for {:%Y-%m-%d}", date);
    return -1;
  }

  std::print("{}", it->second);
  return 0;
}
