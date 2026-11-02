#include "hardware/HardwareComponent.hpp"

namespace neon::hardware {

double HardwareItemDef::stat(const std::string& key, double fallback) const {
  if (!stats.is_object()) return fallback;
  auto it = stats.find(key);
  if (it == stats.end() || !it->is_number()) return fallback;
  return it->get<double>();
}

HardwareItemDef HardwareItemDef::from_json(const json& j) {
  HardwareItemDef d;
  d.id = j.at("id").get<std::string>();
  d.name = j.value("name", d.id);
  d.type = j.value("type", "misc");
  d.tier = j.value("tier", 1);
  d.price = j.value("price", 0.0);
  d.power_draw = j.value("power_draw", 0.0);
  d.heat_output = j.value("heat_output", 0.0);
  d.durability = j.value("durability", 100.0);
  d.noise = j.value("noise", 0.0);
  d.stats = j.value("stats", json::object());
  d.size_u = j.value("size_u", 1);
  d.slot_type = j.value("slot_type", "");
  if (j.contains("compatibility") && j["compatibility"].is_array())
    for (auto& c : j["compatibility"]) d.compatibility.push_back(c.get<std::string>());
  d.description = j.value("description", "");
  return d;
}

json HardwareItemDef::to_json() const {
  return json{
      {"id", id},           {"name", name},         {"type", type},       {"tier", tier},
      {"price", price},     {"power_draw", power_draw}, {"heat_output", heat_output},
      {"durability", durability}, {"noise", noise}, {"stats", stats},     {"size_u", size_u},
      {"slot_type", slot_type},   {"compatibility", compatibility}, {"description", description},
  };
}

json Component::to_json() const {
  return json{
      {"uid", uid},
      {"def_id", def_id},
      {"tile_index", tile_index},
      {"parent_uid", parent_uid},
      {"slots", slots},
      {"installed", installed},
      {"dust_level", dust_level},
      {"dust_frac", dust_frac},
      {"wear_level", wear_level},
      {"durability", durability},
      {"powered", powered},
      {"online", online},
  };
}

Component Component::from_json(const json& j) {
  Component c;
  c.uid = j.at("uid").get<std::string>();
  c.def_id = j.at("def_id").get<std::string>();
  c.tile_index = j.value("tile_index", -1);
  c.parent_uid = j.value("parent_uid", "");
  if (j.contains("slots") && j["slots"].is_object())
    for (auto& [k, v] : j["slots"].items()) c.slots[k] = v.get<std::string>();
  if (j.contains("installed") && j["installed"].is_array())
    for (auto& v : j["installed"]) c.installed.push_back(v.get<std::string>());
  c.dust_level = j.value("dust_level", 0);
  c.dust_frac = j.value("dust_frac", 0.0);
  c.wear_level = j.value("wear_level", 0);
  c.durability = j.value("durability", 100);
  c.powered = j.value("powered", true);
  c.online = j.value("online", true);
  return c;
}

} // namespace neon::hardware
