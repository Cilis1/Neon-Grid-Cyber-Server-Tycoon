#include "ui/MainMenu.hpp"

#include "core/Game.hpp"
#include "ui/UITheme.hpp"

#include <imgui.h>

namespace neon::ui {

MenuAction MainMenu::render(neon::core::Game& game, bool has_save) {
  UITheme& theme_ui = UITheme::instance();
  theme_ui.draw_grid_background(static_cast<float>(ImGui::GetTime()));

  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.30f),
                          ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(520, 0));
  ImGui::Begin("##mainmenu", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

  ImGui::PushFont(theme_ui.fonts.heading);
  ImGui::PushStyleColor(ImGuiCol_Text, theme::CYAN);
  ImGui::TextUnformatted("NEON GRID");
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::PushFont(theme_ui.fonts.heading);
  theme_ui.glitch_text("CYBER SERVER TYCOON", theme::MAGENTA);
  ImGui::PopFont();
  ImGui::PushFont(theme_ui.fonts.mono);
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM),
                     "jack in. build the grid. own the city.");
  ImGui::PopFont();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  MenuAction action = MenuAction::None;
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 0.55f, 0.8f));
  if (ImGui::Button("NEW GAME", ImVec2(-1, 40))) action = MenuAction::NewGame;
  ImGui::BeginDisabled(!has_save);
  if (ImGui::Button("LOAD GAME", ImVec2(-1, 40))) action = MenuAction::LoadGame;
  ImGui::EndDisabled();
  if (ImGui::Button("SETTINGS", ImVec2(-1, 40))) settings_open = true;
  if (ImGui::Button("QUIT", ImVec2(-1, 40))) action = MenuAction::Quit;
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::PushFont(theme_ui.fonts.mono);
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM), "v1.0.0  |  F1 help  |  SDL2 + Dear ImGui + Lua");
  ImGui::PopFont();
  ImGui::End();

  if (settings_open) render_settings(game);
  (void)game;
  return action;
}

bool MainMenu::render_settings(neon::core::Game& game) {
  bool open = settings_open;
  if (ImGui::Begin("Settings", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Checkbox("Scanlines", &game.scanlines);
    ImGui::Checkbox("Glitch text", &game.glitch_text);
    ImGui::Checkbox("Fullscreen", &game.fullscreen);
    ImGui::SliderFloat("Volume", &game.volume, 0.0f, 1.0f);
    int speed = static_cast<int>(game.speed);
    if (ImGui::SliderInt("Sim speed (1x=1s/min)", &speed, 0, 3)) {
      static const core::SimSpeed speeds[] = {core::SimSpeed::Paused, core::SimSpeed::Normal, core::SimSpeed::Fast,
                                              core::SimSpeed::VeryFast};
      game.speed = speeds[speed];
    }
    ImGui::TextWrapped("Windowed 1280x720 default. Fullscreen toggles borderless via Alt+Enter/F11.");
  }
  ImGui::End();
  settings_open = open;
  return open;
}

} // namespace neon::ui
