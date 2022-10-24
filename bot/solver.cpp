#include "solver.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "../common/GameState.h"

using namespace std;

void solver::startMessage(const vector<string>& startInfos) {
  for (const auto& line : startInfos) {
    cerr << line << endl;
  }
  ai.initial_data = InitialData(startInfos);
}

vector<string> solver::processTick(const vector<string>& infos) {
  for (const auto& line : infos) {
    cerr << line << endl;
  }

  vector<string> commands{infos[0]};
  commands[0][2] = 'S';

  GameState state(infos);
  if (!state.end) {
    Grid(state, ai.initial_data).print(cerr);
    ai.set_state(move(state));
    ai.score_calculator.print(cerr);
    commands.push_back(ai.get_step().to_string());

    // ai.path_finder.print(cerr);

    for (const auto& line : commands) {
      cerr << line << endl;
    }
    cerr << endl;
  }

  return commands;
}
