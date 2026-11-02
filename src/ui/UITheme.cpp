#include "ui/UITheme.hpp"

#include <cmath>
#include <cstdlib>

namespace neon::ui {

UITheme& UITheme::instance() {
  static UITheme t;
  return t;
}

void UITheme::apply() {
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 4.0f;
  style.FrameRounding = 3.0f;
  style.GrabRounding = 3.0f;
  style.ScrollbarRounding = 3.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.WindowPadding = ImVec2(8, 8);
  style.FramePadding = ImVec2(6, 4);

  ImVec4* c = style.Colors;
  c[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(theme::PANEL);
  c[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.075f, 0.10f, 0.95f);
  c[ImGuiCol_Border] = ImVec4(0.0f, 0.94f, 1.0f, 0.25f);
  c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.09f, 0.12f, 1.0f);
  c[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.55f, 0.65f, 0.6f);
  c[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.7f, 0.8f, 0.7f);
  c[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.06f, 0.09f, 1.0f);
  c[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.35f, 0.42f, 1.0f);
  c[ImGuiCol_MenuBarBg] = ImVec4(0.04f, 0.06f, 0.09f, 1.0f);
  c[ImGuiCol_Header] = ImVec4(0.0f, 0.45f, 0.55f, 0.5f);
  c[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.6f, 0.7f, 0.6f);
  c[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.7f, 0.8f, 0.8f);
  c[ImGuiCol_Button] = ImVec4(0.0f, 0.45f, 0.55f, 0.55f);
  c[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.75f, 0.9f, 0.8f);
  c[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.94f, 1.0f, 1.0f);
  c[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(theme::TEXT);
  c[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(theme::TEXT_DIM);
  c[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(theme::CYAN);
  c[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(theme::CYAN);
  c[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.63f, 1.0f);
  c[ImGuiCol_Separator] = ImVec4(0.0f, 0.7f, 0.8f, 0.4f);
  c[ImGuiCol_Tab] = ImVec4(0.05f, 0.08f, 0.11f, 1.0f);
  c[ImGuiCol_TabHovered] = ImVec4(0.0f, 0.6f, 0.7f, 0.8f);
  c[ImGuiCol_TabSelected] = ImVec4(0.0f, 0.5f, 0.62f, 1.0f);
  c[ImGuiCol_TabDimmed] = ImVec4(0.05f, 0.08f, 0.11f, 1.0f);
  c[ImGuiCol_TabDimmedSelected] = ImVec4(0.0f, 0.4f, 0.5f, 1.0f);
  c[ImGuiCol_DockingPreview] = ImVec4(0.0f, 0.94f, 1.0f, 0.4f);
}

void UITheme::load_fonts(const char* heading_path, const char* mono_path) {
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->Clear();
  fonts.regular = io.Fonts->AddFontDefault();
  if (heading_path) fonts.heading = io.Fonts->AddFontFromFileTTF(heading_path, 22.0f);
  if (mono_path) fonts.mono = io.Fonts->AddFontFromFileTTF(mono_path, 15.0f);
  if (!fonts.heading) fonts.heading = fonts.regular;
  if (!fonts.mono) fonts.mono = fonts.regular;
  fonts_loaded = true;
  io.Fonts->Build();
}

void UITheme::draw_scanlines() {
  ImDrawList* dl = ImGui::GetBackgroundDrawList();
  ImVec2 size = ImGui::GetIO().DisplaySize;
  for (float y = 0; y < size.y; y += 3.0f) dl->AddLine(ImVec2(0, y), ImVec2(size.x, y), IM_COL32(0, 0, 0, 13));
}

void UITheme::glitch_text(const std::string& text, ImU32 color) {
  // deterministic per-frame flicker
  std::string t = text;
  unsigned seed = static_cast<unsigned>(ImGui::GetFrameCount());
  if ((seed % 17) == 0 && t.size() > 4) {
    std::size_t i = (seed * 7) % t.size();
    t[i] = "#@%&*?"[seed % 6];
  }
  ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color), "%s", t.c_str());
}

void UITheme::draw_grid_background(float time_sec) {
  ImDrawList* dl = ImGui::GetBackgroundDrawList();
  ImVec2 size = ImGui::GetIO().DisplaySize;
  dl->AddRectFilled(ImVec2(0, 0), size, theme::BG);
  const float spacing = 48.0f;
  float offset = std::fmod(time_sec * 20.0f, spacing);
  for (float x = -offset; x < size.x; x += spacing) dl->AddLine(ImVec2(x, 0), ImVec2(x, size.y), theme::GRID);
  for (float y = -offset; y < size.y; y += spacing) dl->AddLine(ImVec2(0, y), ImVec2(size.x, y), theme::GRID);
  // horizon glow
  ImVec2 center(size.x * 0.5f, size.y * 0.62f);
  for (int i = 0; i < 24; ++i) {
    float r = 60.0f + i * 26.0f + std::fmod(time_sec * 40.0f, 26.0f);
    dl->AddCircle(center, r, IM_COL32(0xFF, 0x00, 0xA0, 10), 0, 1.5f);
  }
}

void UITheme::badge(const char* text, ImU32 color) {
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  ImGui::TextUnformatted(text);
  ImGui::PopStyleColor();
}

ImU32 UITheme::branch_color(const std::string& branch) {
  if (branch == "Hardware") return IM_COL32(0x4F, 0xA3, 0xFF, 0xFF);
  if (branch == "Software") return IM_COL32(0x39, 0xFF, 0x14, 0xFF);
  if (branch == "Netrunning") return IM_COL32(0xFF, 0x00, 0xA0, 0xFF);
  if (branch == "Economy") return IM_COL32(0xFF, 0xD7, 0x00, 0xFF);
  if (branch == "Stealth") return IM_COL32(0xB2, 0x6B, 0xFF, 0xFF);
  if (branch == "Infrastructure") return IM_COL32(0x00, 0xF0, 0xFF, 0xFF);
  return theme::TEXT;
}

} // namespace neon::ui
