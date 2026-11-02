#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "techtree/Effect.hpp"

namespace neon::techtree {

using nlohmann::json;

// Static definition of a tech tree node (data/tech_tree.json).
struct TechNode {
  std::string id;
  std::string name;
  std::string description;
  std::string branch; // Hardware, Software, Netrunning, Economy, Stealth, Infrastructure
  int tier = 1;
  double cost_credits = 0.0;
  double cost_rp = 0.0;
  std::vector<std::string> prereqs;
  std::vector<Effect> effects;
  float x = 0.0f; // UI layout
  float y = 0.0f;

  static TechNode from_json(const json& j);
  json to_json() const;
};

} // namespace neon::techtree
