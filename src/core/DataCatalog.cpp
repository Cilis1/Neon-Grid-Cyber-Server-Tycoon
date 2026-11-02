#include "core/DataCatalog.hpp"

#include "utils/FileSystem.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>

namespace neon::core {

EventDef EventDef::from_json(const json& j) {
  EventDef e;
  e.id = j.at("id").get<std::string>();
  e.name = j.value("name", e.id);
  e.description = j.value("description", "");
  e.weight = j.value("weight", 1.0);
  e.conditions = j.value("conditions", json::object());
  if (j.contains("effects") && j["effects"].is_array())
    for (auto& ef : j["effects"]) e.effects.push_back(techtree::Effect::from_json(ef));
  e.lua_script = j.value("lua_script", "");
  return e;
}

json EventDef::to_json() const {
  json effects_j = json::array();
  for (const auto& e : effects) effects_j.push_back(e.to_json());
  return json{{"id", id}, {"name", name}, {"description", description}, {"weight", weight},
              {"conditions", conditions}, {"effects", effects_j}, {"lua_script", lua_script}};
}

namespace {

template <typename T>
bool load_map_file(const std::filesystem::path& dir, const std::string& filename,
                   std::map<std::string, T>& out, std::string* error_out, bool required = true) {
  auto path = dir / filename;
  auto text = utils::fs::read_text_file(path);
  if (!text) {
    if (required && error_out) *error_out = "missing data file: " + path.string();
    return !required;
  }
  try {
    json j = json::parse(*text);
    const json* arr = &j;
    for (const char* key : {"items", "nodes", "templates"})
      if (j.is_object() && j.contains(key) && j[key].is_array()) arr = &j[key];
    if (!arr->is_array()) {
      if (error_out) *error_out = filename + ": expected array or {\"items\": [...]}";
      return false;
    }
    for (auto& item : *arr) {
      T def = T::from_json(item);
      out[def.id] = def;
    }
    return true;
  } catch (const std::exception& e) {
    if (error_out) *error_out = filename + ": parse error: " + e.what();
    return false;
  }
}

} // namespace

bool DataCatalog::load_from_dir(const std::filesystem::path& dir, std::string* error_out) {
  dir_ = dir;
  std::string err;
  bool ok = true;

  if (!load_map_file(dir, "hardware.json", hardware_, &err)) {
    spdlog::error("hardware.json: {}", err);
    ok = false;
  }
  if (!load_map_file(dir, "tech_tree.json", tech_, &err)) {
    spdlog::error("tech_tree.json: {}", err);
    ok = false;
  }
  if (!load_map_file(dir, "factions.json", factions_, &err)) {
    spdlog::error("factions.json: {}", err);
    ok = false;
  }
  if (!load_map_file(dir, "market_items.json", market_, &err)) {
    spdlog::error("market_items.json: {}", err);
    ok = false;
  }
  if (!load_map_file(dir, "netrun_programs.json", programs_, &err)) {
    spdlog::error("netrun_programs.json: {}", err);
    ok = false;
  }
  if (!load_map_file(dir, "events.json", events_, &err)) {
    spdlog::error("events.json: {}", err);
    ok = false;
  }
  // contracts is a plain vector
  {
    auto text = utils::fs::read_text_file(dir / "contracts.json");
    if (text) {
      try {
        json j = json::parse(*text);
        auto& arr = j.contains("templates") ? j["templates"] : j;
        for (auto& item : arr) contracts_.push_back(economy::ContractTemplate::from_json(item));
      } catch (const std::exception& e) {
        spdlog::error("contracts.json: parse error: {}", e.what());
        ok = false;
      }
    } else {
      spdlog::error("contracts.json missing");
      ok = false;
    }
  }

  loaded_ = ok;
  if (error_out) *error_out = err;
  if (ok) spdlog::info("data catalog loaded: {} hardware, {} tech nodes, {} contracts, {} factions, {} market items, {} programs, {} events",
                       hardware_.size(), tech_.size(), contracts_.size(), factions_.size(), market_.size(), programs_.size(), events_.size());
  return ok;
}

const hardware::HardwareItemDef* DataCatalog::hardware(const std::string& id) const {
  auto it = hardware_.find(id);
  return it == hardware_.end() ? nullptr : &it->second;
}
const techtree::TechNode* DataCatalog::tech_node(const std::string& id) const {
  auto it = tech_.find(id);
  return it == tech_.end() ? nullptr : &it->second;
}
const netrunning::ProgramDef* DataCatalog::program(const std::string& id) const {
  auto it = programs_.find(id);
  return it == programs_.end() ? nullptr : &it->second;
}
const economy::FactionDef* DataCatalog::faction(const std::string& id) const {
  auto it = factions_.find(id);
  return it == factions_.end() ? nullptr : &it->second;
}
const EventDef* DataCatalog::event(const std::string& id) const {
  auto it = events_.find(id);
  return it == events_.end() ? nullptr : &it->second;
}

} // namespace neon::core
