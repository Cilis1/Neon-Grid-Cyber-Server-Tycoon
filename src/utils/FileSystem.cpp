#include "utils/FileSystem.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace neon::utils::fs {

stdfs::path app_data_dir() {
#ifdef _WIN32
  const char* local = std::getenv("LOCALAPPDATA");
  stdfs::path base = local ? stdfs::path(local) : stdfs::path(std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : ".");
  return base / "NeonGrid";
#else
  const char* xdg = std::getenv("XDG_DATA_HOME");
  stdfs::path base = xdg && *xdg ? stdfs::path(xdg) : stdfs::path(std::getenv("HOME") ? std::getenv("HOME") : ".") / ".local" / "share";
  return base / "NeonGrid";
#endif
}

stdfs::path save_dir() { return app_data_dir() / "saves"; }
stdfs::path log_dir() { return app_data_dir() / "logs"; }
stdfs::path config_dir() { return app_data_dir() / "config"; }

stdfs::path exe_dir() {
#ifdef _WIN32
  wchar_t buf[MAX_PATH];
  DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  if (n > 0) return stdfs::path(std::wstring(buf, n)).parent_path();
#else
  char buf[4096];
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    return stdfs::path(buf).parent_path();
  }
#endif
  return stdfs::current_path();
}

std::optional<stdfs::path> find_data_dir(const std::optional<stdfs::path>& override_dir) {
  std::vector<stdfs::path> candidates;
  if (override_dir) candidates.push_back(*override_dir);
  candidates.push_back(exe_dir() / "data");
  candidates.push_back(stdfs::current_path() / "data");
  candidates.push_back(stdfs::current_path().parent_path() / "data");
  candidates.push_back(stdfs::current_path() / ".." / "data");
  for (const auto& c : candidates) {
    std::error_code ec;
    if (stdfs::is_directory(c, ec) && stdfs::exists(c / "tech_tree.json", ec)) return c;
  }
  return std::nullopt;
}

bool ensure_dir(const stdfs::path& p) {
  std::error_code ec;
  stdfs::create_directories(p, ec);
  return stdfs::is_directory(p, ec);
}

bool exists(const stdfs::path& p) {
  std::error_code ec;
  return stdfs::exists(p, ec);
}

std::optional<std::string> read_text_file(const stdfs::path& p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return std::nullopt;
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool write_text_file(const stdfs::path& p, const std::string& content) {
  ensure_dir(p.parent_path());
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out << content;
  return static_cast<bool>(out);
}

} // namespace neon::utils::fs
