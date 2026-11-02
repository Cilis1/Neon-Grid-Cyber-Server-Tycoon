#include "ui/TechTreeView.hpp"

#include "core/Game.hpp"
#include "ui/UITheme.hpp"
#include "utils/StringUtils.hpp"

#include <imgui.h>

#include <cmath>

namespace neon::ui {

void TechTreeView::render(neon::core::Game& game) {
  if (!ImGui::Begin("Tech Tree")) {
    ImGui::End();
    return;
  }
  ImGui::Text("RP: %.0f   Credits: %s", game.state().rp, utils::format_credits(game.state().credits).c_str());
  ImGui::SameLine();
  ImGui::TextDisabled("(drag to pan, wheel to zoom)");
  ImGui::Separator();

  ImVec2 canvas_size = ImGui::GetContentRegionAvail();
  ImGui::InvisibleButton("##canvas", canvas_size);
  bool hovered = ImGui::IsItemHovered();
  ImVec2 origin = ImGui::GetItemRectMin();
  ImDrawList* dl = ImGui::GetWindowDrawList();
  dl->PushClipRect(origin, ImVec2(origin.x + canvas_size.x, origin.y + canvas_size.y));

  // pan with drag, zoom with wheel
  if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    pan_x += ImGui::GetIO().MouseDelta.x;
    pan_y += ImGui::GetIO().MouseDelta.y;
  }
  if (hovered && ImGui::GetIO().MouseWheel != 0) zoom = std::clamp(zoom * (1.0f + ImGui::GetIO().MouseWheel * 0.1f), 0.4f, 2.5f);

  const float node_w = 150.0f * zoom;
  const float node_h = 44.0f * zoom;
  auto node_pos = [&](const techtree::TechNode& n) {
    return ImVec2(origin.x + pan_x + n.x * zoom, origin.y + pan_y + n.y * zoom);
  };

  // edges first
  for (const auto& [id, n] : game.catalog().tech_nodes()) {
    for (const auto& p : n.prereqs) {
      const auto* pn = game.catalog().tech_node(p);
      if (!pn) continue;
      ImVec2 a = node_pos(*pn);
      ImVec2 b = node_pos(n);
      bool done = game.state().purchased_tech.count(p) > 0;
      dl->AddLine(ImVec2(a.x + node_w, a.y + node_h / 2), ImVec2(b.x, b.y + node_h / 2),
                  done ? IM_COL32(0x00, 0xF0, 0xFF, 0xFF) : IM_COL32(0x40, 0x50, 0x60, 0xFF), zoom);
    }
  }
  // nodes
  ImVec2 mouse = ImGui::GetMousePos();
  for (const auto& [id, n] : game.catalog().tech_nodes()) {
    ImVec2 p = node_pos(n);
    bool purchased = game.state().purchased_tech.count(id) > 0;
    bool available = game.tech().can_purchase(game, id);
    bool locked = !purchased && !game.tech().prereqs_met(game.state().purchased_tech, id);
    ImU32 fill = purchased ? IM_COL32(0x0A, 0x4A, 0x3A, 0xFF)
                           : available ? IM_COL32(0x0A, 0x3A, 0x4A, 0xFF)
                                       : locked ? IM_COL32(0x18, 0x1C, 0x22, 0xFF) : IM_COL32(0x2A, 0x22, 0x10, 0xFF);
    ImU32 border = purchased ? theme::GREEN : available ? theme::CYAN : theme::TEXT_DIM;
    dl->AddRectFilled(p, ImVec2(p.x + node_w, p.y + node_h), fill, 6.0f * zoom);
    dl->AddRect(p, ImVec2(p.x + node_w, p.y + node_h), border, 6.0f * zoom, 0, purchased || available ? 2.0f * zoom : 1.0f);
    ImU32 branch_col = UITheme::branch_color(n.branch);
    dl->AddRectFilled(p, ImVec2(p.x + 6 * zoom, p.y + node_h), branch_col); // branch stripe
    dl->AddText(ImVec2(p.x + 6 * zoom, p.y + 4 * zoom), purchased ? theme::GREEN : theme::TEXT, n.name.c_str());
    char meta[64];
    std::snprintf(meta, sizeof(meta), "T%d %.0fcr %.0frp", n.tier, n.cost_credits, n.cost_rp);
    dl->AddText(ImVec2(p.x + 6 * zoom, p.y + node_h - 14 * zoom), theme::TEXT_DIM, meta);
    if (hovered && mouse.x >= p.x && mouse.x <= p.x + node_w && mouse.y >= p.y && mouse.y <= p.y + node_h &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      selected_node = id;
  }
  dl->PopClipRect();

  // side panel
  ImGui::SameLine();
  ImGui::BeginChild("##nodedetail", ImVec2(300, 0), true);
  if (selected_node.empty()) {
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "Click a node.");
  } else {
    const auto* n = game.catalog().tech_node(selected_node);
    if (n) {
      ImU32 bc = UITheme::branch_color(n->branch);
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(bc), "%s", n->name.c_str());
      ImGui::Text("branch %s | tier %d", n->branch.c_str(), n->tier);
      ImGui::TextWrapped("%s", n->description.c_str());
      ImGui::Text("cost: %.0f cr, %.0f RP", n->cost_credits, n->cost_rp);
      if (!n->prereqs.empty()) {
        ImGui::Text("prereqs:");
        for (auto& p : n->prereqs) {
          bool have = game.state().purchased_tech.count(p) > 0;
          ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(have ? theme::GREEN : theme::RED), "  %s %s", have ? "[x]" : "[ ]",
                             p.c_str());
        }
      }
      bool purchased = game.state().purchased_tech.count(selected_node) > 0;
      if (purchased) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::GREEN), "RESEARCHED");
      } else if (ImGui::Button("Research") && game.research(selected_node)) {
        // purchased this frame
      }
      if (!game.tech().can_purchase(game, selected_node) && !purchased)
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "(locked or unaffordable)");
    }
  }
  ImGui::EndChild();
  ImGui::End();
}

} // namespace neon::ui
