#pragma once

#include <string>

namespace neon::core {
class Game;
}

namespace neon::ui {

// Contract board: offers, active contracts with progress, courier minigame.
class ContractView {
public:
  // Renders the contract board content (called inside a tab in the main console).
  void render_content(neon::core::Game& game);

private:
  std::string courier_contract_uid; // contract currently picking a route
};

} // namespace neon::ui
