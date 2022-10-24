#ifndef ITECH21_UTILITY_H
#define ITECH21_UTILITY_H

#include <fstream>
#include <string>

#ifndef NDEBUG
#define NDEBUG
#endif

void error(const std::string& message);

// TODO Use boost: https://stackoverflow.com/questions/998072/tee-ing-input-cin-out-to-a-log-file-or-clog
class InTee {};

class OutTee {
 public:
  std::ofstream pipe;
  std::ofstream log;
  OutTee(std::string pipe_name) : pipe{pipe_name}, log{pipe_name + ".log"} {}
};
template <typename T>
OutTee& operator<<(OutTee& out, const T& t) {
  out.pipe << t;
  out.log << t;
  return out;
}

#endif  // ITECH21_UTILITY_H
