#include "ui/MarketView.hpp"

#include "core/Game.hpp"
#include "economy/Market.hpp"
#include "ui/UITheme.hpp"
#include "utils/StringUtils.hpp"

#include <imgui.h>

namespace neon::ui {

void MarketView::render(neon::core::Game& game) {
  auto& s = game.state();
  if (!ImGui::Begin("Market")) {
    ImGui::End();
    return;
  }
  ImGui::Text("Credits: %s", utils::format_credits(s.credits).c_str());
  ImGui::Separator();
  if (ImGui::BeginTabBar("##markettabs")) {
    if (ImGui::BeginTabItem("Hardware Shop")) {
      ImGui::BeginChild("##shop", ImVec2(0, 0), true);
      for (const auto& [id, def] : game.catalog().hardware_items()) {
        if (def.type == "consumable") continue;
        bool unlocked = game.is_item_unlocked(id);
        ImGui::PushID(id.c_str());
        if (!unlocked) ImGui::BeginDisabled();
        ImGui::Text("%s", def.name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("t%d", def.tier);
        ImGui::TextWrapped("%s", def.description.c_str());
        ImGui::Text("%s | %.0fW | %.0fW heat%s", utils::format_credits(def.price).c_str(), def.power_draw,
                    def.heat_output, def.slot_type.empty() ? "" : (" | slot " + def.slot_type).c_str());
        if (ImGui::SmallButton("Buy")) game.buy_hardware(id);
        if (!unlocked) {
          ImGui::EndDisabled();
          ImGui::SameLine();
          ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "(locked - research to unlock)");
        }
        ImGui::PopID();
        ImGui::Separator();
        if (ImGui::GetColumnIndex() == 0) ImGui::NextColumn();
      }
      ImGui::Columns(1);
      ImGui::EndChild();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Commodity Market")) {
      neon::economy::Market market;
      market.load(game.catalog().market_items());
      ImGui::BeginChild("##commodities", ImVec2(0, 0), true);
      ImGui::Columns(5, "##mktcols", true);
      ImGui::Text("Item");
      ImGui::NextColumn();
      ImGui::Text("Price");
      ImGui::NextColumn();
      ImGui::Text("Owned");
      ImGui::NextColumn();
      ImGui::Text("Buy");
      ImGui::NextColumn();
      ImGui::Text("Sell");
      ImGui::NextColumn();
      for (const auto& [id, def] : game.catalog().market_items()) {
        double price = market.price(s.market_prices, id);
        int owned = s.inventory_count.count(id) ? s.inventory_count.at(id) : 0;
        ImGui::PushID(id.c_str());
        if (def.black_market) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::MAGENTA), "%s *", def.name.c_str());
        else ImGui::Text("%s", def.name.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", utils::format_credits(price).c_str());
        ImGui::NextColumn();
        ImGui::Text("%d", owned);
        ImGui::NextColumn();
        ImGui::BeginDisabled(s.credits < price);
        if (ImGui::SmallButton("Buy")) market.buy(game, id, 1);
        ImGui::EndDisabled();
        ImGui::NextColumn();
        ImGui::BeginDisabled(owned <= 0);
        if (ImGui::SmallButton("Sell")) market.sell(game, id, 1);
        ImGui::EndDisabled();
        ImGui::NextColumn();
        ImGui::PopID();
      }
      ImGui::Columns(1);
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "* black market - purchases add heat. Prices refresh daily.");
      ImGui::EndChild();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Crypto")) {
      double price = s.market_prices.count("crypto_neon") ? s.market_prices.at("crypto_neon") : 2.0;
      ImGui::Text("NEON coin price: %s", utils::format_credits(price).c_str());
      ImGui::Text("Mined NEON: %.4f", s.crypto_amount);
      ImGui::Text("Hash power: %.0f MH/s -> %.2f cr/h", game.room_total_hash(), game.mining_credits_per_hour());
      if (ImGui::Button("Sell all mined NEON")) game.sell_crypto(1.0);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();
}

} // namespace neon::ui
