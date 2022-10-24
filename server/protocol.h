#ifndef ITECH21_PROTOCOL_H
#define ITECH21_PROTOCOL_H

#include <array>
#include <istream>
#include <ostream>
#include <string>

#include "../common/GameState.h"
#include "config.h"

namespace protocol {

class Login {
 public:
  const std::string keyword = "LOGIN";
  std::string token;
  explicit Login(std::istream& in);
};

class Request {
 public:
  int game_id;
  int tick;
  int vampire_id;
  const std::string keyword = "REQ";
};
std::ostream& operator<<(std::ostream& out, const Request& request);

struct EndMessage {
  const char dot = '.';
};
std::ostream& operator<<(std::ostream& out, const EndMessage& end);

class Response {
 public:
  int game_id;
  int tick;
  int vampire_id;
  const std::string keyword = "RES";
  Step step;
  explicit Response(std::istream& in);
};
std::ostream& operator<<(std::ostream& out, const Response& response);

class Wrong {
 public:
  std::string reason;
  const std::string keyword = "WRONG";
};
std::ostream& operator<<(std::ostream& out, const Wrong& wrong);

class Success {
 public:
  double score;
  const std::string keyword = "SUCCESS";
};
std::ostream& operator<<(std::ostream& out, const Success& success);

}  // namespace protocol

#endif  // ITECH21_PROTOCOL_H
