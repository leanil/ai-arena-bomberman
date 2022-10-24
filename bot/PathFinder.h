#ifndef ITECH21_PATHFINDER_H
#define ITECH21_PATHFINDER_H

#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include "../common/GameState.h"
#include "../common/Grid.h"
#include "../common/positions.h"
#include "SafetyChecker.h"

class PathFinder {
 public:
  static const int INF = 10000;

  std::vector<Grid> grids;  // indexed by tick
  Vampire self;

  void init(const Grid& starting_grid, Vampire self, bool _previous_obj_placed_grenade = false);
  void init_step_safety_checker(const GameState& state);
  void init_bt_result(std::vector<std::vector<bool>>&& is_safe_move);
  void init_with_grenade_placed(const Grid& starting_grid, const Vampire& self, TickPos at);
  int get_distance(Pos target);
  std::optional<std::vector<Step>> find_path(Pos target, int min_ticks = 0, int max_ticks = 100);
  std::optional<std::vector<Step>> find_path_to_place_grenade(bool can_throw, Pos target, int min_ticks = 0,
                                                              int max_ticks = 100);
  bool is_survivable();

  int trim_tick(int tick) const;

  void print(std::ostream& os);

 private:
  struct QueueEntry {
    int tick;
    Pos pos;
    int move_idx;
    int heuristic;

    bool operator<(const QueueEntry& other) const { return heuristic > other.heuristic; }
  };

  struct BTResult {
    bool has_safe_move;
    bool has_safe_move_with_grenade;
    std::vector<std::vector<bool>> is_safe_move;
    BTResult() = default;
    BTResult(std::vector<std::vector<bool>>&& is_safe_move);
  };

  void set_heuristic(QueueEntry& entry);

  std::unique_ptr<SafetyChecker> step_safety_checker;
  std::unique_ptr<BTResult> bt_result;
  std::vector<std::vector<int>> distance;
  std::vector<std::vector<std::vector<int>>> last_move;
  std::priority_queue<QueueEntry> dijkstra_queue;

  // This is for survivability checking
  std::vector<std::vector<std::vector<bool>>> dfs_vis;
  bool safe_path_exists;
  bool previous_obj_placed_grenade;

  void dijkstra(Pos target, int min_ticks = 0, int max_ticks = 100);
  void dfs_to_survive(TickPos curr);
  void init_internals(int grid_ticks);
  bool do_move(Pos& pos, int move_idx, int tick);
};

#endif  // ITECH21_PATHFINDER_H
