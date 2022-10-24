#include "positions.h"

#include <iostream>
#include <string>

using namespace std;

size_t Pos::hash::operator()(const Pos& p) const { return 100 * p.y + p.x; }

Pos& Pos::operator+=(const Pos& p2) {
  y += p2.y;
  x += p2.x;
  return *this;
}

Pos& Pos::operator-=(const Pos& p2) {
  y -= p2.y;
  x -= p2.x;
  return *this;
}

bool operator==(const Pos& p1, const Pos& p2) { return (p1.y == p2.y && p1.x == p2.x); }

bool operator!=(const Pos& p1, const Pos& p2) { return !(p1 == p2); }

bool operator<(const Pos& p1, const Pos& p2) { return p1.y < p2.y || (p1.y == p2.y && p1.x < p2.x); }

Pos operator+(const Pos& p1, const Pos& p2) { return {p1.y + p2.y, p1.x + p2.x}; }

Pos operator-(const Pos& p1, const Pos& p2) { return {p1.y - p2.y, p1.x - p2.x}; }

Pos operator*(const int& lambda, const Pos& p) { return {lambda * p.y, lambda * p.x}; }

int manhattan_distance(const Pos& p1, const Pos& p2) { return abs(p1.y - p2.y) + abs(p1.x - p2.x); }

ostream& operator<<(ostream& os, const Pos& p) {
  os << p.y << " " << p.x;
  return os;
}

istream& operator>>(istream& is, Pos& p) {
  is >> p.y >> p.x;
  return is;
}

string to_string(const Pos& p) { return to_string(p.y) + " " + to_string(p.x); }

optional<Direction> get_direction_towards(const Pos& from, const Pos& to) {
  Pos diff = to - from;
  if ((diff.y != 0 && diff.x != 0) || (diff.y == 0 && diff.x == 0)) return nullopt;
  if (diff.y == 0) return diff.x < 0 ? Direction::LEFT : Direction::RIGHT;
  return diff.y < 0 ? Direction::UP : Direction::DOWN;
}
