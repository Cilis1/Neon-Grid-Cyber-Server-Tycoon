#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/Game.hpp"
#include "core/SaveLoad.hpp"
#include "utils/FileSystem.hpp"

using namespace neon;

static core::Game make_game() {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(2026);
  return game;
}

TEST_CASE("save/load round trip produces identical state", "[saveload]") {
  core::Game game = make_game();
  // mutate state meaningfully
  game.state().credits = 4321.5;
  game.state().heat = 33;
  game.state().unlocked_items.insert("hw_rack_t3");
  game.state().unlocked_programs.push_back("prog_cloak");
  REQUIRE(game.research("hw1"));
  game.tick_minutes(180); // advance time, accumulate dust etc.
  game.state().rng_state = game.rng().state();

  auto path = std::filesystem::temp_directory_path() / "neongrid_test_save.json";
  REQUIRE(core::save_game(game, path));

  core::Game game2;
  REQUIRE(game2.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  REQUIRE(game2.load(path));

  auto a = game.state().to_json();
  auto b = game2.state().to_json();
  // rng_state is stored alongside; normalize both to compare pure state
  a.erase("rng_state");
  b.erase("rng_state");
  REQUIRE(a == b);
  REQUIRE(game2.state().credits == Catch::Approx(game.state().credits));
  REQUIRE(game2.state().heat == game.state().heat);
  std::filesystem::remove(path);
}

TEST_CASE("serialize/deserialize via json is stable", "[saveload]") {
  core::Game game = make_game();
  game.tick_minutes(60);
  auto j1 = core::serialize_state(game.state());
  auto state2 = core::deserialize_state(j1);
  auto j2 = core::serialize_state(state2);
  REQUIRE(j1 == j2);
}

TEST_CASE("loading a corrupt save fails gracefully", "[saveload]") {
  auto path = std::filesystem::temp_directory_path() / "neongrid_corrupt.json";
  utils::fs::write_text_file(path, "{ this is not json !!!");
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  REQUIRE_FALSE(game.load(path));
  std::filesystem::remove(path);
}

TEST_CASE("loading a newer-version save is rejected", "[saveload]") {
  core::Game game = make_game();
  auto j = core::serialize_state(game.state());
  j["version"] = 999;
  auto path = std::filesystem::temp_directory_path() / "neongrid_future.json";
  utils::fs::write_text_file(path, j.dump());
  core::Game game2;
  REQUIRE(game2.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  REQUIRE_FALSE(game2.load(path));
  std::filesystem::remove(path);
}

TEST_CASE("default save path is under the app data dir", "[saveload]") {
  auto p = core::default_save_path();
  REQUIRE(p.filename() == "savegame.json");
  REQUIRE(p.parent_path().filename() == "saves");
}
