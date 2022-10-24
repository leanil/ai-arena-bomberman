#include "SafetyChecker.h"

#include <algorithm>

using namespace std;

void SafetyChecker::bfs() {
  while (!bfs_queue.empty()) {
    TickPos curr = bfs_queue.front();
    bfs_queue.pop();
    for (const auto& move : moves_3) {
      if (curr.tick >= self.shoes && move.size() > 2) break;
      Pos pos = curr.pos;
      bool valid = true;
      for (Direction dir : move) {
        if (!grids[curr.tick - 1][pos].can_step_here()) {
          valid = false;
          break;
        }
        pos -= pos_deltas[(int)dir];
      }
      TickPos prev{curr.tick - 1, pos};
      if (valid && !grids[prev.tick].field_at(pos).has_light && !is_safe_state[prev.tick][pos.y][pos.x]) {
        is_safe_state[prev.tick][pos.y][pos.x] = true;
        if (prev.tick > 0) {
          bfs_queue.push(prev);
        }
      }
    }
  }
}

bool SafetyChecker::cleithrophobia() {
  vector<Pos> free_neighbors;
  for (Direction dir : directions) {
    Pos neighbor = self.pos + pos_deltas[(int)dir];
    if (grids[0][neighbor].can_step_here()) {
      free_neighbors.push_back(neighbor);
    }
  };
  // TODO check real distance after manhattan, once we have pathing for enemies
  return free_neighbors.size() == 1 && any_of(state->vampires.begin(), state->vampires.end(), [&](const Vampire& v) {
           return v.id != self.id && manhattan_distance(v.pos, free_neighbors[0]) <= (v.shoes ? 3 : 2);
         });
}

void SafetyChecker::init(const GameState& _state, const Grid& starting_grid, const Vampire& _self) {
  self = _self;
  state = &_state;
  grids.clear();
  grids.push_back(starting_grid);
  grids[0].place_possible_grenades(self.id);
  for (int i = 1; i <= GRENADE_TICKS + 1; i++) {
    grids.emplace_back(grids.back());  // copy the last
    grids.back().step();
  }

  int n = (int)starting_grid.fields.size();
  is_safe_state.assign(grids.size(), vector<vector<bool>>(n, vector<bool>(n, false)));
  bfs_queue = queue<TickPos>();
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      if (grids.back().fields[y][x].can_step_here()) {
        bfs_queue.push({(int)grids.size() - 1, {y, x}});
        is_safe_state[(int)grids.size() - 1][y][x] = true;
      }
    }
  }

  bfs();

  if (cleithrophobia()) {
    is_safe_state[1][self.pos.y][self.pos.x] = false;
  }
  is_safe_first_step.assign(moves_3.size(), false);
  for (int move_idx = 0; move_idx < (int)moves_3.size(); ++move_idx) {
    if (!self.shoes && moves_3[move_idx].size() > 2) break;
    is_safe_first_step[move_idx] = true;
    Pos pos = self.pos;
    for (Direction dir : moves_3[move_idx]) {
      Pos next_pos = pos + pos_deltas[(int)dir];
      if (!grids[1][next_pos].can_step_here() && !is_safe_state[1][pos.y][pos.x]) {
        is_safe_first_step[move_idx] = false;
        break;
      }
      if (grids[1][next_pos].is_bush) break;
      pos = next_pos;
    }
    if (!is_safe_state[1][pos.y][pos.x]) {
      is_safe_first_step[move_idx] = false;
    }
  }
  safe_step_exists = any_of(is_safe_first_step.begin(), is_safe_first_step.end(), [](bool is_safe) { return is_safe; });
}
