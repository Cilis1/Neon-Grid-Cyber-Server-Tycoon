#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace neon::utils::fs {

namespace stdfs = std::filesystem;

// Base application data dir:
//   Windows: %LOCALAPPDATA%\NeonGrid
//   other:   $XDG_DATA_HOME/NeonGrid or ~/.local/share/NeonGrid
stdfs::path app_data_dir();
stdfs::path save_dir();   // <app_data>/saves
stdfs::path log_dir();    // <app_data>/logs
stdfs::path config_dir(); // <app_data>/config

// Directory that contains the game executable (SDL base path or argv0 dir).
stdfs::path exe_dir();

// Locate the runtime `data/` directory: next to the exe, CWD, or CWD/..
std::optional<stdfs::path> find_data_dir(const std::optional<stdfs::path>& override_dir = std::nullopt);

bool ensure_dir(const stdfs::path& p);
bool exists(const stdfs::path& p);
std::optional<std::string> read_text_file(const stdfs::path& p);
bool write_text_file(const stdfs::path& p, const std::string& content);

} // namespace neon::utils::fs
