// Neon Grid: Cyber Server Tycoon - entry point.
// SDL2 window + Dear ImGui (docking) UI. Fixed-timestep simulation:
// 1 real second = 1 in-game minute at 1x speed.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

// We provide our own console `main` (no SDL2main); stop SDL_main.h from
// renaming `main` to `SDL_main` on Windows.
#define SDL_MAIN_HANDLED
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <spdlog/spdlog.h>

#include "core/Game.hpp"
#include "core/SaveLoad.hpp"
#include "ui/ContractView.hpp"
#include "ui/LogWindow.hpp"
#include "ui/MainMenu.hpp"
#include "ui/MarketView.hpp"
#include "ui/NetrunView.hpp"
#include "ui/ServerRoomView.hpp"
#include "ui/StatusPanel.hpp"
#include "ui/TechTreeView.hpp"
#include "ui/UITheme.hpp"
#include "utils/FileSystem.hpp"
#include "utils/Logger.hpp"

using namespace neon;

namespace {

struct AppState {
  bool in_game = false;
  bool quit = false;
  bool show_help = false;
  bool show_settings = false;
  std::uint64_t last_saved_day = 0;
};

void render_help() {
  if (ImGui::Begin("Help / Tutorial (F1)", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted(
        "GOAL: build a server empire in the neon city.\n"
        "\n"
        "CORE LOOP\n"
        "  1. Inspect your server room (Room tab). Watch power/heat gauges.\n"
        "  2. Buy hardware in the Market, place it on empty tiles, cable it up\n"
        "     (Cable mode: power = yellow, net = cyan). Servers need BOTH a\n"
        "     power cable to a PDU and a net cable to a switch.\n"
        "  3. Accept contracts (Contracts tab) - courier runs are a route minigame.\n"
        "  4. Research the Tech Tree (RP from research lab tech + contracts).\n"
        "  5. Netrun (Netrun tab): click adjacent tiles to move, use programs,\n"
        "     reach ROOT and Backdoor/Data-Spike it to claim the server.\n"
        "  6. Manage HEAT (bribes, proxies) - at 76+ you get raided.\n"
        "\n"
        "CONTROLS\n"
        "  F1 help | F11 fullscreen | ESC menu | speed buttons top-right\n"
        "  1 real second = 1 in-game minute at 1x.\n"
        "\n"
        "TIPS\n"
        "  - Dust builds up over time; clean components with canned air.\n"
        "  - Overload the PDU and the breaker trips. Overcool or components fry.\n"
        "  - Claimed abandoned servers give passive income and act as proxies.\n");
  }
  ImGui::End();
}

void render_settings_dialog(core::Game& game, bool& open) {
  if (ImGui::Begin("Settings", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Checkbox("Scanlines", &game.scanlines);
    ImGui::Checkbox("Glitch text", &game.glitch_text);
    ImGui::Checkbox("Fullscreen", &game.fullscreen);
    ImGui::SliderFloat("Volume", &game.volume, 0.0f, 1.0f);
    int speed = static_cast<int>(game.speed);
    if (ImGui::SliderInt("Sim speed", &speed, 0, 3)) {
      static const core::SimSpeed speeds[] = {core::SimSpeed::Paused, core::SimSpeed::Normal, core::SimSpeed::Fast,
                                              core::SimSpeed::VeryFast};
      game.speed = speeds[speed];
    }
    ImGui::Separator();
    if (ImGui::Button("Save now")) core::save_game(game, core::default_save_path());
    ImGui::SameLine();
    ImGui::TextDisabled("autosaves each in-game day");
  }
  ImGui::End();
}

} // namespace

int main(int argc, char** argv) {
  // ---- args ----
  std::optional<std::filesystem::path> data_override;
  bool reset_save = false;
  bool installed_flag = false;
  bool start_fullscreen = false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--reset") reset_save = true;
    else if (a == "--installed") installed_flag = true;
    else if (a == "--fullscreen") start_fullscreen = true;
    else if ((a == "--data" || a == "--data-dir") && i + 1 < argc) data_override = std::filesystem::path(argv[++i]);
  }
  (void)installed_flag;

  utils::init_logging();
  spdlog::info("=== Neon Grid starting ===");

  if (reset_save) {
    std::error_code ec;
    std::filesystem::remove(core::default_save_path(), ec);
    spdlog::info("--reset: save file deleted");
  }

  // ---- SDL ----
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
    spdlog::error("SDL_Init failed: {}", SDL_GetError());
    return 1;
  }
  SDL_Window* window = SDL_CreateWindow("Neon Grid: Cyber Server Tycoon", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window) {
    spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
    return 1;
  }
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (!renderer) {
    spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
    return 1;
  }
  SDL_SetRenderDrawColor(renderer, 0x0A, 0x0E, 0x14, 0xFF);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // Persist window/tab layout in app data (not next to the exe, which may be read-only).
  std::error_code mk_ec;
  std::filesystem::create_directories(utils::fs::app_data_dir(), mk_ec);
  static const std::string ini_path = (utils::fs::app_data_dir() / "imgui.ini").string();
  io.IniFilename = ini_path.c_str();
  ImGui::StyleColorsDark();
  ui::UITheme::instance().apply();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  // fonts (downloaded by scripts/download_fonts.py; fallback to default)
  {
    auto exe = utils::fs::exe_dir();
    std::string heading = (exe / "assets" / "fonts" / "Orbitron-Bold.ttf").string();
    std::string mono = (exe / "assets" / "fonts" / "VT323-Regular.ttf").string();
    if (!utils::fs::exists(heading)) heading = (std::filesystem::current_path() / "assets" / "fonts" / "Orbitron-Bold.ttf").string();
    if (!utils::fs::exists(mono)) mono = (std::filesystem::current_path() / "assets" / "fonts" / "VT323-Regular.ttf").string();
    if (utils::fs::exists(heading) && utils::fs::exists(mono))
      ui::UITheme::instance().load_fonts(heading.c_str(), mono.c_str());
    else
      ui::UITheme::instance().load_fonts(nullptr, nullptr);
  }

  // optional audio (no bundled sounds in MVP; just init the mixer)
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0) {
    spdlog::info("SDL_mixer initialized");
  } else {
    spdlog::warn("SDL_mixer init failed (audio disabled): {}", Mix_GetError());
  }
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();

  // ---- game ----
  core::Game game;
  if (!game.init(data_override)) {
    spdlog::error("failed to init game (data dir not found)");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Neon Grid", "Could not find the data/ directory next to the executable.", window);
    return 1;
  }
  game.fullscreen = start_fullscreen;

  ui::MainMenu menu;
  ui::StatusPanel status_panel;
  ui::ServerRoomView room_view;
  ui::TechTreeView tech_view;
  ui::NetrunView netrun_view;
  ui::ContractView contract_view;
  ui::MarketView market_view;
  ui::LogWindow log_window;
  AppState app;

  const bool has_save = utils::fs::exists(core::default_save_path());

  // ---- main loop ----
  using clock = std::chrono::steady_clock;
  auto prev = clock::now();
  double accumulator = 0.0;
  bool running = true;
  while (running && !app.quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) running = false;
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_F1) app.show_help = !app.show_help;
        if (event.key.keysym.sym == SDLK_F11) {
          game.fullscreen = !game.fullscreen;
          SDL_SetWindowFullscreen(window, game.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
        if (event.key.keysym.sym == SDLK_ESCAPE && app.in_game) {
          // ESC: save + back to menu
          core::save_game(game, core::default_save_path());
          app.in_game = false;
        }
      }
    }

    // fixed-timestep simulation
    auto now = clock::now();
    double dt = std::chrono::duration<double>(now - prev).count();
    prev = now;
    if (dt > 0.25) dt = 0.25; // clamp huge frames
    double spm = core::seconds_per_minute(game.speed);
    if (app.in_game && spm > 0.0) {
      accumulator += dt;
      int ticks = 0;
      while (accumulator >= spm && ticks < 8) {
        game.tick();
        accumulator -= spm;
        ++ticks;
      }
      if (ticks == 8) accumulator = 0.0; // avoid spiral of death
      // autosave once per in-game day
      if (game.state().time.day() != app.last_saved_day) {
        app.last_saved_day = game.state().time.day();
        core::autosave(game);
      }
    }

    // ---- UI ----
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (!app.in_game) {
      auto action = menu.render(game, utils::fs::exists(core::default_save_path()));
      switch (action) {
        case ui::MenuAction::NewGame:
          game.new_game();
          app.in_game = true;
          app.last_saved_day = game.state().time.day();
          break;
        case ui::MenuAction::LoadGame:
          if (game.load(core::default_save_path())) {
            app.in_game = true;
            app.last_saved_day = game.state().time.day();
          }
          break;
        case ui::MenuAction::Quit: app.quit = true; break;
        default: break;
      }
      if (menu.settings_open) render_settings_dialog(game, menu.settings_open);
    } else {
      // dockspace + menu bar
      ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

      if (ImGui::Begin("Neon Grid", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
          if (ImGui::BeginMenu("Game")) {
            if (ImGui::MenuItem("Save")) core::save_game(game, core::default_save_path());
            if (ImGui::MenuItem("Settings")) app.show_settings = true;
            if (ImGui::MenuItem("Help (F1)")) app.show_help = !app.show_help;
            if (ImGui::MenuItem("Main menu")) {
              core::save_game(game, core::default_save_path());
              app.in_game = false;
            }
            if (ImGui::MenuItem("Quit")) {
              core::save_game(game, core::default_save_path());
              app.quit = true;
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenuBar();
        }
      }
      ImGui::End();

      // One console window with a tab per game system: tabs always switch
      // fully (and are reorderable/dockable via the docking branch).
      if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoCollapse)) {
        status_panel.render_top_bar_content(game);
        ImGui::Separator();
        if (ImGui::BeginTabBar("##maintabs", ImGuiTabBarFlags_Reorderable)) {
          if (ImGui::BeginTabItem("Server Room")) { room_view.render_content(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("Inventory")) { room_view.render_inventory(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("Tech Tree")) { tech_view.render_content(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("Netrun")) { netrun_view.render_content(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("Contracts")) { contract_view.render_content(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("Market")) { market_view.render_content(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("HQ")) { status_panel.render_hq_content(game); ImGui::EndTabItem(); }
          if (ImGui::BeginTabItem("Log")) { log_window.render_content(game); ImGui::EndTabItem(); }
          ImGui::EndTabBar();
        }
      }
      ImGui::End();

      if (app.show_help) render_help();
      if (app.show_settings) render_settings_dialog(game, app.show_settings);
    }

    if (game.scanlines && app.in_game) ui::UITheme::instance().draw_scanlines();

    ImGui::Render();
    SDL_SetRenderDrawColor(renderer, 0x0A, 0x0E, 0x14, 0xFF);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  // save on exit
  if (app.in_game) core::save_game(game, core::default_save_path());

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  Mix_CloseAudio();
  TTF_Quit();
  IMG_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  spdlog::info("=== Neon Grid shutdown ===");
  return 0;
}

#ifdef _WIN32
// The game is built as a GUI-subsystem executable (no console window). The
// CRT entry point for /SUBSYSTEM:WINDOWS is WinMain, which forwards to main
// (SDL_MAIN_HANDLED above keeps SDL from renaming main to SDL_main).
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
#endif
