#ifndef SOLVER_H_INCLUDED
#define SOLVER_H_INCLUDED

#include <string>
#include <utility>
#include <vector>

#include "AI.h"

class solver {
 public:
  AI ai;
  void startMessage(const std::vector<std::string>& startInfos);
  std::vector<std::string> processTick(const std::vector<std::string>& infos);
};

#endif  // SOLVER_H_INCLUDED
