#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace neon::netrunning {

using nlohmann::json;

// The player's netrunning deck. Base stats come from installed hardware
// (cyberdeck item) + cyberware + tech tree bonuses (applied by Game).
struct Cyberdeck {
  int ram = 8;          // buffer size (program RAM budget)
  double cpu_speed = 1.0; // action points per turn multiplier
  int shield = 100;
  int max_shield = 100;
  int program_slots = 4;

  json to_json() const;
  static Cyberdeck from_json(const json& j);
};

} // namespace neon::netrunning
