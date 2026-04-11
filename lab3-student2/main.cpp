#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <print>
#include <random>
#include <string>
#include <vector>

using namespace std;

// ─── Room base class ─────────────────────────────────────────────────────────

/// \brief Abstract base class representing a room in the castle.
/// \details
/// Each room has a name and a set of named exits leading to other rooms.
/// Derived classes override `onEnter` to define what happens when the
/// player enters the room.
class Room {
 public:
  explicit Room(string name) : name_(move(name)) {}

  virtual ~Room() = default;

  /// \brief Called when the player enters this room.
  /// \return false if the game should end (monster/princess), true otherwise
  virtual auto onEnter(bool has_princess) -> bool = 0;

  /// \brief Displays the room information and prompt.
  virtual auto showInfo() const -> void = 0;

  auto name() const -> const string& { return name_; }
  auto exitCount() const -> size_t { return exits_.size(); }

  auto exitNames() const -> vector<string> {
    vector<string> names;
    for (const auto& [dir, _] : exits_) {
      names.push_back(dir);
    }
    return names;
  }

  void addExit(string direction, string target_name) {
    exits_[move(direction)] = move(target_name);
  }

  auto findExit(const string& direction) const -> const string* {
    auto it = exits_.find(direction);
    return it != exits_.end() ? &it->second : nullptr;
  }

 protected:
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

 private:
  static auto formatExits(const vector<string>& names) -> string {
    if (names.empty()) return "none";
    if (names.size() == 1) return names[0];
    string result;
    for (size_t i = 0; i < names.size() - 1; ++i) {
      result += names[i];
      result += (i < names.size() - 2) ? ", " : " and ";
    }
    result += names.back();
    return result;
  }
};

// ─── MonsterRoom ─────────────────────────────────────────────────────────────

/// \brief A room containing a deadly monster.
/// \details When the player enters, the game ends with a defeat message.
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
/// \details Triggers the rescue dialogue. The player must then navigate
/// back to the lobby to escape the castle.
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

 private:
  static auto formatExits(const vector<string>& names) -> string {
    if (names.empty()) return "none";
    if (names.size() == 1) return names[0];
    string result;
    for (size_t i = 0; i < names.size() - 1; ++i) {
      result += names[i];
      result += (i < names.size() - 2) ? ", " : " and ";
    }
    result += names.back();
    return result;
  }
};

// ─── TreasureRoom ────────────────────────────────────────────────────────────

/// \brief A room containing a mysterious treasure chest.
/// \details The player finds a clue hinting at the princess's location.
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

 private:
  static auto formatExits(const vector<string>& names) -> string {
    if (names.empty()) return "none";
    if (names.size() == 1) return names[0];
    string result;
    for (size_t i = 0; i < names.size() - 1; ++i) {
      result += names[i];
      result += (i < names.size() - 2) ? ", " : " and ";
    }
    result += names.back();
    return result;
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

 private:
  static auto formatExits(const vector<string>& names) -> string {
    if (names.empty()) return "none";
    if (names.size() == 1) return names[0];
    string result;
    for (size_t i = 0; i < names.size() - 1; ++i) {
      result += names[i];
      result += (i < names.size() - 2) ? ", " : " and ";
    }
    result += names.back();
    return result;
  }
};

// ─── Game class ──────────────────────────────────────────────────────────────

/// \brief Orchestrates the castle exploration game.
/// \details
/// Builds the room graph, randomly places the monster and princess,
/// and runs the main game loop.
class Game {
 public:
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

auto main() -> int {
  Game game;
  game.run();
  return 0;
}
