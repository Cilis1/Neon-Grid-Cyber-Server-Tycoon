#include "core/LuaEngine.hpp"

#include <spdlog/spdlog.h>

namespace neon::core {

#ifdef NEON_USE_LUA

namespace {

json to_json(const sol::table& t) {
  json j = json::object();
  for (auto& [k, v] : t) {
    if (k.is<std::string>()) {
      std::string key = k.as<std::string>();
      if (v.is<bool>()) j[key] = v.as<bool>();
      else if (v.is<int>()) j[key] = v.as<int>();
      else if (v.is<double>()) j[key] = v.as<double>();
      else if (v.is<std::string>()) j[key] = v.as<std::string>();
      else if (v.is<sol::table>()) j[key] = to_json(v.as<sol::table>());
      else if (v.is<sol::nil_t>()) j[key] = nullptr;
    }
  }
  return j;
}

} // namespace

bool LuaEngine::init(const std::filesystem::path& scripts_dir) {
  scripts_dir_ = scripts_dir;
  try {
    lua_.emplace();
    sol::state& lua = *lua_;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::package);
    // make scripts require-able
    std::string pkg_path = (scripts_dir / "lua" / "?.lua").string();
    sol::table package = lua["package"];
    std::string existing = package["path"].get_or(std::string(""));
    package["path"] = pkg_path + ";" + existing;
    available_ = true;
    spdlog::info("Lua engine initialized (scripts dir: {})", scripts_dir.string());
  } catch (const std::exception& e) {
    spdlog::warn("Lua init failed, falling back to C++ effects: {}", e.what());
    lua_.reset();
    available_ = false;
  }
  return available_;
}

json LuaEngine::call_script(const std::filesystem::path& path, const json& context) {
  if (!available_ || !std::filesystem::exists(path)) return json();
  try {
    sol::state& lua = *lua_;
    lua["ctx"] = context;
    sol::protected_function_result result = lua.script_file(path.string(), sol::script_pass_on_error);
    if (!result.valid()) {
      sol::error err = result;
      spdlog::warn("lua error in {}: {}", path.string(), err.what());
      return json();
    }
    sol::object ret = result.return_count() > 0 ? result.get<sol::object>(0) : sol::nil;
    if (ret.is<sol::table>()) return to_json(ret.as<sol::table>());
    return json();
  } catch (const std::exception& e) {
    spdlog::warn("lua exception in {}: {}", path.string(), e.what());
    return json();
  }
}

json LuaEngine::run_event_script(const std::string& file, const json& context) {
  return call_script(scripts_dir_ / "lua" / "events" / file, context);
}

json LuaEngine::run_program_script(const std::string& file, const json& context) {
  return call_script(scripts_dir_ / "lua" / "programs" / file, context);
}

#else // !NEON_USE_LUA

bool LuaEngine::init(const std::filesystem::path& scripts_dir) {
  scripts_dir_ = scripts_dir;
  available_ = false;
  return false;
}

json LuaEngine::run_event_script(const std::string&, const json&) { return json(); }
json LuaEngine::run_program_script(const std::string&, const json&) { return json(); }

#endif

} // namespace neon::core
