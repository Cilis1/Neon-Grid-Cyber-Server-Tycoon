#include "netrunning/ICE.hpp"

namespace neon::netrunning {

json ICE::to_json() const {
  json patrol_j = json::array();
  for (auto& p : patrol) patrol_j.push_back({p.first, p.second});
  return json{{"uid", uid}, {"x", x}, {"y", y}, {"hp", hp}, {"max_hp", max_hp}, {"damage", damage},
              {"alert", alert}, {"patrol", patrol_j}, {"patrol_idx", patrol_idx}};
}

ICE ICE::from_json(const json& j) {
  ICE ice;
  ice.uid = j.at("uid").get<std::string>();
  ice.x = j.value("x", 0);
  ice.y = j.value("y", 0);
  ice.hp = j.value("hp", 20);
  ice.max_hp = j.value("max_hp", ice.hp);
  ice.damage = j.value("damage", 10);
  ice.alert = j.value("alert", false);
  if (j.contains("patrol") && j["patrol"].is_array())
    for (auto& p : j["patrol"]) ice.patrol.emplace_back(p.at(0).get<int>(), p.at(1).get<int>());
  ice.patrol_idx = j.value("patrol_idx", 0);
  return ice;
}

} // namespace neon::netrunning
