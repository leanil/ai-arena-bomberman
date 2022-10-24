#include "AI.h"

#include <algorithm>
// #include <chrono>
#include <utility>

#include "../common/utility.h"
#include "Backtrack.h"

using namespace std;

AI::ThrowOption::ThrowOption(std::vector<Grenade> grenades) : min_tick{6}, max_range{0}, grenades{std::move(grenades)} {
  for (const Grenade& grenade : grenades) {
    min_tick = min(min_tick, grenade.tick);
    max_range = max(max_range, grenade.range);
  }
}

AI::AI() {
  auto* batObjective = new BatObjective();
  auto* powerupObjective = new PowerupObjective();
  auto* positioningObjective = new PositioningObjective();

  objectives = {batObjective,
                powerupObjective,
                positioningObjective,
                new AttackObjective(),
                new AttackObjective2(),
                new AttackPowerupObjective(),
                new ChainAttackObjective()};
  objectives2 = {batObjective, powerupObjective, positioningObjective};
}

void AI::set_state(GameState&& game_state) {
  state = move(game_state);
  score_calculator.update(state, initial_data);
  if (protection) --protection;
  if (self.health < prev_health) {
    protection = 3;
    cerr << "WE LOST A LIFE!!!" << endl;
  }
  prev_health = self.health;
  int opponent_max_health = 0;
  for (const Vampire& vampire : state.vampires) {
    if (vampire.id != self.id) {
      opponent_max_health = max(opponent_max_health, vampire.health);
    }
  }
  offensive_mode = self.health > 1 || (state.vampires.size() == 2 && opponent_max_health <= 2) ||
                   state.tick >= initial_data.max_tick - 100;

  for (const Vampire& vampire : state.vampires) {
    if (vampire.id == state.vampire_id) {
      self = vampire;
    }
  }
  grid.init(state, initial_data);

  if (self.pos == prev_pos && grid[self.pos].powerup.has_value() && grid[self.pos].powerup.value().ticks > -1) {
    ++protect_steps;
  } else if (grid[self.pos].powerup.has_value() && grid[self.pos].powerup.value().ticks >= -1) {
    protect_steps = 1;
  } else {
    protect_steps = 0;
  }

  grid.step();
  path_finder.init(grid, self);
  path_finder.init_step_safety_checker(state);

  // std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  // Backtrack bt;
  // path_finder.init_bt_result(bt.findUnsafeMoves(*this, grid, 2, false, -1).second);

  //   std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  //   std::cerr << "Backtrack findUnsafeMoves time = "
  //             << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;

  opponent_path_finders.clear();
  for (const Vampire& vampire : state.vampires) {
    if (vampire.id != self.id) {
      opponent_path_finders[vampire.id].init(grid, vampire);
      // TODO decide if we want to use safety checker for the opponents
    }
  }

  for (auto it = throwable_grenades.begin(); it != throwable_grenades.end();) {
    const auto& grenades = grid[*it].grenades;
    if (find_if(grenades.begin(), grenades.end(),
                [&](const Grenade& grenade) { return grenade.vampire_id == self.id; }) == grenades.end()) {
      it = throwable_grenades.erase(it);
    } else {
      ++it;
    }
  }
  prev_pos = self.pos;
}

Step AI::get_step() {
  auto result = evaluate_objectives(false);
  Objective::EvalResult best = result.first;
  if (best.step.place_grenade) {
    grenade_owner_objective[self.pos] = result.second;
    if (!best.step.move.has_value()) {
      auto self_ptr =
          find_if(state.vampires.begin(), state.vampires.end(), [&](const Vampire& v) { return v.id == self.id; });
      --self_ptr->grenades;
      --self.grenades;
      state.grenades.push_back({self.pos, self.id, GRENADE_TICKS, self.range});
      grid[self.pos].grenades.push_back(state.grenades.back());
      path_finder.init(grid, self, true);
      path_finder.init_step_safety_checker(state);
      cerr << "Objective finished, starting a new one" << endl;
      Objective::EvalResult next_best = evaluate_objectives(true).first;
      best.step.move = next_best.step.move;
    }
  } else if (best.step.throw_grenades.has_value()) {
    if (!best.step.move.has_value()) {
      grid.handle_throw(best.step.throw_grenades.value(), self);
      state.grenades = grid.get_state().grenades;
      path_finder.init(grid, self);
      path_finder.init_step_safety_checker(state);
      cerr << "Objective finished, starting a new one" << endl;
      Objective::EvalResult next_best = evaluate_objectives(true).first;
      best.step.move = next_best.step.move;
    }
  } else {
    if (!best.step.move.has_value()) {
      cerr << "Can't do anything meaningful" << endl;
    } else if (best.step.move.value().empty()) {
      cerr << "Waiting" << endl;
    }
  }
  return best.step;
}

vector<Pos> AI::grenade_positions_for(Pos target) const { return positions_for(target, self.range, true); }

vector<Pos> AI::setup_positions_for(Pos target) const {
  unordered_set<Pos, Pos::hash> positions;
  for (const Pos& grenade_pos : grenade_positions_for(target)) {
    auto setup_positions = grenade_positions_for(grenade_pos);
    positions.insert(setup_positions.begin(), setup_positions.end());
  }
  return {positions.begin(), positions.end()};
}

vector<AI::ThrowOption> AI::throw_options_from(Pos pos) const {
  vector<AI::ThrowOption> result;
  vector<Pos> throwable_deltas{{0, 0}};
  throwable_deltas.insert(throwable_deltas.end(), pos_deltas.begin(), pos_deltas.end());
  for (const Pos& delta : throwable_deltas) {
    const auto& grenades = grid[self.pos + delta].grenades;
    int can_throw = -1;
    for (const Grenade& grenade : grenades) {
      if (grenade.vampire_id == self.id) {
        if (throwable_grenades.count(grenade.pos))
          can_throw = 1;
        else {
          can_throw = 0;
          break;
        }
      }
    }
    if (can_throw == 1) {
      ThrowOption throw_opt(grenades);
      vector<Pos> targets;
      if (delta == Pos{0, 0}) {
        throw_opt.throw_step.from_place = true;
        targets = positions_for(pos, initial_data.grenade_radius + 1, false);
      } else {
        throw_opt.throw_step.from_place = false;
        for (int range = 2; range <= initial_data.grenade_radius + 2; ++range) {
          Pos target = self.pos + range * delta;
          if (!grid[target].is_bush && !grid[target].bat.has_value()) {
            targets.push_back(target);
          }
        }
      }
      for (const Pos& target : targets) {
        throw_opt.target_pos = target;
        throw_opt.throw_step.dir = *get_direction_towards(self.pos, target);
        throw_opt.throw_step.length = manhattan_distance(self.pos, target);
        result.push_back(throw_opt);
      }
    }
  }
  return result;
}

int AI::ticks_to_wait_until_grenade() {
  if (self.grenades > 0) return 0;
  int min_tick = 5;
  for (const Grenade& grenade : state.grenades) {
    if (grenade.vampire_id == self.id && grenade.tick < min_tick) {
      min_tick = grenade.tick;
    }
  }
  return min_tick;
}

std::pair<Objective::EvalResult, Objective*> AI::evaluate_objectives(bool secondary) {
  Objective::EvalResult best;
  Objective* bestobj = nullptr;
  const auto& objectives_used = secondary ? objectives2 : objectives;
  for (Objective* objective : objectives_used) {
    Objective::EvalResult result = objective->evaluate(*this, secondary);
    if (result.score > best.score) {
      best = result;
      bestobj = objective;
    }
  }
  cerr << endl << "Winning objective: " << best.description << endl;
  cerr << "Score: " << best.score << endl;
  return {best, bestobj};
}

std::vector<Pos> AI::positions_for(Pos target, int range, bool break_on_obstacle) const {
  vector<Pos> positions;
  for (const Pos& delta : pos_deltas) {
    Pos pos = target;
    for (int i = 1; i <= range; i++) {
      pos += delta;
      if (grid[pos].is_bush || grid[pos].bat.has_value()) {
        if (break_on_obstacle)
          break;
        else
          continue;
      }
      positions.push_back(pos);
    }
  }
  return positions;
}
