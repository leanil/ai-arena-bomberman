#include "Simulation.h"

#include <fstream>

#include "../common/ScoreCalculator.h"
#include "protocol.h"

using namespace std;

Simulation::Simulation(vector<Player> players, int seed, const string& level_file)
    : players{move(players)}, grid(seed, true /* server */) {
  ifstream f(level_file);

  vector<string> lines;
  string line;
  getline(f, line);
  while (line != ".") {
    lines.push_back(line);
    getline(f, line);
  }
  init_data = InitialData(lines);
  cerr << init_data << endl;

  lines = {};
  getline(f, line);
  while (line != ".") {
    lines.push_back(line);
    getline(f, line);
  }
  GameState state(lines);
  grid.init(state, init_data);
}

void Simulation::run() {
  for (Player& player : players) {
    player.input << init_data << protocol::EndMessage{};
  }
  int num_vampires = grid.get_state().vampires.size();
  ofstream match_log("match.log");
  while (num_vampires > 0) {
    //    grid.print(cerr);
    GameState game_state = grid.get_state();
    num_vampires = game_state.vampires.size();
    match_log << game_state << endl;
    map<int, Step> vampire_steps;
    for (Player& player : players) {
      for (const auto& vampire : game_state.vampires) {
        // Only send if alive
        if (player.vampire_id == vampire.id) {
          send_request(player, game_state);
          vampire_steps[player.vampire_id] = receive_response(player);
        }
      }
    }
    grid.step(vampire_steps);
  }
  ScoreCalculator score_calculator;
  score_calculator.init_from_grid(grid);
  score_calculator.print(cerr);
  ofstream scores("scores.json");
  scores << "[";
  bool first = true;
  for (auto vampire_score : score_calculator.total_score) {
    scores << (first ? "" : ", ") << vampire_score.second;
    first = false;
  }
  scores << "]\n";
}

void Simulation::send_request(Player& player, const GameState& game_state) const {
  player.input << protocol::Request{init_data.game_id, grid.tick, player.vampire_id};
  player.input << game_state << protocol::EndMessage{};
}

Step Simulation::receive_response(Player& player) const {
  try {
    auto request_sent = chrono::high_resolution_clock::now();
    protocol::Response response(player.output);
    //    cerr << response << endl;
    auto response_received = chrono::high_resolution_clock::now();
    auto round_time = chrono::duration_cast<chrono::milliseconds>(response_received - request_sent);
    if (round_time > config::round_timeout) {
      // player.input << protocol::Wrong{"Round time limit exceeded"};
      // return {};
      cerr << "Player " << player.vampire_id << ": Round time limit exceeded, time: " << round_time.count() << endl;
    }
    if (response.game_id != init_data.game_id || response.tick != grid.tick ||
        response.vampire_id != player.vampire_id) {
      player.input << protocol::Wrong{"Invalid response"};
      return {};
    }
    return response.step;
  } catch (const runtime_error& error) {
    player.input << protocol::Wrong{error.what()};
    return {};
  }
}
