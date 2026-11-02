#pragma once

namespace neon::core {
class Game;
}

namespace neon::ui {

// Market (fluctuating prices, buy/sell) + hardware shop.
class MarketView {
public:
  // Renders the market content (called inside a tab in the main console).
  void render_content(neon::core::Game& game);
};

} // namespace neon::ui
