#pragma once

#include <functional>
#include <optional>
#include <string>

namespace neon::net {

// Result of an HTTP GET, including the status code so callers can tell
// "404 Not Found" (e.g. private repo / missing release) apart from transport
// errors (DNS, TLS, proxy) which leave status == 0.
struct HttpResponse {
  long status = 0;    // HTTP status code; 0 = no response received
  std::string body;
  std::string error;  // transport error text (empty when an HTTP response arrived)
  bool ok() const { return status >= 200 && status < 300; }
};

// Thin HTTP client. Uses libcurl when NEON_USE_CURL is enabled; otherwise all
// calls fail gracefully (the game treats network features as optional).
class HttpClient {
public:
  // GET a URL and return the full response (status + body + error).
  // `bearer_token` (optional) is sent as "Authorization: Bearer ..." — needed
  // for private GitHub repos/releases when using a personal access token.
  static HttpResponse get_ex(const std::string& url, int timeout_seconds = 10,
                             const std::string& bearer_token = "");

  // GET a URL, returning the body only on HTTP 2xx.
  static std::optional<std::string> get(const std::string& url, int timeout_seconds = 10,
                                        const std::string& bearer_token = "");

  // Download a URL to `dest_path`. `progress` (0..100) is called periodically.
  // Returns true on HTTP 2xx and successful write.
  static bool download(const std::string& url, const std::string& dest_path, int timeout_seconds = 60,
                       const std::function<void(double percent)>& progress = nullptr,
                       const std::string& bearer_token = "");

  static bool available();
};

} // namespace neon::net
