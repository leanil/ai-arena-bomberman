#ifndef ITECH21_GRID_H
#define ITECH21_GRID_H

#include <map>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GameState.h"
#include "RandomGenerator.h"

class Grid;

enum ScoreType { POWERUP, BAT, ATTACK, MINUS };

class Field {
 public:
  bool is_bush;
  bool has_light;  // if light reaches this field in this tick
  std::unordered_set<int> illuminated_by_vampire;
  std::vector<Grenade> grenades;
  std::vector<Vampire> vampires;
  std::optional<Bat> bat;
  std::optional<Powerup> powerup;

  bool can_step_here() const;
  bool illuminate(std::queue<Grenade>& explodes_queue,
                  int started_by_vampire);  // returns false if light is stopped here
  void reset_light();
  void evaluate_light(Grid& grid);
  void step_powerup(Grid& grid);

  void print_vampire(std::ostream& os) const;
  void print_grenade(std::ostream& os, int rowIdx) const;
  void print_powerup(std::ostream& os, int colIdx) const;
};

class Grid {
 public:
  RandomGenerator random_generator;
  int tick = 0, max_tick = 100, max_throw_length = 2;
  std::vector<std::vector<Field>> fields;
  // This is ugly but we need to know the state before step...
  std::unordered_map<int, int> grenades_before_step, shoes_before_step;
  // For how many ticks the vampires have been standing on the powerup
  std::unordered_map<int, int> powerup_protection;
  // The first index is ScoreType, and the inner maps are indexed by vampire id
  std::vector<std::unordered_map<int, double>> scores;
  bool server;

  Grid(int seed = 0, bool server = false);
  Grid(const GameState& state, const InitialData& init_data);

  Field& field_at(const Pos& pos);
  const Field& field_at(const Pos& pos) const;
  Field& operator[](const Pos& pos);
  const Field& operator[](const Pos& pos) const;

  void init(const GameState& state, const InitialData& init_data);
  const GameState& get_state() const;
  void place_possible_grenades(int self_id);
  std::vector<Pos> from_where_can_throw_to(Pos target);

  void step(const std::map<int, Step>& vampire_steps = {});

  void print(std::ostream& os) const;

  std::optional<Vampire> get_vampire(int id) const;
  std::vector<Vampire> get_enemies(int self_id) const;

  void step_powerups();
  void step_grenades();
  void step_vampires(std::map<int, Step> vampire_steps);  // We make a copy of the map here
  void handle_throw(const Throw& thro, const Vampire& vampire);
  void switch_lights_at_end();

 private:
  mutable std::optional<GameState> state_cache;
};

#endif  // ITECH21_GRID_H
