#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "hardware/HardwareComponent.hpp"

namespace neon::core {
class DataCatalog;
class Game;
} // namespace neon::core

namespace neon::hardware {

using nlohmann::json;

enum class TileKind { Empty, Rack, Cooler, PDU, UPS, CableTrunk };

struct Tile {
  TileKind kind = TileKind::Empty;
  std::string occupant_uid; // uid of the rack/cooler/pdu component placed here
  json to_json() const;
  static Tile from_json(const json& j);
};

struct Cable {
  std::string uid;
  std::string from_uid;
  std::string from_port; // "power" or "net"
  std::string to_uid;
  std::string to_port;
  json to_json() const;
  static Cable from_json(const json& j);
};

struct Room {
  std::string id;
  std::string name;
  int width = 16;
  int height = 12;
  std::vector<Tile> tiles;                 // width * height
  std::vector<Component> components;       // every component located in this room
  std::vector<Cable> cables;
  double temperature_c = 22.0;
  bool breaker_tripped = false;
  // cached aggregates (recomputed by recompute_room_stats)
  double power_draw_w = 0.0;
  double power_capacity_w = 0.0;
  double heat_output_w = 0.0;
  double cooling_capacity_w = 0.0;
  // uptime tracking for storage/hosting SLAs
  double sla_ok_minutes = 0.0;
  double sla_total_minutes = 0.0;

  void init_grid();
  bool in_bounds(int x, int y) const;
  int tile_index(int x, int y) const;
  std::pair<int, int> tile_coords(int index) const;
  Tile* tile_at(int x, int y);
  Component* find(const std::string& uid);
  const Component* find(const std::string& uid) const;
  bool place_on_tile(const std::string& component_uid, int x, int y);
  bool remove_from_tile(const std::string& component_uid);

  json to_json() const;
  static Room from_json(const json& j);
};

// Aggregated power/cooling/temperature numbers for a room.
struct RoomStats {
  double power_draw_w = 0.0;
  double power_capacity_w = 0.0;
  double heat_output_w = 0.0;
  double cooling_capacity_w = 0.0;
  double temperature_c = 22.0;
  bool overloaded = false; // draw > capacity
  bool overheating = false; // heat > cooling
  int online_servers = 0;
  int total_servers = 0;
};

// Recompute cached aggregates (power draw / capacity, heat / cooling).
RoomStats recompute_room_stats(const core::DataCatalog& catalog, Room& room, double cooling_efficiency = 1.0);

// Advance the physical simulation of a room by `minutes` in-game minutes:
// dust accumulation, wear, temperature drift, overload handling.
// Emits events on the game's EventBus (power.breaker_trip, room.overheat, ...).
void simulate_room(core::Game& game, Room& room, std::uint64_t minutes);

} // namespace neon::hardware
