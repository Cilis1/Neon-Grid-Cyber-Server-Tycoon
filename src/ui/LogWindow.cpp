#include "ui/LogWindow.hpp"

#include "ui/UITheme.hpp"
#include "utils/Logger.hpp"

#include <imgui.h>

namespace neon::ui {

void LogWindow::render(neon::core::Game& game) {
  (void)game;
  if (!visible) return;
  if (ImGui::Begin("Log", &visible)) {
    ImGui::PushFont(UITheme::instance().fonts.mono);
    auto lines = utils::LogBuffer::instance().snapshot(400);
    ImGui::BeginChild("##logscroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : lines) {
      ImU32 col = theme::TEXT_DIM;
      if (line.find("[error]") != std::string::npos) col = theme::RED;
      else if (line.find("[warn]") != std::string::npos) col = theme::AMBER;
      else if (line.find("[info]") != std::string::npos) col = theme::TEXT;
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::PopFont();
  }
  ImGui::End();
}

} // namespace neon::ui
