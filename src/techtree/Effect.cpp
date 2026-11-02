#include "techtree/Effect.hpp"

#include "core/Game.hpp"

#include <spdlog/spdlog.h>

namespace neon::techtree {

// Applies one effect to the game. Returns a short human-readable description.
std::string apply_effect(neon::core::Game& game, Effect effect) {
  auto& s = game.state();
  const std::string& t = effect.type;
  const std::string& k = effect.key;
  double v = effect.value;

  if (t == "credits" || t == "add_credits") {
    s.credits += v;
    return "credits " + std::to_string(static_cast<int>(v));
  }
  if (t == "rp" || t == "add_rp") {
    s.rp += v;
    return "rp " + std::to_string(static_cast<int>(v));
  }
  if (t == "data_shards" || t == "add_shards") {
    s.data_shards += v;
    return "data shards " + std::to_string(static_cast<int>(v));
  }
  if (t == "heat") {
    game.add_heat(static_cast<int>(v));
    return "heat " + std::to_string(static_cast<int>(v));
  }
  if (t == "unlock_hardware") {
    s.unlocked_items.insert(k);
    return "unlocked hardware " + k;
  }
  if (t == "unlock_program") {
    if (std::find(s.unlocked_programs.begin(), s.unlocked_programs.end(), k) == s.unlocked_programs.end())
      s.unlocked_programs.push_back(k);
    return "unlocked program " + k;
  }
  if (t == "unlock_contract_type") {
    s.unlocked_contract_types.insert(k);
    return "unlocked contract type " + k;
  }
  if (t == "unlock_cyberware") {
    s.cyberware[k] = true;
    return "cyberware " + k;
  }
  if (t == "stat_multiplier") {
    s.stat_multipliers[k] = game.get_multiplier(k) * v;
    return k + " x" + std::to_string(v);
  }
  if (t == "passive_income") {
    s.passive_income_per_day += v;
    return "passive income +" + std::to_string(static_cast<int>(v)) + "/day";
  }
  if (t == "damage_hardware") {
    // damage a random component in a random room
    std::vector<std::pair<neon::hardware::Room*, neon::hardware::Component*>> candidates;
    for (auto& [id, r] : s.rooms)
      for (auto& c : r.components)
        if (c.online) candidates.emplace_back(&r, &c);
    if (!candidates.empty()) {
      auto& [r, c] = candidates[game.rng().next_int(0, static_cast<int>(candidates.size()) - 1)];
      c->durability -= static_cast<int>(v);
      if (c->durability <= 0) {
        c->durability = 0;
        c->online = false;
      }
      return "damaged " + c->def_id;
    }
    return "";
  }
  if (t == "repair_all") {
    for (auto& [id, r] : s.rooms)
      for (auto& c : r.components) {
        c.durability = 100;
        c.online = true;
      }
    return "all hardware repaired";
  }
  if (t == "power_outage") {
    for (auto& [id, r] : s.rooms) {
      r.breaker_tripped = true;
      for (auto& c : r.components) c.powered = false;
    }
    return "city-wide blackout!";
  }
  if (t == "reputation") {
    s.reputation += static_cast<int>(v);
    return "reputation " + std::to_string(static_cast<int>(v));
  }
  // All remaining known additive bonuses:
  static const std::vector<std::string> additive = {
      "max_ram_bonus", "program_slots", "shield_bonus", "ice_damage_bonus", "action_points",
      "trace_reduction", "trace_time_bonus", "cooling_efficiency", "power_efficiency", "heat_reduction",
      "contract_slots", "employee_slots", "mining_bonus", "income_bonus", "detection_reduction",
      "cpu_speed_bonus", "storage_bonus", "bandwidth_bonus",
  };
  for (const auto& a : additive) {
    if (t == a || t == a.substr(0, a.size())) {
      if (t == a) {
        s.stat_bonuses[a] += v;
        return a + " +" + std::to_string(v);
      }
    }
  }
  // unknown effect: may be handled by Lua
  return "";
}

} // namespace neon::techtree
