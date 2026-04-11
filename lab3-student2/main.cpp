// ═══════════════════════════════════════════════════════════════════════════════
// Adventure — A CLI Castle Exploration Game
// Lab 3, Object-Oriented Programming, ZJU 2026 Spring
//
// Compile: clang++ -std=c++23 -DUSE_MODERN_CPP -Wall -Wextra -o adventure main.cpp
//   or:    clang++ -std=c++23 -Wall -Wextra -o adventure main.cpp
// Run:     ./adventure < test_input.txt
// ═══════════════════════════════════════════════════════════════════════════════

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

// ─── Modern C++ section ──────────────────────────────────────────────────────

#if __cplusplus >= 202302L && defined(USE_MODERN_CPP)

// ─── Room base class ─────────────────────────────────────────────────────────

/// \brief Abstract base class representing a room in the castle.
/// \details
/// Each room has a name and a set of named exits leading to other rooms.
/// Derived classes override `onEnter` to define what happens when the
/// player enters the room, and `showInfo` to display the prompt.
class Room {
 public:
  explicit Room(string name) : name_(std::move(name)) {}

  virtual ~Room() = default;

  /// \brief Called when the player enters this room.
  /// \param has_princess Whether the player has already rescued the princess.
  /// \return false if the game should end, true otherwise.
  virtual auto onEnter(bool has_princess) -> bool = 0;

  /// \brief Displays room info and the command prompt.
  virtual auto showInfo() const -> void = 0;

  auto name() const -> const string& { return name_; }
  auto exitCount() const -> size_t { return exits_.size(); }

  auto exitNames() const -> vector<string> {
    return exits_ | views::keys | ranges::to<vector>();
  }

  void addExit(string direction, string target_name) {
    exits_[std::move(direction)] = std::move(target_name);
  }

  auto findExit(const string& direction) const -> const string* {
    auto it = exits_.find(direction);
    return it != exits_.end() ? &it->second : nullptr;
  }

 protected:
  /// \brief Formats a list of exit names into a human-readable string.
  /// \param names Vector of exit direction names.
  /// \return Formatted string, e.g. "east, west and up".
  static auto formatExits(const vector<string>& names) -> string {
    if (names.empty()) return "none";
    if (names.size() == 1) return names[0];
    if (names.size() == 2) return names[0] + " and " + names[1];
    string result;
    for (size_t i = 0; i < names.size() - 1; ++i) {
      result += names[i];
      result += (i < names.size() - 2) ? ", " : " and ";
    }
    result += names.back();
    return result;
  }

  string name_;
  map<string, string> exits_;  // direction -> target room name
};

// ─── NormalRoom ──────────────────────────────────────────────────────────────

/// \brief A regular room with no special events.
class NormalRoom : public Room {
 public:
  using Room::Room;

  auto onEnter(bool /*has_princess*/) -> bool override { return true; }

  auto showInfo() const -> void override {
    const auto exits = exitNames();
    println("Welcome to the {}. There are {} exit{}: {}.",
            name_, exits.size(), exits.size() == 1 ? "" : "s",
            formatExits(exits));
    print("Enter your command: ");
    cout.flush();
  }
};

// ─── MonsterRoom ─────────────────────────────────────────────────────────────

/// \brief A room containing a deadly monster.
/// \details Entering this room ends the game with a defeat message.
class MonsterRoom : public Room {
 public:
  using Room::Room;

  auto onEnter(bool /*has_princess*/) -> bool override {
    println("\nYou cautiously step into the {}...", name_);
    println("A terrifying monster lunges at you from the shadows!");
    println("GAME OVER — You have been devoured by the monster.");
    return false;
  }

  auto showInfo() const -> void override {}
};

// ─── PrincessRoom ────────────────────────────────────────────────────────────

/// \brief The secret room where the princess is imprisoned.
/// \details
/// First visit triggers the rescue dialogue. Subsequent visits just
/// acknowledge the return. The player must then navigate back to
/// the lobby to win.
class PrincessRoom : public Room {
 public:
  using Room::Room;

  auto onEnter(bool has_princess) -> bool override {
    if (has_princess) {
      println("\nYou return to the {} with the princess.", name_);
      return true;
    }
    println("\nYou push open the heavy door of the {}...", name_);
    println("Inside, a princess sits quietly by the window.");
    println(R"(
  Princess: "Oh, thank goodness you've come! I've been trapped
  here for days. Please, take me out of this dreadful castle!"

  You: "Of course, Your Highness. Let's go — we need to
  find our way back to the lobby to leave the castle.")");
    return true;
  }

  auto showInfo() const -> void override {
    const auto exits = exitNames();
    println("Welcome to the {}. There are {} exit{}: {}.",
            name_, exits.size(), exits.size() == 1 ? "" : "s",
            formatExits(exits));
    print("Enter your command: ");
    cout.flush();
  }
};

// ─── TreasureRoom ────────────────────────────────────────────────────────────

/// \brief A room containing a mysterious treasure chest with a clue.
class TreasureRoom : public Room {
 public:
  using Room::Room;

  auto onEnter(bool /*has_princess*/) -> bool override {
    println("\nYou enter the {}...", name_);
    println("A dusty treasure chest sits in the corner. Inside you find a");
    println("crumpled note:");
    println(R"(
  "The princess is held in a secret chamber. Beware — a monster
  guards one of these rooms. Choose your path wisely.")");
    return true;
  }

  auto showInfo() const -> void override {
    const auto exits = exitNames();
    println("Welcome to the {}. There are {} exit{}: {}.",
            name_, exits.size(), exits.size() == 1 ? "" : "s",
            formatExits(exits));
    print("Enter your command: ");
    cout.flush();
  }
};

// ─── DarkCorridor ────────────────────────────────────────────────────────────

/// \brief A dark, eerie corridor connecting deeper parts of the castle.
/// \details Purely atmospheric — no special game logic.
class DarkCorridor : public Room {
 public:
  using Room::Room;

  auto onEnter(bool /*has_princess*/) -> bool override {
    println("\nYou walk through the {}...", name_);
    println("The corridor is dimly lit. Your footsteps echo against the");
    println("cold stone walls.");
    return true;
  }

  auto showInfo() const -> void override {
    const auto exits = exitNames();
    println("Welcome to the {}. There are {} exit{}: {}.",
            name_, exits.size(), exits.size() == 1 ? "" : "s",
            formatExits(exits));
    print("Enter your command: ");
    cout.flush();
  }
};

// ─── Game class ──────────────────────────────────────────────────────────────

/// \brief Orchestrates the castle exploration game.
/// \details
/// Builds the room graph, randomly places the monster and princess,
/// and runs the main game loop.
class Game {
 public:
  /// \brief Builds the castle and starts the game.
  void run() {
    buildCastle();
    placeSpecialRooms();

    println("══════════════════════════════════════════════════");
    println("         A D V E N T U R E");
    println("══════════════════════════════════════════════════");
    println();
    println("You stand before an ancient castle. Your mission: find");
    println("the princess and bring her safely to the lobby.");
    println("But beware — a monster lurks in one of the rooms...");
    println();

    gameLoop();
  }

 private:
  map<string, unique_ptr<Room>> rooms_;
  string current_ = "lobby";
  bool has_princess_ = false;

  /// \brief Constructs the castle with all rooms and their connections.
  void buildCastle() {
    // Ground floor
    rooms_["lobby"]       = make_unique<NormalRoom>("lobby");
    rooms_["dining hall"] = make_unique<NormalRoom>("dining hall");
    rooms_["guard room"]  = make_unique<NormalRoom>("guard room");

    // Upper floor
    rooms_["grand staircase"] = make_unique<NormalRoom>("grand staircase");
    rooms_["library"]        = make_unique<NormalRoom>("library");
    rooms_["armory"]         = make_unique<NormalRoom>("armory");

    // Tower (deeper area — monster and princess hide here)
    rooms_["tower base"]    = make_unique<NormalRoom>("tower base");
    rooms_["east wing"]     = make_unique<NormalRoom>("east wing");
    rooms_["west wing"]     = make_unique<NormalRoom>("west wing");
    rooms_["treasure room"] = make_unique<TreasureRoom>("treasure room");
    rooms_["dark corridor"] = make_unique<DarkCorridor>("dark corridor");

    // Ground floor connections
    rooms_["lobby"]->addExit("east", "dining hall");
    rooms_["lobby"]->addExit("west", "guard room");
    rooms_["lobby"]->addExit("up", "grand staircase");
    rooms_["dining hall"]->addExit("west", "lobby");
    rooms_["guard room"]->addExit("east", "lobby");

    // Upper floor connections
    rooms_["grand staircase"]->addExit("down", "lobby");
    rooms_["grand staircase"]->addExit("east", "library");
    rooms_["grand staircase"]->addExit("west", "armory");
    rooms_["grand staircase"]->addExit("up", "tower base");
    rooms_["library"]->addExit("west", "grand staircase");
    rooms_["armory"]->addExit("east", "grand staircase");

    // Tower connections
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

  /// \brief Randomly replaces two normal rooms with MonsterRoom and PrincessRoom.
  /// \details
  /// Candidate rooms exclude the lobby, treasure room, and dark corridor
  /// to ensure balanced gameplay.
  void placeSpecialRooms() {
    static const vector<string> candidates = {
      "dining hall", "guard room", "library", "armory",
      "east wing", "west wing", "tower base",
    };

    static mt19937 rng{random_device{}()};
    auto shuffled = candidates;
    ranges::shuffle(shuffled, rng);

    replaceWith<MonsterRoom>(shuffled[0]);
    replaceWith<PrincessRoom>(shuffled[1]);
  }

  /// \brief Replaces the room at `key` with a new derived Room, preserving exits.
  template <typename DerivedRoom>
  void replaceWith(const string& key) {
    auto& old = rooms_[key];
    auto new_room = make_unique<DerivedRoom>(old->name());
    for (const auto& dir : old->exitNames()) {
      if (auto* target = old->findExit(dir)) {
        new_room->addExit(dir, *target);
      }
    }
    old = std::move(new_room);
  }

  /// \brief Main game loop: enter room, read command, move player.
  void gameLoop() {
    while (true) {
      auto it = rooms_.find(current_);
      if (it == rooms_.end()) break;
      Room& room = *it->second;

      // Win condition: return to lobby with the princess
      if (current_ == "lobby" && has_princess_) {
        println();
        println("You burst through the lobby doors into the sunlight!");
        println("The princess smiles warmly at you.");
        println(R"(
  Princess: "Thank you, brave hero! You have saved me!")");
        println();
        println("══════════════════════════════════════════════════");
        println("  C O N G R A T U L A T I O N S — YOU WIN!");
        println("══════════════════════════════════════════════════");
        return;
      }

      if (!room.onEnter(has_princess_)) return;

      // Track princess rescue
      if (!has_princess_ && dynamic_cast<PrincessRoom*>(&room)) {
        has_princess_ = true;
      }

      room.showInfo();

      string line;
      if (!getline(cin, line)) break;
      line.erase(0, line.find_first_not_of(" \t"));
      line.erase(line.find_last_not_of(" \t") + 1);
      if (line.empty()) continue;

      if (!line.starts_with("go ")) {
        println("Unknown command. Usage: go <direction>");
        continue;
      }

      string direction = line.substr(3);
      direction.erase(0, direction.find_first_not_of(" \t"));

      if (auto* target = room.findExit(direction)) {
        current_ = *target;
      } else {
        println("There is no exit \"{}\". Try another direction.", direction);
      }
    }
  }
};

// ─── Entry point ─────────────────────────────────────────────────────────────

auto main() -> int {
  Game game;
  game.run();
  return 0;
}

#else  // ─── Traditional OOP section ──────────────────────────────────────────

// ─── Room base class ─────────────────────────────────────────────────────────

/// \brief Abstract base class representing a room in the castle.
/// \details
/// Each room has a name and a set of named exits leading to other rooms.
/// Derived classes override `onEnter` to define what happens when the
/// player enters the room, and `showInfo` to display the prompt.
class Room {
 public:
  /// \brief Constructs a room with a given name.
  /// \param name The display name of the room.
  explicit Room(const string& name) : name_(name) {}

  virtual ~Room() {}

  /// \brief Called when the player enters this room.
  /// \param has_princess Whether the player has already rescued the princess.
  /// \return false if the game should end, true otherwise.
  virtual bool onEnter(bool has_princess) = 0;

  /// \brief Displays room info and the command prompt.
  virtual void showInfo() const = 0;

  /// \brief Gets the room's display name.
  const string& name() const { return name_; }

  /// \brief Gets the number of exits from this room.
  size_t exitCount() const { return exits_.size(); }

  /// \brief Gets the list of exit direction names.
  vector<string> exitNames() const {
    vector<string> names;
    for (map<string, string>::const_iterator it = exits_.begin();
         it != exits_.end(); ++it) {
      names.push_back(it->first);
    }
    return names;
  }

  /// \brief Adds an exit leading to another room by name.
  /// \param direction The direction name (e.g. "east").
  /// \param target_name The name of the destination room.
  void addExit(const string& direction, const string& target_name) {
    exits_[direction] = target_name;
  }

  /// \brief Looks up the target room name for a given direction.
  /// \param direction The direction to look up.
  /// \return Pointer to the direction string if found, nullptr otherwise.
  const string* findExit(const string& direction) const {
    map<string, string>::const_iterator it = exits_.find(direction);
    return it != exits_.end() ? &it->second : nullptr;
  }

 protected:
  /// \brief Formats a list of exit names into a human-readable string.
  /// \param names Vector of exit direction names.
  /// \return Formatted string, e.g. "east, west and up".
  static string formatExits(const vector<string>& names) {
    if (names.empty()) return "none";
    if (names.size() == 1) return names[0];
    if (names.size() == 2) return names[0] + " and " + names[1];
    string result;
    for (size_t i = 0; i < names.size() - 1; ++i) {
      result += names[i];
      if (i < names.size() - 2) result += ", ";
    }
    result += " and " + names.back();
    return result;
  }

  string name_;
  map<string, string> exits_;  // direction -> target room name
};

// ─── NormalRoom ──────────────────────────────────────────────────────────────

/// \brief A regular room with no special events.
class NormalRoom : public Room {
 public:
  /// \brief Constructs a normal room with a given name.
  explicit NormalRoom(const string& name) : Room(name) {}

  bool onEnter(bool /*has_princess*/) override { return true; }

  void showInfo() const override {
    vector<string> exits = exitNames();
    cout << "Welcome to the " << name_ << ". There are " << exits.size()
         << " exit" << (exits.size() == 1 ? "" : "s") << ": "
         << formatExits(exits) << "." << endl;
    cout << "Enter your command: " << flush;
  }
};

// ─── MonsterRoom ─────────────────────────────────────────────────────────────

/// \brief A room containing a deadly monster.
/// \details Entering this room ends the game with a defeat message.
class MonsterRoom : public Room {
 public:
  explicit MonsterRoom(const string& name) : Room(name) {}

  bool onEnter(bool /*has_princess*/) override {
    cout << "\nYou cautiously step into the " << name_ << "..." << endl;
    cout << "A terrifying monster lunges at you from the shadows!" << endl;
    cout << "GAME OVER — You have been devoured by the monster." << endl;
    return false;
  }

  void showInfo() const override {}
};

// ─── PrincessRoom ────────────────────────────────────────────────────────────

/// \brief The secret room where the princess is imprisoned.
/// \details
/// First visit triggers the rescue dialogue. Subsequent visits just
/// acknowledge the return. The player must then navigate back to
/// the lobby to win.
class PrincessRoom : public Room {
 public:
  explicit PrincessRoom(const string& name) : Room(name) {}

  bool onEnter(bool has_princess) override {
    if (has_princess) {
      cout << "\nYou return to the " << name_ << " with the princess." << endl;
      return true;
    }
    cout << "\nYou push open the heavy door of the " << name_ << "..." << endl;
    cout << "Inside, a princess sits quietly by the window." << endl;
    cout << endl;
    cout << "  Princess: \"Oh, thank goodness you've come! I've been trapped" << endl;
    cout << "  here for days. Please, take me out of this dreadful castle!\"" << endl;
    cout << endl;
    cout << "  You: \"Of course, Your Highness. Let's go — we need to" << endl;
    cout << "  find our way back to the lobby to leave the castle.\"" << endl;
    return true;
  }

  void showInfo() const override {
    vector<string> exits = exitNames();
    cout << "Welcome to the " << name_ << ". There are " << exits.size()
         << " exit" << (exits.size() == 1 ? "" : "s") << ": "
         << formatExits(exits) << "." << endl;
    cout << "Enter your command: " << flush;
  }
};

// ─── TreasureRoom ────────────────────────────────────────────────────────────

/// \brief A room containing a mysterious treasure chest with a clue.
class TreasureRoom : public Room {
 public:
  explicit TreasureRoom(const string& name) : Room(name) {}

  bool onEnter(bool /*has_princess*/) override {
    cout << "\nYou enter the " << name_ << "..." << endl;
    cout << "A dusty treasure chest sits in the corner. Inside you find a" << endl;
    cout << "crumpled note:" << endl;
    cout << endl;
    cout << "  \"The princess is held in a secret chamber. Beware — a monster" << endl;
    cout << "  guards one of these rooms. Choose your path wisely.\"" << endl;
    return true;
  }

  void showInfo() const override {
    vector<string> exits = exitNames();
    cout << "Welcome to the " << name_ << ". There are " << exits.size()
         << " exit" << (exits.size() == 1 ? "" : "s") << ": "
         << formatExits(exits) << "." << endl;
    cout << "Enter your command: " << flush;
  }
};

// ─── DarkCorridor ────────────────────────────────────────────────────────────

/// \brief A dark, eerie corridor connecting deeper parts of the castle.
/// \details Purely atmospheric — no special game logic.
class DarkCorridor : public Room {
 public:
  explicit DarkCorridor(const string& name) : Room(name) {}

  bool onEnter(bool /*has_princess*/) override {
    cout << "\nYou walk through the " << name_ << "..." << endl;
    cout << "The corridor is dimly lit. Your footsteps echo against the" << endl;
    cout << "cold stone walls." << endl;
    return true;
  }

  void showInfo() const override {
    vector<string> exits = exitNames();
    cout << "Welcome to the " << name_ << ". There are " << exits.size()
         << " exit" << (exits.size() == 1 ? "" : "s") << ": "
         << formatExits(exits) << "." << endl;
    cout << "Enter your command: " << flush;
  }
};

// ─── Game class ──────────────────────────────────────────────────────────────

/// \brief Orchestrates the castle exploration game.
/// \details
/// Builds the room graph, randomly places the monster and princess,
/// and runs the main game loop.
class Game {
 public:
  /// \brief Builds the castle and starts the game.
  void run() {
    buildCastle();
    placeSpecialRooms();

    cout << "══════════════════════════════════════════════════" << endl;
    cout << "         A D V E N T U R E" << endl;
    cout << "══════════════════════════════════════════════════" << endl;
    cout << endl;
    cout << "You stand before an ancient castle. Your mission: find" << endl;
    cout << "the princess and bring her safely to the lobby." << endl;
    cout << "But beware — a monster lurks in one of the rooms..." << endl;
    cout << endl;

    gameLoop();
  }

  Game() : current_("lobby"), has_princess_(false) {}

  ~Game() {
    for (map<string, Room*>::iterator it = rooms_.begin();
         it != rooms_.end(); ++it) {
      delete it->second;
    }
  }

 private:
  map<string, Room*> rooms_;
  string current_;
  bool has_princess_;

  // Non-copyable
  Game(const Game&);
  Game& operator=(const Game&);

  /// \brief Constructs the castle with all rooms and their connections.
  void buildCastle() {
    // Ground floor
    rooms_["lobby"]       = new NormalRoom("lobby");
    rooms_["dining hall"] = new NormalRoom("dining hall");
    rooms_["guard room"]  = new NormalRoom("guard room");

    // Upper floor
    rooms_["grand staircase"] = new NormalRoom("grand staircase");
    rooms_["library"]        = new NormalRoom("library");
    rooms_["armory"]         = new NormalRoom("armory");

    // Tower (deeper area — monster and princess hide here)
    rooms_["tower base"]    = new NormalRoom("tower base");
    rooms_["east wing"]     = new NormalRoom("east wing");
    rooms_["west wing"]     = new NormalRoom("west wing");
    rooms_["treasure room"] = new TreasureRoom("treasure room");
    rooms_["dark corridor"] = new DarkCorridor("dark corridor");

    // Ground floor connections
    rooms_["lobby"]->addExit("east", "dining hall");
    rooms_["lobby"]->addExit("west", "guard room");
    rooms_["lobby"]->addExit("up", "grand staircase");
    rooms_["dining hall"]->addExit("west", "lobby");
    rooms_["guard room"]->addExit("east", "lobby");

    // Upper floor connections
    rooms_["grand staircase"]->addExit("down", "lobby");
    rooms_["grand staircase"]->addExit("east", "library");
    rooms_["grand staircase"]->addExit("west", "armory");
    rooms_["grand staircase"]->addExit("up", "tower base");
    rooms_["library"]->addExit("west", "grand staircase");
    rooms_["armory"]->addExit("east", "grand staircase");

    // Tower connections
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

  /// \brief Randomly replaces two normal rooms with MonsterRoom and PrincessRoom.
  void placeSpecialRooms() {
    vector<string> candidates;
    candidates.push_back("dining hall");
    candidates.push_back("guard room");
    candidates.push_back("library");
    candidates.push_back("armory");
    candidates.push_back("east wing");
    candidates.push_back("west wing");
    candidates.push_back("tower base");

    static mt19937 rng(random_device{}());
    shuffle(candidates.begin(), candidates.end(), rng);

    replaceRoom(candidates[0], new MonsterRoom(candidates[0]));
    replaceRoom(candidates[1], new PrincessRoom(candidates[1]));
  }

  /// \brief Replaces a room, preserving its exit connections.
  /// \param key The room name to replace.
  /// \param new_room The new Room object (takes ownership).
  void replaceRoom(const string& key, Room* new_room) {
    Room* old = rooms_[key];
    vector<string> exits = old->exitNames();
    for (size_t i = 0; i < exits.size(); ++i) {
      const string* target = old->findExit(exits[i]);
      if (target != nullptr) {
        new_room->addExit(exits[i], *target);
      }
    }
    delete old;
    rooms_[key] = new_room;
  }

  /// \brief Trims leading and trailing whitespace from a string.
  static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
  }

  /// \brief Main game loop: enter room, read command, move player.
  void gameLoop() {
    while (true) {
      map<string, Room*>::iterator it = rooms_.find(current_);
      if (it == rooms_.end()) break;
      Room& room = *it->second;

      // Win condition: return to lobby with the princess
      if (current_ == "lobby" && has_princess_) {
        cout << endl;
        cout << "You burst through the lobby doors into the sunlight!" << endl;
        cout << "The princess smiles warmly at you." << endl;
        cout << endl;
        cout << "  Princess: \"Thank you, brave hero! You have saved me!\"" << endl;
        cout << endl;
        cout << "══════════════════════════════════════════════════" << endl;
        cout << "  C O N G R A T U L A T I O N S — YOU WIN!" << endl;
        cout << "══════════════════════════════════════════════════" << endl;
        return;
      }

      if (!room.onEnter(has_princess_)) return;

      // Track princess rescue via dynamic_cast
      if (!has_princess_) {
        PrincessRoom* pr = dynamic_cast<PrincessRoom*>(&room);
        if (pr != nullptr) {
          has_princess_ = true;
        }
      }

      room.showInfo();

      string line;
      if (!getline(cin, line)) break;
      line = trim(line);
      if (line.empty()) continue;

      if (line.find("go ") != 0) {
        cout << "Unknown command. Usage: go <direction>" << endl;
        continue;
      }

      string direction = trim(line.substr(3));

      const string* target = room.findExit(direction);
      if (target != nullptr) {
        current_ = *target;
      } else {
        cout << "There is no exit \"" << direction
             << "\". Try another direction." << endl;
      }
    }
  }
};

// ─── Entry point ─────────────────────────────────────────────────────────────

int main() {
  Game game;
  game.run();
  return 0;
}

#endif
