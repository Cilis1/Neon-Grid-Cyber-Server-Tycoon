#pragma once

#include <functional>
#include <optional>
#include <string>

namespace neon::installer {

// HTTP helpers for the bootstrap. Thin wrappers over the game's HttpClient
// (libcurl-backed when NEON_USE_CURL is enabled).

// GET a URL into a string. Nullopt on failure.
std::optional<std::string> http_get(const std::string& url, int timeout_seconds = 10);

// Download a URL to dest_path with progress callback (0..100).
bool http_download(const std::string& url, const std::string& dest_path, int timeout_seconds = 120,
                   const std::function<void(double)>& progress = nullptr);

} // namespace neon::installer
