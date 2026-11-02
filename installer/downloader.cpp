#include "downloader.hpp"

#include "net/HttpClient.hpp"

namespace neon::installer {

std::optional<std::string> http_get(const std::string& url, int timeout_seconds) {
  return neon::net::HttpClient::get(url, timeout_seconds);
}

bool http_download(const std::string& url, const std::string& dest_path, int timeout_seconds,
                   const std::function<void(double)>& progress) {
  return neon::net::HttpClient::download(url, dest_path, timeout_seconds, progress);
}

} // namespace neon::installer
