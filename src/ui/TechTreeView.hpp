#pragma once

#include <string>

namespace neon::core {
class Game;
}

namespace neon::ui {

// Scrollable/zoomable tech tree canvas with pan, node details and purchase.
class TechTreeView {
public:
  void render(neon::core::Game& game);

private:
  float pan_x = 40.0f;
  float pan_y = 40.0f;
  float zoom = 1.0f;
  std::string selected_node;
};

} // namespace neon::ui
