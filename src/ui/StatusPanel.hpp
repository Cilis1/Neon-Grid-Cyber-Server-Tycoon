#pragma once

#include <string>

namespace neon::core {
class Game;
}

namespace neon::ui {

// Top status bar (credits/RP/heat/time/speed) + HQ tabs (employees, remote
// servers, cyberware, factions, bribes).
class StatusPanel {
public:
  bool hq_visible = true;
  void render(neon::core::Game& game);

private:
  void render_top_bar(neon::core::Game& game);
  void render_hq(neon::core::Game& game);
  std::string selected_employee;
};

} // namespace neon::ui
