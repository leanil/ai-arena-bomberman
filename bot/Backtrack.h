#ifndef ITECH21_BACKTRACK_H
#define ITECH21_BACKTRACK_H

#include "../common/GameState.h"
#include "../common/Grid.h"

class AI;

class Backtrack {
 public:
  std::pair<bool, std::vector<std::vector<bool>>> findUnsafeMoves(const AI &ai, const Grid &grid, int simulateSteps,
                                                                  bool returnOnFirstSafe, int enemy_id);
};

#endif  // ITECH21_BACKTRACK_H
