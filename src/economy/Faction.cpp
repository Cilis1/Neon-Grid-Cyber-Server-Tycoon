#include "economy/Faction.hpp"

#include <algorithm>

namespace neon::economy {

FactionDef FactionDef::from_json(const json& j) {
  FactionDef f;
  f.id = j.at("id").get<std::string>();
  f.name = j.value("name", f.id);
  f.description = j.value("description", "");
  f.color = j.value("color", "#00F0FF");
  f.base_rep = j.value("base_rep", 0);
  if (j.contains("likes") && j["likes"].is_array())
    for (auto& v : j["likes"]) f.likes.push_back(v.get<std::string>());
  if (j.contains("dislikes") && j["dislikes"].is_array())
    for (auto& v : j["dislikes"]) f.dislikes.push_back(v.get<std::string>());
  return f;
}

json FactionDef::to_json() const {
  return json{{"id", id}, {"name", name}, {"description", description}, {"color", color},
              {"base_rep", base_rep}, {"likes", likes}, {"dislikes", dislikes}};
}

const FactionDef* FactionTable::get(const std::string& id) const {
  auto it = defs_.find(id);
  return it == defs_.end() ? nullptr : &it->second;
}

void FactionTable::adjust_rep(std::map<std::string, int>& rep, const std::string& faction_id, int delta) {
  int& r = rep[faction_id];
  r = std::clamp(r + delta, -100, 100);
}

} // namespace neon::economy
