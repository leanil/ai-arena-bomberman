#include "Backtrack.h"

#include <algorithm>
#include <map>
#include <string>

#include "AI.h"

using namespace std;

vector<int> get_possible_moves(const Grid &grid, vector<vector<bool>> &is_move_unsafe, const Vampire &vampire,
                               bool is_self) {
  vector<int> possible_moves{};
  for (int move_idx = 0; move_idx < (int)moves_3.size(); move_idx++) {
    const auto &move = moves_3[move_idx];

    // unsafe: no shoe, long step
    if (!grid.shoes_before_step.at(vampire.id) && move.size() > 2) {
      if (is_self) {
        is_move_unsafe[0][move_idx] = true;
        is_move_unsafe[1][move_idx] = true;
        continue;
      } else {
        break;
      }
    }

    bool valid = true;
    Pos pos = vampire.pos;
    for (Direction dir : move) {
      pos += pos_deltas[(int)dir];
      if (!grid.field_at(pos).can_step_here()) {
        valid = false;
        break;
      }
    }

    // unsafe: move collides
    if (!valid) {
      if (is_self) {
        is_move_unsafe[0][move_idx] = true;
        is_move_unsafe[1][move_idx] = true;
      }
      continue;
    }

    Grid new_grid = Grid{grid};
    new_grid.step_vampires(map<int, Step>{{vampire.id, Step{false, nullopt, moves_3[move_idx]}}});
    new_grid.step_grenades();
    optional<Vampire> vampire_result = new_grid.get_vampire(vampire.id);

    // unsafe: losses life
    if (!vampire_result.has_value() || vampire_result.value().health != vampire.health) {
      if (is_self) {
        is_move_unsafe[0][move_idx] = true;
        is_move_unsafe[1][move_idx] = true;
      }
      continue;
    }

    possible_moves.push_back(move_idx);
  }
  return possible_moves;
}

// The return value is a bitmap of unsafe flags for moves_3 entries with placing grenade and without
// eg.: value[1][move_idx] == true  =>  moves_3[move_idx] with placing grenade is a bad choice
std::pair<bool, vector<vector<bool>>> Backtrack::findUnsafeMoves(const AI &ai, const Grid &grid, int simulate_steps,
                                                                 bool return_on_first_safe, int only_enemy_id) {
  vector<vector<bool>> is_move_unsafe(2, vector<bool>(moves_3.size(), false));

  auto self = grid.get_vampire(ai.self.id);

  // unsafe: already dead
  if (!self.has_value()) {
    return {false, {}};
  }
  vector<int> self_moves = get_possible_moves(grid, is_move_unsafe, self.value(), true);

  bool has_safe_move = false;

  vector<std::pair<Vampire, vector<int>>> enemies;
  // for each enemy vampire
  for (int enemy_id = (only_enemy_id == -1 ? 1 : only_enemy_id); enemy_id <= (only_enemy_id == -1 ? 4 : only_enemy_id);
       enemy_id++) {
    if (enemy_id == ai.self.id) continue;
    auto enemy_vampire = grid.get_vampire(enemy_id);
    if (enemy_vampire.has_value()) {
      enemies.push_back(
          {enemy_vampire.value(),
           (simulate_steps == 1 ? vector<int>{0}
                                : get_possible_moves(grid, is_move_unsafe, enemy_vampire.value(), false))});
    }
  }

  // for each self move + grenade placement
  for (const auto &self_move_idx : self_moves) {
    for (int self_place_grenade = 0; self_place_grenade < (simulate_steps == 1 ? 1 : 2) &&
                                     self_place_grenade <= grid.grenades_before_step.at(self.value().id);
         self_place_grenade++) {
      // for each enemy with move + grenade placement
      for (const auto &enemy : enemies) {
        // move is already unsafe because of other vampire can kill us
        if (is_move_unsafe[1][self_move_idx]) continue;

        for (const auto &enemy_move_idx : enemy.second) {
          for (int enemy_places_grenade =
                   (simulate_steps == 1 ? min(1, grid.grenades_before_step.at(enemy.first.id)) : 0);
               enemy_places_grenade < 2 && enemy_places_grenade <= grid.grenades_before_step.at(enemy.first.id);
               enemy_places_grenade++) {
            auto next_grid = Grid{grid};
            next_grid.step({{ai.self.id, {(bool)self_place_grenade, nullopt, moves_3[self_move_idx]}},
                            {enemy.first.id, {(bool)enemy_places_grenade, nullopt, moves_3[enemy_move_idx]}}});

            auto next_self = next_grid.get_vampire(ai.self.id);
            // unsafe: dead
            if (!next_self.has_value()) goto unsafe;

            PathFinder pf;
            pf.init(next_grid, next_self.value());
            bool is_survivable = pf.is_survivable();
            // next_grid.print(cerr);
            if (!is_survivable || (simulate_steps > 1 &&
                                   !findUnsafeMoves(ai, next_grid, simulate_steps - 1, true, enemy.first.id).first)) {
              goto unsafe;
              is_move_unsafe[self_place_grenade][self_move_idx] = true;
            }
          }
        }
      }

      has_safe_move = true;
      if (return_on_first_safe) {
        return {true, is_move_unsafe};
      }
      continue;
    unsafe:
      is_move_unsafe[self_place_grenade][self_move_idx] = true;
    }
    if (grid.grenades_before_step.at(self.value().id) == 0) {
      is_move_unsafe[1][self_move_idx] = true;
    }
  }
  return {has_safe_move, is_move_unsafe};
}