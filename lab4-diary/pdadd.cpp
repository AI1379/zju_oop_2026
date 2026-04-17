//
// Created by Renatus Madrigal on 2026/04/17.
//

#include <iostream>
#include <print>
#include <string>

#include "diary_base.hpp"
using namespace std;

int main(int argc, char* argv[]) {
  auto diary_filename = Diary::diary_filename();
  if (argc < 2) {
    std::println("Usage: {} <date>", argv[0]);
    std::println("Example: {} 2026-04-17", argv[0]);
    return -1;
  }
  Diary::DiaryStore store(diary_filename);
  try {
    store.load();
  } catch (const std::exception& e) {
    std::println(std::cerr, "Error loading diary: {}", e.what());
    return -1;
  }

  std::string date_str = argv[1];
  Diary::DiaryEntry::Date date;
  try {
    date = Diary::parse_date(date_str);
  } catch (const std::exception& e) {
    std::println(std::cerr, "Error parsing date: {}", e.what());
    return -1;
  }

  std::string line;
  std::println("Enter diary content (end with a line with a dot or EOF):");
  std::stringstream content;
  while (std::getline(std::cin, line)) {
    if (line == ".") {
      break;
    }
    content << line << "\n";
  }
  Diary::DiaryEntry entry(date, content.str());
  store.add_entry(entry);
  try {
    store.save();
    std::println("Diary saved successfully.");
  } catch (const std::exception& e) {
    std::println(std::cerr, "Error saving diary: {}", e.what());
    return -1;
  }

  return 0;
}