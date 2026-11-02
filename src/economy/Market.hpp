#pragma once

#include <map>
#include <string>

#include <nlohmann/json.hpp>

namespace neon::core {
class Game;
}

namespace neon::economy {

using nlohmann::json;

struct MarketItemDef {
  std::string id;
  std::string name;
  std::string category; // data, crypto, consumable, hardware, shard
  double min_price = 0.0;
  double max_price = 0.0;
  double volatility = 0.1; // daily price swing fraction
  bool black_market = false;
  std::string description;

  static MarketItemDef from_json(const json& j);
  json to_json() const;
};

// Market with daily-fluctuating prices. Buy/sell data shards, crypto, consumables.
class Market {
public:
  void load(const std::map<std::string, MarketItemDef>& defs);
  const std::map<std::string, MarketItemDef>& defs() const { return defs_; }
  const MarketItemDef* def(const std::string& id) const;

  double price(const std::map<std::string, double>& prices, const std::string& id) const;

  // Re-roll all prices (called once per in-game day).
  void update_prices(neon::core::Game& game, std::map<std::string, double>& prices) const;

  // Buy one unit of `item_id` at current price. Returns false if unaffordable/unknown.
  bool buy(neon::core::Game& game, const std::string& item_id, int qty = 1);
  // Sell one unit. Returns false if the player doesn't own it.
  bool sell(neon::core::Game& game, const std::string& item_id, int qty = 1);

  json to_json() const;

private:
  std::map<std::string, MarketItemDef> defs_;
};

} // namespace neon::economy
