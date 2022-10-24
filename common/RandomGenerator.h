#ifndef ITECH21_RANDOMGENERATOR_H
#define ITECH21_RANDOMGENERATOR_H

#include <random>

#include "GameState.h"

class RandomGenerator {
  std::mt19937 rng;
  std::uniform_int_distribution<int> powerup_positive_ticks{10, 25};
  std::uniform_int_distribution<int> powerup_negative_ticks{-9, -4};
  std::uniform_int_distribution<int> powerup_protect{1, 5};
  std::normal_distribution<double> powerup_relative_distance_from_center{0.0, 0.2};
  int ticks_between_powerups, ticks_until_next_powerup;

 public:
  RandomGenerator(int seed = 0) : rng(seed) {
    // This seems to be constant for one game in the logs.
    ticks_between_powerups = std::uniform_int_distribution<int>{20, 50}(rng);
    // Until the first powerup there is more time.
    ticks_until_next_powerup = std::uniform_int_distribution<int>{50, 80}(rng);
  }

  int get_powerup_positive_ticks();

  std::vector<Powerup> get_possible_powerups_to_place(int grid_size, int num_vampires);
};

#endif  // ITECH21_RANDOMGENERATOR_H
