#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "core/GameState.hpp"

namespace neon::core {

class Game;

// JSON save/load helpers. Save file: %LOCALAPPDATA%\NeonGrid\saves\savegame.json
std::filesystem::path default_save_path();

bool save_game(const Game& game, const std::filesystem::path& path);
// Returns the loaded state; nullopt on failure (missing/corrupt/newer version).
std::optional<GameState> load_game_state(const std::filesystem::path& path);
// Convenience: save to the default location.
bool autosave(const Game& game);

// Serialize/deserialize raw state (used by tests for round-trip checks).
json serialize_state(const GameState& state);
GameState deserialize_state(const json& j);

} // namespace neon::core
