#include "net/HttpClient.hpp"

#ifdef NEON_USE_CURL
#include <curl/curl.h>
#endif

#include <cstdio>

namespace neon::net {

#ifdef NEON_USE_CURL

namespace {

size_t write_string_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* s = static_cast<std::string*>(userdata);
  s->append(ptr, size * nmemb);
  return size * nmemb;
}

size_t write_file_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* f = static_cast<FILE*>(userdata);
  return std::fwrite(ptr, size, nmemb, f);
}

struct ProgressCtx {
  std::function<void(double)> cb;
  curl_off_t last = -1;
};

int progress_cb(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
  auto* ctx = static_cast<ProgressCtx*>(clientp);
  if (ctx && ctx->cb && dltotal > 0) {
    double pct = 100.0 * static_cast<double>(dlnow) / static_cast<double>(dltotal);
    if (dlnow != ctx->last) {
      ctx->last = dlnow;
      ctx->cb(pct);
    }
  }
  return 0;
}

} // namespace

bool HttpClient::available() { return true; }

std::optional<std::string> HttpClient::get(const std::string& url, int timeout_seconds) {
  CURL* curl = curl_easy_init();
  if (!curl) return std::nullopt;
  std::string body;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "NeonGridBootstrap/1.0");
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  CURLcode res = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK || code < 200 || code >= 300) return std::nullopt;
  return body;
}

bool HttpClient::download(const std::string& url, const std::string& dest_path, int timeout_seconds,
                          const std::function<void(double percent)>& progress) {
  CURL* curl = curl_easy_init();
  if (!curl) return false;
  FILE* f = std::fopen(dest_path.c_str(), "wb");
  if (!f) {
    curl_easy_cleanup(curl);
    return false;
  }
  ProgressCtx ctx{progress, -1};
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "NeonGridBootstrap/1.0");
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
  CURLcode res = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  curl_easy_cleanup(curl);
  std::fclose(f);
  if (res != CURLE_OK || code < 200 || code >= 300) {
    std::remove(dest_path.c_str());
    return false;
  }
  return true;
}

#else // !NEON_USE_CURL

bool HttpClient::available() { return false; }
std::optional<std::string> HttpClient::get(const std::string&, int) { return std::nullopt; }
bool HttpClient::download(const std::string&, const std::string&, int, const std::function<void(double)>&) {
  return false;
}

#endif

} // namespace neon::net
