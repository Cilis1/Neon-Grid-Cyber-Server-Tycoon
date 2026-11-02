#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace neon::economy {

using nlohmann::json;

struct FactionDef {
  std::string id;
  std::string name;
  std::string description;
  std::string color; // hex
  int base_rep = 0;
  std::vector<std::string> likes;   // preferred contract types
  std::vector<std::string> dislikes;

  static FactionDef from_json(const json& j);
  json to_json() const;
};

// Faction catalog + runtime reputation helpers.
class FactionTable {
public:
  void load(const std::map<std::string, FactionDef>& defs) { defs_ = defs; }
  const std::map<std::string, FactionDef>& defs() const { return defs_; }
  const FactionDef* get(const std::string& id) const;
  // Adjust reputation (clamped -100..100).
  static void adjust_rep(std::map<std::string, int>& rep, const std::string& faction_id, int delta);

private:
  std::map<std::string, FactionDef> defs_;
};

} // namespace neon::economy
