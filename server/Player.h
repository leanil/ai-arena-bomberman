#ifndef ITECH21_PLAYER_H
#define ITECH21_PLAYER_H

#include <chrono>
#include <fstream>
#include <string>

#include "../common/utility.h"

using namespace std::chrono_literals;

class Player {
 public:
  int vampire_id;
  std::string name;
  OutTee input;
  std::ifstream output;  // the player's output is an input for the server

  Player(int vampire_id)
      : vampire_id{vampire_id},
        name{"player" + std::to_string(vampire_id)},
        input{name + ".in"},
        output{name + ".out"} {}
};

#endif  // ITECH21_PLAYER_H
