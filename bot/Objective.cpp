#include "Objective.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "AI.h"

using namespace std;

const Objective::EvalResult not_applicable{{false, nullopt, nullopt}, 0, "Not applicable"};

Objective::EvalResult BatObjective::evaluate(AI& ai, bool secondary) {
  EvalResult result;

  set<pair<int, Pos>> ordered_pos;
  for (const Bat& bat : ai.state.bats) {
    const auto& grenade_positions_for_bat = ai.grenade_positions_for(bat.pos);
    for (const Pos& pos : grenade_positions_for_bat) {
      ordered_pos.insert({manhattan_distance(ai.self.pos, pos), pos});
    }
  }

  int count = 0;
  int max_dist = 0;
  for (const auto& dist_pos : ordered_pos) {
    max_dist = dist_pos.first;
    count++;
    if (count > 20) break;
  }

  map<Pos, vector<Bat>> possible_grenade_kills;
  for (const Bat& bat : ai.state.bats) {
    const auto& grenade_positions_for_bat = ai.grenade_positions_for(bat.pos);
    for (const Pos& pos : grenade_positions_for_bat) {
      if (manhattan_distance(ai.self.pos, pos) > max_dist) continue;
      auto grenade_pos = possible_grenade_kills.find(pos);
      if (grenade_pos != possible_grenade_kills.end()) {
        grenade_pos->second.push_back(bat);
      } else {
        possible_grenade_kills.insert(make_pair(pos, vector<Bat>{bat}));
      }
    }
  }

  // TODO unreachable positions should be filtered out before (performance only)
  for (const auto& grenade_kill : possible_grenade_kills) {
    // Check if we have a grenade there and don't put another one.
    bool we_have_grenade = false;
    const Field& field = ai.grid.field_at(grenade_kill.first);
    for (const auto& grenade : field.grenades) {
      if (grenade.vampire_id == ai.self.id) we_have_grenade = true;
    }
    if (we_have_grenade) continue;
    bool can_throw = (ai.grenade_owner_objective[ai.self.pos] == this);
    auto path =
        ai.path_finder.find_path_to_place_grenade(can_throw, grenade_kill.first, ai.ticks_to_wait_until_grenade());
    if (!path.has_value()) continue;
    double score = 0;
    int explosion_tick = (int)path.value().size() + GRENADE_TICKS - 1;
    if (path.value().back().throw_grenades.has_value()) --explosion_tick;
    // This is a temp fix to avoid considering the original effect of the grenade that we throw.
    auto& grid_when_explodes = path.value().back().throw_grenades.has_value()
                                   ? ai.path_finder.grids[0]
                                   : ai.path_finder.grids[ai.path_finder.trim_tick(explosion_tick)];
    vector<Bat> hit_bats;
    for (const auto& bat : grenade_kill.second) {
      auto& future_bat = grid_when_explodes.field_at(bat.pos).bat;
      if (future_bat.has_value()) {
        hit_bats.push_back(future_bat.value());
        score += 12.0 / (int)path.value().size() + (future_bat.value().density == 1 ? 0.1 : 0);
      }
    }

    if (score > result.score) {
      result.score = score;
      result.step = path.value()[0];

      result.description = "BatObjective: killing " + to_string(hit_bats.size()) + " bats (";
      for (const auto& bat : hit_bats) {
        result.description += to_string(bat.pos) + ",";
      }
      result.description += ") with placing grenade at " + to_string(grenade_kill.first) + " in " +
                            to_string(path.value().size()) + " ticks.";
    }
  }

  return result;
}

Objective::EvalResult PowerupObjective::evaluate(AI& ai, bool secondary) {
  EvalResult result;
  for (const Powerup& powerup : ai.state.powerups) {
    if (ai.self.pos == powerup.pos && powerup.ticks >= 0 && ai.protect_steps >= powerup.protect) continue;
    int ticks_until_appears = powerup.ticks >= 0 ? 0 : -powerup.ticks - 1;
    auto path = ai.path_finder.find_path(
        powerup.pos, max(ai.self.pos == powerup.pos ? ticks_until_appears + powerup.protect - ai.protect_steps : 0, 1));
    if (!path.has_value() || (powerup.ticks >= 0 && powerup.ticks <= (int)path.value().size()))
      continue;  // At tick 1 we have to be already standing on
    int ticks = ai.self.pos == powerup.pos ? (int)path.value().size()
                                           : max((int)path.value().size(), ticks_until_appears) + powerup.protect - 1;
    double score = 48.0 / ticks;
    if ((powerup.type == PowerupType::TOMATO && ai.self.health < 3) ||
        (powerup.type == PowerupType::GRENADE && ai.self.grenades < 5) || powerup.type == PowerupType::BATTERY ||
        powerup.type == PowerupType::SHOE) {
      score *= 2;
    }
    if (score > result.score) {
      result.score = score;
      result.step = path.value()[0];

      result.description =
          "PowerupObjective: getting powerup at " + to_string(powerup.pos) + " in " + to_string(ticks) + " ticks.";
    }
  }
  return result;
}

Objective::EvalResult PositioningObjective::evaluate(AI& ai, bool secondary) {
  EvalResult result;
  if (ai.offensive_mode) {  // follow
    for (const Vampire& vampire : ai.state.vampires) {
      if (vampire.id == ai.state.vampire_id) continue;
      double score = 1.0 / max((double)ai.path_finder.get_distance(vampire.pos), 0.5);
      if (score > result.score) {
        auto path = ai.path_finder.find_path(vampire.pos, 1);
        if (path.has_value()) {
          result.score = score;
          result.step = path.value()[0];

          result.description = "PositioningObjective: following V" + to_string(vampire.id) +
                               " distance: " + to_string(path.value().size());
        }
      }
    }
  } else {  // hide in a corner
    int n = (int)ai.grid.fields.size();
    for (Pos corner : array<Pos, 4>{{{3, 3}, {3, n - 4}, {n - 4, 3}, {n - 4, n - 4}}}) {
      auto path = ai.path_finder.find_path(corner, 1);
      if (!path.has_value()) continue;
      Pos next_pos = ai.self.pos;
      for (Direction dir : (*path)[0].move.value()) {
        next_pos += pos_deltas[(int)dir];
      }
      double score = 0;
      for (const Vampire& vampire : ai.state.vampires) {
        if (vampire.id == ai.self.id) continue;
        score += manhattan_distance(next_pos, vampire.pos);
      }
      score = score / ((int)ai.state.vampires.size() - 1) / (2 * (int)ai.grid.fields.size());
      if (score > result.score) {
        result.score = score;
        result.step = (*path)[0];
        result.description = "PositioningObjective: going towards corner " + to_string(corner);
      }
    }
  }
  return result;
}

Objective::EvalResult AttackObjective::evaluate(AI& ai, bool secondary) {
  if (ai.protection || !ai.self.grenades || ai.self.pos.y % 2 == 0 || ai.self.pos.x % 2 == 0 || !ai.offensive_mode ||
      !ai.grid[ai.self.pos].grenades.empty())
    return not_applicable;
  vector<Vampire> close_opponents;
  for (const Vampire& vampire : ai.state.vampires) {
    if (vampire.id == ai.state.vampire_id) continue;
    if (manhattan_distance(ai.self.pos, vampire.pos) <= 2) {
      close_opponents.push_back(vampire);
    }
  }
  EvalResult result = not_applicable;
  if (close_opponents.empty()) return result;
  PathFinder grenade_planner;  // TODO use SafetyChecker
  grenade_planner.init_with_grenade_placed(ai.grid, ai.self, {0, {ai.self.pos}});
  grenade_planner.init_step_safety_checker(ai.state);
  if (!grenade_planner.is_survivable()) return result;
  for (const Vampire& vampire : close_opponents) {
    double score = vampire.health == 1 ? 144 : 48;
    auto dir = get_direction_towards(ai.self.pos, vampire.pos);
    if (manhattan_distance(ai.self.pos, vampire.pos) == 1) {
      auto path = grenade_planner.find_path(ai.self.pos + 2 * pos_deltas[(int)*dir]);
      if (score > result.score) {
        result = {
            {true, nullopt,
             path.has_value() && path.value()[0].move.value() == vector<Direction>{*dir, *dir} ? path.value()[0].move
                                                                                               : nullopt},
            score,
            "Trapping V" + to_string(vampire.id)};
      }
    } else if (manhattan_distance(ai.self.pos, vampire.pos) == 2 &&
               ai.grid[ai.self.pos + pos_deltas[(int)*dir]].can_step_here()) {
      if (score > result.score) {
        result = {{true, nullopt, nullopt}, score, "Attacking V" + to_string(vampire.id)};
      }
    }
  }
  return result;
}

Objective::EvalResult AttackObjective2::evaluate(AI& ai, bool secondary) {
  EvalResult result;

  auto is_move_valid = [&ai](const Vampire& vampire, const vector<Direction>& move) {
    if (!ai.grid.shoes_before_step.at(vampire.id) && move.size() > 2) {
      return false;
    }

    bool valid = true;
    Pos pos = vampire.pos;
    for (Direction dir : move) {
      pos += pos_deltas[(int)dir];
      if (!ai.grid.field_at(pos).can_step_here()) {
        valid = false;
        break;
      }
    }

    return valid;
  };

  auto get_granade_options = [&ai](const Vampire& vampire) {
    auto options = std::vector<std::pair<bool, std::optional<Throw>>>{};
    if (vampire.grenades) {
      options.push_back({ai.self.grenades, nullopt});
    }

    for (int from_place = 0; from_place <= 1; from_place++) {
      for (const auto& dir : directions) {
        for (int length = 1; length <= ai.grid.max_throw_length; length++) {
          Pos from_pos = vampire.pos;
          if (!from_place) {
            from_pos += pos_deltas[(int)dir];
          }
          Field& from_field = ai.grid.field_at(from_pos);
          bool has_grenade = false;
          for (const auto& grenade : from_field.grenades)
            if (grenade.vampire_id == vampire.id) has_grenade = true;
          if (!has_grenade) continue;
          Pos to_pos = from_pos + length * pos_deltas[(int)dir];
          if (to_pos.x < 0 || to_pos.y < 0 || to_pos.x >= (int)ai.grid.fields.size() ||
              to_pos.y >= (int)ai.grid.fields.size())
            continue;
          Field& to_field = ai.grid.field_at(to_pos);
          if (to_field.is_bush || to_field.bat.has_value()) continue;

          options.push_back({false, Throw{(bool)from_place, dir, length}});
        }
      }
    }

    return options;
  };

  auto enemies = ai.grid.get_enemies(ai.self.id);
  enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                               [&ai](const Vampire& enemy) {
                                 PathFinder pf;
                                 pf.init(ai.grid, enemy);
                                 return !pf.is_survivable();
                               }),
                enemies.end());

  const auto& self_granade_options = get_granade_options(ai.self);
  for (const auto& self_granade_option : self_granade_options) {
    auto self_step = Step{self_granade_option.first, self_granade_option.second, nullopt};
    for (const auto& enemy : enemies) {
      if (manhattan_distance(enemy.pos, ai.self.pos) > 6) continue;
      int enemy_possible_moves = 0;
      int enemy_fatal_moves = 0;
      for (const auto& enemy_move : moves_3) {
        if (is_move_valid(enemy, enemy_move)) {
          {
            Grid new_grid = Grid{ai.grid};
            new_grid.step({{enemy.id, Step{false, nullopt, enemy_move}}});
            const auto& enemy_future = new_grid.get_vampire(enemy.id);
            PathFinder pf;
            if (!enemy_future.has_value()) continue;
            pf.init(new_grid, enemy_future.value());
            // Dies whithout us
            if (!pf.is_survivable()) continue;
          }

          Grid new_grid = Grid{ai.grid};
          new_grid.step({{ai.self.id, self_step}, {enemy.id, Step{false, nullopt, enemy_move}}});
          // new_grid.print(cerr);
          const auto& enemy_future = new_grid.get_vampire(enemy.id);
          bool fatal_for_enemy = !enemy_future.has_value();
          if (!fatal_for_enemy) {
            PathFinder pf;
            pf.init(new_grid, enemy_future.value());
            fatal_for_enemy = !pf.is_survivable();
          }

          const auto& self_future = new_grid.get_vampire(ai.self.id);
          if (!self_future.has_value()) goto unsafe_choice;
          {
            PathFinder pf;
            pf.init(new_grid, self_future.value());
            if (!pf.is_survivable()) goto unsafe_choice;
          }

          enemy_possible_moves++;
          if (fatal_for_enemy) {
            // cerr << "FATAL" << endl;
            enemy_fatal_moves++;
          }
        }
      }

      if (enemy_possible_moves > 0) {
        int score = (enemy.health == 1 ? 144 : 48) * enemy_fatal_moves / enemy_possible_moves;
        // cerr << "RESULT " << enemy.id << " " << enemy_fatal_moves << "/" << enemy_possible_moves << " " << score <<
        // endl;
        if (score > result.score) {
          result.score = score;
          result.step = self_step;
          result.description = "AttackObjective2: attacking vampire #" + to_string(enemy.id) +
                               " possibility: " + to_string(enemy_fatal_moves) + "/" + to_string(enemy_possible_moves);
        }
      }
    }
  unsafe_choice:
    continue;
  }

  return result;
}

double ChainAttackObjective::yolo_grenade_score = 5;

Objective::EvalResult ChainAttackObjective::evaluate(AI& ai, bool secondary) {
  EvalResult result;
  if (ai.path_finder.grids[1][ai.self.pos].has_light && ai.self.grenades > 0) {
    bool can_throw = (ai.grenade_owner_objective[ai.self.pos] == this);
    auto path = ai.path_finder.find_path_to_place_grenade(can_throw, ai.self.pos, 0, 1);
    if (path.has_value() && path.value().size() == 1) {
      update_result(result, evaluateChainAttackPos(ai, ai.self.pos), path.value()[0], ai.self.pos, AttackMode::PLACE);
    }
  }
  for (const auto& throw_option : ai.throw_options_from(ai.self.pos)) {
    if (ai.path_finder.grids[1][throw_option.target_pos].has_light) {
      bool can_throw = (ai.grenade_owner_objective[ai.self.pos] == this);
      auto path = ai.path_finder.find_path_to_place_grenade(can_throw, throw_option.target_pos, 0, 1);
      if (path.has_value() && path.value().size() == 1) {
        update_result(result, evaluateChainAttackPos(ai, throw_option.target_pos), path.value()[0], ai.self.pos,
                      AttackMode::THROW);
      }
    }
  }
  if (ai.self.grenades > 2) {
    bool can_throw = (ai.grenade_owner_objective[ai.self.pos] == this);
    auto path = ai.path_finder.find_path_to_place_grenade(can_throw, ai.self.pos, 0, 1);
    if (path.has_value() && path.value().size() == 1) {
      update_result(result, yolo_grenade_score, path.value()[0], ai.self.pos, AttackMode::YOLO);
    }
  }
  return result;
}

double ChainAttackObjective::evaluateChainAttackPos(const AI& ai, const Pos& grenade_pos) {
  auto explosion_vec = ai.grenade_positions_for(grenade_pos);  // TODO calculate with the actual grenades thrown
  unordered_set<Pos, Pos::hash> explosion(explosion_vec.begin(), explosion_vec.end());
  double result = 0;
  for (const Vampire& vampire : ai.grid.get_state().vampires) {
    int hit_steps = 0, total_steps = 0;
    for (int move_idx = 0; move_idx < (int)moves_3.size(); ++move_idx) {
      if (moves_3[move_idx].size() > 2 && !ai.grid.shoes_before_step.at(vampire.id)) break;
      Pos pos = vampire.pos;
      bool valid;
      for (Direction dir : moves_3[move_idx]) {
        pos += pos_deltas[(int)dir];
        if (!ai.grid.field_at(pos).can_step_here()) {
          valid = false;
          break;
        }
      }
      if (valid) {
        ++total_steps;
        hit_steps += explosion.count(pos);
      }
    }
    result += (double)hit_steps / total_steps * (vampire.health == 1 ? 144 : 48);
  }
  return result;
}

void ChainAttackObjective::update_result(Objective::EvalResult& result, double score, const Step& step, Pos attack_pos,
                                         AttackMode attack_mode) {
  if (score <= result.score) return;
  result.score = score;
  result.step = step;
  string mode_str;
  switch (attack_mode) {
    case AttackMode::PLACE:
      mode_str = "place";
      break;
    case AttackMode::THROW:
      mode_str = "throw";
      break;
    case AttackMode::YOLO:
      mode_str = "yolo";
      break;
  }
  result.description = "ChainAttackObjective ("s + mode_str + "): attacking with grenade at " + to_string(attack_pos);
}

double AttackPowerupObjective::success_probability = 0.5;
double AttackPowerupObjective::indirect_success_probability = 0.75;

Objective::EvalResult AttackPowerupObjective::evaluate(AI& ai, bool secondary) {
  EvalResult result;
  for (const Powerup& powerup : ai.state.powerups) {
    int ticks_until_appears = powerup.ticks >= 0 ? 0 : -powerup.ticks - 1;
    if (ticks_until_appears == 0) continue;
    int other_attacker_count = 0;
    if ((int)ai.path_finder.grids.size() > ticks_until_appears) {
      if (ai.path_finder.grids[ticks_until_appears][powerup.pos].illuminated_by_vampire.count(ai.self.id)) continue;
      other_attacker_count = ai.path_finder.grids[ticks_until_appears][powerup.pos].illuminated_by_vampire.size();
    }
    int rival_count = 0;
    for (auto& opponent : ai.opponent_path_finders) {
      auto opponent_path = opponent.second.find_path(powerup.pos, ticks_until_appears);
      rival_count += opponent_path.has_value() && (int)opponent_path->size() <= ticks_until_appears;
    }
    if (rival_count == 0) continue;
    int ticks_until_grenade_placement = ticks_until_appears - GRENADE_TICKS;
    auto grenade_positions = ai.grenade_positions_for(powerup.pos);
    auto setup_positions = ai.setup_positions_for(powerup.pos);
    vector<vector<Pos>> position_sets{grenade_positions, setup_positions};
    if (ticks_until_grenade_placement >= 0) {
      for (int pos_set = 0; pos_set < 2; ++pos_set) {
        if (ai.self.grenades < 2 && pos_set == 1) break;  // no setup with 1 grenade
        for (const Pos& pos : position_sets[pos_set]) {
          bool illuminated_too_soon = false;
          for (int tick = ticks_until_grenade_placement + 1;
               !illuminated_too_soon && tick < min((int)ai.path_finder.grids.size(), ticks_until_appears); ++tick) {
            illuminated_too_soon = ai.path_finder.grids[tick][pos].has_light;
          }
          if (!illuminated_too_soon) {
            bool can_throw = (ai.grenade_owner_objective[ai.self.pos] == this);
            auto path = ai.path_finder.find_path_to_place_grenade(can_throw, pos, ticks_until_grenade_placement);
            double score = pos_set == 0 ? success_probability * 48.0 * rival_count / (other_attacker_count + 1) /
                                              (ticks_until_grenade_placement + 1)  // direct attack
                                        : indirect_success_probability * 48 * rival_count / (other_attacker_count + 1) /
                                              (ticks_until_appears + 1);  // attack setup
            if (path.has_value() && (int)path.value().size() <= ticks_until_grenade_placement + 1) {
              update_result(result, score, *path, powerup.pos, pos_set == 0 ? AttackMode::DIRECT : AttackMode::SETUP);
            }
          }
        }
      }
    }
    for (const Pos& pos : grenade_positions) {
      if ((int)ai.path_finder.grids.size() > ticks_until_appears &&
          ai.path_finder.grids[ticks_until_appears][pos].has_light) {  // indirect attack
        bool can_throw = (ai.grenade_owner_objective[ai.self.pos] == this);
        auto path = ai.path_finder.find_path_to_place_grenade(can_throw, pos, ticks_until_appears - 1);
        double score =
            indirect_success_probability * 48.0 * rival_count / (other_attacker_count + 1) / ticks_until_appears;
        if (path.has_value() && (int)path.value().size() <= ticks_until_appears) {
          update_result(result, score, *path, powerup.pos, AttackMode::INDIRECT);
        }
      }
    }
  }
  return result;
}

void AttackPowerupObjective::update_result(Objective::EvalResult& result, double score, std::vector<Step> path,
                                           Pos powerup_pos, AttackMode attack_mode) {
  if (score <= result.score) return;
  result.score = score;
  result.step = path[0];
  string mode_str;
  switch (attack_mode) {
    case AttackMode::DIRECT:
      mode_str = "direct";
      break;
    case AttackMode::INDIRECT:
      mode_str = "indirect";
      break;
    case AttackMode::SETUP:
      mode_str = "setup";
      break;
  }
  result.description = "AttackPowerupObjective ("s + mode_str + "): attacking powerup at " + to_string(powerup_pos) +
                       " in " + to_string(path.size()) + " ticks.";
}
