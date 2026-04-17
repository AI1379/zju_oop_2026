//
// Created by Renatus Madrigal on 2026/04/17.
//

#include <iostream>
#include <print>

#include "diary_base.hpp"

int main(int argc, char* argv[]) {
  Diary::DiaryStore store(Diary::diary_filename());
  try {
    store.load();
  } catch (const std::exception& e) {
    std::println(std::cerr, "Error loading diary: {}", e.what());
    return -1;
  }

  Diary::DiaryEntryList entries;
  if (argc == 3) {
    try {
      auto begin = Diary::parse_date(argv[1]);
      auto end = Diary::parse_date(argv[2]);
      entries = store.get_entries(begin, end);
    } catch (const std::exception& e) {
      std::println(std::cerr, "Error parsing date: {}", e.what());
      return -1;
    }
  } else {
    entries = store.get_entries();
  }

  for (const auto& entry : entries) {
    std::println("{}", entry);
  }

  return 0;
}
