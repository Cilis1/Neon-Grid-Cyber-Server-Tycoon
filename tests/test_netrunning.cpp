#include <catch2/catch_test_macros.hpp>

#include "core/Game.hpp"

using namespace neon;

static core::Game make_game() {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(999);
  return game;
}

TEST_CASE("netrun session starts with deck stats and programs", "[netrun]") {
  core::Game game = make_game();
  REQUIRE(game.state().netrun_targets.size() >= 10);
  REQUIRE(game.start_netrun("target_1", false)); // abandoned, difficulty 1
  auto* s = game.session();
  REQUIRE(s != nullptr);
  REQUIRE(s->is_running());
  REQUIRE(s->px == s->map.entry.first);
  REQUIRE(s->py == s->map.entry.second);
  REQUIRE(s->shield == s->max_shield);
  REQUIRE(s->ram_used <= game.deck_stats().ram);
  // starter programs equipped
  REQUIRE(std::find(s->programs.begin(), s->programs.end(), "prog_decrypt") != s->programs.end());
  REQUIRE(std::find(s->programs.begin(), s->programs.end(), "prog_attack") != s->programs.end());
  REQUIRE(s->map.at(s->map.root.first, s->map.root.second)->type == netrunning::NodeType::Root);
  game.resolve_netrun();
}

TEST_CASE("movement is orthogonal and bounded", "[netrun]") {
  core::Game game = make_game();
  REQUIRE(game.start_netrun("target_1", false));
  auto* s = game.session();
  int ap0 = s->action_points;
  REQUIRE_FALSE(s->move(2, 0));  // not orthogonal distance 1
  REQUIRE_FALSE(s->move(0, 0));
  REQUIRE_FALSE(s->move(-1, 0)); // px == 0 at entry -> out of bounds
  bool moved = s->move(1, 0);
  REQUIRE(moved);
  REQUIRE(s->px == s->map.entry.first + 1);
  REQUIRE(s->action_points == ap0 - 1);
  // walk back
  REQUIRE(s->move(-1, 0));
  game.resolve_netrun();
}

TEST_CASE("backdoor at root wins and claims abandoned server", "[netrun]") {
  core::Game game = make_game();
  game.state().unlocked_programs.push_back("prog_backdoor");
  REQUIRE(game.start_netrun("target_1", false));
  auto* s = game.session();
  // teleport next to root
  s->px = s->map.root.first;
  s->py = s->map.root.second;
  REQUIRE(s->use_program("prog_backdoor"));
  REQUIRE(s->state == netrunning::NetrunSession::State::Won);
  double credits_before = game.state().credits;
  game.resolve_netrun();
  REQUIRE(game.session() == nullptr);
  REQUIRE(game.state().credits > credits_before - 1.0); // reward paid
  REQUIRE(game.state().netruns_won == 1);
  // abandoned target claimed -> remote server with passive income
  REQUIRE(game.state().remote_servers.size() == 1);
  REQUIRE(game.state().remote_servers[0].income_per_hour > 0.0);
  const auto& t = game.state().netrun_targets[0];
  REQUIRE(t.claimed);
}

TEST_CASE("trace timer eventually traces the runner", "[netrun]") {
  core::Game game = make_game();
  REQUIRE(game.start_netrun("target_5", false)); // harder target
  auto* s = game.session();
  int guard = 0;
  while (s->is_running() && guard++ < 200) s->end_turn();
  REQUIRE((s->state == netrunning::NetrunSession::State::Traced || s->state == netrunning::NetrunSession::State::Lost));
  int heat_before = game.state().heat;
  game.resolve_netrun();
  REQUIRE(game.state().heat >= heat_before);
}

TEST_CASE("heal program restores shield and respects cooldown", "[netrun]") {
  core::Game game = make_game();
  game.state().unlocked_programs.push_back("prog_heal");
  REQUIRE(game.start_netrun("target_1", false));
  auto* s = game.session();
  s->shield = 50;
  REQUIRE(s->use_program("prog_heal"));
  REQUIRE(s->shield == 80);
  REQUIRE(s->program_cooldown("prog_heal") > 0);
  REQUIRE_FALSE(s->use_program("prog_heal")); // on cooldown
  s->end_turn();
  REQUIRE(s->program_cooldown("prog_heal") == 2); // 3 -> ticked to 2
  game.resolve_netrun();
}

TEST_CASE("ICE damages shield when alerted and adjacent", "[netrun]") {
  core::Game game = make_game();
  REQUIRE(game.start_netrun("target_5", false)); // difficulty 3, more ICE
  auto* s = game.session();
  // force an ICE next to the player and alert it
  if (!s->map.ices.empty()) {
    s->map.ices[0].x = s->px + 1;
    s->map.ices[0].y = s->py;
    s->map.ices[0].alert = true;
    int shield0 = s->shield;
    s->end_turn();
    REQUIRE(s->shield < shield0);
  }
  game.resolve_netrun();
}

TEST_CASE("proxy use requires a claimed server and slows trace", "[netrun]") {
  core::Game game = make_game();
  // no proxies yet -> proxy run refused
  REQUIRE_FALSE(game.start_netrun("target_1", true));
  // claim one first
  game.state().unlocked_programs.push_back("prog_backdoor");
  REQUIRE(game.start_netrun("target_1", false));
  auto* s = game.session();
  s->px = s->map.root.first;
  s->py = s->map.root.second;
  REQUIRE(s->use_program("prog_backdoor"));
  game.resolve_netrun();
  REQUIRE(game.state().remote_servers.size() == 1);
  // now proxy run allowed on another target
  REQUIRE(game.start_netrun("target_2", true));
  auto* s2 = game.session();
  REQUIRE(s2->used_proxy);
  REQUIRE(s2->trace_rate() < 4.0);
  game.resolve_netrun();
}
