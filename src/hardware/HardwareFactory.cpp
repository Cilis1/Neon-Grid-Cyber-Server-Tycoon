#include "hardware/HardwareFactory.hpp"

#include "core/DataCatalog.hpp"
#include "hardware/Rack.hpp"

namespace neon::hardware {

Component make_component(const std::string& def_id, const std::string& uid) {
  Component c;
  c.uid = uid;
  c.def_id = def_id;
  c.tile_index = -1;
  return c;
}

Cable& add_cable(Room& room, const std::function<std::string(const std::string&)>& uid_gen,
                 const std::string& from_uid, const std::string& from_port,
                 const std::string& to_uid, const std::string& to_port) {
  for (auto& c : room.cables) {
    if ((c.from_uid == from_uid && c.from_port == from_port && c.to_uid == to_uid && c.to_port == to_port) ||
        (c.from_uid == to_uid && c.from_port == to_port && c.to_uid == from_uid && c.to_port == from_port))
      return c;
  }
  Cable c;
  c.uid = uid_gen("cable");
  c.from_uid = from_uid;
  c.from_port = from_port;
  c.to_uid = to_uid;
  c.to_port = to_port;
  room.cables.push_back(c);
  return room.cables.back();
}

Room create_starter_room(const core::DataCatalog& catalog, const std::function<std::string(const std::string&)>& uid_gen) {
  Room room;
  room.id = "room1";
  room.name = "Basement Rack Room";
  room.width = 16;
  room.height = 12;
  room.init_grid();

  auto add = [&](const std::string& def_id, const std::string& prefix) {
    Component c = make_component(def_id, uid_gen(prefix));
    room.components.push_back(c);
    return room.components.back().uid;
  };

  std::string rack_uid = add("hw_rack_t1", "rack");
  std::string pdu_uid = add("hw_pdu_t1", "pdu");
  std::string cooler_uid = add("hw_cooler_t1", "cooler");
  std::string switch_uid = add("hw_switch_t1", "switch");
  std::string server_uid = add("hw_server_t1", "srv");
  std::string cpu_uid = add("hw_cpu_t1", "cpu");
  std::string ram_uid = add("hw_ram_t1", "ram");
  std::string storage_uid = add("hw_storage_t1", "sto");
  std::string psu_uid = add("hw_psu_t1", "psu");
  std::string fan_uid = add("hw_fan_t1", "fan");

  auto set_tile = [&](const std::string& uid, int x, int y, TileKind kind) {
    Component* c = room.find(uid);
    Tile* t = room.tile_at(x, y);
    if (c && t) {
      c->tile_index = room.tile_index(x, y);
      t->kind = kind;
      t->occupant_uid = uid;
    }
  };
  set_tile(rack_uid, 2, 2, TileKind::Rack);
  set_tile(pdu_uid, 2, 8, TileKind::PDU);
  set_tile(cooler_uid, 4, 2, TileKind::Cooler);
  set_tile(switch_uid, 2, 4, TileKind::CableTrunk);

  Component* rack = room.find(rack_uid);
  Component* server = room.find(server_uid);
  if (rack && server) {
    rack->installed.push_back(server_uid);
    server->parent_uid = rack_uid;
    server->slots["cpu"] = cpu_uid;
    server->slots["ram"] = ram_uid;
    server->slots["storage"] = storage_uid;
    server->slots["psu"] = psu_uid;
    server->slots["fan"] = fan_uid;
    for (const std::string& part : {cpu_uid, ram_uid, storage_uid, psu_uid, fan_uid}) {
      if (Component* p = room.find(part)) p->parent_uid = server_uid;
    }
  }

  add_cable(room, uid_gen, pdu_uid, "power", rack_uid, "power");
  add_cable(room, uid_gen, pdu_uid, "power", cooler_uid, "power");
  add_cable(room, uid_gen, pdu_uid, "power", switch_uid, "power");
  add_cable(room, uid_gen, pdu_uid, "power", server_uid, "power");
  add_cable(room, uid_gen, server_uid, "net", switch_uid, "net");

  (void)catalog;
  return room;
}

} // namespace neon::hardware
