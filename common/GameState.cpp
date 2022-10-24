#include "GameState.h"

#include <sstream>
#include <unordered_set>

#include "utility.h"

using namespace std;

InitialData::InitialData(const std::vector<std::string>& lines) {
  unordered_set<string> initial_commands{"LEVEL", "GAMEID", "TEST", "MAXTICK", "GRENADERADIUS", "SIZE"};
  for (const string& line : lines) {
    stringstream stream(line);
    string keyword;
    stream >> keyword;
    if (keyword == "MESSAGE")
      stream >> message;
    else if (keyword == "LEVEL")
      stream >> level;
    else if (keyword == "GAMEID")
      stream >> game_id;
    else if (keyword == "TEST")
      stream >> test;
    else if (keyword == "MAXTICK")
      stream >> max_tick;
    else if (keyword == "GRENADERADIUS")
      stream >> grenade_radius;
    else if (keyword == "SIZE")
      stream >> size;
    else
      error("Unknown command keyword: " + keyword);
    if (stream.fail()) {
      error("Failed to parse command from line: " + line);
    }
    if (keyword != "MESSAGE" && !initial_commands.erase(keyword)) {
      error("Duplicate command: " + line);
    }
  }
  if (!initial_commands.empty()) {
    string message = "Missing commands:";
    for (const string& keyword : initial_commands) {
      message += " " + keyword;
    }
    error(message);
  }
}

std::ostream& operator<<(ostream& out, const InitialData& initial_data) {
  if (!initial_data.message.empty()) {
    out << "MESSAGE " << initial_data.message << '\n';
  }
  return out << "LEVEL " << initial_data.level << '\n'
             << "GAMEID " << initial_data.game_id << '\n'
             << "TEST " << (int)initial_data.test << '\n'
             << "MAXTICK " << initial_data.max_tick << '\n'
             << "GRENADERADIUS " << initial_data.grenade_radius << '\n'
             << "SIZE " << initial_data.size << endl;
}

GameState::GameState(const vector<string>& lines) {
  for (const string& line : lines) {
    stringstream stream(line);
    string keyword;
    stream >> keyword;
    if (keyword == "REQ") {
      stream >> game_id >> tick >> vampire_id;
    } else if (keyword == "WARN") {
      string warning;
      stream >> warning;
      warnings.push_back(move(warning));
    } else if (keyword == "VAMPIRE") {
      Vampire vampire{};
      stream >> vampire.id >> vampire.pos >> vampire.health >> vampire.grenades >> vampire.range >> vampire.shoes;
      vampires.push_back(vampire);
    } else if (keyword == "GRENADE") {
      Grenade grenade{};
      stream >> grenade.vampire_id >> grenade.pos >> grenade.tick >> grenade.range;
      grenades.push_back(grenade);
    } else if (keyword == "POWERUP") {
      Powerup powerup{};
      string type;
      stream >> type >> powerup.ticks >> powerup.pos >> powerup.protect;
      if (type == "TOMATO")
        powerup.type = PowerupType::TOMATO;
      else if (type == "GRENADE")
        powerup.type = PowerupType::GRENADE;
      else if (type == "BATTERY")
        powerup.type = PowerupType::BATTERY;
      else if (type == "SHOE")
        powerup.type = PowerupType::SHOE;
      else
        error("Unknown powerup type: " + type);
      powerups.push_back(powerup);
    } else if (keyword.substr(0, 3) == "BAT") {
      int density = stoi(keyword.substr(3));
      Pos pos;
      while (stream >> pos) {
        bats.push_back({pos, density});
      }
    } else if (keyword == "END") {
      end = true;
    } else {
      error("Unknown command keyword: " + keyword);
    }
    // This would be triggered because of the while (stream >> target_pos) loop
    // if (stream.fail()) {
    //   error("Failed to parse command from line: " + line);
    // }
  }
}

std::ostream& operator<<(std::ostream& out, const Vampire& vampire) {
  return out << "VAMPIRE " << vampire.id << ' ' << vampire.pos << ' ' << vampire.health << ' ' << vampire.grenades
             << ' ' << vampire.range << ' ' << vampire.shoes << endl;
}

std::ostream& operator<<(std::ostream& out, const Grenade& grenade) {
  return out << "GRENADE " << grenade.vampire_id << ' ' << grenade.pos << ' ' << grenade.tick << ' ' << grenade.range
             << endl;
}

const string powerup_type_names[] = {"TOMATO", "GRENADE", "BATTERY", "SHOE"};
std::ostream& operator<<(std::ostream& out, const Powerup& powerup) {
  return out << "POWERUP " << powerup_type_names[(int)powerup.type] << ' ' << powerup.ticks << ' ' << powerup.pos << ' '
             << powerup.protect << endl;
}

std::ostream& operator<<(std::ostream& out, const GameState& game_state) {
  for (const auto& vampire : game_state.vampires) out << vampire;
  for (const auto& grenade : game_state.grenades) out << grenade;
  for (const auto& powerup : game_state.powerups) out << powerup;
  vector<Pos> bats_by_density[4];
  for (const auto& bat : game_state.bats) bats_by_density[bat.density].push_back(bat.pos);
  for (int i = 1; i <= 3; i++) {
    if (bats_by_density[i].empty()) continue;
    out << "BAT" << i;
    for (const Pos& pos : bats_by_density[i]) out << ' ' << pos;
    out << endl;
  }
  return out;
}

string Throw::to_string() const {
  string command = "THROW ";
  if (from_place) command += "X";
  command += "URDL"[(int)dir];
  command += " " + std::to_string(length);
  return command;
}

std::optional<Throw> Throw::between(const Pos& from, const Pos& to) {
  auto dir = get_direction_towards(from, to);
  if (!dir.has_value()) return nullopt;
  return Throw{true, dir.value(), manhattan_distance(from, to)};
}

string Step::to_string() const {
  string command;
  if (place_grenade)
    command = "GRENADE\n";
  else if (throw_grenades.has_value())
    command = throw_grenades.value().to_string() + "\n";
  if (!move.has_value() || move->empty()) return command;
  command += "MOVE";
  for (Direction dir : *move) {
    command += ' ';
    command += "URDL"[(int)dir];
  }
  return command;
}