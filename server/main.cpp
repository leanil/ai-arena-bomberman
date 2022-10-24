#include <iostream>
#include <sstream>
#include <vector>

#include "Player.h"
#include "Simulation.h"
#include "protocol.h"

using namespace std;

int main(int argc, char** argv) {
  if (argc < 3) {
    cerr << "usage: server <map> <bot1> ... <botN>" << endl;
    return 1;
  }
  const int player_count = argc - 2;
  vector<Player> players;
  players.reserve(player_count);
  ostringstream mkfifo_command("mkfifo", ios_base::ate);
  for (int i = 1; i <= player_count; ++i)
    mkfifo_command << " player" << i << ".in"
                   << " player" << i << ".out";
  cout << mkfifo_command.str() << endl;
  if (system(mkfifo_command.str().c_str())) {
    cerr << "failed to create fifos" << endl;
    return 1;
  }
  for (int i = 1; i <= player_count; ++i) {
    ostringstream start_bot_command(argv[i + 1], std::ios_base::ate);
    start_bot_command << " <player" << i << ".in"
                      << " >player" << i << ".out"
                      << " 2>player" << i << ".log"
                      << " &";
    cout << start_bot_command.str() << endl;
    if (system(start_bot_command.str().c_str())) {
      cerr << "failed to start bot #" << i << ":" << endl << start_bot_command.str() << endl;
      return 1;
    }
    players.emplace_back(i);
  }

  for (Player& player : players) {
    try {
      protocol::Login login(player.output);
    } catch (const runtime_error& error) {
      player.input << protocol::Wrong{error.what()};
      return 0;
    }
    cerr << player.name << " logged in" << endl;
  }

  Simulation simulation{move(players), 0, argv[1]};
  simulation.run();
}