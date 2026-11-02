#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace neon::net {

// Manifest published as a release asset:
// { "version": "1.0.0", "game_zip_url": "...", "sha256": "...", "changelog": "..." }
struct ReleaseManifest {
  std::string version;
  std::string game_zip_url;
  std::string sha256;
  std::string changelog;

  static std::optional<ReleaseManifest> from_json(const nlohmann::json& j);
};

// Fetches the latest release manifest from GitHub releases.
class VersionChecker {
public:
  // `repo` like "owner/repo". If empty, uses the default Neon Grid repo.
  static std::optional<ReleaseManifest> fetch_latest(const std::string& repo = "", int timeout_seconds = 10);

  static std::string default_release_url(const std::string& repo);
};

} // namespace neon::net
