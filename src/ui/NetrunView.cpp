#include "ui/NetrunView.hpp"

#include "core/Game.hpp"
#include "ui/UITheme.hpp"
#include "utils/StringUtils.hpp"

#include <imgui.h>

namespace neon::ui {

using neon::netrunning::NetrunSession;
using neon::netrunning::NodeType;

void NetrunView::render(neon::core::Game& game) {
  if (game.session()) {
    if (ImGui::Begin("Netrun")) {
      render_session(game);
      ImGui::End();
    }
  } else {
    if (ImGui::Begin("Netrun")) {
      render_target_picker(game);
      ImGui::End();
    }
  }
}

void NetrunView::render_target_picker(neon::core::Game& game) {
  auto& s = game.state();
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::CYAN), "JACK IN - choose a target");
  ImGui::Text("Deck: RAM %d | shield %d | slots %d", game.deck_stats().ram, game.deck_stats().max_shield,
              game.deck_stats().program_slots);
  ImGui::Text("Claimed proxies: %d (using a proxy halves trace rate)", static_cast<int>(s.remote_servers.size()));
  ImGui::Separator();
  ImGui::BeginChild("##targets", ImVec2(0, 260), true);
  for (const auto& t : s.netrun_targets) {
    ImGui::PushID(t.id.c_str());
    ImU32 col = t.kind == "corporate" ? theme::RED : (t.kind == "faction" ? theme::MAGENTA : theme::GREEN);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s [%s] diff %d", t.name.c_str(), t.kind.c_str(), t.difficulty);
    ImGui::SameLine();
    if (t.claimed) {
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::GREEN), "CLAIMED");
    } else {
      bool can_proxy = !s.remote_servers.empty();
      ImGui::BeginDisabled(!can_proxy);
      if (ImGui::SmallButton("via proxy")) {
        pending_target = t.id;
        pending_proxy = true;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::SmallButton("direct")) {
        pending_target = t.id;
        pending_proxy = false;
      }
    }
    ImGui::PopID();
  }
  ImGui::EndChild();
  if (!pending_target.empty()) {
    if (game.start_netrun(pending_target, pending_proxy)) pending_target.clear();
    else ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::RED), "cannot start run");
  }
}

static ImU32 node_color(NodeType t, bool visited, bool looted) {
  if (looted) return IM_COL32(0x2A, 0x1A, 0x05, 0xFF);
  switch (t) {
    case NodeType::Entry: return IM_COL32(0x0A, 0x4A, 0x2A, 0xFF);
    case NodeType::Root: return IM_COL32(0x4A, 0x0A, 0x2A, 0xFF);
    case NodeType::Data: return visited ? IM_COL32(0x0A, 0x3A, 0x4A, 0xFF) : IM_COL32(0x10, 0x30, 0x44, 0xFF);
    case NodeType::ICE: return IM_COL32(0x4A, 0x10, 0x10, 0xFF);
    case NodeType::Control: return IM_COL32(0x3A, 0x2A, 0x0A, 0xFF);
    case NodeType::Exit: return IM_COL32(0x0A, 0x3A, 0x3A, 0xFF);
    default: return IM_COL32(0x12, 0x18, 0x20, 0xFF);
  }
}

void NetrunView::render_session(neon::core::Game& game) {
  auto* s = game.session();
  if (!s) return;
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::MAGENTA), "TARGET: %s", s->target.name.c_str());
  ImGui::SameLine();
  ImGui::Text("turn %d | AP %d", s->turn, s->action_points);

  // meters
  ImGui::Text("Detection");
  ImGui::SameLine();
  ImGui::ProgressBar(static_cast<float>(s->detection / 100.0), ImVec2(160, 0), s->detection >= 60 ? "ALERT" : "hidden");
  ImGui::SameLine();
  ImGui::Text("Trace");
  ImGui::SameLine();
  ImGui::ProgressBar(static_cast<float>(s->trace / 100.0), ImVec2(160, 0), s->trace >= 80 ? "TRACING" : "ok");
  ImGui::SameLine();
  ImGui::Text("Shield %d/%d", s->shield, s->max_shield);

  const float cell = 34.0f;
  ImVec2 origin = ImGui::GetCursorScreenPos();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  ImGui::InvisibleButton("##netgrid", ImVec2(s->map.width * cell, s->map.height * cell));
  bool hovered = ImGui::IsItemHovered();
  ImVec2 mouse = ImGui::GetMousePos();

  for (int y = 0; y < s->map.height; ++y) {
    for (int x = 0; x < s->map.width; ++x) {
      const auto* n = s->map.at(x, y);
      ImVec2 p0(origin.x + x * cell, origin.y + y * cell);
      ImVec2 p1(p0.x + cell - 2, p0.y + cell - 2);
      dl->AddRectFilled(p0, p1, node_color(n->type, n->visited, n->looted), 3.0f);
      dl->AddRect(p0, p1, IM_COL32(0x00, 0xF0, 0xFF, 30), 3.0f);
      if (n->type == NodeType::Data && !n->looted)
        dl->AddText(ImVec2(p0.x + 8, p0.y + 10), theme::CYAN, "DAT");
      if (n->type == NodeType::Root) dl->AddText(ImVec2(p0.x + 4, p0.y + 10), theme::MAGENTA, "ROOT");
      if (n->type == NodeType::Entry) dl->AddText(ImVec2(p0.x + 4, p0.y + 10), theme::GREEN, "IN");
      if (n->type == NodeType::Exit) dl->AddText(ImVec2(p0.x + 6, p0.y + 10), theme::CYAN, "OUT");
      if (n->type == NodeType::ICE) dl->AddText(ImVec2(p0.x + 8, p0.y + 10), theme::RED, "ICE");
    }
  }
  // ICE units
  for (const auto& ice : s->map.ices) {
    ImVec2 c(origin.x + ice.x * cell + cell / 2, origin.y + ice.y * cell + cell / 2);
    dl->AddCircleFilled(c, cell / 2 - 4, ice.alert ? theme::RED : IM_COL32(0x8B, 0x00, 0x00, 0xFF));
    dl->AddText(ImVec2(c.x - 8, c.y - 7), theme::TEXT, "ICE");
  }
  // player avatar
  {
    ImVec2 c(origin.x + s->px * cell + cell / 2, origin.y + s->py * cell + cell / 2);
    dl->AddCircleFilled(c, cell / 2 - 4, theme::CYAN);
    dl->AddCircle(c, cell / 2 - 4, theme::MAGENTA, 0, 2.0f);
  }
  // click to move one step toward the clicked cell
  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    int tx = static_cast<int>((mouse.x - origin.x) / cell);
    int ty = static_cast<int>((mouse.y - origin.y) / cell);
    int dx = tx - s->px;
    int dy = ty - s->py;
    if (std::abs(dx) + std::abs(dy) == 1) s->move(dx, dy);
    else if (std::abs(dx) > 0 && std::abs(dy) > 0) {
      // diagonal click: step in the dominant direction
      if (std::abs(dx) >= std::abs(dy)) s->move(dx > 0 ? 1 : -1, 0);
      else s->move(0, dy > 0 ? 1 : -1);
    }
  }

  ImGui::Text("Click an adjacent tile to move. Use programs below, then End Turn.");
  // program bar
  for (const auto& pid : s->programs) {
    const auto* def = game.catalog().program(pid);
    if (!def) continue;
    ImGui::PushID(pid.c_str());
    int cd = s->program_cooldown(pid);
    bool can = s->is_running() && s->action_points > 0 && cd == 0;
    ImGui::BeginDisabled(!can);
    if (ImGui::Button(def->name.c_str())) s->use_program(pid);
    ImGui::EndDisabled();
    if (cd > 0) {
      ImGui::SameLine();
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "cd %d", cd);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\nRAM %d | cd %d | %s", def->description.c_str(), def->ram_cost,
                                                   def->cooldown, def->effect_type.c_str());
    ImGui::PopID();
  }
  ImGui::SameLine();
  if (ImGui::Button("End Turn")) s->end_turn();
  ImGui::SameLine();
  if (ImGui::Button("Flee (at exit)")) s->flee();
  ImGui::SameLine();
  if (ImGui::Button("Abort run")) {
    game.resolve_netrun();
  }

  // outcome + log
  if (!s->is_running()) {
    const char* msg = s->state == NetrunSession::State::Won ? "RUN WON - server exploited!"
                     : s->state == NetrunSession::State::Fled ? "Jacked out cleanly with loot."
                     : s->state == NetrunSession::State::Traced ? "TRACED - heat increased!"
                                                             : "RUN FAILED - deck flatlined.";
    ImU32 col = s->state == NetrunSession::State::Won || s->state == NetrunSession::State::Fled ? theme::GREEN : theme::RED;
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", msg);
    ImGui::Text("Loot: %d shards, %d credits", s->shards_stolen, s->credits_stolen);
    if (ImGui::Button("Close")) game.resolve_netrun();
  }
  ImGui::Separator();
  ImGui::BeginChild("##netlog", ImVec2(0, 120), true);
  ImGui::PushFont(UITheme::instance().fonts.mono);
  for (const auto& line : s->log) ImGui::TextUnformatted(line.c_str());
  ImGui::PopFont();
  ImGui::EndChild();
}

} // namespace neon::ui
