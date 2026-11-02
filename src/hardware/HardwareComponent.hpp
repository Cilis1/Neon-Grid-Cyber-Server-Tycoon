#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace neon::hardware {

using nlohmann::json;

// Static catalog definition of a hardware item (loaded from data/hardware.json).
struct HardwareItemDef {
  std::string id;
  std::string name;
  std::string type;       // rack, server_unit, cpu, ram, storage, psu, cooler, network_card, pdu, cyberdeck, gpu, ups, cable, consumable
  int tier = 1;           // 1 scavenged .. 5 military/experimental
  double price = 0.0;
  double power_draw = 0.0;  // W
  double heat_output = 0.0; // W
  double durability = 100.0;
  double noise = 0.0;       // dB
  json stats;               // free-form: ghz, gb, tb, mbps, wattage, cooling_w, hash_mh, slots_u, capacity_w ...
  int size_u = 1;           // rack units for rack-mounted gear
  std::string slot_type;    // which slot this item fits into (cpu, ram, storage, pcie, psu, fan, ...)
  std::vector<std::string> compatibility;
  std::string description;

  double stat(const std::string& key, double fallback = 0.0) const;

  static HardwareItemDef from_json(const json& j);
  json to_json() const;
};

// Runtime instance of an installed (or stored) hardware component.
struct Component {
  std::string uid;
  std::string def_id;
  int tile_index = -1;      // placement on the room grid; -1 when nested in a rack/server or in inventory
  std::string parent_uid;   // rack uid (server units) or server uid (parts)
  // Server-unit slot map: slot_type -> component uid (cpu, ram, storage, pcie, psu, fan)
  std::map<std::string, std::string> slots;
  // Rack content: server unit uids
  std::vector<std::string> installed;
  int dust_level = 0;    // 0..100, reduces cooling efficiency
  double dust_frac = 0.0; // fractional dust accumulator (0..1)
  int wear_level = 0;    // 0..100, increases failure chance
  int durability = 100;  // 0..100
  bool powered = true;
  bool online = true;

  json to_json() const;
  static Component from_json(const json& j);
};

} // namespace neon::hardware
