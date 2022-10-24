#ifndef ITECH21_SAFETYCHECKER_H
#define ITECH21_SAFETYCHECKER_H

#include <queue>
#include <vector>

#include "../common/GameState.h"
#include "../common/Grid.h"
#include "../common/positions.h"

class SafetyChecker {
 public:
  std::vector<Grid> grids;  // indexed by tick
  Vampire self{};
  std::vector<std::vector<std::vector<bool>>> is_safe_state;
  bool safe_step_exists;
  std::vector<bool> is_safe_first_step;

  void init(const GameState& _state, const Grid& starting_grid, const Vampire& self);

 private:
  std::queue<TickPos> bfs_queue;
  const GameState* state{};
  void bfs();
  bool cleithrophobia();
};

#endif  // ITECH21_SAFETYCHECKER_H
