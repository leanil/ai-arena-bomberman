#include "PathFinder.h"

#include <algorithm>

#include "../common/utility.h"
#include "Objective.h"

using namespace std;

const int PathFinder::INF;

void PathFinder::init(const Grid& starting_grid, Vampire self, bool _previous_obj_placed_grenade) {
  this->self = self;
  grids.clear();
  grids.push_back(starting_grid);
  init_internals(GRENADE_TICKS);
  previous_obj_placed_grenade = _previous_obj_placed_grenade;
}

void PathFinder::init_step_safety_checker(const GameState& state) {
  if (grids.empty()) {
    error("init_step_safety_checker error: PathFinder is not initialized");
  }
  if (!step_safety_checker) {
    step_safety_checker = make_unique<SafetyChecker>();
  }
  step_safety_checker->init(state, grids[0], self);
}

int PathFinder::get_distance(Pos target) {
  dijkstra(target);
  return distance[target.y][target.x];
}

optional<vector<Step>> PathFinder::find_path(Pos target, int min_ticks, int max_ticks) {
  if (target == self.pos && min_ticks == 0) {
    return vector<Step>{};
  }
  dijkstra(target, min_ticks);
  if (distance[target.y][target.x] > max_ticks) return nullopt;
  int tick = trim_tick(max(distance[target.y][target.x], min_ticks));
  while (tick < (int)grids.size() - 1 && last_move[tick][target.y][target.x] == -1) {
    ++tick;
  }
  if (distance[target.y][target.x] == INF || last_move[tick][target.y][target.x] == -1) {
    return nullopt;
  }
  // we shorten the path as much as we can, but keep one extra step so it waits at junction
  while (tick > 1 && last_move[tick - 1][target.y][target.x] != -1 && last_move[tick - 2][target.y][target.x] != -1)
    --tick;

  std::vector<Step> steps;

  for (Pos pos = target; pos != self.pos || tick > 0;) {
    int move_idx = last_move[tick][pos.y][pos.x];
    steps.push_back(Step{false, nullopt, moves_3[move_idx]});
    for (Direction dir : moves_3[move_idx]) {
      pos -= pos_deltas[(int)dir];
    }
    if (last_move[tick - 1][pos.y][pos.x] != -1) {
      --tick;
    }
  }
  reverse(steps.begin(), steps.end());
  while ((int)steps.size() < min_ticks) steps.push_back({false, nullopt, {}});
  return steps;
}

optional<vector<Step>> PathFinder::find_path_to_place_grenade(bool can_throw, Pos target, int min_ticks,
                                                              int max_ticks) {
  // We can throw a grenade onto another one, so skipping this check
  // for (const auto& grenade : grids[0][target].grenades) {
  //   min_ticks = max(min_ticks, grenade.tick);
  // }

  // Check whether we are standing on a grenade that we can throw
  auto maybe_throw = Throw::between(self.pos, target);
  if (can_throw && maybe_throw.has_value() && maybe_throw.value().length <= grids[0].max_throw_length) {
    for (const auto& grenade : grids[0][self.pos].grenades) {
      if (grenade.vampire_id == self.id) {
        Step step{false, maybe_throw.value(), nullopt};
        Grid new_grid{grids[0]};
        new_grid.step({{self.id, step}});
        PathFinder grenade_planner;
        grenade_planner.init(new_grid, self);
        if (grenade_planner.is_survivable()) return vector<Step>{step};
      }
    }
  }

  vector<pair<int, Pos>> candidates;
  auto target_path = find_path(target, min_ticks, max_ticks);
  if (target_path.has_value()) candidates.emplace_back(target_path.value().size() * 2, target);

  vector<Pos> throw_from = grids[trim_tick(min_ticks)].from_where_can_throw_to(target);
  for (const auto& pos : throw_from) {
    auto throw_path = find_path(pos, max(0, min_ticks - 1), max_ticks);
    // We should add 1 to the path length. With the priority of (2 * path length) + 1,
    // we favor the throwing if there would be same number of steps.
    if (throw_path.has_value()) candidates.emplace_back(throw_path.value().size() * 2 + 1, pos);
  }
  if (candidates.empty()) return nullopt;
  sort(candidates.begin(), candidates.end());
  for (const auto& candidate : candidates) {
    const Pos& pos = candidate.second;
    auto path = find_path(pos, max(0, min_ticks - (pos != target)), max_ticks);
    // If we put down the grenade and want to stay there to throw, there should not be light
    if (pos != target && grids[trim_tick(path.value().size() + 1)][pos].has_light) continue;
    // We check the placed grenade after throwing, at the target position
    TickPos at{(int)path.value().size() + (pos != target), target};
    Vampire future_self = self;
    future_self.pos = pos;
    PathFinder grenade_planner;  // TODO use SafetyChecker
    grenade_planner.init_with_grenade_placed(grids[trim_tick(at.tick)], future_self, at);
    if (!grenade_planner.is_survivable()) continue;
    // TODO if the path is short, try placing the grenade 1..GRENADE_TICKS later
    // (because a nearby grenade can interfere with our placement)
    // BUT check whether there _is_ any grenade nearby, as setting up additional PathFinders is expensive
    if (pos == target) {
      path.value().push_back({true, nullopt, nullopt});
    } else {
      path.value().push_back({true, nullopt, vector<Direction>{}});
      path.value().push_back({false, Throw::between(pos, target), nullopt});
    }
    if (path.value()[0].place_grenade && bt_result && !bt_result->has_safe_move_with_grenade) return nullopt;
    return path;
  }
  return nullopt;
}

bool PathFinder::is_survivable() {
  dfs_to_survive({0, self.pos});
  return safe_path_exists;
}

void PathFinder::set_heuristic(QueueEntry& entry) {
  // We take the elapsed ticks, prioritize some positions
  entry.heuristic = 5 * entry.tick;
  // Here we favor fields where we have options to move
  int good_neighbors = 0;
  int curr_tick = trim_tick(entry.tick);
  for (Pos delta : pos_deltas) {
    Pos neighbor = entry.pos + delta;
    if (grids[curr_tick][neighbor].can_step_here()) {
      good_neighbors++;
    }
  }
  entry.heuristic -= good_neighbors;
}

void PathFinder::dijkstra(Pos target, int min_ticks, int max_ticks) {
  int check_at_tick = trim_tick(min_ticks);
  while (!dijkstra_queue.empty() &&
         (distance[target.y][target.x] == INF || last_move[check_at_tick][target.y][target.x] == -1) &&
         dijkstra_queue.top().tick <= max_ticks) {
    QueueEntry curr = dijkstra_queue.top();
    if (curr.tick == (int)grids.size() - 1) {
      safe_path_exists = true;
    }
    dijkstra_queue.pop();
    for (int move_idx = 0; move_idx < (int)moves_3.size(); move_idx++) {
      if (curr.tick >= self.shoes && moves_3[move_idx].size() > 2) break;
      Pos pos = curr.pos;
      int next_tick = trim_tick(curr.tick + 1);
      if (do_move(pos, move_idx, curr.tick) && !grids[next_tick].field_at(pos).has_light &&
          last_move[next_tick][pos.y][pos.x] == -1) {
        last_move[next_tick][pos.y][pos.x] = move_idx;
        distance[pos.y][pos.x] = min(distance[pos.y][pos.x], curr.tick + 1);
        QueueEntry next{curr.tick + 1, pos, move_idx, -1};
        set_heuristic(next);
        dijkstra_queue.push(next);
      }
    }
  }
}

bool PathFinder::do_move(Pos& pos, int move_idx, int tick) {
  if (step_safety_checker && step_safety_checker->safe_step_exists && tick == 0 &&
      !step_safety_checker->is_safe_first_step[move_idx]) {
    return false;
  }
  if (bt_result && bt_result->has_safe_move && tick == 0 &&
      !bt_result->is_safe_move[previous_obj_placed_grenade][move_idx]) {
    return false;
  }
  int current_tick = trim_tick(tick);
  for (Direction dir : moves_3[move_idx]) {
    pos += pos_deltas[(int)dir];
    if (!grids[current_tick].field_at(pos).can_step_here()) {
      return false;
    }
  }
  return true;
}

void PathFinder::dfs_to_survive(TickPos curr) {
  if (curr.tick == (int)grids.size() - 1) {
    safe_path_exists = true;
  }
  if (safe_path_exists) return;
  int current_tick = trim_tick(curr.tick);
  if (dfs_vis[current_tick][curr.pos.y][curr.pos.x]) return;
  dfs_vis[current_tick][curr.pos.y][curr.pos.x] = true;
  for (int move_idx = 0; move_idx < (int)moves_3.size(); move_idx++) {
    if (curr.tick >= self.shoes && moves_3[move_idx].size() > 2) break;
    Pos pos = curr.pos;
    int next_tick = trim_tick(curr.tick + 1);
    if (do_move(pos, move_idx, curr.tick) && !grids[next_tick].field_at(pos).has_light) {
      dfs_to_survive({curr.tick + 1, pos});
    }
  }
}

int PathFinder::trim_tick(int tick) const { return tick < (int)grids.size() ? tick : (int)grids.size() - 1; }

void PathFinder::init_with_grenade_placed(const Grid& starting_grid, const Vampire& self, TickPos at) {
  this->self = self;
  grids.clear();
  grids.push_back(starting_grid);
  if (!grids[0][at.pos].grenades.empty()) {
    error("There is already a grenade at the specified location");
  }
  grids[0][at.pos].grenades.push_back({at.pos, self.id, GRENADE_TICKS, self.range});
  init_internals(GRENADE_TICKS + 1);
}

void PathFinder::init_internals(int grid_ticks) {
  for (int i = 1; i <= grid_ticks; i++) {
    grids.emplace_back(grids.back());  // copy the last
    grids.back().step();
  }
  int size = grids[0].fields.size();
  distance.assign(size, vector<int>(size, INF));
  last_move.assign(grids.size(), vector<vector<int>>(size, vector<int>(size, -1)));
  dfs_vis.assign(grids.size(), vector<vector<bool>>(size, vector<bool>(size)));
  dijkstra_queue = priority_queue<QueueEntry>();
  QueueEntry start{0, self.pos, -2, 0};  // move_idx = -2 is important, see below
  dijkstra_queue.push(start);
  distance[self.pos.y][self.pos.x] = 0;
  last_move[0][self.pos.y][self.pos.x] = -2;  // Not a move, but not unreachable either...
  safe_path_exists = false;
}

void PathFinder::print(std::ostream& os) {
  for (const auto& row : distance) {
    for (int d : row) {
      if (d == INF)
        os << "   ";
      else if (d < 10)
        os << " " << d << " ";
      else
        os << d << " ";
    }
    os << endl;
  }
  os << endl;
}

void PathFinder::init_bt_result(vector<std::vector<bool>>&& is_safe_move) {
  bt_result = make_unique<BTResult>(move(is_safe_move));
}

PathFinder::BTResult::BTResult(std::vector<std::vector<bool>>&& is_safe_move) : is_safe_move(move(is_safe_move)) {
  for (int place_grenade = 0; place_grenade < 2; ++place_grenade) {
    for (bool is_safe : this->is_safe_move[place_grenade]) {
      if (is_safe) {
        has_safe_move = true;
        if (place_grenade) has_safe_move_with_grenade = true;
      }
    }
  }
}
