#pragma once

#include <string>

#include <imgui.h>

namespace neon::ui {

// Cyberpunk UI theme: dark navy panels, neon cyan primary, magenta warnings,
// green success, red danger.
namespace theme {
inline constexpr ImU32 BG = IM_COL32(0x0A, 0x0E, 0x14, 0xFF);
inline constexpr ImU32 PANEL = IM_COL32(0x11, 0x16, 0x1D, 0xF2);
inline constexpr ImU32 PANEL_LIGHT = IM_COL32(0x1A, 0x21, 0x2B, 0xFF);
inline constexpr ImU32 CYAN = IM_COL32(0x00, 0xF0, 0xFF, 0xFF);
inline constexpr ImU32 MAGENTA = IM_COL32(0xFF, 0x00, 0xA0, 0xFF);
inline constexpr ImU32 GREEN = IM_COL32(0x39, 0xFF, 0x14, 0xFF);
inline constexpr ImU32 RED = IM_COL32(0xFF, 0x31, 0x31, 0xFF);
inline constexpr ImU32 AMBER = IM_COL32(0xFF, 0xD7, 0x00, 0xFF);
inline constexpr ImU32 TEXT = IM_COL32(0xC9, 0xD4, 0xE3, 0xFF);
inline constexpr ImU32 TEXT_DIM = IM_COL32(0x6B, 0x7A, 0x90, 0xFF);
inline constexpr ImU32 GRID = IM_COL32(0x00, 0xF0, 0xFF, 0x14);
} // namespace theme

struct Fonts {
  ImFont* heading = nullptr; // Orbitron
  ImFont* mono = nullptr;    // Share Tech Mono
  ImFont* regular = nullptr; // ImGui default
};

// Global UI state shared by all views.
class UITheme {
public:
  static UITheme& instance();

  Fonts fonts;
  bool fonts_loaded = false;

  void apply();             // ImGui style colors
  void load_fonts(const char* heading_path, const char* mono_path); // nullptr -> skip
  // Full-screen scanline overlay (call last in the frame).
  void draw_scanlines();
  // Glitchy title text: random character flicker (deterministic per frame seed).
  void glitch_text(const std::string& text, ImU32 color);
  // Neon grid background for menus.
  void draw_grid_background(float time_sec);
  // Small colored badge.
  void badge(const char* text, ImU32 color);

  // Branch colors for the tech tree.
  static ImU32 branch_color(const std::string& branch);
};

} // namespace neon::ui
