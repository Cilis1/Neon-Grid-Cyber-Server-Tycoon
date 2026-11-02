#pragma once

#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace neon::netrunning {

using nlohmann::json;

// An ICE (Intrusion Countermeasure Electronics) unit patrolling the netrun grid.
struct ICE {
  std::string uid;
  int x = 0;
  int y = 0;
  int hp = 20;
  int max_hp = 20;
  int damage = 10;
  bool alert = false;                 // currently chasing the player
  std::vector<std::pair<int, int>> patrol; // waypoints
  std::size_t patrol_idx = 0;

  json to_json() const;
  static ICE from_json(const json& j);
};

} // namespace neon::netrunning
