#pragma once

namespace neon::core {
class Game;
}

namespace neon::ui {

// Scrolling log window fed by the in-memory log buffer.
class LogWindow {
public:
  // Renders the log view (called inside a tab in the main console).
  void render_content(neon::core::Game& game);
};

} // namespace neon::ui
