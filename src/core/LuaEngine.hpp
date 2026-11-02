#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#ifdef NEON_USE_LUA
#include <sol/sol.hpp>
#endif

namespace neon::core {

using nlohmann::json;

// Optional Lua 5.4 scripting layer (sol2). When NEON_USE_LUA is off or a script
// is missing, callers fall back to the built-in C++ effect system.
class LuaEngine {
public:
  // Point at the repo `scripts/` dir (containing lua/events, lua/programs).
  bool init(const std::filesystem::path& scripts_dir);
  bool available() const { return available_; }
  const std::filesystem::path& scripts_dir() const { return scripts_dir_; }

  // Run scripts/lua/events/<file> with `context` exposed as global `ctx`.
  // Returns the script's returned table (or null json if none / failure).
  json run_event_script(const std::string& file, const json& context);
  // Run scripts/lua/programs/<file> with `context` exposed as global `ctx`.
  json run_program_script(const std::string& file, const json& context);

private:
  bool available_ = false;
  std::filesystem::path scripts_dir_;
#ifdef NEON_USE_LUA
  std::optional<sol::state> lua_;
  json call_script(const std::filesystem::path& path, const json& context);
#endif
};

} // namespace neon::core
