#ifndef ITECH21_GAMESTATE_H
#define ITECH21_GAMESTATE_H

#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "positions.h"

enum class PowerupType { TOMATO, GRENADE, BATTERY, SHOE };

const int GRENADE_TICKS = 5;

struct InitialData {
  std::string message;
  int level{};
  int game_id{};
  bool test{};
  int max_tick{};
  int grenade_radius{};
  int size{};
  explicit InitialData(const std::vector<std::string>& lines);
  InitialData() = default;
};
std::ostream& operator<<(std::ostream& out, const InitialData& initial_data);

struct Vampire {
  Pos pos;
  int id, health = 0, grenades, range, shoes, invulnerable = 0;
};
std::ostream& operator<<(std::ostream& out, const Vampire& vampire);

struct Grenade {
  Pos pos;
  int vampire_id, tick, range;
};
std::ostream& operator<<(std::ostream& out, const Grenade& grenade);

struct Powerup {
  PowerupType type;
  Pos pos;
  int ticks, protect;
};
std::ostream& operator<<(std::ostream& out, const Powerup& powerup);

struct Bat {
  Pos pos;
  int density;
};

struct GameState {
  int game_id{};
  int tick{};
  int vampire_id{};
  std::vector<std::string> warnings;
  std::vector<Vampire> vampires;
  std::vector<Grenade> grenades;
  std::vector<Powerup> powerups;
  std::vector<Bat> bats;
  bool end = false;
  explicit GameState(const std::vector<std::string>& lines);
  GameState() = default;
};
std::ostream& operator<<(std::ostream& out, const GameState& game_state);

struct Throw {
  bool from_place = false;
  Direction dir;
  int length;

  std::string to_string() const;
  static std::optional<Throw> between(const Pos& from, const Pos& to);
};

struct Step {
  bool place_grenade = false;
  std::optional<Throw> throw_grenades;
  // nullopt: another Objective can be started in the same Step
  // empty vector: we want to stay in place
  std::optional<std::vector<Direction>> move;

  std::string to_string() const;
};

#endif  // ITECH21_GAMESTATE_H
