#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "economy/Contract.hpp"
#include "economy/Faction.hpp"
#include "economy/Market.hpp"
#include "hardware/HardwareComponent.hpp"
#include "netrunning/Program.hpp"
#include "techtree/TechNode.hpp"

namespace neon::core {

using nlohmann::json;

// Random event definition (data/events.json).
struct EventDef {
  std::string id;
  std::string name;
  std::string description;
  double weight = 1.0;
  json conditions;               // e.g. {"heat_min": 50, "time_of_day": "night"}
  std::vector<techtree::Effect> effects;
  std::string lua_script;        // optional scripts/lua/events/<file>.lua

  static EventDef from_json(const json& j);
  json to_json() const;
};

// Loads and validates all data/*.json files. Throws std::runtime_error on
// missing/invalid files (the game falls back to defaults with an error log).
class DataCatalog {
public:
  bool load_from_dir(const std::filesystem::path& dir, std::string* error_out = nullptr);
  bool loaded() const { return loaded_; }
  const std::filesystem::path& dir() const { return dir_; }

  const std::map<std::string, hardware::HardwareItemDef>& hardware_items() const { return hardware_; }
  const hardware::HardwareItemDef* hardware(const std::string& id) const;

  const std::map<std::string, techtree::TechNode>& tech_nodes() const { return tech_; }
  const techtree::TechNode* tech_node(const std::string& id) const;

  const std::vector<economy::ContractTemplate>& contracts() const { return contracts_; }
  const std::map<std::string, economy::FactionDef>& factions() const { return factions_; }
  const std::map<std::string, economy::MarketItemDef>& market_items() const { return market_; }
  const std::map<std::string, netrunning::ProgramDef>& programs() const { return programs_; }
  const std::map<std::string, EventDef>& events() const { return events_; }

  const netrunning::ProgramDef* program(const std::string& id) const;
  const economy::FactionDef* faction(const std::string& id) const;
  const EventDef* event(const std::string& id) const;

private:
  bool loaded_ = false;
  std::filesystem::path dir_;
  std::map<std::string, hardware::HardwareItemDef> hardware_;
  std::map<std::string, techtree::TechNode> tech_;
  std::vector<economy::ContractTemplate> contracts_;
  std::map<std::string, economy::FactionDef> factions_;
  std::map<std::string, economy::MarketItemDef> market_;
  std::map<std::string, netrunning::ProgramDef> programs_;
  std::map<std::string, EventDef> events_;
};

} // namespace neon::core
