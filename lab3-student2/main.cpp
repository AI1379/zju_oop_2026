// Adventure - CLI Castle Exploration Game
// Lab 3, Object-Oriented Programming, ZJU 2026 Spring
// Compile: clang++ -std=c++23 -DUSE_MODERN_CPP -Wall -Wextra -o adventure main.cpp
//    or:   clang++ -std=c++23 -Wall -Wextra -o adventure main.cpp

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace std;

#if __cplusplus >= 202302L && defined(USE_MODERN_CPP)
#include <print>
#include <ranges>
#endif

// ========== Modern C++ ==========
#if __cplusplus >= 202302L && defined(USE_MODERN_CPP)

class Room {
 public:
  explicit Room(string name) : name_(std::move(name)) {}
  virtual ~Room() = default;

  // return false to end the game
  virtual auto onEnter(bool has_princess) -> bool = 0;
  virtual auto showInfo() const -> void = 0;

  auto name() const -> const string& { return name_; }

  auto exitNames() const -> vector<string> {
    return exits_ | views::keys | ranges::to<vector>();
  }

  void addExit(string dir, string target) {
    exits_[std::move(dir)] = std::move(target);
  }

  auto findExit(const string& dir) const -> const string* {
    auto it = exits_.find(dir);
    return it != exits_.end() ? &it->second : nullptr;
  }

 protected:
  // helper to print "east, west and up"
  static auto fmtExits(const vector<string>& v) -> string {
    if (v.empty()) return "none";
    if (v.size() == 1) return v[0];
    if (v.size() == 2) return v[0] + " and " + v[1];
    string s;
    for (size_t i = 0; i + 1 < v.size(); ++i) {
      s += v[i];
      s += (i + 2 < v.size()) ? ", " : " and ";
    }
    s += v.back();
    return s;
  }

  void printRoomInfo() const {
    auto exits = exitNames();
    println("Welcome to the {}. There are {} exit{}: {}.",
            name_, exits.size(), exits.size() == 1 ? "" : "s", fmtExits(exits));
    print("> ");
    cout.flush();
  }

  string name_;
  map<string, string> exits_;
};

class NormalRoom : public Room {
 public:
  using Room::Room;
  auto onEnter(bool) -> bool override { return true; }
  auto showInfo() const -> void override { printRoomInfo(); }
};

class MonsterRoom : public Room {
 public:
  using Room::Room;
  auto onEnter(bool) -> bool override {
    println("\nYou step into the {}...", name_);
    println("A monster jumps out! You're dead.");
    println("GAME OVER");
    return false;
  }
  auto showInfo() const -> void override {}
};

class PrincessRoom : public Room {
 public:
  using Room::Room;
  auto onEnter(bool has_princess) -> bool override {
    if (has_princess) {
      println("\nYou come back to the {} with the princess.", name_);
      return true;
    }
    println("\nYou open the door of the {}...", name_);
    println("A princess is sitting inside. She looks relieved.");
    println("  Princess: \"Please get me out of here!\"");
    println("  You: \"Let's go back to the lobby!\"");
    return true;
  }
  auto showInfo() const -> void override { printRoomInfo(); }
};

class TreasureRoom : public Room {
 public:
  using Room::Room;
  auto onEnter(bool) -> bool override {
    println("\nYou find a dusty treasure chest in the {}.", name_);
    println("Inside there's a note: \"Watch out — a monster is hiding");
    println("somewhere in this castle. Choose your path carefully.\"");
    return true;
  }
  auto showInfo() const -> void override { printRoomInfo(); }
};

class DarkCorridor : public Room {
 public:
  using Room::Room;
  auto onEnter(bool) -> bool override {
    println("\nYou walk through the {}...", name_);
    println("It's dark. You can hear your footsteps echoing.");
    return true;
  }
  auto showInfo() const -> void override { printRoomInfo(); }
};

class Game {
 public:
  void run() {
    buildCastle();
    placeSpecialRooms();

    println("======= A D V E N T U R E =======");
    println("Find the princess and bring her back to the lobby.");
    println("Beware of monsters!");
    println();

    gameLoop();
  }

 private:
  map<string, unique_ptr<Room>> rooms_;
  string cur_ = "lobby";
  bool hasPrincess_ = false;

  void buildCastle() {
    // 3 floors: ground / upper / tower
    rooms_["lobby"]            = make_unique<NormalRoom>("lobby");
    rooms_["dining hall"]      = make_unique<NormalRoom>("dining hall");
    rooms_["guard room"]       = make_unique<NormalRoom>("guard room");
    rooms_["grand staircase"]  = make_unique<NormalRoom>("grand staircase");
    rooms_["library"]          = make_unique<NormalRoom>("library");
    rooms_["armory"]           = make_unique<NormalRoom>("armory");
    rooms_["tower base"]       = make_unique<NormalRoom>("tower base");
    rooms_["east wing"]        = make_unique<NormalRoom>("east wing");
    rooms_["west wing"]        = make_unique<NormalRoom>("west wing");
    rooms_["treasure room"]    = make_unique<TreasureRoom>("treasure room");
    rooms_["dark corridor"]    = make_unique<DarkCorridor>("dark corridor");

    // ground floor
    rooms_["lobby"]->addExit("east", "dining hall");
    rooms_["lobby"]->addExit("west", "guard room");
    rooms_["lobby"]->addExit("up", "grand staircase");
    rooms_["dining hall"]->addExit("west", "lobby");
    rooms_["guard room"]->addExit("east", "lobby");
    // upper floor
    rooms_["grand staircase"]->addExit("down", "lobby");
    rooms_["grand staircase"]->addExit("east", "library");
    rooms_["grand staircase"]->addExit("west", "armory");
    rooms_["grand staircase"]->addExit("up", "tower base");
    rooms_["library"]->addExit("west", "grand staircase");
    rooms_["armory"]->addExit("east", "grand staircase");
    // tower
    rooms_["tower base"]->addExit("down", "grand staircase");
    rooms_["tower base"]->addExit("east", "east wing");
    rooms_["tower base"]->addExit("west", "west wing");
    rooms_["east wing"]->addExit("west", "tower base");
    rooms_["east wing"]->addExit("up", "dark corridor");
    rooms_["west wing"]->addExit("east", "tower base");
    rooms_["dark corridor"]->addExit("down", "east wing");
    rooms_["dark corridor"]->addExit("north", "treasure room");
    rooms_["treasure room"]->addExit("south", "dark corridor");
  }

  // randomly put monster & princess in two of the candidate rooms
  void placeSpecialRooms() {
    static const vector<string> cands = {
      "dining hall", "guard room", "library", "armory",
      "east wing", "west wing", "tower base",
    };
    static mt19937 rng{random_device{}()};
    auto shuffled = cands;
    ranges::shuffle(shuffled, rng);
    replaceWith<MonsterRoom>(shuffled[0]);
    replaceWith<PrincessRoom>(shuffled[1]);
  }

  template <typename T>
  void replaceWith(const string& key) {
    auto& old = rooms_[key];
    auto p = make_unique<T>(old->name());
    for (auto& dir : old->exitNames()) {
      if (auto* t = old->findExit(dir))
        p->addExit(dir, *t);
    }
    old = std::move(p);
  }

  static auto trim(const string& s) -> string {
    auto b = s.find_first_not_of(" \t");
    if (b == string::npos) return "";
    return s.substr(b, s.find_last_not_of(" \t") - b + 1);
  }

  void gameLoop() {
    while (true) {
      auto it = rooms_.find(cur_);
      if (it == rooms_.end()) break;
      Room& room = *it->second;

      if (cur_ == "lobby" && hasPrincess_) {
        println("\nYou made it! The princess is safe.");
        println("  Princess: \"Thank you!\"");
        println("YOU WIN!");
        return;
      }

      if (!room.onEnter(hasPrincess_)) return;
      if (!hasPrincess_ && dynamic_cast<PrincessRoom*>(&room))
        hasPrincess_ = true;

      room.showInfo();

      string line;
      if (!getline(cin, line)) break;
      line = trim(line);
      if (line.empty()) continue;

      if (!line.starts_with("go ")) {
        println("Invalid command. Try: go <direction>");
        continue;
      }
      string dir = trim(line.substr(3));
      if (auto* t = room.findExit(dir)) {
        cur_ = *t;
      } else {
        println("No exit \"{}\".", dir);
      }
    }
  }
};

auto main() -> int {
  Game game;
  game.run();
  return 0;
}

// ========== Traditional OOP ==========
#else

class Room {
 public:
  explicit Room(const string& name) : name_(name) {}
  virtual ~Room() {}

  // return false to end the game
  virtual bool onEnter(bool has_princess) = 0;
  virtual void showInfo() const = 0;

  const string& name() const { return name_; }

  vector<string> exitNames() const {
    vector<string> v;
    for (auto it = exits_.begin(); it != exits_.end(); ++it)
      v.push_back(it->first);
    return v;
  }

  void addExit(const string& dir, const string& target) {
    exits_[dir] = target;
  }

  const string* findExit(const string& dir) const {
    auto it = exits_.find(dir);
    return it != exits_.end() ? &it->second : nullptr;
  }

 protected:
  static string fmtExits(const vector<string>& v) {
    if (v.empty()) return "none";
    if (v.size() == 1) return v[0];
    if (v.size() == 2) return v[0] + " and " + v[1];
    string s;
    for (size_t i = 0; i + 1 < v.size(); ++i) {
      s += v[i];
      s += (i + 2 < v.size()) ? ", " : " and ";
    }
    s += v.back();
    return s;
  }

  void printRoomInfo() const {
    vector<string> exits = exitNames();
    cout << "Welcome to the " << name_ << ". There are " << exits.size()
         << " exit" << (exits.size() == 1 ? "" : "s") << ": "
         << fmtExits(exits) << "." << endl;
    cout << "> " << flush;
  }

  string name_;
  map<string, string> exits_;
};

class NormalRoom : public Room {
 public:
  explicit NormalRoom(const string& name) : Room(name) {}
  bool onEnter(bool) override { return true; }
  void showInfo() const override { printRoomInfo(); }
};

class MonsterRoom : public Room {
 public:
  explicit MonsterRoom(const string& name) : Room(name) {}
  bool onEnter(bool) override {
    cout << "\nYou step into the " << name_ << "..." << endl;
    cout << "A monster jumps out! You're dead." << endl;
    cout << "GAME OVER" << endl;
    return false;
  }
  void showInfo() const override {}
};

class PrincessRoom : public Room {
 public:
  explicit PrincessRoom(const string& name) : Room(name) {}
  bool onEnter(bool has_princess) override {
    if (has_princess) {
      cout << "\nYou come back to the " << name_ << " with the princess." << endl;
      return true;
    }
    cout << "\nYou open the door of the " << name_ << "..." << endl;
    cout << "A princess is sitting inside. She looks relieved." << endl;
    cout << "  Princess: \"Please get me out of here!\"" << endl;
    cout << "  You: \"Let's go back to the lobby!\"" << endl;
    return true;
  }
  void showInfo() const override { printRoomInfo(); }
};

class TreasureRoom : public Room {
 public:
  explicit TreasureRoom(const string& name) : Room(name) {}
  bool onEnter(bool) override {
    cout << "\nYou find a dusty treasure chest in the " << name_ << "." << endl;
    cout << "Inside there's a note: \"Watch out — a monster is hiding" << endl;
    cout << "somewhere in this castle. Choose your path carefully.\"" << endl;
    return true;
  }
  void showInfo() const override { printRoomInfo(); }
};

class DarkCorridor : public Room {
 public:
  explicit DarkCorridor(const string& name) : Room(name) {}
  bool onEnter(bool) override {
    cout << "\nYou walk through the " << name_ << "..." << endl;
    cout << "It's dark. You can hear your footsteps echoing." << endl;
    return true;
  }
  void showInfo() const override { printRoomInfo(); }
};

class Game {
 public:
  Game() : cur_("lobby"), hasPrincess_(false) {}
  ~Game() {
    for (auto it = rooms_.begin(); it != rooms_.end(); ++it)
      delete it->second;
  }

  void run() {
    buildCastle();
    placeSpecialRooms();

    cout << "======= A D V E N T U R E =======" << endl;
    cout << "Find the princess and bring her back to the lobby." << endl;
    cout << "Beware of monsters!" << endl;
    cout << endl;

    gameLoop();
  }

 private:
  map<string, Room*> rooms_;
  string cur_;
  bool hasPrincess_;

  Game(const Game&);
  Game& operator=(const Game&);

  void buildCastle() {
    rooms_["lobby"]            = new NormalRoom("lobby");
    rooms_["dining hall"]      = new NormalRoom("dining hall");
    rooms_["guard room"]       = new NormalRoom("guard room");
    rooms_["grand staircase"]  = new NormalRoom("grand staircase");
    rooms_["library"]          = new NormalRoom("library");
    rooms_["armory"]           = new NormalRoom("armory");
    rooms_["tower base"]       = new NormalRoom("tower base");
    rooms_["east wing"]        = new NormalRoom("east wing");
    rooms_["west wing"]        = new NormalRoom("west wing");
    rooms_["treasure room"]    = new TreasureRoom("treasure room");
    rooms_["dark corridor"]    = new DarkCorridor("dark corridor");

    rooms_["lobby"]->addExit("east", "dining hall");
    rooms_["lobby"]->addExit("west", "guard room");
    rooms_["lobby"]->addExit("up", "grand staircase");
    rooms_["dining hall"]->addExit("west", "lobby");
    rooms_["guard room"]->addExit("east", "lobby");
    rooms_["grand staircase"]->addExit("down", "lobby");
    rooms_["grand staircase"]->addExit("east", "library");
    rooms_["grand staircase"]->addExit("west", "armory");
    rooms_["grand staircase"]->addExit("up", "tower base");
    rooms_["library"]->addExit("west", "grand staircase");
    rooms_["armory"]->addExit("east", "grand staircase");
    rooms_["tower base"]->addExit("down", "grand staircase");
    rooms_["tower base"]->addExit("east", "east wing");
    rooms_["tower base"]->addExit("west", "west wing");
    rooms_["east wing"]->addExit("west", "tower base");
    rooms_["east wing"]->addExit("up", "dark corridor");
    rooms_["west wing"]->addExit("east", "tower base");
    rooms_["dark corridor"]->addExit("down", "east wing");
    rooms_["dark corridor"]->addExit("north", "treasure room");
    rooms_["treasure room"]->addExit("south", "dark corridor");
  }

  void placeSpecialRooms() {
    vector<string> cands;
    cands.push_back("dining hall");
    cands.push_back("guard room");
    cands.push_back("library");
    cands.push_back("armory");
    cands.push_back("east wing");
    cands.push_back("west wing");
    cands.push_back("tower base");

    static mt19937 rng(random_device{}());
    shuffle(cands.begin(), cands.end(), rng);

    replaceRoom(cands[0], new MonsterRoom(cands[0]));
    replaceRoom(cands[1], new PrincessRoom(cands[1]));
  }

  void replaceRoom(const string& key, Room* nr) {
    Room* old = rooms_[key];
    vector<string> exits = old->exitNames();
    for (size_t i = 0; i < exits.size(); ++i) {
      const string* t = old->findExit(exits[i]);
      if (t) nr->addExit(exits[i], *t);
    }
    delete old;
    rooms_[key] = nr;
  }

  static string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == string::npos) return "";
    return s.substr(b, s.find_last_not_of(" \t") - b + 1);
  }

  void gameLoop() {
    while (true) {
      auto it = rooms_.find(cur_);
      if (it == rooms_.end()) break;
      Room& room = *it->second;

      if (cur_ == "lobby" && hasPrincess_) {
        cout << "\nYou made it! The princess is safe." << endl;
        cout << "  Princess: \"Thank you!\"" << endl;
        cout << "YOU WIN!" << endl;
        return;
      }

      if (!room.onEnter(hasPrincess_)) return;
      if (!hasPrincess_) {
        PrincessRoom* pr = dynamic_cast<PrincessRoom*>(&room);
        if (pr) hasPrincess_ = true;
      }

      room.showInfo();

      string line;
      if (!getline(cin, line)) break;
      line = trim(line);
      if (line.empty()) continue;

      if (line.find("go ") != 0) {
        cout << "Invalid command. Try: go <direction>" << endl;
        continue;
      }
      string dir = trim(line.substr(3));
      const string* t = room.findExit(dir);
      if (t) {
        cur_ = *t;
      } else {
        cout << "No exit \"" << dir << "\"." << endl;
      }
    }
  }
};

int main() {
  Game game;
  game.run();
  return 0;
}

#endif
