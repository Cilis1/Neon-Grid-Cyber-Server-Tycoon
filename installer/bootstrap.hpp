#pragma once

#include <filesystem>
#include <string>

namespace neon::installer {

struct BootstrapOptions {
  std::string version_url;              // manifest URL (default: GitHub latest release)
  std::string install_root;             // default: %LOCALAPPDATA%\NeonGrid
  bool silent = false;                  // hide console output (console still works if attached)
  bool no_launch = false;               // download+install but don't launch the game
  std::string repo = "Cilis1/Neon-Grid-Cyber-Server-Tycoon";
};

// Runs the full bootstrap flow. Returns process exit code (0 = success).
int run_bootstrap(const BootstrapOptions& opts);

// Individual steps (exposed for testing).
std::filesystem::path default_install_root();
std::string default_version_url(const std::string& repo);
bool create_desktop_shortcut(const std::filesystem::path& game_exe, const std::filesystem::path& icon,
                             const std::filesystem::path& link_path, std::string* error_out = nullptr);
bool launch_game(const std::filesystem::path& game_exe);

} // namespace neon::installer
