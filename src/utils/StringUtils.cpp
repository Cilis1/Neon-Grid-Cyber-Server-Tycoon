#include "utils/StringUtils.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace neon::utils {

std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string to_upper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return s;
}

std::string trim(const std::string& s) {
  std::size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  std::size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) out.push_back(item);
  return out;
}

bool starts_with(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string format_credits(double value) {
  bool neg = value < 0;
  double v = neg ? -value : value;
  std::uint64_t cents = static_cast<std::uint64_t>(v * 100.0 + 0.5);
  std::uint64_t whole = cents / 100;
  std::uint64_t frac = cents % 100;
  std::string digits = std::to_string(whole);
  std::string grouped;
  int count = 0;
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
    grouped.push_back(*it);
    if (++count % 3 == 0 && it + 1 != digits.rend()) grouped.push_back(',');
  }
  std::reverse(grouped.begin(), grouped.end());
  char buf[16];
  std::snprintf(buf, sizeof(buf), ".%02llu", static_cast<unsigned long long>(frac));
  return (neg ? "-" : "") + std::string("\xE2\x82\xA1") + grouped + buf; // ₡ prefix
}

std::string format_signed(double value) {
  std::string s = format_credits(std::abs(value));
  return (value < 0 ? "-" : "+") + s;
}

int compare_version(const std::string& a, const std::string& b) {
  auto pa = split(a, '.');
  auto pb = split(b, '.');
  std::size_t n = std::max(pa.size(), pb.size());
  for (std::size_t i = 0; i < n; ++i) {
    long va = i < pa.size() ? std::stol(pa[i]) : 0;
    long vb = i < pb.size() ? std::stol(pb[i]) : 0;
    if (va < vb) return -1;
    if (va > vb) return 1;
  }
  return 0;
}

std::string random_handle(std::uint64_t seed, const std::string& prefix) {
  static const char* syll1[] = {"neo", "hex", "glitch", "volt", "chrome", "onyx", "pixel", "static",
                                 "ion", "flux", "zero", "dark", "neon", "rust", "void", "byte"};
  static const char* syll2[] = {"runner", "fox", "wire", "shade", "viper", "ghost", "monk", "pulse",
                                 "sage", "reaper", "hawk", "drift", "cipher", "wraith", "spark", "mite"};
  std::uint64_t z = seed * 0x9E3779B97F4A7C15ULL + 0x1234567;
  int a = static_cast<int>((z >> 8) % 16);
  int b = static_cast<int>((z >> 20) % 16);
  int n = static_cast<int>((z >> 32) % 900 + 100);
  return prefix + "-" + syll1[a] + syll2[b] + std::to_string(n);
}

} // namespace neon::utils
