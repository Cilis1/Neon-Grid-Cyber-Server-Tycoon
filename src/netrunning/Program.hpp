#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace neon::netrunning {

using nlohmann::json;

// Static definition of a netrun program (data/netrun_programs.json).
struct ProgramDef {
  std::string id;
  std::string name;
  std::string description;
  int ram_cost = 1;
  int cooldown = 0;      // turns
  std::string effect_type; // cloak, decrypt, attack, heal, data_spike, backdoor, shield_boost, trace_scrub
  double effect_value = 0.0;
  std::string rarity;    // common, uncommon, rare, epic, legendary
  std::string lua_script; // optional scripts/lua/programs/<file>.lua

  static ProgramDef from_json(const json& j);
  json to_json() const;
};

} // namespace neon::netrunning
