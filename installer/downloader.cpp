#include "downloader.hpp"

namespace neon::installer {

neon::net::HttpResponse http_get_ex(const std::string& url, int timeout_seconds,
                                    const std::string& bearer_token) {
  return neon::net::HttpClient::get_ex(url, timeout_seconds, bearer_token);
}

std::optional<std::string> http_get(const std::string& url, int timeout_seconds,
                                    const std::string& bearer_token) {
  return neon::net::HttpClient::get(url, timeout_seconds, bearer_token);
}

bool http_download(const std::string& url, const std::string& dest_path, int timeout_seconds,
                   const std::function<void(double)>& progress, const std::string& bearer_token) {
  return neon::net::HttpClient::download(url, dest_path, timeout_seconds, progress, bearer_token);
}

} // namespace neon::installer
