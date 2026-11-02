#pragma once

#include <string>

namespace neon::core {
class Game;
}

namespace neon::ui {

// The server room: tile grid, racks, components, cables, inventory, gauges.
class ServerRoomView {
public:
  // Server Room tab content: room switcher, gauges, grid + side panel.
  void render_content(neon::core::Game& game);
  // Inventory tab content.
  void render_inventory(neon::core::Game& game);

private:
  void render_room_tabs(neon::core::Game& game);
  void render_grid(neon::core::Game& game);
  void render_side_panel(neon::core::Game& game);
  void render_gauges(neon::core::Game& game);
  void render_component_detail(neon::core::Game& game, const std::string& uid);
  void render_rack_detail(neon::core::Game& game, const std::string& rack_uid);
  void render_server_detail(neon::core::Game& game, const std::string& server_uid);

  std::string selected_uid;      // selected component (room or inventory)
  std::string cable_from_uid;    // cable placement: first endpoint
  std::string cable_from_port = "power";
  bool cable_mode = false;
  int hover_tile = -1;
};

} // namespace neon::ui
