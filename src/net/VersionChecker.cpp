#include "net/VersionChecker.hpp"

#include "net/HttpClient.hpp"
#include "utils/StringUtils.hpp"

namespace neon::net {

std::optional<ReleaseManifest> ReleaseManifest::from_json(const nlohmann::json& j) {
  if (!j.is_object() || !j.contains("version") || !j.contains("game_zip_url")) return std::nullopt;
  ReleaseManifest m;
  m.version = j.at("version").get<std::string>();
  m.game_zip_url = j.at("game_zip_url").get<std::string>();
  m.sha256 = j.value("sha256", "");
  m.changelog = j.value("changelog", "");
  return m;
}

std::string VersionChecker::default_release_url(const std::string& repo) {
  std::string r = repo.empty() ? "Cilis1/Neon-Grid-Cyber-Server-Tycoon" : repo;
  return "https://github.com/" + r + "/releases/latest/download/version.json";
}

std::optional<ReleaseManifest> VersionChecker::fetch_latest(const std::string& repo, int timeout_seconds) {
  auto body = HttpClient::get(default_release_url(repo), timeout_seconds);
  if (!body) return std::nullopt;
  try {
    return ReleaseManifest::from_json(nlohmann::json::parse(*body));
  } catch (...) {
    return std::nullopt;
  }
}

} // namespace neon::net
