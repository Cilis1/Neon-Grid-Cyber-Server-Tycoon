#include "bootstrap.hpp"

#include "downloader.hpp"
#include "sha256.hpp"
#include "unzipper.hpp"

#include <nlohmann/json.hpp>
#include <utils/FileSystem.hpp>
#include <utils/StringUtils.hpp>
#include <utils/WindowsShortcut.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace neon::installer {

using nlohmann::json;

namespace {

void log_line(std::ofstream& log, const std::string& msg, bool console = true) {
  if (console) std::printf("[neongrid-setup] %s\n", msg.c_str());
  if (log.is_open()) {
    log << msg << "\n";
    log.flush();
  }
}

void show_error(const std::string& msg, const std::filesystem::path& log_path) {
  std::string full = msg + "\n\nSee log: " + log_path.string();
#ifdef _WIN32
  MessageBoxA(nullptr, full.c_str(), "Neon Grid - Cyber Server Tycoon Setup", MB_OK | MB_ICONERROR);
#else
  std::fprintf(stderr, "[neongrid-setup] ERROR: %s\n", full.c_str());
#endif
}

// Optional GitHub PAT (for private repos). NEONGRID_GITHUB_TOKEN wins over GITHUB_TOKEN.
std::string github_token() {
  const char* t = std::getenv("NEONGRID_GITHUB_TOKEN");
  if (!t || !*t) t = std::getenv("GITHUB_TOKEN");
  return t ? t : "";
}

std::string describe(const neon::net::HttpResponse& r) {
  std::string s = "HTTP " + std::to_string(r.status);
  if (!r.error.empty()) s += " (" + r.error + ")";
  return s;
}

} // namespace

std::filesystem::path default_install_root() { return neon::utils::fs::app_data_dir(); }

std::string default_version_url(const std::string& repo) {
  return "https://github.com/" + repo + "/releases/latest/download/version.json";
}

bool create_desktop_shortcut(const std::filesystem::path& game_exe, const std::filesystem::path& icon,
                             const std::filesystem::path& link_path, std::string* error_out) {
  neon::utils::ShortcutSpec spec;
  spec.target = game_exe;
  spec.working_dir = game_exe.parent_path();
  spec.icon = icon;
  spec.link_path = link_path;
  spec.description = "Neon Grid: Cyber Server Tycoon";
  spec.arguments = "--installed";
  bool ok = neon::utils::create_shortcut(spec);
#ifdef _WIN32
  if (!ok && error_out) *error_out = "COM IShellLink failed (HRESULT). Shortcut not created; game still installed.";
#else
  if (!ok && error_out) *error_out = "shortcut creation is only supported on Windows";
#endif
  return ok;
}

bool launch_game(const std::filesystem::path& game_exe) {
#ifdef _WIN32
  std::wstring cmd = L"\"" + game_exe.wstring() + L"\" --installed";
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  // DETACHED_PROCESS: the game gets no console and is not killed when the
  // installer's console (if any) closes - it runs standalone.
  if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                     DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, nullptr,
                     game_exe.parent_path().wstring().c_str(), &si, &pi)) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
  }
  return false;
#else
  std::string cmd = "\"" + game_exe.string() + "\" --installed &";
  return std::system(cmd.c_str()) == 0;
#endif
}

int run_bootstrap(const BootstrapOptions& opts) {
  const std::filesystem::path root = opts.install_root.empty() ? default_install_root() : std::filesystem::path(opts.install_root);
  const std::filesystem::path game_dir = root / "game";
  const std::filesystem::path log_path = root / "installer.log";
  const std::filesystem::path version_file = root / "version.txt";
  const std::string zip_path =
      (std::filesystem::temp_directory_path() / "neongrid_game.zip").string();

  std::filesystem::create_directories(root);
  std::ofstream log(log_path, std::ios::app);
  const bool console = !opts.silent;
  log_line(log, "=== Neon Grid bootstrap started " + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) + " ===", console);

  auto fail = [&](const std::string& msg) {
    log_line(log, "FAIL: " + msg, console);
    show_error(msg, log_path);
    return 1;
  };

  // 1. Fetch the release manifest.
  const std::string token = github_token();
  if (!token.empty())
    log_line(log, "using GitHub token from environment (NEONGRID_GITHUB_TOKEN/GITHUB_TOKEN)", console);
  std::string url = opts.version_url.empty() ? default_version_url(opts.repo) : opts.version_url;
  log_line(log, "fetching manifest: " + url, console);
  auto resp = http_get_ex(url, 10, token);
  log_line(log, "GET " + url + " -> " + describe(resp), console);
  if (!resp.ok() && opts.version_url.empty()) {
    // Fallback: resolve the latest release via the GitHub API and use the
    // version.json asset URL (needed for private repos with a PAT, and more
    // robust than the /latest/ redirect in general).
    const std::string api = "https://api.github.com/repos/" + opts.repo + "/releases/latest";
    log_line(log, "fallback: querying " + api, console);
    auto api_resp = http_get_ex(api, 15, token);
    log_line(log, "GET " + api + " -> " + describe(api_resp), console);
    if (api_resp.ok()) {
      try {
        const auto rel = json::parse(api_resp.body);
        for (const auto& a : rel.value("assets", json::array())) {
          if (a.value("name", std::string()) == "version.json" && a.contains("browser_download_url")) {
            url = a["browser_download_url"].get<std::string>();
            log_line(log, "resolved version.json via API: " + url, console);
            resp = http_get_ex(url, 10, token);
            log_line(log, "GET " + url + " -> " + describe(resp), console);
            break;
          }
        }
      } catch (const std::exception& e) {
        log_line(log, "release API parse error: " + std::string(e.what()), console);
      }
    }
  }
  if (!resp.ok()) {
    std::string hint;
    if (resp.status == 404)
      hint = " - HTTP 404 means the repository is private or has no published release;"
             " make the repo public (Settings > Danger Zone) or set NEONGRID_GITHUB_TOKEN to a PAT with access";
    else if (resp.status == 401 || resp.status == 403)
      hint = " - HTTP " + std::to_string(resp.status) + ": bad or missing token for a private repo";
    else if (resp.status == 0)
      hint = " - no HTTP response (DNS/TLS/proxy problem)";
    return fail("could not fetch version.json from " + url + " [" + describe(resp) + "]" + hint);
  }
  const std::string body = resp.body;

  json manifest;
  try {
    manifest = json::parse(body);
  } catch (const std::exception& e) {
    return fail(std::string("version.json parse error: ") + e.what());
  }
  if (!manifest.contains("version") || !manifest.contains("game_zip_url"))
    return fail("version.json missing required fields (version, game_zip_url)");
  const std::string remote_version = manifest["version"].get<std::string>();
  const std::string zip_url = manifest["game_zip_url"].get<std::string>();
  const std::string remote_sha = manifest.value("sha256", "");
  log_line(log, "remote version: " + remote_version + ", zip: " + zip_url, console);

  // 2. Local version.
  std::string local_version;
  if (std::filesystem::exists(version_file)) {
    std::ifstream vf(version_file);
    std::getline(vf, local_version);
    local_version = neon::utils::trim(local_version);
  }
  const std::filesystem::path game_exe = game_dir / "NeonGrid.exe";
  const bool need_install = local_version.empty() || neon::utils::compare_version(remote_version, local_version) > 0 ||
                            !std::filesystem::exists(game_exe);
  log_line(log, "local version: '" + local_version + "', need install: " + (need_install ? "yes" : "no"), console);

  if (need_install) {
    // 3. Download game.zip with progress.
    log_line(log, "downloading game.zip ...", console);
    int attempts = 0;
    bool ok = false;
    while (attempts < 2 && !ok) {
      ++attempts;
      ok = http_download(zip_url, zip_path, 300, [&](double pct) {
        if (console && static_cast<int>(pct) % 10 == 0) std::printf("[neongrid-setup]   %d%%\n", static_cast<int>(pct));
      }, token);
      if (!ok) log_line(log, "download attempt " + std::to_string(attempts) + " failed", console);
    }
    if (!ok) return fail("download failed after retries: " + zip_url);

    // 4. Verify SHA-256 (retry once on mismatch).
    if (!remote_sha.empty()) {
      std::string actual = Sha256::file_hex(zip_path);
      log_line(log, "sha256: expected " + remote_sha + " got " + actual, console);
      if (neon::utils::to_lower(actual) != neon::utils::to_lower(remote_sha)) {
        log_line(log, "checksum mismatch, retrying download once", console);
        if (!http_download(zip_url, zip_path, 300, nullptr, token)) return fail("re-download failed");
        actual = Sha256::file_hex(zip_path);
        if (neon::utils::to_lower(actual) != neon::utils::to_lower(remote_sha))
          return fail("sha256 mismatch after retry - download corrupted");
      }
    } else {
      log_line(log, "warning: manifest has no sha256, skipping verification", console);
    }

    // 5. Extract.
    log_line(log, "extracting to " + game_dir.string(), console);
    std::string err;
    auto files = unzip(zip_path, game_dir.string(), &err);
    if (files.empty() && !err.empty()) return fail("unzip failed: " + err);
    log_line(log, "extracted " + std::to_string(files.size()) + " files", console);

    // 6. Write version.txt.
    {
      std::ofstream vf(version_file);
      vf << remote_version << "\n";
    }
    std::error_code ec;
    std::filesystem::remove(zip_path, ec);
  }

  if (!std::filesystem::exists(game_exe)) {
    // On non-Windows the exe is named the same; check anyway
    return fail("game executable not found after install: " + game_exe.string());
  }

  // 7. Desktop shortcut with icon.
  const std::filesystem::path icon = game_dir / "assets" / "icon" / "icon.ico";
  const std::filesystem::path link = neon::utils::desktop_dir() / "Neon Grid - Cyber Server Tycoon.lnk";
  std::string shortcut_err;
  if (create_desktop_shortcut(game_exe, icon, link, &shortcut_err)) {
    log_line(log, "desktop shortcut created: " + link.string(), console);
  } else {
    log_line(log, "shortcut not created: " + shortcut_err, console);
  }

  // 8. Launch.
  if (!opts.no_launch) {
    log_line(log, "launching game...", console);
    if (!launch_game(game_exe)) {
      log_line(log, "warning: failed to auto-launch; run " + game_exe.string() + " manually", console);
    }
  }
  log_line(log, "bootstrap finished OK (version " + remote_version + ")", console);
  return 0;
}

} // namespace neon::installer

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
#ifdef _WIN32
  // GUI-subsystem executable: when launched from a console (cmd/PowerShell),
  // attach to it so progress logs still appear there. From a shortcut or
  // Explorer there is no parent console; output goes to installer.log and
  // errors are shown as message boxes.
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    FILE* con = nullptr;
    freopen_s(&con, "CONOUT$", "w", stdout);
    freopen_s(&con, "CONOUT$", "w", stderr);
  }
#endif
  neon::installer::BootstrapOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--silent") opts.silent = true;
    else if (a == "--no-launch") opts.no_launch = true;
    else if (a == "--url" && i + 1 < argc) opts.version_url = argv[++i];
    else if (a == "--repo" && i + 1 < argc) opts.repo = argv[++i];
    else if (a == "--install-dir" && i + 1 < argc) opts.install_root = argv[++i];
    else if (a == "--help" || a == "-h") {
      std::printf("CyberServerTycoonSetup - Neon Grid bootstrap installer\n"
                  "Usage: CyberServerTycoonSetup.exe [--silent] [--no-launch] [--url <version.json url>]\n"
                  "                                  [--repo <owner/repo>] [--install-dir <dir>]\n");
      return 0;
    }
  }
  return neon::installer::run_bootstrap(opts);
}

#ifdef _WIN32
// GUI-subsystem executable (WIN32_EXECUTABLE): the CRT entry point is
// WinMain, which forwards to main() above.
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
#endif
