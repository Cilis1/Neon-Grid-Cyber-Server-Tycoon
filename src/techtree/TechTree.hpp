#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "techtree/TechNode.hpp"

namespace neon::core {
class Game;
}

namespace neon::techtree {

// Owns the catalog of tech nodes plus the player's purchased set.
class TechTree {
public:
  void load(const std::map<std::string, TechNode>& nodes);

  const std::map<std::string, TechNode>& nodes() const { return nodes_; }
  const TechNode* node(const std::string& id) const;

  bool is_purchased(const std::set<std::string>& purchased, const std::string& id) const;
  // All prereqs of `id` are in `purchased`.
  bool prereqs_met(const std::set<std::string>& purchased, const std::string& id) const;
  // Can the player afford + unlock this node right now?
  bool can_purchase(const neon::core::Game& game, const std::string& id) const;
  // Purchase: deducts credits/RP, applies effects. Returns false if not allowed.
  bool purchase(neon::core::Game& game, const std::string& id);

  // Nodes purchasable right now (for UI highlighting).
  std::vector<std::string> available(const neon::core::Game& game) const;

private:
  std::map<std::string, TechNode> nodes_;
};

} // namespace neon::techtree
