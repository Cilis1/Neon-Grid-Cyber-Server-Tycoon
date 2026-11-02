#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/Game.hpp"
#include "core/SaveLoad.hpp"

using namespace neon;

// Simulates 10 in-game days and checks the state stays valid.
TEST_CASE("10 in-game day simulation stays valid", "[integration]") {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(31337);

  // buy + install some hardware to make the sim interesting
  game.state().credits = 100000;
  game.state().unlocked_items.insert("hw_rack_t2");
  game.state().unlocked_items.insert("hw_cpu_t3");
  game.state().unlocked_items.insert("hw_gpu_t2");
  REQUIRE(game.buy_hardware("hw_cpu_t3"));
  std::string cpu_uid = game.state().inventory.back().uid;
  auto& room = game.active_room();
  std::string server_uid;
  for (auto& c : room.components)
    if (game.catalog().hardware(c.def_id)->type == "server_unit") server_uid = c.uid;
  REQUIRE(game.install_part(cpu_uid, server_uid, "cpu"));

  // accept a courier contract if offered
  for (auto& o : game.state().contracts.offers) {
    if (o.type == "courier") {
      if (auto* a = game.state().contracts.accept(game, o.uid)) {
        game.state().contracts.start_courier(game, a->uid, 1);
      }
      break;
    }
  }

  // research something
  REQUIRE(game.research("hw1"));

  const std::uint64_t ten_days = 10 * 1440;
  for (std::uint64_t i = 0; i < ten_days; ++i) {
    game.tick();
    // invariants every tick
    REQUIRE(game.state().heat >= 0);
    REQUIRE(game.state().heat <= 100);
    REQUIRE(std::isfinite(game.state().credits));
    REQUIRE(game.state().rooms.count("room1") == 1);
    for (const auto& [id, r] : game.state().rooms) {
      REQUIRE(r.temperature_c > -50.0);
      REQUIRE(r.temperature_c < 200.0);
      for (const auto& c : r.components) {
        REQUIRE(c.dust_level >= 0);
        REQUIRE(c.dust_level <= 100);
        REQUIRE(c.durability >= 0);
        REQUIRE(c.durability <= 100);
      }
    }
  }
  REQUIRE(game.state().time.day() == 10);
  // daily rollovers happened: offers refreshed, autosave day recorded
  REQUIRE(game.state().last_autosave_day == 10);
  REQUIRE(game.state().contracts.offers.size() > 0);
  // state must still serialize
  auto j = core::serialize_state(game.state());
  REQUIRE(j.contains("version"));
  auto s2 = core::deserialize_state(j);
  REQUIRE(s2.time.minute == game.state().time.minute);

  // a full save/load cycle mid-simulation
  auto path = std::filesystem::temp_directory_path() / "neongrid_integration_save.json";
  REQUIRE(core::save_game(game, path));
  core::Game game2;
  REQUIRE(game2.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  REQUIRE(game2.load(path));
  REQUIRE(game2.state().time.minute == game.state().time.minute);
  REQUIRE(game2.state().credits == Catch::Approx(game.state().credits));
  std::filesystem::remove(path);
}

TEST_CASE("netrun -> claim -> passive income over a day", "[integration]") {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(555);
  game.state().unlocked_programs.push_back("prog_backdoor");
  REQUIRE(game.start_netrun("target_1", false));
  auto* s = game.session();
  s->px = s->map.root.first;
  s->py = s->map.root.second;
  REQUIRE(s->use_program("prog_backdoor"));
  double before = game.state().credits;
  game.resolve_netrun();
  REQUIRE(game.state().remote_servers.size() == 1);
  game.tick_minutes(1440); // one day
  REQUIRE(game.state().credits > before - 2000.0); // income minus daily expenses
}
