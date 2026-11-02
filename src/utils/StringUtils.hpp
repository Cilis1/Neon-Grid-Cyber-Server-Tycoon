#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neon::utils {

std::string to_lower(std::string s);
std::string to_upper(std::string s);
std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
bool starts_with(const std::string& s, const std::string& prefix);
bool ends_with(const std::string& s, const std::string& suffix);

// "1234567.5" -> "1,234,567.50"
std::string format_credits(double value);
// Compact signed integer: "+250" / "-1,000"
std::string format_signed(double value);

// Semver-ish compare: returns -1/0/1 for a < b, a == b, a > b (numeric dot parts)
int compare_version(const std::string& a, const std::string& b);

std::string random_handle(std::uint64_t seed, const std::string& prefix = "runner");

} // namespace neon::utils
