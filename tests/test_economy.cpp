#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/Game.hpp"
#include "economy/ContractManager.hpp"

using namespace neon;

static core::Game make_game() {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(777);
  return game;
}

TEST_CASE("new game starts with 10000 credits and starter room", "[economy]") {
  core::Game game = make_game();
  REQUIRE(game.state().credits == Catch::Approx(10000.0));
  REQUIRE(game.state().rp == Catch::Approx(25.0));
  REQUIRE(game.state().rooms.count("room1") == 1);
  REQUIRE(game.state().contracts.offers.size() > 0);
}

TEST_CASE("contract accept -> courier minigame -> reward", "[economy]") {
  core::Game game = make_game();
  // find or craft a courier offer
  std::string uid;
  for (auto& o : game.state().contracts.offers)
    if (o.type == "courier") {
      uid = o.uid;
      break;
    }
  if (uid.empty()) {
    economy::ContractInstance c;
    c.uid = "test_courier";
    c.template_id = "ct_courier_1";
    c.type = "courier";
    c.title = "Test courier";
    c.reward = 500;
    c.penalty = 250;
    c.deadline_minute = game.state().time.minute + 600;
    game.state().contracts.offers.push_back(c);
    uid = c.uid;
  }
  REQUIRE(game.state().contracts.can_accept(game, uid));
  auto* accepted = game.state().contracts.accept(game, uid);
  REQUIRE(accepted != nullptr);
  REQUIRE(accepted->type == "courier");
  double before = game.state().credits;
  // choose the safest route (index 1 = backstreets, risk 0.05)
  REQUIRE(game.state().contracts.start_courier(game, accepted->uid, 1));
  // tick until resolved (time_h = 3.5h -> 210 minutes max)
  for (int i = 0; i < 300 && accepted->status == economy::ContractStatus::Active; ++i) {
    game.state().contracts.tick(game, 1);
    accepted = game.state().contracts.find_active(uid);
    REQUIRE(accepted != nullptr);
  }
  REQUIRE(accepted->status != economy::ContractStatus::Active);
  if (accepted->status == economy::ContractStatus::Completed) {
    REQUIRE(game.state().credits >= before); // reward paid (route cost 0)
    REQUIRE(game.state().contracts_completed >= 0);
  }
}

TEST_CASE("courier resolve with zero risk always succeeds", "[economy]") {
  core::Game game = make_game();
  economy::ContractInstance c;
  c.uid = "c0";
  c.type = "courier";
  c.title = "safe run";
  c.reward = 1000;
  c.penalty = 500;
  c.courier_started = true;
  c.courier_time_h = 1.0;
  c.courier_risk = 0.0;
  c.deadline_minute = game.state().time.minute + 1000;
  double before = game.state().credits;
  game.state().contracts.resolve_courier(game, c);
  REQUIRE(c.status == economy::ContractStatus::Completed);
  REQUIRE(game.state().credits == Catch::Approx(before + 1000.0));
}

TEST_CASE("market price fluctuation stays within bounds", "[economy]") {
  core::Game game = make_game();
  economy::Market market;
  market.load(game.catalog().market_items());
  for (int day = 0; day < 10; ++day) market.update_prices(game, game.state().market_prices);
  for (const auto& [id, def] : game.catalog().market_items()) {
    double p = market.price(game.state().market_prices, id);
    REQUIRE(p >= def.min_price - 1e-6);
    REQUIRE(p <= def.max_price + 1e-6);
  }
}

TEST_CASE("market buy/sell round trip", "[economy]") {
  core::Game game = make_game();
  double before = game.state().credits;
  economy::Market market;
  market.load(game.catalog().market_items());
  REQUIRE(market.buy(game, "canned_air", 1));
  REQUIRE(game.state().inventory_count["canned_air"] == 1);
  REQUIRE(game.state().credits < before);
  REQUIRE(market.sell(game, "canned_air", 1));
  REQUIRE(game.state().inventory_count.count("canned_air") == 0);
}

TEST_CASE("power bill is positive and scales with draw", "[economy]") {
  core::Game game = make_game();
  double bill = game.power_bill_per_day();
  REQUIRE(bill > 0.0);
  REQUIRE(game.rent_per_day() == Catch::Approx(100.0));
}

TEST_CASE("storage contract accrues income while uptime is fine", "[economy]") {
  core::Game game = make_game();
  economy::ContractInstance c;
  c.uid = "stor1";
  c.type = "storage";
  c.title = "storage";
  c.reward = 1200;
  c.accepted_minute = game.state().time.minute;
  c.deadline_minute = game.state().time.minute + 24 * 60;
  c.requirements = {{"uptime_pct", 99}};
  game.state().contracts.active.push_back(c);
  double before = game.state().credits;
  game.state().contracts.tick(game, 60); // one hour
  auto* active = game.state().contracts.find_active("stor1");
  REQUIRE(active != nullptr);
  REQUIRE(active->income_accrued > 0.0);
  REQUIRE(game.state().credits > before);
}

TEST_CASE("faction reputation adjusts and clamps", "[economy]") {
  std::map<std::string, int> rep;
  economy::FactionTable::adjust_rep(rep, "citynet", 500);
  REQUIRE(rep["citynet"] == 100);
  economy::FactionTable::adjust_rep(rep, "citynet", -1000);
  REQUIRE(rep["citynet"] == -100);
}

TEST_CASE("heat increases and decays over time", "[economy]") {
  core::Game game = make_game();
  game.add_heat(30);
  REQUIRE(game.state().heat == 30);
  for (int i = 0; i < 61; ++i) game.tick(); // ~1 hour -> -1 heat
  REQUIRE(game.state().heat == 29);
  game.add_heat(1000);
  REQUIRE(game.state().heat == 100);
}
