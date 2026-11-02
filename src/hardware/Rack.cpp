#include "hardware/Rack.hpp"

#include "core/DataCatalog.hpp"
#include "hardware/ServerRoom.hpp"

namespace neon::hardware {

int rack_capacity_u(const core::DataCatalog& catalog, const Component& rack) {
  const HardwareItemDef* def = catalog.hardware(rack.def_id);
  if (!def) return 0;
  return static_cast<int>(def->stat("slots_u", 42));
}

int rack_used_u(const core::DataCatalog& catalog, const Room& room, const Component& rack) {
  int used = 0;
  for (const auto& uid : rack.installed) {
    const Component* server = room.find(uid);
    if (!server) continue;
    const HardwareItemDef* def = catalog.hardware(server->def_id);
    if (def) used += def->size_u;
  }
  return used;
}

bool rack_add_server(Room& room, Component& rack, const std::string& server_uid) {
  for (const auto& u : rack.installed)
    if (u == server_uid) return false; // already installed
  rack.installed.push_back(server_uid);
  if (Component* s = room.find(server_uid)) s->parent_uid = rack.uid;
  return true;
}

bool rack_remove_server(Room& room, Component& rack, const std::string& server_uid) {
  for (auto it = rack.installed.begin(); it != rack.installed.end(); ++it) {
    if (*it == server_uid) {
      rack.installed.erase(it);
      if (Component* s = room.find(server_uid)) s->parent_uid.clear();
      return true;
    }
  }
  return false;
}

} // namespace neon::hardware
