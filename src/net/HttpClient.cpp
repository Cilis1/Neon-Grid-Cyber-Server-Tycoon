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

// Sets common options on a curl handle; returns the Authorization header list
// (or nullptr) that the caller must free with curl_slist_free_all.
curl_slist* apply_common(CURL* curl, const std::string& url, long timeout_seconds,
                         const std::string& bearer_token) {
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "NeonGridBootstrap/1.0");
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_slist* headers = nullptr;
  if (!bearer_token.empty()) {
    headers = curl_slist_append(headers, ("Authorization: Bearer " + bearer_token).c_str());
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }
  return headers;
}

} // namespace

bool HttpClient::available() { return true; }

HttpResponse HttpClient::get_ex(const std::string& url, int timeout_seconds, const std::string& bearer_token) {
  HttpResponse resp;
  CURL* curl = curl_easy_init();
  if (!curl) {
    resp.error = "curl_easy_init failed";
    return resp;
  }
  curl_slist* headers = apply_common(curl, url, timeout_seconds, bearer_token);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status);
  if (res != CURLE_OK) resp.error = curl_easy_strerror(res);
  if (headers) curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  return resp;
}

std::optional<std::string> HttpClient::get(const std::string& url, int timeout_seconds,
                                           const std::string& bearer_token) {
  auto resp = get_ex(url, timeout_seconds, bearer_token);
  if (resp.ok()) return resp.body;
  return std::nullopt;
}

bool HttpClient::download(const std::string& url, const std::string& dest_path, int timeout_seconds,
                          const std::function<void(double percent)>& progress,
                          const std::string& bearer_token) {
  CURL* curl = curl_easy_init();
  if (!curl) return false;
  FILE* f = std::fopen(dest_path.c_str(), "wb");
  if (!f) {
    curl_easy_cleanup(curl);
    return false;
  }
  curl_slist* headers = apply_common(curl, url, timeout_seconds, bearer_token);
  ProgressCtx ctx{progress, -1};
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
  CURLcode res = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  if (headers) curl_slist_free_all(headers);
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
HttpResponse HttpClient::get_ex(const std::string&, int, const std::string&) {
  HttpResponse r;
  r.error = "built without libcurl (NEON_USE_CURL=OFF)";
  return r;
}
std::optional<std::string> HttpClient::get(const std::string&, int, const std::string&) { return std::nullopt; }
bool HttpClient::download(const std::string&, const std::string&, int, const std::function<void(double)>&,
                          const std::string&) {
  return false;
}

#endif

} // namespace neon::net
