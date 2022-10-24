#ifndef ITECH21_SCORECALCULATOR_H
#define ITECH21_SCORECALCULATOR_H

#include <map>

#include "GameState.h"
#include "Grid.h"

class ScoreCalculator {
 public:
  // The first index is ScoreType, and the inner maps are indexed by vampire id
  std::vector<std::map<int, double>> scores;
  std::map<int, double> total_score;

  ScoreCalculator() { scores.resize(4); }

  void init_from_grid(const Grid& grid);
  void update(const GameState& game_state, const InitialData& init_data);

  void print(std::ostream& os) const;
};

#endif  // ITECH21_SCORECALCULATOR_H
