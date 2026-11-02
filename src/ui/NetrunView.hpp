#pragma once

#include <string>

namespace neon::core {
class Game;
}

namespace neon::ui {

// Netrun screen: grid, avatar, ICE, program bar, trace meter, log.
class NetrunView {
public:
  void render(neon::core::Game& game);

private:
  void render_target_picker(neon::core::Game& game);
  void render_session(neon::core::Game& game);
  std::string pending_target;
  bool pending_proxy = false;
};

} // namespace neon::ui
