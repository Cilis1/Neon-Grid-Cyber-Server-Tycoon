#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace neon::installer {

// Extracts a zip archive into `dest_dir` (created if needed), overwriting
// existing files. Returns the list of extracted file paths.
// Uses miniz. Returns empty vector + sets error on failure.
std::vector<std::string> unzip(const std::string& zip_path, const std::string& dest_dir, std::string* error_out = nullptr);

} // namespace neon::installer
