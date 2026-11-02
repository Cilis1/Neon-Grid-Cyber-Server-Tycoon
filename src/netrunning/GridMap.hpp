#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "netrunning/ICE.hpp"

namespace neon::core {
class Game;
}

namespace neon::netrunning {

using nlohmann::json;

enum class NodeType { Empty, Entry, Data, ICE, Control, Exit, Root };

struct NetrunNode {
  NodeType type = NodeType::Empty;
  bool visited = false;
  bool looted = false;
  int data_value = 0; // shards granted when looted
  json to_json() const;
  static NetrunNode from_json(const json& j);
};

// A netrun target (a server somewhere in the city).
struct NetrunTarget {
  std::string id;
  std::string name;
  std::string kind; // abandoned, corporate, faction
  std::string faction_id;   // for faction targets
  int difficulty = 1;       // 1..5
  int reward_credits = 0;
  int reward_shards = 0;
  bool claimed = false;     // hacked & claimed by the player
  double passive_income = 0.0; // credits/hour when claimed
  double heat_risk = 0.0;
  bool proxy = false;       // can be used as a netrun proxy (trace bonus)

  json to_json() const;
  static NetrunTarget from_json(const json& j);
};

// The 2D grid of a netrun session.
struct GridMap {
  int width = 12;
  int height = 8;
  std::vector<NetrunNode> nodes; // width*height
  std::vector<ICE> ices;
  std::pair<int, int> entry{0, 0};
  std::pair<int, int> root{0, 0};

  bool in_bounds(int x, int y) const;
  NetrunNode* at(int x, int y);
  const NetrunNode* at(int x, int y) const;

  // Generate a map for a target. `rng` drives layout; difficulty scales ICE count.
  static GridMap generate(neon::core::Game& game, const NetrunTarget& target);

  json to_json() const;
  static GridMap from_json(const json& j);
};

} // namespace neon::netrunning
