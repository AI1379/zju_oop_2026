//
// Created by Renatus Madrigal on 2026/04/17.
//

#include "diary_base.hpp"
using namespace Diary;
using namespace std;

#ifdef DIARY_USE_BINARY_PERSISTENCE
// If DIARY_USE_BINARY_PERSISTENCE is defined, we use binary format for
// persistence, just for fun. This is not human-readable, but it can be more
// compact and may be faster to read/write.

#include <cstdint>
#include <fstream>
#include <iostream>
#include <print>

// Begin of the whole binary file
static constexpr char MAGIC_BEGIN[] = "Yoimiya!";
static constexpr size_t MAGIC_BEGIN_LEN = 8;
// Begin of each diary entry block
static constexpr char MAGIC_ENTRY[] = "Klee";
static constexpr size_t MAGIC_ENTRY_LEN = 4;
static constexpr uint64_t MAGIC_ENTRY_END =
    0x4B6C65654B6C6565ULL;  // "KleeKlee" in big-endian

// Pure C struct for binary serialization. This is not meant to be used
// directly, but rather as a helper for DiaryEntry serialization.
struct DiaryBlock {
  // 12 bits year, 4 bits month, 5 bits day
  alignas(4) uint32_t timestamp;
  // Content length in bytes, up to 1 MiB
  alignas(4) uint32_t content_length;
  // Content data (variable length)
  // Note that we use a GNU C extension here, just a trick.
  char content[0];
};

constexpr uint64_t hash_for_diary_block(const DiaryBlock& block,
                                        const char* content) {
  // A simple hash function for demonstration. In a real application, you may
  // want to use a better hash function.
  uint64_t hash = 0xcbf29ce484222325;  // FNV-1a 64-bit offset basis
  const char* data = reinterpret_cast<const char*>(&block);
  for (size_t i = 0; i < sizeof(DiaryBlock); ++i) {
    hash ^= static_cast<uint64_t>(data[i]);
    hash *= 0x100000001b3;  // FNV-1a 64-bit prime
  }
  for (size_t i = 0; i < block.content_length; ++i) {
    hash ^= static_cast<uint64_t>(content[i]);
    hash *= 0x100000001b3;
  }
  return hash;
}

constexpr uint64_t hash_for_diary_block(const DiaryBlock& block) {
  return hash_for_diary_block(
      block, reinterpret_cast<const char*>(&block) + sizeof(block));
}

void serialize(std::ostream& os, const DiaryEntry& entry) {
  // Convert DiaryEntry to DiaryBlock
  DiaryBlock* block_ptr = reinterpret_cast<DiaryBlock*>(
      new char[sizeof(DiaryBlock) + entry.get_content().size()]);

  auto ymd = entry.get_timestamp();
  block_ptr->timestamp = (static_cast<int>(ymd.year()) << 16) |
                         (static_cast<uint32_t>(ymd.month()) << 8) |
                         static_cast<uint32_t>(ymd.day());
  block_ptr->content_length = static_cast<uint32_t>(entry.get_content().size());

  memcpy(&block_ptr->content, entry.get_content().data(),
         entry.get_content().size());

  // Calculate the hash of the block for integrity check
  uint64_t hash = hash_for_diary_block(*block_ptr);
  auto end_tag = MAGIC_ENTRY_END ^ hash;  // XOR with hash to prevent tampering

  os.write(MAGIC_ENTRY, MAGIC_ENTRY_LEN);
  os.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
  os.write(reinterpret_cast<const char*>(block_ptr),
           sizeof(DiaryBlock) + block_ptr->content_length);
  os.write(reinterpret_cast<const char*>(&end_tag), sizeof(end_tag));

  // Clean up the allocated memory
  delete[] reinterpret_cast<char*>(block_ptr);
}

DiaryEntry deserialize(std::istream& is) {
  static char magic[MAGIC_ENTRY_LEN];
  is.read(magic, MAGIC_ENTRY_LEN);
  if (std::string(magic, MAGIC_ENTRY_LEN) != MAGIC_ENTRY) {
    throw std::runtime_error("Invalid diary entry format");
  }

  uint64_t hash;
  is.read(reinterpret_cast<char*>(&hash), sizeof(hash));

  DiaryBlock block;
  is.read(reinterpret_cast<char*>(&block), sizeof(DiaryBlock));
  char* content = new char[block.content_length];
  is.read(content, block.content_length);

  uint64_t end_tag;
  is.read(reinterpret_cast<char*>(&end_tag), sizeof(end_tag));

  uint64_t expected_hash = hash_for_diary_block(block, content);
  uint64_t expected_end_tag = MAGIC_ENTRY_END ^ expected_hash;

  if (end_tag != expected_end_tag || hash != expected_hash) {
    throw std::runtime_error(
        "Diary entry integrity check failed: Hash mismatch");
  }

  auto year = static_cast<int>(block.timestamp >> 16);
  auto month = static_cast<unsigned>((block.timestamp >> 8) & 0xFF);
  auto day = static_cast<unsigned>(block.timestamp & 0xFF);
  return DiaryEntry(
      {
          std::chrono::year{year},
          std::chrono::month{month},
          std::chrono::day{day},
      },
      std::string(content, block.content_length));
}

template <typename Func>
struct Guard {
  Func func;
  explicit Guard(Func func) : func(func) {}
  ~Guard() { func(); }
};

void DiaryStore::save() const {
  std::ofstream ofs(filename_, std::ios::binary);
  Guard guard([&ofs]() {
    if (ofs.is_open()) {
      ofs.close();
    }
  });
  if (!ofs) {
    throw std::runtime_error("Failed to open file for writing: " + filename_);
  }
  ofs.write(MAGIC_BEGIN, MAGIC_BEGIN_LEN);
  for (const auto& [date, entry] : entries_) {
    serialize(ofs, entry);
  }
}

void DiaryStore::load() {
  std::ifstream ifs(filename_, std::ios::binary);
  Guard guard([&ifs]() {
    if (ifs.is_open()) {
      ifs.close();
    }
  });
  if (!ifs) {
    // If the file does not exist, we can just start with an empty diary.
    return;
  }
  char magic[MAGIC_BEGIN_LEN];
  ifs.read(magic, MAGIC_BEGIN_LEN);
  if (std::string(magic, MAGIC_BEGIN_LEN) != MAGIC_BEGIN) {
    std::println(std::cerr, "Read MAGIC_BEGIN: {}", magic);
    throw std::runtime_error("Invalid diary file format: MAGIC_BEGIN mismatch");
  }
  while (ifs.peek() != EOF) {
    DiaryEntry entry = deserialize(ifs);
    entries_.insert({entry.get_timestamp(), entry});
  }
}

#else
// Else, we use nlohmann/json for JSON persistence
// For simplicity, we include the header here. In a real project, it may be
// better to use FetchContent or a submodule or CPM.cmake to manage this
// dependency.
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <print>

using json = nlohmann::json;

void DiaryStore::save() const {
  json j = json::array();
  for (const auto& [date, entry] : entries_) {
    auto ymd = entry.get_timestamp();
    j.push_back({
        {"date",
         std::format("{:%Y-%m-%d}", ymd)},
        {"content", entry.get_content()},
    });
  }
  std::ofstream ofs(filename_);
  if (!ofs) {
    throw std::runtime_error("Failed to open file for writing: " + filename_);
  }
  ofs << j.dump(2);
}

void DiaryStore::load() {
  std::ifstream ifs(filename_);
  if (!ifs) {
    return;
  }
  json j;
  ifs >> j;
  for (const auto& item : j) {
    auto date = parse_date(item["date"].get<std::string>());
    entries_.insert({date, DiaryEntry(date, item["content"].get<std::string>())});
  }
}

#endif

DiaryEntry::Date Diary::parse_date(const std::string& date_str) {
  static constexpr auto possible_formats = {
      "%Y-%m-%d",
      "%Y/%m/%d",
      "%Y.%m.%d",
  };
  std::istringstream iss(date_str);
  std::chrono::year_month_day ymd;
  for (const auto& fmt : possible_formats) {
    iss.seekg(0);
    iss >> std::chrono::parse(fmt, ymd);
    if (!iss.fail()) {
      return ymd;
    }
  }
  int y;
  unsigned m, d;
  sscanf(date_str.c_str(), "%4d%2d%2d", &y, &m, &d);
  if (y > 0 && m > 0 && d > 0) {
    return std::chrono::year_month_day{
        std::chrono::year{y},
        std::chrono::month{m},
        std::chrono::day{d},
    };
  }
  throw std::runtime_error("Invalid date format: " + date_str);
}