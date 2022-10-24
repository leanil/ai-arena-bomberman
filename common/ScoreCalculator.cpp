#include "ScoreCalculator.h"

#include <iomanip>

using namespace std;

void ScoreCalculator::init_from_grid(const Grid& grid) {
  scores.resize(grid.scores.size());
  for (size_t i = 0; i < scores.size(); i++) {
    scores[i] = map<int, double>(grid.scores[i].begin(), grid.scores[i].end());
    for (const auto& vampire_score : scores[i]) total_score[vampire_score.first] += vampire_score.second;
  }
}

void ScoreCalculator::update(const GameState& game_state, const InitialData& init_data) {
  Grid grid(game_state, init_data);
  grid.step();
  for (const Vampire& vampire : game_state.vampires) {
    for (size_t i = 0; i < scores.size(); i++) {
      scores[i][vampire.id] += grid.scores[i][vampire.id];
      total_score[vampire.id] += grid.scores[i][vampire.id];
    }
  }
}

void print_helper(ostream& os, const string& score_type, const map<int, double>& scores) {
  os << score_type;
  os << fixed << setprecision(2);
  for (const auto& vampire_score : scores) {
    os << setw(8) << vampire_score.second;
  }
  os << endl;
}

void ScoreCalculator::print(ostream& os) const {
  print_helper(os, "Powerup scores", scores[ScoreType::POWERUP]);
  print_helper(os, "Bat scores    ", scores[ScoreType::BAT]);
  print_helper(os, "Attack scores ", scores[ScoreType::ATTACK]);
  print_helper(os, "Minus scores  ", scores[ScoreType::MINUS]);
  print_helper(os, "Total scores  ", total_score);
}