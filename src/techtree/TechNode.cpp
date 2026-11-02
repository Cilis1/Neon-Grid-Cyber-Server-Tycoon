#include "techtree/TechNode.hpp"

namespace neon::techtree {

Effect Effect::from_json(const json& j) {
  Effect e;
  e.raw = j;
  e.type = j.value("type", "");
  // Common alias keys: stat/id/key/target
  e.key = j.value("stat", j.value("id", j.value("key", j.value("target", ""))));
  e.value = j.value("value", 0.0);
  return e;
}

json Effect::to_json() const { return raw; }

TechNode TechNode::from_json(const json& j) {
  TechNode n;
  n.id = j.at("id").get<std::string>();
  n.name = j.value("name", n.id);
  n.description = j.value("description", "");
  n.branch = j.value("branch", "Software");
  n.tier = j.value("tier", 1);
  n.cost_credits = j.value("cost_credits", 0.0);
  n.cost_rp = j.value("cost_rp", 0.0);
  if (j.contains("prereqs") && j["prereqs"].is_array())
    for (auto& p : j["prereqs"]) n.prereqs.push_back(p.get<std::string>());
  if (j.contains("effects") && j["effects"].is_array())
    for (auto& e : j["effects"]) n.effects.push_back(Effect::from_json(e));
  n.x = j.value("x", 0.0f);
  n.y = j.value("y", 0.0f);
  return n;
}

json TechNode::to_json() const {
  json effects_j = json::array();
  for (const auto& e : effects) effects_j.push_back(e.to_json());
  return json{
      {"id", id}, {"name", name}, {"description", description}, {"branch", branch}, {"tier", tier},
      {"cost_credits", cost_credits}, {"cost_rp", cost_rp}, {"prereqs", prereqs}, {"effects", effects_j},
      {"x", x}, {"y", y},
  };
}

} // namespace neon::techtree
