#pragma once

#include <string>

namespace neon::core {
class Game;
}

namespace neon::ui {

// Contract board: offers, active contracts with progress, courier minigame.
class ContractView {
public:
  void render(neon::core::Game& game);

private:
  std::string courier_contract_uid; // contract currently picking a route
};

} // namespace neon::ui
