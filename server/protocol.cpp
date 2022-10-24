#include "protocol.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace protocol {
string read_message(istream& in) {
  string message, line;
  getline(in, line);
  while (in && line != ".") {
    message += line + '\n';
    getline(in, line);
  }
  if (in.fail()) {
    throw runtime_error("Failed to read command: " + message + ", last line: " + line);
  }
  return message;
}

Login::Login(istream& in) {
  const string message = read_message(in);
  stringstream parser(message);
  string message_keyword;
  parser >> message_keyword >> token;
  if (parser.fail() || message_keyword != keyword) {
    throw runtime_error("Failed to parse command " + keyword + " from message " + message);
  }
}

ostream& operator<<(ostream& out, const EndMessage& end) { return out << end.dot << endl; }

ostream& operator<<(ostream& out, const Request& request) {
  return out << request.keyword << ' ' << request.game_id << ' ' << request.tick << ' ' << request.vampire_id << endl;
}

Direction dir_from_char(char dir, string message) {
  const string directions = "URDL";
  if (directions.find(dir) == string::npos) {
    throw runtime_error("Failed to interpret direction " + string(1, dir) + " from message " + message);
  }
  return static_cast<Direction>(directions.find(dir));
}

Response::Response(istream& in) {
  const string message = read_message(in);
  stringstream parser(message);
  string message_keyword;
  parser >> message_keyword >> game_id >> tick >> vampire_id;
  if (parser.fail() || message_keyword != keyword) {
    throw runtime_error("Failed to parse command " + keyword + " from message " + message);
  }
  string step_keyword;
  parser >> step_keyword;
  if (step_keyword == "GRENADE") {
    step.place_grenade = true;
    parser >> step_keyword;
  } else if (step_keyword == "THROW") {
    Throw thro;
    char dir;
    parser >> dir;
    if (dir == 'X') {
      thro.from_place = true;
      parser >> dir;
    }
    thro.dir = dir_from_char(dir, message);
    parser >> thro.length;
    if (parser.fail()) {
      throw runtime_error("Failed to parse command THROW from message " + message);
    }
    step.throw_grenades = thro;
    parser >> step_keyword;
  }
  step.move = vector<Direction>();
  if (!parser.fail() && step_keyword != "MOVE") {
    throw runtime_error("Failed to parse command MOVE from message " + message);
  }
  char dir;
  while (parser >> dir) {
    step.move.value().push_back(dir_from_char(dir, message));
  }
}

std::ostream& operator<<(std::ostream& out, const Response& response) {
  return out << response.keyword << ' ' << response.game_id << ' ' << response.tick << ' ' << response.vampire_id
             << '\n'
             << response.step.to_string() << endl;
}

ostream& operator<<(ostream& out, const Wrong& wrong) {
  return out << wrong.keyword << ' ' << wrong.reason << "\n." << endl;
}

ostream& operator<<(ostream& out, const Success& success) {
  return out << success.keyword << ' ' << success.score << "\n." << endl;
}

}  // namespace protocol
