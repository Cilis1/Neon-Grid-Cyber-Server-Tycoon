#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace neon::core {
class Game;
struct GameState;
} // namespace neon::core

namespace neon::techtree {

using nlohmann::json;

// A single data-driven effect, e.g. {"type":"stat_multiplier","stat":"cpu_speed","value":1.1}
struct Effect {
  std::string type;
  std::string key;   // stat name / item id / program id depending on type
  double value = 0.0;
  json raw;          // original object (for Lua-driven or unknown effects)

  static Effect from_json(const json& j);
  json to_json() const;
};

// Applies an effect to the game state. Returns a human-readable description of
// what happened (for logs / UI toasts). Unknown types are ignored (they may be
// handled by Lua scripts instead).
std::string apply_effect(neon::core::Game& game, Effect effect);

} // namespace neon::techtree
