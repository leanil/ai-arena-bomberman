#ifndef ITECH21_OBJECTIVE_H
#define ITECH21_OBJECTIVE_H

#include <vector>

#include "../common/GameState.h"
#include "../common/Grid.h"

class AI;

class Objective {
 public:
  struct EvalResult {
    Step step;
    double score = 0.0;
    std::string description = "";
  };
  virtual EvalResult evaluate(AI& ai, bool secondary) = 0;
};

class BatObjective : public Objective {
 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

class PowerupObjective : public Objective {
 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

class PositioningObjective : public Objective {
 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

class AttackObjective : public Objective {
 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

class AttackObjective2 : public Objective {
 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

class ChainAttackObjective : public Objective {
  enum class AttackMode { PLACE, THROW, YOLO };
  static double yolo_grenade_score;
  static void update_result(EvalResult& result, double score, const Step& step, Pos attack_pos, AttackMode attack_mode);
  double evaluateChainAttackPos(const AI& ai, const Pos& grenade_pos);

 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

class AttackPowerupObjective : public Objective {
  enum class AttackMode { DIRECT, INDIRECT, SETUP };
  static double success_probability;
  static double indirect_success_probability;
  static void update_result(EvalResult& result, double score, std::vector<Step> path, Pos powerup_pos,
                            AttackMode attack_mode);

 public:
  EvalResult evaluate(AI& ai, bool secondary) override;
};

#endif  // ITECH21_OBJECTIVE_H