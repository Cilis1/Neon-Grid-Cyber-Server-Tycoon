#include "techtree/TechTree.hpp"

#include "core/Game.hpp"

#include <spdlog/spdlog.h>

namespace neon::techtree {

void TechTree::load(const std::map<std::string, TechNode>& nodes) { nodes_ = nodes; }

const TechNode* TechTree::node(const std::string& id) const {
  auto it = nodes_.find(id);
  return it == nodes_.end() ? nullptr : &it->second;
}

bool TechTree::is_purchased(const std::set<std::string>& purchased, const std::string& id) const {
  return purchased.count(id) > 0;
}

bool TechTree::prereqs_met(const std::set<std::string>& purchased, const std::string& id) const {
  const TechNode* n = node(id);
  if (!n) return false;
  for (const auto& p : n->prereqs)
    if (!purchased.count(p)) return false;
  return true;
}

bool TechTree::can_purchase(const neon::core::Game& game, const std::string& id) const {
  const TechNode* n = node(id);
  if (!n) return false;
  if (is_purchased(game.state().purchased_tech, id)) return false;
  if (!prereqs_met(game.state().purchased_tech, id)) return false;
  return game.state().credits >= n->cost_credits && game.state().rp >= n->cost_rp;
}

bool TechTree::purchase(neon::core::Game& game, const std::string& id) {
  if (!can_purchase(game, id)) return false;
  const TechNode* n = node(id);
  game.state().credits -= n->cost_credits;
  game.state().rp -= n->cost_rp;
  game.state().purchased_tech.insert(id);
  for (const auto& e : n->effects) {
    std::string desc = apply_effect(game, e);
    if (!desc.empty()) spdlog::info("research '{}' effect: {}", n->name, desc);
  }
  game.bus().publish("tech.purchased", {{"id", id}, {"name", n->name}});
  return true;
}

std::vector<std::string> TechTree::available(const neon::core::Game& game) const {
  std::vector<std::string> out;
  for (const auto& [id, n] : nodes_) {
    (void)n;
    if (can_purchase(game, id)) out.push_back(id);
  }
  return out;
}

} // namespace neon::techtree
