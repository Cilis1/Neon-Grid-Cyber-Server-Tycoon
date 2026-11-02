#include "hardware/ServerRoom.hpp"

#include "core/DataCatalog.hpp"
#include "core/Game.hpp"
#include "hardware/Server.hpp"
#include "utils/Random.hpp"

#include <spdlog/spdlog.h>

namespace neon::hardware {

json Tile::to_json() const {
  return json{{"kind", static_cast<int>(kind)}, {"occupant_uid", occupant_uid}};
}

Tile Tile::from_json(const json& j) {
  Tile t;
  t.kind = static_cast<TileKind>(j.value("kind", 0));
  t.occupant_uid = j.value("occupant_uid", "");
  return t;
}

json Cable::to_json() const {
  return json{{"uid", uid}, {"from_uid", from_uid}, {"from_port", from_port},
              {"to_uid", to_uid}, {"to_port", to_port}};
}

Cable Cable::from_json(const json& j) {
  Cable c;
  c.uid = j.at("uid").get<std::string>();
  c.from_uid = j.value("from_uid", "");
  c.from_port = j.value("from_port", "power");
  c.to_uid = j.value("to_uid", "");
  c.to_port = j.value("to_port", "power");
  return c;
}

void Room::init_grid() { tiles.assign(static_cast<std::size_t>(width) * height, Tile{}); }

bool Room::in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }

int Room::tile_index(int x, int y) const { return y * width + x; }

std::pair<int, int> Room::tile_coords(int index) const {
  return {index % width, index / width};
}

Tile* Room::tile_at(int x, int y) {
  if (!in_bounds(x, y)) return nullptr;
  return &tiles[static_cast<std::size_t>(tile_index(x, y))];
}

Component* Room::find(const std::string& uid) {
  for (auto& c : components)
    if (c.uid == uid) return &c;
  return nullptr;
}

const Component* Room::find(const std::string& uid) const {
  for (const auto& c : components)
    if (c.uid == uid) return &c;
  return nullptr;
}

bool Room::place_on_tile(const std::string& component_uid, int x, int y) {
  Tile* t = tile_at(x, y);
  if (!t || t->kind != TileKind::Empty) return false;
  Component* c = find(component_uid);
  if (!c) return false;
  if (c->tile_index >= 0) {
    if (Tile* old = tile_at(tile_coords(c->tile_index).first, tile_coords(c->tile_index).second)) {
      old->kind = TileKind::Empty;
      old->occupant_uid.clear();
    }
  }
  t->occupant_uid = component_uid;
  c->tile_index = tile_index(x, y);
  return true;
}

bool Room::remove_from_tile(const std::string& component_uid) {
  Component* c = find(component_uid);
  if (!c || c->tile_index < 0) return false;
  auto [x, y] = tile_coords(c->tile_index);
  if (Tile* t = tile_at(x, y)) {
    t->kind = TileKind::Empty;
    t->occupant_uid.clear();
  }
  c->tile_index = -1;
  return true;
}

json Room::to_json() const {
  json tiles_j = json::array();
  for (const auto& t : tiles) tiles_j.push_back(t.to_json());
  json comps = json::array();
  for (const auto& c : components) comps.push_back(c.to_json());
  json cables_j = json::array();
  for (const auto& c : cables) cables_j.push_back(c.to_json());
  return json{
      {"id", id}, {"name", name}, {"width", width}, {"height", height}, {"tiles", tiles_j},
      {"components", comps}, {"cables", cables_j}, {"temperature_c", temperature_c},
      {"breaker_tripped", breaker_tripped}, {"power_draw_w", power_draw_w}, {"power_capacity_w", power_capacity_w},
      {"heat_output_w", heat_output_w}, {"cooling_capacity_w", cooling_capacity_w},
      {"sla_ok_minutes", sla_ok_minutes}, {"sla_total_minutes", sla_total_minutes},
  };
}

Room Room::from_json(const json& j) {
  Room r;
  r.id = j.at("id").get<std::string>();
  r.name = j.value("name", r.id);
  r.width = j.value("width", 16);
  r.height = j.value("height", 12);
  r.tiles.clear();
  if (j.contains("tiles") && j["tiles"].is_array())
    for (auto& t : j["tiles"]) r.tiles.push_back(Tile::from_json(t));
  if (r.tiles.size() != static_cast<std::size_t>(r.width) * r.height) r.init_grid();
  if (j.contains("components") && j["components"].is_array())
    for (auto& c : j["components"]) r.components.push_back(Component::from_json(c));
  if (j.contains("cables") && j["cables"].is_array())
    for (auto& c : j["cables"]) r.cables.push_back(Cable::from_json(c));
  r.temperature_c = j.value("temperature_c", 22.0);
  r.breaker_tripped = j.value("breaker_tripped", false);
  r.power_draw_w = j.value("power_draw_w", 0.0);
  r.power_capacity_w = j.value("power_capacity_w", 0.0);
  r.heat_output_w = j.value("heat_output_w", 0.0);
  r.cooling_capacity_w = j.value("cooling_capacity_w", 0.0);
  r.sla_ok_minutes = j.value("sla_ok_minutes", 0.0);
  r.sla_total_minutes = j.value("sla_total_minutes", 0.0);
  return r;
}

namespace {

bool has_power_cable(const Room& room, const std::string& uid) {
  for (const auto& cable : room.cables) {
    if (cable.from_port != "power" && cable.to_port != "power") continue;
    bool from_ok = cable.from_uid == uid && cable.from_port == "power";
    bool to_ok = cable.to_uid == uid && cable.to_port == "power";
    if (from_ok && !cable.to_uid.empty()) return true;
    if (to_ok && !cable.from_uid.empty()) return true;
  }
  return false;
}

} // namespace

RoomStats recompute_room_stats(const core::DataCatalog& catalog, Room& room, double cooling_efficiency) {
  RoomStats s;
  s.temperature_c = room.temperature_c;

  // Resolve powered state from cables + breaker.
  if (room.breaker_tripped) {
    for (auto& c : room.components) c.powered = false;
  } else {
    for (auto& c : room.components) {
      const HardwareItemDef* def = catalog.hardware(c.def_id);
      if (!def) continue;
      if (def->type == "pdu" || def->type == "ups") {
        c.powered = true;
      } else {
        c.powered = has_power_cable(room, c.uid);
      }
    }
  }

  double cooler_dust_sum = 0.0;
  int cooler_count = 0;
  for (const auto& c : room.components) {
    const HardwareItemDef* def = catalog.hardware(c.def_id);
    if (!def || !c.powered) continue;
    if (def->type == "pdu" || def->type == "ups") {
      s.power_capacity_w += def->stat("capacity_w", def->type == "ups" ? 1500.0 : 3000.0);
    } else if (def->type == "cooler") {
      s.cooling_capacity_w += def->stat("cooling_w", 400.0);
      cooler_dust_sum += c.dust_level;
      ++cooler_count;
    }
  }
  // Baseline ambient cooling + dust penalty on coolers.
  s.cooling_capacity_w += 500.0;
  if (cooler_count > 0) {
    double avg_dust = cooler_dust_sum / cooler_count;
    s.cooling_capacity_w *= (1.0 - 0.005 * avg_dust) * cooling_efficiency;
  } else {
    s.cooling_capacity_w *= cooling_efficiency;
  }

  for (auto& c : room.components) {
    const HardwareItemDef* def = catalog.hardware(c.def_id);
    if (!def || !c.powered || !c.online) continue;
    if (def->type == "server_unit") {
      ServerStats ss = compute_server_stats(catalog, room, c);
      s.power_draw_w += ss.power_w;
      s.heat_output_w += ss.heat_w;
      ++s.total_servers;
      if (server_is_runnable(catalog, room, c)) ++s.online_servers;
    } else if (def->type != "pdu" && def->type != "ups") {
      s.power_draw_w += def->power_draw;
      s.heat_output_w += def->heat_output;
    }
  }

  s.overloaded = s.power_draw_w > s.power_capacity_w + 1e-9;
  s.overheating = s.heat_output_w > s.cooling_capacity_w + 1e-9;

  room.power_draw_w = s.power_draw_w;
  room.power_capacity_w = s.power_capacity_w;
  room.heat_output_w = s.heat_output_w;
  room.cooling_capacity_w = s.cooling_capacity_w;
  return s;
}

void simulate_room(core::Game& game, Room& room, std::uint64_t minutes) {
  const core::DataCatalog& catalog = game.catalog();
  double cooling_eff = 1.0 + game.get_stat("cooling_efficiency"); // additive bonus fraction
  RoomStats stats = recompute_room_stats(catalog, room, cooling_eff);

  // Overload handling: random shutdowns or breaker trip.
  if (stats.overloaded && !room.breaker_tripped) {
    for (std::uint64_t m = 0; m < minutes; ++m) {
      if (game.rng().chance(0.02)) {
        room.breaker_tripped = true;
        for (auto& c : room.components) c.powered = false;
        game.bus().publish("power.breaker_trip", {{"room", room.id}, {"draw", stats.power_draw_w}, {"capacity", stats.power_capacity_w}});
        spdlog::warn("[{}] BREAKER TRIPPED: draw {:.0f}W > capacity {:.0f}W", room.name, stats.power_draw_w, stats.power_capacity_w);
        break;
      }
      if (game.rng().chance(0.08)) {
        // shut down the highest-draw online component
        Component* victim = nullptr;
        double best = -1.0;
        for (auto& c : room.components) {
          const HardwareItemDef* def = catalog.hardware(c.def_id);
          if (!def || !c.powered || !c.online) continue;
          if (def->type == "pdu" || def->type == "ups") continue;
          double draw = def->type == "server_unit" ? compute_server_stats(catalog, room, c).power_w : def->power_draw;
          if (draw > best) {
            best = draw;
            victim = &c;
          }
        }
        if (victim) {
          victim->powered = false;
          game.bus().publish("power.component_shutdown", {{"room", room.id}, {"uid", victim->uid}, {"def", victim->def_id}});
          spdlog::warn("[{}] overload: shut down {}", room.name, victim->def_id);
        }
      }
      stats = recompute_room_stats(catalog, room, cooling_eff);
      if (!stats.overloaded) break;
    }
  }

  // Temperature drift.
  if (stats.overheating) {
    double excess = stats.heat_output_w - stats.cooling_capacity_w;
    room.temperature_c += excess * 0.0005 * static_cast<double>(minutes);
  } else {
    room.temperature_c += (22.0 - room.temperature_c) * 0.02 * static_cast<double>(minutes);
  }
  if (room.temperature_c > 70.0 && minutes > 0) {
    game.bus().publish("room.overheat", {{"room", room.id}, {"temperature", room.temperature_c}});
  }

  // Dust + wear + random heat failures.
  for (auto& c : room.components) {
    // ~100 dust in 33 in-game hours, accumulated fractionally so per-minute ticks work
    c.dust_frac += static_cast<double>(minutes) * (100.0 / (33.0 * 60.0));
    while (c.dust_frac >= 1.0) {
      c.dust_frac -= 1.0;
      c.dust_level = std::min(100, c.dust_level + 1);
    }
    const HardwareItemDef* def = catalog.hardware(c.def_id);
    if (def && def->type == "server_unit" && c.powered && c.online) {
      c.wear_level = std::min(100, c.wear_level + static_cast<int>(minutes) / 100); // ~100 wear in 7 days
    }
    if (room.temperature_c > 70.0 && c.online) {
      double p = (room.temperature_c - 70.0) / 100.0 * 0.02 * static_cast<double>(minutes);
      if (game.rng().chance(p)) {
        c.durability -= game.rng().next_int(5, 20);
        if (c.durability <= 0) {
          c.durability = 0;
          c.online = false;
          game.bus().publish("hardware.failure", {{"room", room.id}, {"uid", c.uid}, {"def", c.def_id}, {"cause", "overheat"}});
          spdlog::error("[{}] component {} destroyed by heat", room.name, c.def_id);
        }
      }
    }
  }

  // SLA uptime tracking.
  room.sla_total_minutes += static_cast<double>(minutes);
  if (!room.breaker_tripped && stats.online_servers == stats.total_servers && stats.total_servers > 0)
    room.sla_ok_minutes += static_cast<double>(minutes);
}

} // namespace neon::hardware
