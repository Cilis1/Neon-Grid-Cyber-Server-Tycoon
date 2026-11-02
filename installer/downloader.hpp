#pragma once

#include "net/HttpClient.hpp"

#include <functional>
#include <optional>
#include <string>

namespace neon::installer {

// HTTP helpers for the bootstrap. Thin wrappers over the game's HttpClient
// (libcurl-backed when NEON_USE_CURL is enabled).

// GET a URL and return the full response (status/body/error) for diagnostics.
// `bearer_token` is sent as "Authorization: Bearer ..." (GitHub PAT for
// private repos; empty for anonymous public downloads).
neon::net::HttpResponse http_get_ex(const std::string& url, int timeout_seconds = 10,
                                    const std::string& bearer_token = "");

// GET a URL into a string. Nullopt unless HTTP 2xx.
std::optional<std::string> http_get(const std::string& url, int timeout_seconds = 10,
                                    const std::string& bearer_token = "");

// Download a URL to dest_path with progress callback (0..100).
bool http_download(const std::string& url, const std::string& dest_path, int timeout_seconds = 120,
                   const std::function<void(double)>& progress = nullptr,
                   const std::string& bearer_token = "");

} // namespace neon::installer
