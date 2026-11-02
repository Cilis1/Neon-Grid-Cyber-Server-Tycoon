#pragma once

namespace neon::core {
class Game;
}

namespace neon::ui {

// Scrolling log window fed by the in-memory log buffer.
class LogWindow {
public:
  void render(neon::core::Game& game);
  bool visible = true;
};

} // namespace neon::ui
