#pragma once

namespace neon::core {
class Game;
}

namespace neon::ui {

// Market (fluctuating prices, buy/sell) + hardware shop.
class MarketView {
public:
  void render(neon::core::Game& game);
};

} // namespace neon::ui
