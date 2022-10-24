#ifndef POSITIONS_H_INCLUDED
#define POSITIONS_H_INCLUDED

#include <array>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct Pos {
  int y, x;
  Pos() = default;
  Pos(int _y, int _x) : y(_y), x(_x) {}

  struct hash {
    size_t operator()(const Pos& p) const;
  };

  Pos& operator+=(const Pos& p2);
  Pos& operator-=(const Pos& p2);
};

const std::array<Pos, 4> pos_deltas = {Pos{-1, 0}, Pos{0, 1}, Pos{1, 0}, Pos{0, -1}};

bool operator==(const Pos& p1, const Pos& p2);
bool operator!=(const Pos& p1, const Pos& p2);
bool operator<(const Pos& p1, const Pos& p2);
Pos operator+(const Pos& p1, const Pos& p2);
Pos operator-(const Pos& p1, const Pos& p2);
Pos operator*(const int& lambda, const Pos& p);

int manhattan_distance(const Pos& p1, const Pos& p2);

std::ostream& operator<<(std::ostream& os, const Pos& p);
std::istream& operator>>(std::istream& is, Pos& p);
std::string to_string(const Pos& p);

struct TickPos {
  int tick;
  Pos pos;
};

enum class Direction { UP, RIGHT, DOWN, LEFT };
const std::array<Direction, 4> directions = {Direction::UP, Direction::RIGHT, Direction::DOWN, Direction::LEFT};

std::optional<Direction> get_direction_towards(const Pos& from, const Pos& to);

const std::vector<std::vector<Direction>> moves_3 = {{},
                                                     {Direction::UP},
                                                     {Direction::RIGHT},
                                                     {Direction::DOWN},
                                                     {Direction::LEFT},
                                                     {Direction::UP, Direction::UP},
                                                     {Direction::UP, Direction::RIGHT},
                                                     {Direction::UP, Direction::LEFT},
                                                     {Direction::RIGHT, Direction::UP},
                                                     {Direction::RIGHT, Direction::RIGHT},
                                                     {Direction::RIGHT, Direction::DOWN},
                                                     {Direction::DOWN, Direction::RIGHT},
                                                     {Direction::DOWN, Direction::DOWN},
                                                     {Direction::DOWN, Direction::LEFT},
                                                     {Direction::LEFT, Direction::UP},
                                                     {Direction::LEFT, Direction::DOWN},
                                                     {Direction::LEFT, Direction::LEFT},
                                                     {Direction::UP, Direction::UP, Direction::UP},
                                                     {Direction::UP, Direction::UP, Direction::RIGHT},
                                                     {Direction::UP, Direction::UP, Direction::LEFT},
                                                     {Direction::UP, Direction::RIGHT, Direction::UP},
                                                     {Direction::UP, Direction::RIGHT, Direction::RIGHT},
                                                     {Direction::UP, Direction::LEFT, Direction::UP},
                                                     {Direction::UP, Direction::LEFT, Direction::LEFT},
                                                     {Direction::RIGHT, Direction::UP, Direction::UP},
                                                     {Direction::RIGHT, Direction::UP, Direction::RIGHT},
                                                     {Direction::RIGHT, Direction::RIGHT, Direction::UP},
                                                     {Direction::RIGHT, Direction::RIGHT, Direction::RIGHT},
                                                     {Direction::RIGHT, Direction::RIGHT, Direction::DOWN},
                                                     {Direction::RIGHT, Direction::DOWN, Direction::RIGHT},
                                                     {Direction::RIGHT, Direction::DOWN, Direction::DOWN},
                                                     {Direction::DOWN, Direction::RIGHT, Direction::RIGHT},
                                                     {Direction::DOWN, Direction::RIGHT, Direction::DOWN},
                                                     {Direction::DOWN, Direction::DOWN, Direction::RIGHT},
                                                     {Direction::DOWN, Direction::DOWN, Direction::DOWN},
                                                     {Direction::DOWN, Direction::DOWN, Direction::LEFT},
                                                     {Direction::DOWN, Direction::LEFT, Direction::DOWN},
                                                     {Direction::DOWN, Direction::LEFT, Direction::LEFT},
                                                     {Direction::LEFT, Direction::UP, Direction::UP},
                                                     {Direction::LEFT, Direction::UP, Direction::LEFT},
                                                     {Direction::LEFT, Direction::DOWN, Direction::DOWN},
                                                     {Direction::LEFT, Direction::DOWN, Direction::LEFT},
                                                     {Direction::LEFT, Direction::LEFT, Direction::UP},
                                                     {Direction::LEFT, Direction::LEFT, Direction::DOWN},
                                                     {Direction::LEFT, Direction::LEFT, Direction::LEFT}};

#endif  // POSITIONS_H_INCLUDED
