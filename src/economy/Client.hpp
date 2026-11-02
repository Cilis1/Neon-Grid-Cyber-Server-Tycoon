#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace neon::core {
class Game;
}

namespace neon::economy {

using nlohmann::json;

// A contract-giving client (flavor + faction affiliation).
struct Client {
  std::string id;
  std::string name;
  std::string faction_id;
  std::string handle;

  static Client generate(neon::core::Game& game, const std::string& faction_id);
  json to_json() const;
  static Client from_json(const json& j);
};

} // namespace neon::economy
