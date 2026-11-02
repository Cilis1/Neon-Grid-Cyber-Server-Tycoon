#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/Game.hpp"

using namespace neon;

static core::Game make_game() {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(12345);
  return game;
}

TEST_CASE("tech tree loads 50+ nodes across 6 branches", "[techtree]") {
  core::Game game = make_game();
  const auto& nodes = game.catalog().tech_nodes();
  REQUIRE(nodes.size() >= 50);
  std::set<std::string> branches;
  for (const auto& [id, n] : nodes) branches.insert(n.branch);
  REQUIRE(branches.size() >= 6);
}

TEST_CASE("tech tree prereq validation", "[techtree]") {
  core::Game game = make_game();
  // hw2 requires hw1
  REQUIRE_FALSE(game.tech().prereqs_met(game.state().purchased_tech, "hw2"));
  REQUIRE(game.tech().prereqs_met(game.state().purchased_tech, "hw1"));
  REQUIRE_FALSE(game.tech().can_purchase(game, "hw2"));
  REQUIRE(game.tech().can_purchase(game, "hw1")); // 500 cr / 5 rp, we start with 10000 / 25
}

TEST_CASE("research purchase deducts cost and applies effects", "[techtree]") {
  core::Game game = make_game();
  double credits_before = game.state().credits;
  double rp_before = game.state().rp;
  const auto* node = game.catalog().tech_node("hw1");
  REQUIRE(node != nullptr);
  REQUIRE(game.research("hw1"));
  REQUIRE(game.state().credits == Catch::Approx(credits_before - node->cost_credits));
  REQUIRE(game.state().rp == Catch::Approx(rp_before - node->cost_rp));
  REQUIRE(game.state().purchased_tech.count("hw1") == 1);
  // effect: unlock_hardware hw_rack_t2
  REQUIRE(game.is_item_unlocked("hw_rack_t2"));
  // now hw2 prereqs are met
  REQUIRE(game.tech().prereqs_met(game.state().purchased_tech, "hw2"));
  // double purchase fails
  REQUIRE_FALSE(game.research("hw1"));
}

TEST_CASE("stat multiplier effects stack", "[techtree]") {
  core::Game game = make_game();
  REQUIRE(game.get_multiplier("cpu_speed") == Catch::Approx(1.0));
  REQUIRE(game.research("hw1")); // cpu_speed x1.05
  REQUIRE(game.get_multiplier("cpu_speed") == Catch::Approx(1.05));
  REQUIRE(game.research("hw10") == false); // prereq hw2 not met
  game.state().credits = 1e9;
  game.state().rp = 1e9;
  REQUIRE(game.research("hw2"));
  REQUIRE(game.research("hw10")); // cpu_speed x1.25
  REQUIRE(game.get_multiplier("cpu_speed") == Catch::Approx(1.05 * 1.25));
}

TEST_CASE("program unlock effect feeds netrun deck", "[techtree]") {
  core::Game game = make_game();
  game.state().credits = 1e9;
  game.state().rp = 1e9;
  REQUIRE(game.research("nr1")); // unlocks prog_cloak
  REQUIRE(std::find(game.state().unlocked_programs.begin(), game.state().unlocked_programs.end(), "prog_cloak") !=
          game.state().unlocked_programs.end());
  game.state().unlocked_programs.push_back("prog_backdoor");
  REQUIRE(game.start_netrun("target_1", false));
  auto* s = game.session();
  REQUIRE(s != nullptr);
  REQUIRE(std::find(s->programs.begin(), s->programs.end(), "prog_cloak") != s->programs.end());
  game.resolve_netrun();
}
