#ifndef ITECH21_SIMULATION_H
#define ITECH21_SIMULATION_H

#include <array>
#include <random>
#include <vector>

#include "../common/GameState.h"
#include "../common/Grid.h"
#include "Player.h"

class Simulation {
 public:
  std::vector<Player> players;
  Grid grid;
  InitialData init_data;

  Simulation(std::vector<Player> players, int seed, const std::string& level_file);
  void run();

 protected:
  void send_request(Player& player, const GameState& game_state) const;
  Step receive_response(Player& player) const;
};

#endif  // ITECH21_SIMULATION_H
