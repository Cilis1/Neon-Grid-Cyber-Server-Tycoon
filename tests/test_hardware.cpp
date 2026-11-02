#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/Game.hpp"
#include "hardware/HardwareFactory.hpp"
#include "hardware/Rack.hpp"
#include "hardware/Server.hpp"

using namespace neon;

static core::Game make_game() {
  core::Game game;
  REQUIRE(game.init(std::filesystem::path(NEON_TEST_DATA_DIR)));
  game.new_game(42);
  return game;
}

TEST_CASE("starter room has rack, pdu, cooler, server and is powered", "[hardware]") {
  core::Game game = make_game();
  auto& room = game.active_room();
  REQUIRE(room.width == 16);
  REQUIRE(room.height == 12);
  REQUIRE(room.tiles.size() == 16u * 12u);
  int racks = 0, pdus = 0, servers = 0;
  for (auto& c : room.components) {
    const auto* def = game.catalog().hardware(c.def_id);
    REQUIRE(def != nullptr);
    if (def->type == "rack") racks++;
    if (def->type == "pdu") pdus++;
    if (def->type == "server_unit") servers++;
  }
  REQUIRE(racks == 1);
  REQUIRE(pdus == 1);
  REQUIRE(servers == 1);
  auto stats = game.room_stats("room1");
  REQUIRE(stats.power_capacity_w == Catch::Approx(3000.0));
  REQUIRE(stats.power_draw_w > 0.0);
  REQUIRE(stats.power_draw_w < stats.power_capacity_w);
  REQUIRE(stats.online_servers == 1); // pre-cabled and runnable
}

TEST_CASE("server stats aggregate from installed parts", "[hardware]") {
  core::Game game = make_game();
  auto& room = game.active_room();
  hardware::Component* server = nullptr;
  for (auto& c : room.components)
    if (game.catalog().hardware(c.def_id)->type == "server_unit") server = &c;
  REQUIRE(server != nullptr);
  auto s = hardware::compute_server_stats(game.catalog(), room, *server);
  REQUIRE(s.ghz == Catch::Approx(2.4));   // hw_cpu_t1
  REQUIRE(s.ram_gb == Catch::Approx(8.0)); // hw_ram_t1
  REQUIRE(s.storage_tb == Catch::Approx(2.0));
  REQUIRE(s.wattage == Catch::Approx(450.0));
  REQUIRE(s.has_cpu);
  REQUIRE(hardware::server_is_runnable(game.catalog(), room, *server));
}

TEST_CASE("power overload trips breakers or shuts down components", "[hardware]") {
  core::Game game = make_game();
  auto& room = game.active_room();
  // add 4 ASIC miners (900W each) into the server -> massive overload vs 3kW PDU
  auto* server = &*std::find_if(room.components.begin(), room.components.end(), [&](const hardware::Component& c) {
    return game.catalog().hardware(c.def_id)->type == "server_unit";
  });
  for (int i = 0; i < 4; ++i) {
    hardware::Component gpu = hardware::make_component("hw_gpu_t4", "gpu_test" + std::to_string(i));
    gpu.parent_uid = server->uid;
    server->slots["pcie" + std::to_string(i)] = gpu.uid;
    room.components.push_back(gpu);
  }
  auto stats = hardware::recompute_room_stats(game.catalog(), room, 1.0);
  REQUIRE(stats.overloaded);
  // simulate a while: something must happen (breaker or shutdowns)
  for (int i = 0; i < 600 && !room.breaker_tripped; ++i) hardware::simulate_room(game, room, 1);
  // either breaker tripped or some component got shut down
  bool any_off = room.breaker_tripped;
  if (!any_off)
    for (auto& c : room.components)
      if (!c.powered) any_off = true;
  REQUIRE(any_off);
  if (room.breaker_tripped) {
    REQUIRE(game.reset_breaker("room1"));
    REQUIRE_FALSE(room.breaker_tripped);
  }
}

TEST_CASE("dust accumulates and canned air cleans it", "[hardware]") {
  core::Game game = make_game();
  auto& room = game.active_room();
  for (int i = 0; i < 60 * 24; ++i) hardware::simulate_room(game, room, 1); // 24 in-game hours
  int dusty = 0;
  std::string dusty_uid;
  for (auto& c : room.components)
    if (c.dust_level > 0) {
      dusty++;
      dusty_uid = c.uid;
    }
  REQUIRE(dusty > 0);
  REQUIRE(game.state().inventory_count["consumable_canned_air"] >= 1);
  REQUIRE(game.clean_component(dusty_uid));
  auto* c = room.find(dusty_uid);
  REQUIRE(c != nullptr);
  REQUIRE(c->dust_level == 0);
}

TEST_CASE("slot compatibility is enforced", "[hardware]") {
  core::Game game = make_game();
  game.state().credits = 1e9;
  // tier-2 is locked by default
  REQUIRE_FALSE(game.buy_hardware("hw_cpu_t2"));
  game.state().unlocked_items.insert("hw_cpu_t2");
  REQUIRE(game.buy_hardware("hw_cpu_t2"));
  REQUIRE(game.state().inventory.size() >= 1);
  std::string cpu_uid = game.state().inventory.back().uid;
  // find server
  auto& room = game.active_room();
  std::string server_uid;
  for (auto& c : room.components)
    if (game.catalog().hardware(c.def_id)->type == "server_unit") server_uid = c.uid;
  REQUIRE_FALSE(server_uid.empty());
  // wrong slot type must fail
  REQUIRE_FALSE(game.install_part(cpu_uid, server_uid, "ram"));
  // correct slot works
  REQUIRE(game.install_part(cpu_uid, server_uid, "cpu"));
  auto* server = room.find(server_uid);
  REQUIRE(server->slots["cpu"] == cpu_uid);
  auto s = hardware::compute_server_stats(game.catalog(), room, *server);
  REQUIRE(s.ghz == Catch::Approx(3.6));
}

TEST_CASE("temperature performance factor derates when hot", "[hardware]") {
  REQUIRE(hardware::temperature_performance_factor(22.0) == Catch::Approx(1.0));
  REQUIRE(hardware::temperature_performance_factor(45.0) == Catch::Approx(1.0));
  REQUIRE(hardware::temperature_performance_factor(85.0) == Catch::Approx(0.4));
  REQUIRE(hardware::temperature_performance_factor(65.0) == Catch::Approx(0.7));
}

TEST_CASE("rack capacity enforced", "[hardware]") {
  core::Game game = make_game();
  auto& room = game.active_room();
  auto* rack = &*std::find_if(room.components.begin(), room.components.end(), [&](const hardware::Component& c) {
    return game.catalog().hardware(c.def_id)->type == "rack";
  });
  REQUIRE(hardware::rack_capacity_u(game.catalog(), *rack) == 12);
  REQUIRE(hardware::rack_used_u(game.catalog(), room, *rack) == 1); // one 1U server
  hardware::Component big = hardware::make_component("hw_server_t4", "srv_big"); // 4U
  room.components.push_back(big);
  REQUIRE(hardware::rack_add_server(room, *rack, big.uid));
  REQUIRE(hardware::rack_used_u(game.catalog(), room, *rack) == 5);
  // fill up: 12U cap, 5 used -> can add another 4U (9), then 4U more would exceed
  hardware::Component big2 = hardware::make_component("hw_server_t4", "srv_big2");
  room.components.push_back(big2);
  REQUIRE(hardware::rack_add_server(room, *rack, big2.uid));
  REQUIRE(hardware::rack_used_u(game.catalog(), room, *rack) == 9);
  hardware::Component big3 = hardware::make_component("hw_server_t4", "srv_big3");
  room.components.push_back(big3);
  // Game-level install should refuse (9+4 > 12)
  REQUIRE_FALSE(game.install_in_rack(big3.uid, rack->uid));
}

TEST_CASE("heat makes room temperature rise when cooling insufficient", "[hardware]") {
  core::Game game = make_game();
  auto& room = game.active_room();
  // remove cooler power cable so cooling drops to baseline only
  for (auto it = room.cables.begin(); it != room.cables.end();) {
    if (it->to_uid.size() && game.catalog().hardware(it->to_uid) && game.catalog().hardware(it->to_uid)->type == "cooler")
      it = room.cables.erase(it);
    else
      ++it;
  }
  // crank heat: add ASICs
  auto* server = &*std::find_if(room.components.begin(), room.components.end(), [&](const hardware::Component& c) {
    return game.catalog().hardware(c.def_id)->type == "server_unit";
  });
  for (int i = 0; i < 2; ++i) {
    hardware::Component gpu = hardware::make_component("hw_gpu_t4", "gpu_heat" + std::to_string(i));
    gpu.parent_uid = server->uid;
    server->slots["pcie" + std::to_string(i)] = gpu.uid;
    room.components.push_back(gpu);
  }
  double t0 = room.temperature_c;
  for (int i = 0; i < 120 && room.temperature_c < 50.0; ++i) hardware::simulate_room(game, room, 1);
  REQUIRE(room.temperature_c > t0);
}
