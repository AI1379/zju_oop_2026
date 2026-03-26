#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

#if __cplusplus >= 202302L && defined(USE_MODERN_CPP)
#include <print>
#include <ranges>

/// \brief Represents a student with their name and course scores.
/// \details
/// This struct stores a student's name and a map of course names to scores.
/// It provides a method to calculate the student's average score across all
/// courses.
struct Student {
  string name;                    ///< Student's name (unique identifier)
  map<string, int> course_score;  ///< Map from course name to score

  /// \brief Calculates the average score across all courses.
  /// \return The arithmetic mean of all course scores, or 0.0 if no courses.
  auto average() const -> double {
    if (course_score.empty()) {
      return 0.0;
    }
    // Use fold_left to sum all scores in functional style
    const auto sum =
        ranges::fold_left(course_score | views::values, 0, plus<>{});
    return static_cast<double>(sum) / static_cast<double>(course_score.size());
  }
};

/// \brief Statistics for a single course.
/// Holds computed statistics (average, min, max scores) for a course.
struct CourseStat {
  double average = 0.0;    ///< Average score for this course
  int min_score = 0;       ///< Minimum score for this course
  int max_score = 0;       ///< Maximum score for this course
  bool has_value = false;  ///< Whether this course has valid data
};

/// \brief Parses a single line of input into a Student record.
/// \param line Input line with format: Name Course1 Score1 Course2 Score2 ...
/// \param out_student Output Student object (modified in-place)
/// \return true if parsing succeeded, false if line format is invalid
/// \details
/// Expected format: Name followed by alternating course/score pairs.
/// Example: "W.Xu Math 5 OOP 5 DS 5"
auto parse_line(const string& line, Student& out_student) -> bool {
  istringstream iss(line);
  if (!(iss >> out_student.name)) {
    return false;
  }

  string course;
  int score = 0;
  while (iss >> course >> score) {
    out_student.course_score[course] = score;
  }
  return !out_student.name.empty();
}

/// \brief Collects all unique course names from all students.
/// \param students Vector of Student records
/// \return Sorted vector of unique course names
/// \details
/// Uses C++23 ranges to flatten all course names from all students,
/// removes duplicates, and returns sorted course names.
auto collect_course_names(const vector<Student>& students) -> vector<string> {
  set<string> names;
  auto all_courses = students | views::transform([](const Student& st) {
                       return st.course_score | views::keys;
                     }) |
                     views::join;
  ranges::copy(all_courses, inserter(names, names.end()));
  return {names.begin(), names.end()};
}

/// \brief Computes statistics for a specific course.
/// \param students Vector of all student records
/// \param course Course name to compute statistics for
/// \return CourseStat object containing average, min, max, and validity flag
/// \details
/// Filters students who took the course using optional-based pipeline:
/// 1. Transform each student's score to optional<int>
/// 2. Filter only present (non-empty) values
/// 3. Compute min, max, and average across scores
auto compute_stat_for_course(const vector<Student>& students,
                             const string& course) -> CourseStat {
  auto maybe_scores =
      students | views::transform([&course](const Student& st) {
        const auto it = st.course_score.find(course);
        return it == st.course_score.end() ? optional<int>{}
                                           : optional<int>{it->second};
      }) |
      views::filter(
          [](const optional<int>& value) { return value.has_value(); }) |
      views::transform([](const optional<int>& value) { return *value; });

  const auto scores = ranges::to<vector>(maybe_scores);

  CourseStat stat;
  if (scores.empty()) {
    return stat;
  }

  stat.has_value = true;
  stat.min_score = *ranges::min_element(scores);
  stat.max_score = *ranges::max_element(scores);
  const auto sum = ranges::fold_left(scores, 0, plus<>{});
  stat.average = static_cast<double>(sum) / static_cast<double>(scores.size());
  return stat;
}

/// \brief Main entry point for the C++23 modern implementation.
/// \param argc Argument count (must be >= 2)
/// \param argv Command-line arguments (argv[1] = input file path)
/// \return 0 on success, 1 on error
auto main(int argc, char* argv[]) -> int {
  if (argc < 2) {
    println("Usage: {} <input_file>", argv[0]);
    return 1;
  }

  ifstream input(argv[1]);
  if (!input) {
    println("Error opening file: {}", argv[1]);
    return 1;
  }

  vector<Student> students;
  for (string line; getline(input, line);) {
    if (line.empty()) {
      continue;
    }
    Student st;
    if (!parse_line(line, st)) {
      continue;
    }
    students.push_back(std::move(st));
  }

  if (students.empty()) {
    println("No valid student records found.");
    return 1;
  }

  ranges::sort(students, [](const Student& a, const Student& b) {
    if (a.average() != b.average()) {
      return a.average() > b.average();
    }
    return a.name < b.name;
  });

  const auto course_names = collect_course_names(students);
  const auto name_width = max<size_t>(
      7, ranges::max(students | views::transform([](const Student& s) {
                       return s.name.size();
                     })));
  constexpr int no_width = 6;
  constexpr int col_width = 8;
  constexpr int avg_width = 10;

  print("{:<{}} {:<{}}", "no", no_width, "name",
        static_cast<int>(name_width + 1));
  for (const auto& c : course_names) {
    print(" {:<{}}", c, col_width - 1);
  }
  println(" {:<{}}", "average", avg_width - 1);

  for (const auto& [i, st] : views::enumerate(students)) {
    print("{:<{}} {:<{}}", i + 1, no_width, st.name,
          static_cast<int>(name_width + 1));
    for (const auto& c : course_names) {
      const auto it = st.course_score.find(c);
      if (it == st.course_score.end()) {
        print(" {:<{}}", "", col_width - 1);
      } else {
        print(" {:<{}}", it->second, col_width - 1);
      }
    }
    println(" {:<{}.5f}", st.average(), avg_width - 1);
  }

  const auto course_stats = course_names |
                            views::transform([&students](const string& c) {
                              return compute_stat_for_course(students, c);
                            }) |
                            ranges::to<vector>();

  print("{:<{}} {:<{}}", "", no_width, "average",
        static_cast<int>(name_width + 1));
  for (const auto& stat : course_stats) {
    if (stat.has_value) {
      print(" {:<{}}", stat.average, col_width - 1);
    } else {
      print(" {:<{}}", "", col_width - 1);
    }
  }
  println("");

  print("{:<{}} {:<{}}", "", no_width, "min", static_cast<int>(name_width + 1));
  for (const auto& stat : course_stats) {
    if (stat.has_value) {
      print(" {:<{}}", stat.min_score, col_width - 1);
    } else {
      print(" {:<{}}", "", col_width - 1);
    }
  }
  println("");

  print("{:<{}} {:<{}}", "", no_width, "max", static_cast<int>(name_width + 1));
  for (const auto& stat : course_stats) {
    if (stat.has_value) {
      print(" {:<{}}", stat.max_score, col_width - 1);
    } else {
      print(" {:<{}}", "", col_width - 1);
    }
  }
  println("");

  return 0;
}

#else

/// \brief Represents a student with their name and course scores (C++17
/// version).
/// \details
/// This class encapsulates a student's data and provides methods to manage
/// course scores and calculate statistics.
class Student {
 public:
  /// \brief Constructor for Student.
  /// \param student_name The name of the student (default empty string)
  explicit Student(string student_name = "") : name_(std::move(student_name)) {}

  /// \brief Sets the score for a course.
  /// \param course Course name
  /// \param score Score value for the course
  void setScore(const string& course, int score) {
    course_score_[course] = score;
  }

  /// \brief Gets the student's name.
  /// \return Const reference to the student's name
  const string& name() const { return name_; }

  /// \brief Calculates the average score across all courses.
  /// \return Arithmetic mean of all course scores, or 0.0 if no courses
  double average() const {
    if (course_score_.empty()) {
      return 0.0;
    }
    int sum = 0;
    for (map<string, int>::const_iterator it = course_score_.begin();
         it != course_score_.end(); ++it) {
      sum += it->second;
    }
    return static_cast<double>(sum) / static_cast<double>(course_score_.size());
  }

  /// \brief Checks if the student has taken a specific course.
  /// \param course Course name to check
  /// \return true if student has a score for the course, false otherwise
  bool hasCourse(const string& course) const {
    return course_score_.find(course) != course_score_.end();
  }

  /// \brief Gets the score for a specific course.
  /// \param course Course name
  /// \return Score value if course exists, -1 if not found
  int scoreOf(const string& course) const {
    map<string, int>::const_iterator it = course_score_.find(course);
    if (it == course_score_.end()) {
      return -1;
    }
    return it->second;
  }

  /// \brief Gets the map of all course scores.
  /// \return Const reference to the course_score map
  const map<string, int>& scores() const { return course_score_; }

 private:
  string name_;
  map<string, int> course_score_;
};

/// \brief Statistics for a single course (C++17 version).
struct CourseStat {
  double average;  ///< Average score for this course
  int min_score;   ///< Minimum score for this course
  int max_score;   ///< Maximum score for this course
  bool has_value;  ///< Whether this course has valid data

  /// \brief Constructor initializing all fields to zero or false.
  CourseStat() : average(0.0), min_score(0), max_score(0), has_value(false) {}
};

/// \brief Parses a single line of input into a Student record (C++17 version).
/// \param line Input line with format: Name Course1 Score1 Course2 Score2 ...
/// \param student Output Student object (modified in-place)
/// \return true if parsing succeeded, false if line format is invalid
static bool parseLine(const string& line, Student& student) {
  istringstream iss(line);
  string name;
  if (!(iss >> name)) {
    return false;
  }

  student = Student(name);
  string course;
  int score = 0;
  while (iss >> course >> score) {
    student.setScore(course, score);
  }
  return true;
}

/// \brief Collects all unique course names from all students (C++17 version).
/// \param students Vector of Student objects
/// \return Sorted vector of unique course names
static vector<string> collectCourseNames(const vector<Student>& students) {
  set<string> names;
  for (size_t i = 0; i < students.size(); ++i) {
    const map<string, int>& sc = students[i].scores();
    for (map<string, int>::const_iterator it = sc.begin(); it != sc.end();
         ++it) {
      names.insert(it->first);
    }
  }
  return vector<string>(names.begin(), names.end());
}

/// \brief Computes statistics for a specific course (C++17 version).
/// \param students Vector of all student records
/// \param course Course name to compute statistics for
/// \return CourseStat object containing average, min, max, and validity flag
static CourseStat computeStatForCourse(const vector<Student>& students,
                                       const string& course) {
  CourseStat stat;
  int sum = 0;
  int count = 0;
  int min_v = numeric_limits<int>::max();
  int max_v = numeric_limits<int>::min();

  for (size_t i = 0; i < students.size(); ++i) {
    if (!students[i].hasCourse(course)) {
      continue;
    }
    const int v = students[i].scoreOf(course);
    sum += v;
    ++count;
    if (v < min_v) {
      min_v = v;
    }
    if (v > max_v) {
      max_v = v;
    }
  }

  if (count == 0) {
    return stat;
  }

  stat.has_value = true;
  stat.average = static_cast<double>(sum) / static_cast<double>(count);
  stat.min_score = min_v;
  stat.max_score = max_v;
  return stat;
}

/// \brief Main entry point for the C++17 traditional implementation.
/// \param argc Argument count (must be >= 2)
/// \param argv Command-line arguments (argv[1] = input file path)
/// \return 0 on success, 1 on error
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

  vector<Student> students;
  string line;
  while (getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    Student st;
    if (parseLine(line, st)) {
      students.push_back(st);
    }
  }

  if (students.empty()) {
    cerr << "No valid student records found." << endl;
    return 1;
  }

  sort(students.begin(), students.end(),
       [](const Student& a, const Student& b) {
         const double avga = a.average();
         const double avgb = b.average();
         if (avga != avgb) {
           return avga > avgb;
         }
         return a.name() < b.name();
       });

  const vector<string> course_names = collectCourseNames(students);

  size_t name_width = 7;
  for (size_t i = 0; i < students.size(); ++i) {
    if (students[i].name().size() > name_width) {
      name_width = students[i].name().size();
    }
  }
  ++name_width;

  const int no_width = 6;
  const int col_width = 8;
  const int avg_width = 10;

  cout << left << setw(no_width) << "no" << setw(static_cast<int>(name_width))
       << "name";
  for (size_t i = 0; i < course_names.size(); ++i) {
    cout << " " << setw(col_width - 1) << course_names[i];
  }
  cout << " " << setw(avg_width - 1) << "average" << endl;

  for (size_t i = 0; i < students.size(); ++i) {
    const Student& st = students[i];
    cout << left << setw(no_width) << (i + 1)
         << setw(static_cast<int>(name_width)) << st.name();
    for (size_t j = 0; j < course_names.size(); ++j) {
      const int score = st.scoreOf(course_names[j]);
      if (score < 0) {
        cout << " " << setw(col_width - 1) << "";
      } else {
        cout << " " << setw(col_width - 1) << score;
      }
    }
    cout << " " << setw(avg_width - 1) << fixed << setprecision(5)
         << st.average() << endl;
  }

  cout << left << setw(no_width) << "" << setw(static_cast<int>(name_width))
       << "average";
  for (size_t i = 0; i < course_names.size(); ++i) {
    CourseStat stat = computeStatForCourse(students, course_names[i]);
    if (stat.has_value) {
      cout << " " << setw(col_width - 1) << setprecision(1) << fixed
           << stat.average;
    } else {
      cout << " " << setw(col_width - 1) << "";
    }
  }
  cout << endl;

  cout << left << setw(no_width) << "" << setw(static_cast<int>(name_width))
       << "min";
  for (size_t i = 0; i < course_names.size(); ++i) {
    CourseStat stat = computeStatForCourse(students, course_names[i]);
    if (stat.has_value) {
      cout << " " << setw(col_width - 1) << stat.min_score;
    } else {
      cout << " " << setw(col_width - 1) << "";
    }
  }
  cout << endl;

  cout << left << setw(no_width) << "" << setw(static_cast<int>(name_width))
       << "max";
  for (size_t i = 0; i < course_names.size(); ++i) {
    CourseStat stat = computeStatForCourse(students, course_names[i]);
    if (stat.has_value) {
      cout << " " << setw(col_width - 1) << stat.max_score;
    } else {
      cout << " " << setw(col_width - 1) << "";
    }
  }
  cout << endl;

  return 0;
}

#endif
