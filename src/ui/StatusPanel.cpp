#include "ui/StatusPanel.hpp"

#include "core/Game.hpp"
#include "ui/UITheme.hpp"
#include "utils/StringUtils.hpp"

#include <imgui.h>

namespace neon::ui {

void StatusPanel::render_top_bar_content(neon::core::Game& game) {
  auto& s = game.state();
  {
    ImGui::PushFont(UITheme::instance().fonts.mono);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::CYAN), "%s", s.time.format().c_str());
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::GREEN), "CRED %s", utils::format_credits(s.credits).c_str());
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::CYAN), "RP %.0f", s.rp);
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::MAGENTA), "SHARDS %.0f", s.data_shards);
    ImGui::SameLine();
    ImU32 heat_col = s.heat < 26 ? theme::GREEN : (s.heat < 51 ? theme::AMBER : (s.heat < 76 ? theme::MAGENTA : theme::RED));
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(heat_col), "HEAT %d", s.heat);
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "REP %+d", s.reputation);
    ImGui::SameLine();
    // speed controls
    const char* speed_names[] = {"||", "1x", "2x", "4x"};
    for (int i = 0; i < 4; ++i) {
      if (i > 0) ImGui::SameLine();
      bool active = static_cast<int>(game.speed) == i;
      if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 1.0f, 0.9f));
      if (ImGui::SmallButton(speed_names[i])) game.speed = static_cast<core::SimSpeed>(i);
      if (active) ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (s.heat >= 51) {
      ImGui::PushStyleColor(ImGuiCol_Text, theme::RED);
      ImGui::TextUnformatted(s.heat >= 76 ? "!! RAID RISK !!" : "! RAID WARNING !");
      ImGui::PopStyleColor();
      ImGui::SameLine();
      if (ImGui::SmallButton("Bribe (reduce heat)")) game.bribe();
    }
    ImGui::PopFont();
  }
}

void StatusPanel::render_hq_content(neon::core::Game& game) {
  auto& s = game.state();
  if (ImGui::BeginTabBar("##hqtabs")) {
      if (ImGui::BeginTabItem("Employees")) {
        ImGui::Text("Roster (%d/%d)", static_cast<int>(s.employees.size()), game.max_employees());
        if (ImGui::Button("Refresh job board (free)")) {
          // show 3 fresh candidates inline
        }
        static std::vector<core::Employee> candidates;
        if (ImGui::Button("Generate candidates")) candidates = game.generate_hires();
        for (auto& cand : candidates) {
          ImGui::PushID(cand.uid.c_str());
          ImGui::Text("%s - %s (skill %d, %.0f/wk, morale %d)", cand.name.c_str(), cand.role.c_str(), cand.skill,
                      cand.salary_weekly, cand.morale);
          ImGui::SameLine();
          if (ImGui::SmallButton("Hire")) {
            game.hire(cand);
            ImGui::PopID();
            break;
          }
          ImGui::PopID();
        }
        ImGui::Separator();
        for (auto& e : s.employees) {
          ImGui::PushID(e.uid.c_str());
          ImGui::Text("%s | %s | skill %d | %.0f/wk | morale %d", e.name.c_str(), e.role.c_str(), e.skill,
                      e.salary_weekly, e.morale);
          ImGui::SameLine();
          if (ImGui::SmallButton("Fire")) game.fire(e.uid);
          ImGui::PopID();
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Remote Servers")) {
        if (s.remote_servers.empty()) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM),
                                                         "No claimed servers yet. Hack abandoned servers in a netrun.");
        for (const auto& rs : s.remote_servers) {
          ImGui::Text("%s [%s]  +%.0f cr/h  heat %.1f  %s", rs.name.c_str(), rs.kind.c_str(), rs.income_per_hour,
                      rs.heat_risk, rs.proxy ? "(proxy)" : "");
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Cyberware")) {
        struct Cyber { const char* id; const char* name; const char* desc; };
        static const Cyber ware[] = {
            {"neural_link", "Neural Link", "+4 deck RAM"},
            {"reflex_tuner", "Reflex Tuner", "+1 action point per netrun turn"},
            {"ice_breaker", "ICE Breaker", "+8 ICE damage"},
            {"ghost_protocol", "Ghost Protocol", "-1.5 trace rate"},
            {"dermal_plating", "Dermal Plating", "+25 shield"},
        };
        auto deck = game.deck_stats();
        ImGui::Text("Deck: RAM %d  CPU %.1f  Shield %d  Slots %d", deck.ram, deck.cpu_speed, deck.max_shield,
                    deck.program_slots);
        for (const auto& w : ware) {
          bool owned = s.cyberware.count(w.id) && s.cyberware.at(w.id);
          ImGui::PushID(w.id);
          ImGui::Text("%s - %s", w.name, w.desc);
          ImGui::SameLine();
          if (owned) {
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::GREEN), "INSTALLED");
          } else {
            double price = 2500.0;
            if (ImGui::SmallButton("Buy & install")) {
              if (s.credits >= price) {
                s.credits -= price;
                s.cyberware[w.id] = true;
              }
            }
          }
          ImGui::PopID();
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Factions")) {
        for (const auto& [id, rep] : s.faction_rep) {
          const auto* def = game.catalog().faction(id);
          ImGui::Text("%s: %+d", def ? def->name.c_str() : id.c_str(), rep);
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Netrun Targets")) {
        for (const auto& t : s.netrun_targets) {
          ImGui::Text("%s [%s] diff %d  reward %d cr / %d shards%s", t.name.c_str(), t.kind.c_str(), t.difficulty,
                      t.reward_credits, t.reward_shards, t.claimed ? "  CLAIMED" : "");
        }
        ImGui::EndTabItem();
      }
    ImGui::EndTabBar();
  }
}

} // namespace neon::ui
