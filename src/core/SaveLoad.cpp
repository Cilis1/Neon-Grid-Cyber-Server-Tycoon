#include "core/SaveLoad.hpp"

#include "core/Game.hpp"
#include "utils/FileSystem.hpp"

#include <spdlog/spdlog.h>

namespace neon::core {

std::filesystem::path default_save_path() { return utils::fs::save_dir() / "savegame.json"; }

json serialize_state(const GameState& state) { return state.to_json(); }

GameState deserialize_state(const json& j) { return GameState::from_json(j); }

bool save_game(const Game& game, const std::filesystem::path& path) {
  try {
    if (!game.save(path)) return false;
    spdlog::info("game saved to {}", path.string());
    return true;
  } catch (const std::exception& e) {
    spdlog::error("save failed: {}", e.what());
    return false;
  }
}

std::optional<GameState> load_game_state(const std::filesystem::path& path) {
  auto text = utils::fs::read_text_file(path);
  if (!text) {
    spdlog::warn("save file not found: {}", path.string());
    return std::nullopt;
  }
  try {
    json j = json::parse(*text);
    GameState s = GameState::from_json(j);
    if (s.version > SAVE_VERSION) {
      spdlog::error("save version {} is newer than this build ({})", s.version, SAVE_VERSION);
      return std::nullopt;
    }
    // migrations
    if (s.version < 2 && s.unlocked_contract_types.empty())
      s.unlocked_contract_types = {"courier", "storage", "hosting"};
    s.version = SAVE_VERSION;
    return s;
  } catch (const std::exception& e) {
    spdlog::error("save file corrupt ({}): {}", path.string(), e.what());
    return std::nullopt;
  }
}

bool autosave(const Game& game) { return save_game(game, default_save_path()); }

} // namespace neon::core
