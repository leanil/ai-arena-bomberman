#include "RandomGenerator.h"

using namespace std;

int RandomGenerator::get_powerup_positive_ticks() {
  return min(ticks_until_next_powerup - 1, powerup_positive_ticks(rng));
}

vector<Powerup> RandomGenerator::get_possible_powerups_to_place(int grid_size, int num_vampires) {
  --ticks_until_next_powerup;
  if (ticks_until_next_powerup != 0) return {};
  ticks_until_next_powerup = ticks_between_powerups;
  vector<Powerup> powerups;
  double x = grid_size / 2.0 + grid_size * powerup_relative_distance_from_center(rng);
  double y = grid_size / 2.0 + grid_size * powerup_relative_distance_from_center(rng);
  Pos pos{static_cast<int>(y / 2) * 2 + 1, static_cast<int>(x / 2) * 2 + 1};
  pos.x = max(1, min(grid_size - 2, pos.x));
  pos.y = max(1, min(grid_size - 2, pos.y));
  PowerupType type = static_cast<PowerupType>(uniform_int_distribution<int>{0, 3}(rng));
  Powerup powerup{type, pos, powerup_negative_ticks(rng), powerup_protect(rng)};
  powerups.push_back(powerup);
  if (num_vampires > 2) {
    powerup.pos.x = grid_size - 1 - powerup.pos.x;
    powerup.pos.y = grid_size - 1 - powerup.pos.y;
    powerups.push_back(powerup);
  }
  return powerups;
}
