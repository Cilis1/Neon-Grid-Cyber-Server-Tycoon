#pragma once

namespace neon::core {
class Game;
}

namespace neon::ui {

// Result of the main menu for one frame.
enum class MenuAction { None, NewGame, LoadGame, Quit };

class MainMenu {
public:
  // Renders the main menu. Returns the action the player chose this frame.
  MenuAction render(neon::core::Game& game, bool has_save);

  // Settings dialog (also used in-game). Returns true while open.
  bool render_settings(neon::core::Game& game);

  bool settings_open = false;
};

} // namespace neon::ui
