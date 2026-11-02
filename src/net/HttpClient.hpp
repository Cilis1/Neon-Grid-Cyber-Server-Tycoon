#pragma once

#include <functional>
#include <optional>
#include <string>

namespace neon::net {

// Thin HTTP client. Uses libcurl when NEON_USE_CURL is enabled; otherwise all
// calls fail gracefully (the game treats network features as optional).
class HttpClient {
public:
  // GET a URL, returning the body. `timeout_seconds` bounds the whole request.
  static std::optional<std::string> get(const std::string& url, int timeout_seconds = 10);

  // Download a URL to `dest_path`. `progress` (0..100) is called periodically.
  // Returns true on HTTP 2xx and successful write.
  static bool download(const std::string& url, const std::string& dest_path, int timeout_seconds = 60,
                       const std::function<void(double percent)>& progress = nullptr);

  static bool available();
};

} // namespace neon::net
