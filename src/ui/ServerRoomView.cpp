#include "ui/ServerRoomView.hpp"

#include "core/Game.hpp"
#include "hardware/Rack.hpp"
#include "hardware/Server.hpp"
#include "ui/UITheme.hpp"
#include "utils/StringUtils.hpp"

#include <imgui.h>

#include <cmath>

namespace neon::ui {

using neon::hardware::Component;
using neon::hardware::Room;

static const char* tile_kind_name(neon::hardware::TileKind k) {
  switch (k) {
    case neon::hardware::TileKind::Rack: return "RACK";
    case neon::hardware::TileKind::Cooler: return "COOLER";
    case neon::hardware::TileKind::PDU: return "PDU";
    case neon::hardware::TileKind::UPS: return "UPS";
    case neon::hardware::TileKind::CableTrunk: return "SWITCH";
    default: return "";
  }
}

static ImU32 tile_color(neon::core::Game& game, const Room& room, int idx) {
  const auto& t = room.tiles[static_cast<std::size_t>(idx)];
  if (!t.occupant_uid.empty()) {
    const Component* c = room.find(t.occupant_uid);
    if (c && !c->powered) return IM_COL32(0x55, 0x22, 0x22, 0xFF);
    switch (t.kind) {
      case neon::hardware::TileKind::Rack: return IM_COL32(0x1E, 0x3A, 0x5C, 0xFF);
      case neon::hardware::TileKind::Cooler: return IM_COL32(0x0E, 0x4D, 0x64, 0xFF);
      case neon::hardware::TileKind::PDU: return IM_COL32(0x5C, 0x4A, 0x0E, 0xFF);
      case neon::hardware::TileKind::UPS: return IM_COL32(0x3D, 0x5C, 0x0E, 0xFF);
      case neon::hardware::TileKind::CableTrunk: return IM_COL32(0x2E, 0x1E, 0x5C, 0xFF);
      default: return IM_COL32(0x22, 0x2A, 0x35, 0xFF);
    }
  }
  (void)game;
  return IM_COL32(0x14, 0x1A, 0x22, 0xFF);
}

void ServerRoomView::render(neon::core::Game& game) {
  render_room_tabs(game);
  render_gauges(game);
  if (ImGui::Begin("Server Room")) {
    ImGui::Columns(2, "##roomcols", false);
    ImGui::SetColumnWidth(0, 560);
    render_grid(game);
    ImGui::NextColumn();
    render_side_panel(game);
    ImGui::Columns(1);
  }
  ImGui::End();

  if (ImGui::Begin("Inventory")) {
    render_inventory(game);
  }
  ImGui::End();
}

void ServerRoomView::render_room_tabs(neon::core::Game& game) {
  auto& s = game.state();
  if (ImGui::Begin("##roomtabs", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
    for (auto& [id, room] : s.rooms) {
      bool active = id == s.active_room;
      if (ImGui::Button((room.name + (active ? " *" : "")).c_str())) s.active_room = id;
      ImGui::SameLine();
    }
    if (ImGui::Button("+ Buy expansion room")) {
      game.expand_room(s.active_room);
    }
    ImGui::End();
  }
}

void ServerRoomView::render_gauges(neon::core::Game& game) {
  auto& s = game.state();
  Room& room = game.active_room();
  auto stats = game.room_stats(room.id);
  if (ImGui::Begin("##gauges", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
    ImGui::Text("Power %.0f / %.0f W", stats.power_draw_w, stats.power_capacity_w);
    ImGui::SameLine();
    ImGui::ProgressBar(stats.power_capacity_w > 0 ? stats.power_draw_w / stats.power_capacity_w : 1.0f, ImVec2(140, 0),
                       stats.overloaded ? "OVERLOAD" : "OK");
    ImGui::Text("Heat %.0f / %.0f W", stats.heat_output_w, stats.cooling_capacity_w);
    ImGui::SameLine();
    ImGui::ProgressBar(stats.cooling_capacity_w > 0 ? stats.heat_output_w / stats.cooling_capacity_w : 1.0f, ImVec2(140, 0),
                       stats.overheating ? "OVERHEAT" : "OK");
    ImU32 tcol = stats.temperature_c > 70 ? theme::RED : (stats.temperature_c > 55 ? theme::AMBER : theme::GREEN);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(tcol), "Temp %.1f C", stats.temperature_c);
    ImGui::SameLine();
    ImGui::Text("Uptime %.1f%%", game.room_uptime(room.id) * 100.0);
    if (room.breaker_tripped) {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, theme::RED);
      ImGui::TextUnformatted("BREAKER TRIPPED");
      ImGui::PopStyleColor();
      ImGui::SameLine();
      if (ImGui::SmallButton("Reset breaker")) game.reset_breaker(room.id);
    }
    if (stats.overloaded) {
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::MAGENTA), "WARNING: power draw exceeds capacity!");
    }
    ImGui::End();
  }
}

void ServerRoomView::render_grid(neon::core::Game& game) {
  Room& room = game.active_room();
  ImGui::Text("Room: %s  (%dx%d)", room.name.c_str(), room.width, room.height);
  ImGui::SameLine();
  ImGui::Checkbox("Cable mode", &cable_mode);
  if (cable_mode) {
    ImGui::SameLine();
    ImGui::RadioButton("power", cable_from_port == "power");
    if (ImGui::IsItemClicked()) cable_from_port = "power";
    ImGui::SameLine();
    ImGui::RadioButton("net", cable_from_port == "net");
    if (ImGui::IsItemClicked()) cable_from_port = "net";
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::AMBER), "click a component, then another, to connect %s",
                       cable_from_port.c_str());
  }

  const float tile = 30.0f;
  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImGui::InvisibleButton("##grid", ImVec2(room.width * tile, room.height * tile));
  bool hovered = ImGui::IsItemHovered();
  ImVec2 mouse = ImGui::GetMousePos();
  hover_tile = -1;
  if (hovered) {
    int tx = static_cast<int>((mouse.x - origin.x) / tile);
    int ty = static_cast<int>((mouse.y - origin.y) / tile);
    if (room.in_bounds(tx, ty)) hover_tile = room.tile_index(tx, ty);
  }

  for (int y = 0; y < room.height; ++y) {
    for (int x = 0; x < room.width; ++x) {
      int idx = room.tile_index(x, y);
      ImVec2 p0(origin.x + x * tile, origin.y + y * tile);
      ImVec2 p1(p0.x + tile - 2, p0.y + tile - 2);
      bool selected = !selected_uid.empty() && room.tiles[static_cast<std::size_t>(idx)].occupant_uid == selected_uid;
      dl->AddRectFilled(p0, p1, tile_color(game, room, idx), 2.0f);
      dl->AddRect(p0, p1, selected ? theme::CYAN : IM_COL32(0x00, 0xF0, 0xFF, 40), 2.0f);
      const auto& t = room.tiles[static_cast<std::size_t>(idx)];
      if (!t.occupant_uid.empty()) {
        dl->AddText(ImVec2(p0.x + 3, p0.y + 3), theme::TEXT, tile_kind_name(t.kind));
        const Component* c = room.find(t.occupant_uid);
        if (c && !c->powered) dl->AddText(ImVec2(p0.x + 3, p0.y + 16), theme::RED, "OFF");
      }
      if (idx == hover_tile) dl->AddRect(p0, p1, theme::MAGENTA, 2.0f);
    }
  }

  // cables as bezier curves
  for (const auto& cable : room.cables) {
    auto find_tile_center = [&](const std::string& uid) -> ImVec2 {
      const Component* c = room.find(uid);
      if (c && c->tile_index >= 0) {
        auto [cx, cy] = room.tile_coords(c->tile_index);
        return ImVec2(origin.x + cx * tile + tile / 2, origin.y + cy * tile + tile / 2);
      }
      // nested component (server in rack): draw from its rack tile
      if (c && !c->parent_uid.empty()) {
        const Component* parent = room.find(c->parent_uid);
        if (parent && parent->tile_index >= 0) {
          auto [cx, cy] = room.tile_coords(parent->tile_index);
          return ImVec2(origin.x + cx * tile + tile / 2, origin.y + cy * tile + tile / 2);
        }
      }
      return ImVec2(origin.x, origin.y);
    };
    ImVec2 a = find_tile_center(cable.from_uid);
    ImVec2 b = find_tile_center(cable.to_uid);
    ImU32 col = cable.from_port == "power" ? IM_COL32(0xFF, 0xD7, 0x00, 0xB0) : IM_COL32(0x00, 0xF0, 0xFF, 0xB0);
    dl->AddBezierCubic(a, ImVec2(a.x, a.y + 24), ImVec2(b.x, b.y - 24), b, col, 2.0f);
  }

  // click handling
  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    int idx = hover_tile;
    if (idx >= 0) {
      const auto& t = room.tiles[static_cast<std::size_t>(idx)];
      if (cable_mode) {
        if (t.occupant_uid.empty()) {
          // allow selecting nested servers via rack: pick the rack
        } else if (cable_from_uid.empty()) {
          cable_from_uid = t.occupant_uid;
        } else if (cable_from_uid != t.occupant_uid) {
          game.connect_cable(cable_from_uid, cable_from_port, t.occupant_uid, cable_from_port);
          cable_from_uid.clear();
        }
      } else {
        if (!t.occupant_uid.empty()) selected_uid = t.occupant_uid;
        else if (!selected_uid.empty()) {
          // try to place selected inventory item here
          bool placed = false;
          for (std::size_t i = 0; i < game.state().inventory.size(); ++i) {
            if (game.state().inventory[i].uid == selected_uid) {
              if (game.place_on_tile(selected_uid, room.tile_coords(idx).first, room.tile_coords(idx).second)) {
                selected_uid.clear();
                placed = true;
              }
              break;
            }
          }
          if (!placed) selected_uid.clear();
        }
      }
    }
  }
}

void ServerRoomView::render_side_panel(neon::core::Game& game) {
  Room& room = game.active_room();
  if (selected_uid.empty()) {
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM),
                       "Click a rack/cooler/PDU to inspect.\nSelect an inventory item, then click an empty tile to place it.");
    return;
  }
  // inventory item selected?
  for (auto& c : game.state().inventory) {
    if (c.uid == selected_uid) {
      render_component_detail(game, selected_uid);
      return;
    }
  }
  const Component* c = room.find(selected_uid);
  if (!c) {
    selected_uid.clear();
    return;
  }
  const auto* def = game.catalog().hardware(c->def_id);
  if (!def) return;
  if (def->type == "rack") render_rack_detail(game, selected_uid);
  else if (def->type == "server_unit") render_server_detail(game, selected_uid);
  else render_component_detail(game, selected_uid);
}

void ServerRoomView::render_component_detail(neon::core::Game& game, const std::string& uid_str) {
  auto& s = game.state();
  // find in room or inventory
  Component* c = nullptr;
  Room* room = nullptr;
  for (auto& [id, r] : s.rooms)
    if ((c = r.find(uid_str))) {
      room = &r;
      break;
    }
  bool in_inventory = false;
  if (!c) {
    for (auto& ic : s.inventory)
      if (ic.uid == uid_str) {
        c = &ic;
        in_inventory = true;
        break;
      }
  }
  if (!c) return;
  const auto* def = game.catalog().hardware(c->def_id);
  if (!def) return;
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::CYAN), "%s", def->name.c_str());
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "%s", def->description.c_str());
  ImGui::Text("type %s  tier %d  %.0fW  %.0fW heat", def->type.c_str(), def->tier, def->power_draw, def->heat_output);
  if (!in_inventory) {
    ImGui::Text("dust %d  wear %d  durability %d  %s", c->dust_level, c->wear_level, c->durability,
                c->powered ? "powered" : "UNPOWERED");
    if (ImGui::SmallButton("Clean (canned air)")) game.clean_component(uid_str);
    ImGui::SameLine();
    if (ImGui::SmallButton("Repair (150 cr)")) game.repair_component(uid_str);
    if (room && ImGui::SmallButton("Move to inventory")) {
      if (game.move_to_inventory(uid_str)) selected_uid.clear();
    }
  } else {
    ImGui::Text("In inventory. Click an empty tile to place, or select a rack/server to install into.");
    if (ImGui::SmallButton("Sell (60%%)")) {
      for (std::size_t i = 0; i < s.inventory.size(); ++i)
        if (s.inventory[i].uid == uid_str) {
          game.sell_inventory_item(i);
          selected_uid.clear();
          break;
        }
    }
  }
}

void ServerRoomView::render_rack_detail(neon::core::Game& game, const std::string& rack_uid) {
  Room& room = game.active_room();
  Component* rack = room.find(rack_uid);
  if (!rack) return;
  render_component_detail(game, rack_uid);
  ImGui::Separator();
  int cap = neon::hardware::rack_capacity_u(game.catalog(), *rack);
  int used = neon::hardware::rack_used_u(game.catalog(), room, *rack);
  ImGui::Text("Rack space: %d / %d U", used, cap);
  ImGui::ProgressBar(cap > 0 ? static_cast<float>(used) / cap : 1.0f, ImVec2(200, 0));
  ImGui::Text("Installed servers:");
  for (const auto& uid : rack->installed) {
    ImGui::PushID(uid.c_str());
    const Component* srv = room.find(uid);
    const auto* sdef = srv ? game.catalog().hardware(srv->def_id) : nullptr;
    if (ImGui::Selectable(sdef ? sdef->name.c_str() : uid.c_str(), selected_uid == uid)) selected_uid = uid;
    ImGui::PopID();
  }
  ImGui::Text("Install server from inventory:");
  for (auto& c : game.state().inventory) {
    const auto* def = game.catalog().hardware(c.def_id);
    if (!def || def->type != "server_unit") continue;
    ImGui::PushID(c.uid.c_str());
    if (ImGui::SmallButton(("Install " + def->name).c_str())) {
      if (game.install_in_rack(c.uid, rack_uid)) selected_uid = c.uid;
    }
    // drag & drop onto the rack
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMP_UID")) {
        std::string uid_str(static_cast<const char*>(payload->Data), payload->DataSize);
        if (!uid_str.empty() && uid_str.back() == '\0') uid_str.pop_back();
        if (game.install_in_rack(uid_str, rack_uid)) selected_uid = uid_str;
      }
      ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
  }
}

void ServerRoomView::render_server_detail(neon::core::Game& game, const std::string& server_uid) {
  Room& room = game.active_room();
  Component* server = room.find(server_uid);
  if (!server) return;
  render_component_detail(game, server_uid);
  ImGui::Separator();
  auto stats = neon::hardware::compute_server_stats(game.catalog(), room, *server);
  bool runnable = neon::hardware::server_is_runnable(game.catalog(), room, *server);
  ImGui::Text("CPU %.1f GHz | RAM %.0f GB | Storage %.0f TB | Net %.0f Mbps | Hash %.0f MH/s", stats.ghz, stats.ram_gb,
              stats.storage_tb, stats.mbps, stats.hash_mh);
  ImGui::Text("PSU %.0f W | draw %.0f W | heat %.0f W | fan %.0f W", stats.wattage, stats.power_w, stats.heat_w,
              stats.cooling_w);
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(runnable ? theme::GREEN : theme::RED), runnable ? "ONLINE" : "OFFLINE");
  ImGui::Separator();
  const char* slot_names[] = {"cpu", "ram", "storage", "psu", "fan", "nic", "pcie"};
  for (const char* slot : slot_names) {
    ImGui::PushID(slot);
    std::string cur = server->slots.count(slot) ? server->slots.at(slot) : std::string();
    std::string label = std::string(slot) + ": ";
    if (!cur.empty()) {
      const Component* part = room.find(cur);
      const auto* pdef = part ? game.catalog().hardware(part->def_id) : nullptr;
      label += pdef ? pdef->name : cur;
    } else {
      label += "<empty>";
    }
    if (ImGui::Button(label.c_str())) {
      selected_uid = cur.empty() ? std::string() : cur;
    }
    // drag & drop a compatible part into this slot
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMP_UID")) {
        std::string uid_str(static_cast<const char*>(payload->Data), payload->DataSize);
        if (!uid_str.empty() && uid_str.back() == '\0') uid_str.pop_back();
        game.install_part(uid_str, server_uid, slot);
      }
      ImGui::EndDragDropTarget();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("[%s]", slot);
    ImGui::PopID();
  }
  ImGui::Text("Install part from inventory:");
  for (auto& c : game.state().inventory) {
    const auto* def = game.catalog().hardware(c.def_id);
    if (!def || def->slot_type.empty()) continue;
    ImGui::PushID(c.uid.c_str());
    if (ImGui::SmallButton(("-> " + def->slot_type + ": " + def->name).c_str())) {
      game.install_part(c.uid, server_uid, def->slot_type);
    }
    if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("COMP_UID", c.uid.c_str(), c.uid.size() + 1);
      ImGui::Text("%s", def->name.c_str());
      ImGui::EndDragDropSource();
    }
    ImGui::PopID();
  }
}

void ServerRoomView::render_inventory(neon::core::Game& game) {
  auto& inv = game.state().inventory;
  ImGui::Text("Inventory (%d items) - click to select, drag onto racks/slots, or click a tile to place:",
              static_cast<int>(inv.size()));
  if (inv.empty()) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "(empty - buy hardware in the Market)");
  for (auto& c : inv) {
    const auto* def = game.catalog().hardware(c.def_id);
    if (!def) continue;
    ImGui::PushID(c.uid.c_str());
    bool sel = selected_uid == c.uid;
    if (ImGui::Selectable((def->name + "  [" + def->type + "]").c_str(), sel)) selected_uid = c.uid;
    if (ImGui::BeginDragDropSource()) {
      ImGui::SetDragDropPayload("COMP_UID", c.uid.c_str(), c.uid.size() + 1);
      ImGui::Text("%s", def->name.c_str());
      ImGui::EndDragDropSource();
    }
    ImGui::PopID();
  }
  // consumables
  if (!game.state().inventory_count.empty()) {
    ImGui::Separator();
    ImGui::Text("Consumables:");
    for (auto& [id, n] : game.state().inventory_count) ImGui::Text("  %s x%d", id.c_str(), n);
  }
}

} // namespace neon::ui
