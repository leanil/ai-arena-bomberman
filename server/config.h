#ifndef ITECH21_CONFIG_H
#define ITECH21_CONFIG_H

#include <chrono>

namespace config {

using namespace std::chrono_literals;

inline constexpr int target_tower_count = 100;
inline constexpr auto round_timeout = 2000ms;
inline constexpr auto global_timeout = 10 * 60 * 1000ms;

}  // namespace config

#endif  // ITECH21_CONFIG_H
