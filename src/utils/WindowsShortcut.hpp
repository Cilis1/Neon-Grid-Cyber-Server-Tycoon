#pragma once

#include <filesystem>
#include <string>

namespace neon::utils {

struct ShortcutSpec {
  std::filesystem::path target;       // exe to launch
  std::filesystem::path working_dir;  // working directory
  std::filesystem::path icon;         // .ico file
  std::filesystem::path link_path;    // .lnk file to create
  std::string description;
  std::string arguments;
};

// Creates a .lnk shortcut using COM IShellLink + IPersistFile (Windows only).
// Returns false on non-Windows platforms or on failure.
bool create_shortcut(const ShortcutSpec& spec);

// Path of the user's Desktop folder.
std::filesystem::path desktop_dir();

} // namespace neon::utils
