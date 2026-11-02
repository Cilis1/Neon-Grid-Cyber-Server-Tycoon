#include "ui/ContractView.hpp"

#include "core/Game.hpp"
#include "economy/ContractManager.hpp"
#include "ui/UITheme.hpp"
#include "utils/StringUtils.hpp"

#include <imgui.h>

namespace neon::ui {

void ContractView::render_content(neon::core::Game& game) {
  auto& s = game.state();
  ImGui::Text("Active %d/%d - board refreshes daily", static_cast<int>(s.contracts.active.size()),
              game.state().contracts.max_active(game));
  ImGui::Separator();

  if (ImGui::BeginTabBar("##contracttabs")) {
    if (ImGui::BeginTabItem("Board (offers)")) {
      if (s.contracts.offers.empty())
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "No offers right now. Check back tomorrow.");
      for (auto& o : s.contracts.offers) {
        ImGui::PushID(o.uid.c_str());
        ImU32 col = o.type == "botnet" || o.type == "data_broker" ? theme::MAGENTA : theme::CYAN;
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", o.title.c_str());
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "  %s | client %s | %s | risk %.0f%%",
                           o.type.c_str(), o.client.name.c_str(), o.description.c_str(), o.risk * 100.0);
        ImGui::Text("  reward %s | deadline %llu min", utils::format_credits(o.reward).c_str(),
                    static_cast<unsigned long long>(o.deadline_minute - s.time.minute));
        if (o.requirements.is_object() && !o.requirements.empty()) {
          ImGui::Text("  requires:");
          for (auto& [k, v] : o.requirements.items()) ImGui::Text("    %s: %s", k.c_str(), v.dump().c_str());
        }
        if (ImGui::SmallButton("Accept")) game.state().contracts.accept(game, o.uid);
        ImGui::PopID();
        ImGui::Separator();
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Active")) {
      for (auto& c : s.contracts.active) {
        if (c.status != neon::economy::ContractStatus::Active) continue;
        ImGui::PushID(c.uid.c_str());
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::CYAN), "%s", c.title.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", c.type.c_str());
        ImGui::ProgressBar(static_cast<float>(c.progress), ImVec2(220, 0));
        ImGui::Text("reward %s | income so far %s | SLA %.1f%%", utils::format_credits(c.reward).c_str(),
                    utils::format_credits(c.income_accrued).c_str(),
                    c.sla_total > 0 ? c.sla_ok / c.sla_total * 100.0 : 100.0);
        if (c.type == "courier" && !c.courier_started) {
          if (ImGui::SmallButton("Choose route...")) courier_contract_uid = c.uid;
        }
        if (!courier_contract_uid.empty() && courier_contract_uid == c.uid) {
          auto routes = neon::economy::ContractManager::courier_routes();
          for (int i = 0; i < static_cast<int>(routes.size()); ++i) {
            ImGui::PushID(i);
            if (ImGui::Button((routes[i].name + "  (" + std::to_string(routes[i].time_h) + "h, risk " +
                               std::to_string(static_cast<int>(routes[i].risk * 100)) + "%, cost " +
                               utils::format_credits(routes[i].cost) + ")").c_str())) {
              if (game.state().contracts.start_courier(game, c.uid, i)) courier_contract_uid.clear();
            }
            ImGui::PopID();
          }
        }
        ImGui::PopID();
        ImGui::Separator();
      }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("History")) {
      for (auto& c : s.contracts.active) {
        if (c.status == neon::economy::ContractStatus::Active) continue;
        const char* st = c.status == neon::economy::ContractStatus::Completed ? "DONE" : "FAILED";
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(c.status == neon::economy::ContractStatus::Completed ? theme::GREEN : theme::RED),
                           "%s - %s (%s)", st, c.title.c_str(), c.type.c_str());
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

} // namespace neon::ui
