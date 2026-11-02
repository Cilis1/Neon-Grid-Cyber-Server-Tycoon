#include "economy/Market.hpp"

#include "core/Game.hpp"
#include "utils/Random.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace neon::economy {

MarketItemDef MarketItemDef::from_json(const json& j) {
  MarketItemDef d;
  d.id = j.at("id").get<std::string>();
  d.name = j.value("name", d.id);
  d.category = j.value("category", "misc");
  d.min_price = j.value("min_price", 0.0);
  d.max_price = j.value("max_price", 100.0);
  d.volatility = j.value("volatility", 0.1);
  d.black_market = j.value("black_market", false);
  d.description = j.value("description", "");
  return d;
}

json MarketItemDef::to_json() const {
  return json{{"id", id}, {"name", name}, {"category", category}, {"min_price", min_price},
              {"max_price", max_price}, {"volatility", volatility}, {"black_market", black_market},
              {"description", description}};
}

void Market::load(const std::map<std::string, MarketItemDef>& defs) { defs_ = defs; }

const MarketItemDef* Market::def(const std::string& id) const {
  auto it = defs_.find(id);
  return it == defs_.end() ? nullptr : &it->second;
}

double Market::price(const std::map<std::string, double>& prices, const std::string& id) const {
  auto it = prices.find(id);
  if (it != prices.end()) return it->second;
  const MarketItemDef* d = def(id);
  if (!d) return 0.0;
  return (d->min_price + d->max_price) / 2.0;
}

void Market::update_prices(neon::core::Game& game, std::map<std::string, double>& prices) const {
  auto& rng = game.rng();
  bool night = game.state().time.is_night();
  for (const auto& [id, d] : defs_) {
    double base = price(prices, id);
    if (base <= 0.0) base = (d.min_price + d.max_price) / 2.0;
    double swing = 1.0 + (rng.next_double() * 2.0 - 1.0) * d.volatility;
    if (night && (d.category == "data" || d.black_market)) swing *= 1.15; // black market livelier at night
    double p = base * swing;
    p = std::clamp(p, d.min_price, d.max_price);
    prices[id] = p;
  }
}

bool Market::buy(neon::core::Game& game, const std::string& item_id, int qty) {
  const MarketItemDef* d = def(item_id);
  if (!d) return false;
  double p = price(game.state().market_prices, item_id) * qty;
  if (game.state().credits < p) return false;
  game.state().credits -= p;
  game.state().inventory_count[item_id] += qty;
  if (d->black_market) game.add_heat(2);
  spdlog::info("bought {} x{} for {:.0f}", item_id, qty, p);
  return true;
}

bool Market::sell(neon::core::Game& game, const std::string& item_id, int qty) {
  auto it = game.state().inventory_count.find(item_id);
  if (it == game.state().inventory_count.end() || it->second < qty) return false;
  const MarketItemDef* d = def(item_id);
  double p = price(game.state().market_prices, item_id) * qty * 0.9; // 10% spread
  it->second -= qty;
  if (it->second <= 0) game.state().inventory_count.erase(it);
  game.state().credits += p;
  if (d && d->black_market) game.add_heat(1);
  spdlog::info("sold {} x{} for {:.0f}", item_id, qty, p);
  return true;
}

json Market::to_json() const {
  json j = json::object();
  for (const auto& [id, d] : defs_) j[id] = d.to_json();
  return j;
}

} // namespace neon::economy
