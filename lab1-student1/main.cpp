#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <istream>
#include <numeric>
#include <ostream>
#include <sstream>
#include <vector>

using namespace std;

#if __cplusplus >= 202207L and defined(USE_MODERN_CPP)
#include <format>
#include <print>
#include <ranges>

struct Student {
  string name;
  int score1;
  int score2;
  int score3;
};

auto main(int argc, char* argv[]) -> int {
  if (argc < 2) {
    print("Usage: {} <input_file>", argv[0]);
    return 1;
  }

  ifstream input(argv[1]);
  if (!input) {
    print("Error opening file: {}", argv[1]);
    return 1;
  }
  auto content =
      string{istreambuf_iterator<char>{input}, istreambuf_iterator<char>{}};

  auto students =
      content | views::split('\n') | views::transform([](auto&& line) {
        return string{line.begin(), line.end()};
      }) |
      views::filter([](const string& line) { return !line.empty(); }) |
      views::transform([](auto&& line) -> Student {
        Student student;
        istringstream iss(line);
        iss >> student.name >> student.score1 >> student.score2 >>
            student.score3;
        return student;
      }) |
      ranges::to<vector>();

  auto name_length =
      max(ranges::max(students | views::transform([](const Student& s) {
                        return s.name.size();
                      })),
          size_t{7}  // "average" length
          ) +
      1;  // Add 1 for padding

  auto averages = students | views::transform([](const Student& s) {
                    return (s.score1 + s.score2 + s.score3) / 3.0;
                  }) |
                  ranges::to<vector>();
  auto zipped_students = views::zip(students, averages) |
                         views::transform([](auto&& pair) {
                           const auto& [student, average] = pair;
                           return make_pair(student, average);
                         }) |
                         ranges::to<vector>();

  ranges::sort(zipped_students, [](const auto& a, const auto& b) {
    return a.second > b.second;  // Sort by average score in descending order
  });

  // Calculate all avarage min and max

  // views::unzip is not available in C++23, so we can only do it manually
  auto s1 = students | views::transform([](const Student& s) {
              return make_tuple(s.score1, s.score2, s.score3);
            }) |
            views::elements<0>;
  auto s2 = students | views::transform([](const Student& s) {
              return make_tuple(s.score1, s.score2, s.score3);
            }) |
            views::elements<1>;
  auto s3 = students | views::transform([](const Student& s) {
              return make_tuple(s.score1, s.score2, s.score3);
            }) |
            views::elements<2>;

  auto get_avg_min_max = [](auto&& scores) {
    auto min_max = ranges::minmax(scores);
    auto average =
        ranges::fold_left(scores, 0.0, std::plus{}) / ranges::distance(scores);
    return make_tuple(average, min_max.min, min_max.max);
  };

  auto [avg1, min1, max1] = get_avg_min_max(s1);
  auto [avg2, min2, max2] = get_avg_min_max(s2);
  auto [avg3, min3, max3] = get_avg_min_max(s3);

  // Print the results
  println("{:<6} {:<{}} {:<10} {:<10} {:<10} {:<10}",  // Header
          "no", "name", name_length, "score1", "score2", "score3", "average");
  for (auto&& [no, zip_student] : views::enumerate(zipped_students)) {
    const auto& [student, average] = zip_student;
    println("{:<6} {:<{}} {:<10} {:<10} {:<10} {:<10g}",  // Body
            no + 1, student.name, name_length, student.score1, student.score2,
            student.score3, average);
  }

  // Footer
  println("{:<6} {:<{}} {:<10g} {:<10g} {:<10g}", "", "average", name_length,
          avg1, avg2, avg3);
  println("{:<6} {:<{}} {:<10} {:<10} {:<10}", "", "min", name_length, min1,
          min2, min3);
  println("{:<6} {:<{}} {:<10} {:<10} {:<10}", "", "max", name_length, max1,
          max2, max3);

  return 0;
}

#else

struct Student {
  string name;
  int score1;
  int score2;
  int score3;
  double average() const { return (score1 + score2 + score3) / 3.0; }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <input_file>" << endl;
    return 1;
  }
  ifstream input(argv[1]);
  if (!input) {
    cerr << "Error opening file: " << argv[1] << endl;
    return 1;
  }

  vector<string> lines;
  for (string line; getline(input, line);) {
    lines.push_back(line);
  }

  vector<Student> students;
  for (const auto& line : lines) {
    istringstream iss(line);
    Student student;
    iss >> student.name >> student.score1 >> student.score2 >> student.score3;
    students.push_back(student);
  }

  size_t name_length = 7;  // "average" length
  for (const auto& student : students) {
    name_length = max(name_length, student.name.size());
  }
  ++name_length;  // Add 1 for padding

  sort(students.begin(), students.end(),
       [](const Student& a, const Student& b) {
         return a.average() > b.average();
       });

  cout << left << setw(6) << "no" << setw(name_length) << "name" << setw(10)
       << "score1" << setw(10) << "score2" << setw(10) << "score3" << setw(10)
       << "average" << endl;
  for (size_t i = 0; i < students.size(); ++i) {
    const auto& student = students[i];
    cout << left << setw(6) << (i + 1) << setw(name_length) << student.name
         << setw(10) << student.score1 << setw(10) << student.score2 << setw(10)
         << student.score3 << setw(10) << student.average() << endl;
  }

  double avg1 = 0, avg2 = 0, avg3 = 0;
  int min1 = 100, min2 = 100, min3 = 100;
  int max1 = 0, max2 = 0, max3 = 0;
  for (const auto& student : students) {
    avg1 += student.score1;
    avg2 += student.score2;
    avg3 += student.score3;
    min1 = min(min1, student.score1);
    min2 = min(min2, student.score2);
    min3 = min(min3, student.score3);
    max1 = max(max1, student.score1);
    max2 = max(max2, student.score2);
    max3 = max(max3, student.score3);
  }
  avg1 /= students.size();
  avg2 /= students.size();
  avg3 /= students.size();

  cout << left << setw(6) << "" << setw(name_length) << "average" << setw(10)
       << avg1 << setw(10) << avg2 << setw(10) << avg3 << endl;
  cout << left << setw(6) << "" << setw(name_length) << "min" << setw(10)
       << min1 << setw(10) << min2 << setw(10) << min3 << endl;
  cout << left << setw(6) << "" << setw(name_length) << "max" << setw(10)
       << max1 << setw(10) << max2 << setw(10) << max3 << endl;
}

#endif
