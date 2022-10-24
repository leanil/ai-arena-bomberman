#ifndef GAMEMAP_H_INCLUDED
#define GAMEMAP_H_INCLUDED

#include <unordered_map>
#include <vector>

#include "../common/GameState.h"
#include "../common/ScoreCalculator.h"
#include "Objective.h"
#include "PathFinder.h"

class AI {
 public:
  struct ThrowOption {
    Pos target_pos;
    Throw throw_step;
    int min_tick, max_range;
    std::vector<Grenade> grenades;
    ThrowOption() = default;
    ThrowOption(std::vector<Grenade> grenades);
  };

  InitialData initial_data;
  GameState state;
  Grid grid;
  PathFinder path_finder;
  ScoreCalculator score_calculator;
  Vampire self;
  std::vector<Objective*> objectives;
  std::vector<Objective*> objectives2;
  int protection = 0;
  bool offensive_mode;
  std::unordered_map<int, PathFinder> opponent_path_finders;
  std::unordered_set<Pos, Pos::hash> throwable_grenades;
  std::unordered_map<Pos, Objective*, Pos::hash> grenade_owner_objective;
  int protect_steps;
  Pos prev_pos;

  AI();
  void set_state(GameState&& game_state);
  Step get_step();
  std::vector<Pos> grenade_positions_for(Pos target) const;
  std::vector<Pos> setup_positions_for(Pos target) const;
  std::vector<ThrowOption> throw_options_from(Pos pos) const;
  int ticks_to_wait_until_grenade();  // this is naive now, does not consider chain reactions
 private:
  int prev_health = -1;
  std::pair<Objective::EvalResult, Objective*> evaluate_objectives(bool second);
  std::vector<Pos> positions_for(Pos target, int range, bool break_on_obstacle) const;
};

#endif  // GAMEMAP_H_INCLUDED
