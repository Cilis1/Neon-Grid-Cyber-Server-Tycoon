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
  // Inline top status bar (rendered at the top of the main console window).
  void render_top_bar_content(neon::core::Game& game);
  // HQ tab content (employees / remote servers / cyberware / factions / targets).
  void render_hq_content(neon::core::Game& game);

private:
  std::string selected_employee;
};

} // namespace neon::ui
