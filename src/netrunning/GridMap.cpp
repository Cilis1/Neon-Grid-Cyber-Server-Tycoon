#include "netrunning/GridMap.hpp"

#include "core/Game.hpp"
#include "utils/Random.hpp"

namespace neon::netrunning {

json NetrunNode::to_json() const {
  return json{{"type", static_cast<int>(type)}, {"visited", visited}, {"looted", looted}, {"data_value", data_value}};
}

NetrunNode NetrunNode::from_json(const json& j) {
  NetrunNode n;
  n.type = static_cast<NodeType>(j.value("type", 0));
  n.visited = j.value("visited", false);
  n.looted = j.value("looted", false);
  n.data_value = j.value("data_value", 0);
  return n;
}

json NetrunTarget::to_json() const {
  return json{{"id", id}, {"name", name}, {"kind", kind}, {"faction_id", faction_id}, {"difficulty", difficulty},
              {"reward_credits", reward_credits}, {"reward_shards", reward_shards}, {"claimed", claimed},
              {"passive_income", passive_income}, {"heat_risk", heat_risk}, {"proxy", proxy}};
}

NetrunTarget NetrunTarget::from_json(const json& j) {
  NetrunTarget t;
  t.id = j.at("id").get<std::string>();
  t.name = j.value("name", t.id);
  t.kind = j.value("kind", "abandoned");
  t.faction_id = j.value("faction_id", "");
  t.difficulty = j.value("difficulty", 1);
  t.reward_credits = j.value("reward_credits", 0);
  t.reward_shards = j.value("reward_shards", 0);
  t.claimed = j.value("claimed", false);
  t.passive_income = j.value("passive_income", 0.0);
  t.heat_risk = j.value("heat_risk", 0.0);
  t.proxy = j.value("proxy", false);
  return t;
}

bool GridMap::in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }

NetrunNode* GridMap::at(int x, int y) {
  if (!in_bounds(x, y)) return nullptr;
  return &nodes[static_cast<std::size_t>(y * width + x)];
}

const NetrunNode* GridMap::at(int x, int y) const {
  if (!in_bounds(x, y)) return nullptr;
  return &nodes[static_cast<std::size_t>(y * width + x)];
}

GridMap GridMap::generate(neon::core::Game& game, const NetrunTarget& target) {
  auto& rng = game.rng();
  GridMap map;
  map.width = 12;
  map.height = 8;
  map.nodes.assign(static_cast<std::size_t>(map.width) * map.height, NetrunNode{});

  map.entry = {0, map.height / 2};
  map.root = {map.width - 1, map.height / 2};
  map.at(map.entry.first, map.entry.second)->type = NodeType::Entry;
  map.at(map.root.first, map.root.second)->type = NodeType::Root;

  // Scatter data / control / ICE nodes.
  int data_nodes = 3 + target.difficulty;
  int ice_nodes = 2 + target.difficulty * 2;
  int control_nodes = 1 + target.difficulty / 2;

  auto random_empty = [&]() -> std::pair<int, int> {
    for (int tries = 0; tries < 200; ++tries) {
      int x = rng.next_int(1, map.width - 2);
      int y = rng.next_int(0, map.height - 1);
      if (map.at(x, y)->type == NodeType::Empty && !(x == map.entry.first && y == map.entry.second) &&
          !(x == map.root.first && y == map.root.second))
        return {x, y};
    }
    return {1, 1};
  };

  for (int i = 0; i < data_nodes; ++i) {
    auto [x, y] = random_empty();
    auto* n = map.at(x, y);
    n->type = NodeType::Data;
    n->data_value = rng.next_int(1, 3) + target.difficulty;
  }
  for (int i = 0; i < control_nodes; ++i) {
    auto [x, y] = random_empty();
    map.at(x, y)->type = NodeType::Control;
  }
  for (int i = 0; i < ice_nodes; ++i) {
    auto [x, y] = random_empty();
    map.at(x, y)->type = NodeType::ICE;
    ICE ice;
    ice.uid = "ice" + std::to_string(i);
    ice.x = x;
    ice.y = y;
    ice.max_hp = ice.hp = 10 + target.difficulty * 8;
    ice.damage = 6 + target.difficulty * 3;
    // small patrol path around the spawn
    ice.patrol = {{x, y}, {x, std::min(map.height - 1, y + 1)}, {x, std::max(0, y - 1)}};
    map.ices.push_back(ice);
  }
  // exit node adjacent to root
  if (map.root.first - 1 >= 0 && map.at(map.root.first - 1, map.root.second)->type == NodeType::Empty)
    map.at(map.root.first - 1, map.root.second)->type = NodeType::Exit;

  return map;
}

json GridMap::to_json() const {
  json nodes_j = json::array();
  for (const auto& n : nodes) nodes_j.push_back(n.to_json());
  json ices_j = json::array();
  for (const auto& i : ices) ices_j.push_back(i.to_json());
  return json{{"width", width}, {"height", height}, {"nodes", nodes_j}, {"ices", ices_j},
              {"entry", {entry.first, entry.second}}, {"root", {root.first, root.second}}};
}

GridMap GridMap::from_json(const json& j) {
  GridMap map;
  map.width = j.value("width", 12);
  map.height = j.value("height", 8);
  map.nodes.clear();
  if (j.contains("nodes") && j["nodes"].is_array())
    for (auto& n : j["nodes"]) map.nodes.push_back(NetrunNode::from_json(n));
  if (map.nodes.size() != static_cast<std::size_t>(map.width) * map.height)
    map.nodes.assign(static_cast<std::size_t>(map.width) * map.height, NetrunNode{});
  if (j.contains("ices") && j["ices"].is_array())
    for (auto& i : j["ices"]) map.ices.push_back(ICE::from_json(i));
  if (j.contains("entry") && j["entry"].is_array() && j["entry"].size() == 2)
    map.entry = {j["entry"][0].get<int>(), j["entry"][1].get<int>()};
  if (j.contains("root") && j["root"].is_array() && j["root"].size() == 2)
    map.root = {j["root"][0].get<int>(), j["root"][1].get<int>()};
  return map;
}

} // namespace neon::netrunning
