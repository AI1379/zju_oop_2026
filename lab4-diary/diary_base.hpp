//
// Created by Renatus Madrigal on 2026/04/17.
//

#pragma once

#ifndef LAB4_DIARY_DIARY_BASE_HPP
#define LAB4_DIARY_DIARY_BASE_HPP

#include <chrono>
#include <cstdlib>
#include <format>
#include <map>
#include <ranges>

namespace Diary {
inline constexpr std::string diary_filename() {
  const char* env = std::getenv("DIARY_FILE");
  if (env) {
    return std::string(env);
  }
#ifdef DIARY_USE_BINARY_PERSISTENCE
  return "diary.dat";
#else
  return "diary.json";
#endif
}

class DiaryEntry {
 public:
  using Date = std::chrono::year_month_day;

  DiaryEntry() = default;

  DiaryEntry(const Date& timestamp, const std::string& content)
      : timestamp(timestamp), content(content) {}

  explicit DiaryEntry(const std::string& content)
      : timestamp(
            std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(
                std::chrono::system_clock::now())}),
        content(content) {}

  const Date& get_timestamp() const { return timestamp; }
  const std::string get_formatted_time_stamp() const {
    return std::format("{:%Y-%m-%d}", timestamp);
  }
  const std::string& get_content() const { return content; }

 private:
  Date timestamp;
  std::string content;
};

using DiaryEntryList = std::vector<DiaryEntry>;

class DiaryStore {
 public:
  DiaryStore(const std::string& filename) : filename_(filename) {}
  ~DiaryStore() = default;

  void add_entry(const DiaryEntry& entry) {
    entries_[entry.get_timestamp()] = entry;
  }
  void remove_entry(const DiaryEntry::Date& date) { entries_.erase(date); }

  // This is a temporary vector, so we return it by value instead of const ref.
  DiaryEntryList get_entries() const {
    auto result =
        entries_ | std::views::values | std::ranges::to<std::vector>();
    return result;
  }

  DiaryEntryList get_entries(const DiaryEntry::Date& begin,
                             const DiaryEntry::Date& end) const {
    return std::ranges::subrange(entries_.lower_bound(begin),
                                 entries_.upper_bound(end)) |
           std::views::values | std::ranges::to<DiaryEntryList>();
  }

  const std::map<DiaryEntry::Date, DiaryEntry>& get_entries_map() const {
    return entries_;
  }

  void load();
  void save() const;

 private:
  std::string filename_;
  std::map<DiaryEntry::Date, DiaryEntry> entries_;
};
DiaryEntry::Date parse_date(const std::string& date_str);
}  // namespace Diary

template <>
struct std::formatter<Diary::DiaryEntry> : std::formatter<std::string> {
  auto format(const Diary::DiaryEntry& entry, auto& ctx) const {
    return std::formatter<std::string>::format(
        std::format("[{}] {}", entry.get_formatted_time_stamp(),
                    entry.get_content()),
        ctx);
  }
};

#endif