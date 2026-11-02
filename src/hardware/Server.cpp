#include "hardware/Server.hpp"

#include "core/DataCatalog.hpp"
#include "hardware/ServerRoom.hpp"

namespace neon::hardware {

ServerStats compute_server_stats(const core::DataCatalog& catalog, const Room& room, const Component& server) {
  ServerStats s;
  const HardwareItemDef* base = catalog.hardware(server.def_id);
  if (base) {
    s.power_w += base->power_draw;
    s.heat_w += base->heat_output;
    s.compute_units += static_cast<int>(base->stat("compute", 0));
  }
  for (const auto& [slot, uid] : server.slots) {
    const Component* part = room.find(uid);
    if (!part) continue;
    const HardwareItemDef* def = catalog.hardware(part->def_id);
    if (!def) continue;
    s.power_w += def->power_draw;
    s.heat_w += def->heat_output;
    if (slot == "cpu") {
      s.ghz += def->stat("ghz", 0);
      s.compute_units += static_cast<int>(def->stat("cores", 1));
      s.has_cpu = true;
    } else if (slot == "ram") {
      s.ram_gb += def->stat("gb", 0);
      s.has_ram = true;
    } else if (slot == "storage") {
      s.storage_tb += def->stat("tb", 0);
      s.has_storage = true;
    } else if (slot == "nic" || slot == "network") {
      s.mbps += def->stat("mbps", 0);
    } else if (slot == "gpu" || slot == "pcie") {
      s.hash_mh += def->stat("hash_mh", 0);
      s.compute_units += static_cast<int>(def->stat("compute", 0));
    } else if (slot == "psu") {
      s.wattage += def->stat("wattage", 0);
      s.has_psu = true;
    } else if (slot == "fan") {
      s.cooling_w += def->stat("cooling_w", 0);
    }
  }
  return s;
}

bool server_is_runnable(const core::DataCatalog& catalog, const Room& room, const Component& server) {
  if (!server.online || !server.powered) return false;
  ServerStats s = compute_server_stats(catalog, room, server);
  if (!s.has_cpu || !s.has_ram || !s.has_psu) return false;
  if (s.power_w > s.wattage + 1e-9) return false; // PSU cannot supply the load
  // needs a network cable to a switch/router/PDU-connected network gear
  for (const auto& cable : room.cables) {
    if ((cable.from_uid == server.uid || cable.to_uid == server.uid) &&
        (cable.from_port == "net" || cable.to_port == "net"))
      return true;
  }
  return false;
}

double temperature_performance_factor(double temperature_c) {
  if (temperature_c <= 45.0) return 1.0;
  if (temperature_c >= 85.0) return 0.4;
  return 1.0 - (temperature_c - 45.0) / 40.0 * 0.6; // linear derate to 0.4 at 85C
}

} // namespace neon::hardware
